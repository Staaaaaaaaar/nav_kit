#include <array>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_kit/common/utils.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/transform_broadcaster.h"

namespace nav_kit
{

class TopicRelay : public rclcpp::Node
{
public:
  TopicRelay()
  : Node("topic_relay")
  {
    msg_type_ = declare_parameter<std::string>("msg_type", "odometry");
    input_topic_ = declare_parameter<std::string>("input_topic", "");
    output_topic_ = declare_parameter<std::string>("output_topic", "/odom");
    parent_frame_ = declare_parameter<std::string>("parent_frame", "odom");
    child_frame_ = declare_parameter<std::string>("child_frame", "base_link");
    publish_tf_mode_ = declare_parameter<std::string>("publish_tf", "none");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/odom");
    map_frame_ = declare_parameter<std::string>("map_frame", "map");
    odom_frame_ = declare_parameter<std::string>("odom_frame", "odom");

    if (msg_type_ != "pose") {
      msg_type_ = "odometry";
    }

    publisher_ = create_publisher<nav_msgs::msg::Odometry>(output_topic_, 10);
    if (publish_tf_mode_ == "odom_base" || publish_tf_mode_ == "map_odom") {
      tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    }

    if (publish_tf_mode_ == "map_odom") {
      odom_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
        odom_topic_, rclcpp::QoS(20),
        std::bind(&TopicRelay::odomCallback, this, std::placeholders::_1));
    }

    if (msg_type_ == "pose") {
      pose_subscription_ = create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
        input_topic_, rclcpp::QoS(20),
        std::bind(&TopicRelay::poseCallback, this, std::placeholders::_1));
    } else {
      odom_source_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
        input_topic_, rclcpp::QoS(20),
        std::bind(&TopicRelay::odomSourceCallback, this, std::placeholders::_1));
    }

    RCLCPP_INFO(
      get_logger(),
      "topic_relay: %s (%s) -> %s, tf=%s",
      input_topic_.c_str(), msg_type_.c_str(), output_topic_.c_str(),
      publish_tf_mode_.c_str());
  }

private:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    latest_odom_ = *msg;
    has_odom_ = true;
  }

  void odomSourceCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    publishFromPose(
      msg->header.stamp,
      msg->pose.pose,
      msg->pose.covariance,
      &msg->twist);
  }

  void poseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
  {
    publishFromPose(
      msg->header.stamp,
      msg->pose.pose,
      msg->pose.covariance,
      nullptr);
  }

  void publishFromPose(
    const builtin_interfaces::msg::Time & stamp,
    const geometry_msgs::msg::Pose & pose,
    const std::array<double, 36> & covariance,
    const nav_msgs::msg::Odometry::_twist_type * twist)
  {
    nav_msgs::msg::Odometry out;
    out.header.stamp = stamp;
    out.pose.pose = pose;
    out.pose.covariance = covariance;
    if (twist != nullptr) {
      out.twist = *twist;
    }
    utils::normalize_odometry(out, parent_frame_, child_frame_);
    publisher_->publish(out);

    if (!tf_broadcaster_) {
      return;
    }

    if (publish_tf_mode_ == "odom_base") {
      tf_broadcaster_->sendTransform(utils::odometry_to_transform(out));
      return;
    }

    if (publish_tf_mode_ != "map_odom" || !has_odom_) {
      return;
    }

    const auto t_map_base = utils::pose_to_transform(pose);
    const auto t_odom_base = utils::pose_to_transform(latest_odom_.pose.pose);
    const auto t_map_odom = t_map_base * t_odom_base.inverse();

    geometry_msgs::msg::TransformStamped tf;
    tf.header.stamp = stamp;
    tf.header.frame_id = map_frame_;
    tf.child_frame_id = odom_frame_;
    tf.transform = tf2::toMsg(t_map_odom);
    tf_broadcaster_->sendTransform(tf);
  }

  std::string msg_type_;
  std::string input_topic_;
  std::string output_topic_;
  std::string parent_frame_;
  std::string child_frame_;
  std::string publish_tf_mode_;
  std::string odom_topic_;
  std::string map_frame_;
  std::string odom_frame_;

  bool has_odom_{false};
  nav_msgs::msg::Odometry latest_odom_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_subscription_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_source_subscription_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr pose_subscription_;

  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr publisher_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

}  // namespace nav_kit

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<nav_kit::TopicRelay>());
  rclcpp::shutdown();
  return 0;
}
