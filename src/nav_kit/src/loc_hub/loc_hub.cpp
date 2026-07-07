#include <array>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2/LinearMath/Transform.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2_ros/transform_broadcaster.h"

namespace nav_kit
{

class LocHub : public rclcpp::Node
{
public:
  LocHub()
  : Node("loc_hub")
  {
    output_topic_ = declare_parameter<std::string>("output_topic", "/loc");
    source_topic_ = declare_parameter<std::string>("source_topic", "/loc/gps");
    source_type_ = declare_parameter<std::string>("source_type", "odometry");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/odom");
    publish_tf_ = declare_parameter<bool>("publish_tf", true);
    map_frame_ = declare_parameter<std::string>("map_frame", "map");
    odom_frame_ = declare_parameter<std::string>("odom_frame", "odom");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_link");

    publisher_ = create_publisher<nav_msgs::msg::Odometry>(output_topic_, 10);
    if (publish_tf_) {
      tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    }

    odom_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
      odom_topic_, rclcpp::QoS(20),
      std::bind(&LocHub::odomCallback, this, std::placeholders::_1));

    if (source_type_ == "pose") {
      pose_subscription_ = create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
        source_topic_, rclcpp::QoS(20),
        std::bind(&LocHub::poseCallback, this, std::placeholders::_1));
    } else {
      source_type_ = "odometry";
      odom_source_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
        source_topic_, rclcpp::QoS(20),
        std::bind(&LocHub::locOdomCallback, this, std::placeholders::_1));
    }

    RCLCPP_INFO(
      get_logger(), "loc_hub: source=%s (%s), odom=%s -> %s",
      source_topic_.c_str(), source_type_.c_str(), odom_topic_.c_str(), output_topic_.c_str());
  }

private:
  static tf2::Transform poseToTf(const geometry_msgs::msg::Pose & pose)
  {
    tf2::Transform tf;
    tf.setOrigin(tf2::Vector3(pose.position.x, pose.position.y, pose.position.z));
    tf2::Quaternion q;
    tf2::fromMsg(pose.orientation, q);
    tf.setRotation(q);
    return tf;
  }

  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    latest_odom_ = *msg;
    has_odom_ = true;
  }

  void locOdomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    publishFromSource(msg->header.stamp, msg->pose.pose, msg->pose.covariance, &msg->twist, true);
  }

  void poseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
  {
    publishFromSource(msg->header.stamp, msg->pose.pose, msg->pose.covariance, nullptr, false);
  }

  void publishFromSource(
    const builtin_interfaces::msg::Time & stamp,
    const geometry_msgs::msg::Pose & map_to_base_pose,
    const std::array<double, 36> & covariance,
    const nav_msgs::msg::Odometry::_twist_type * twist,
    bool has_twist)
  {
    if (!has_odom_) {
      return;
    }

    const tf2::Transform t_map_base = poseToTf(map_to_base_pose);
    const tf2::Transform t_odom_base = poseToTf(latest_odom_.pose.pose);
    const tf2::Transform t_map_odom = t_map_base * t_odom_base.inverse();

    nav_msgs::msg::Odometry out;
    out.header.stamp = stamp;
    out.header.frame_id = map_frame_;
    out.child_frame_id = base_frame_;
    out.pose.pose = map_to_base_pose;
    out.pose.covariance = covariance;
    if (has_twist && twist != nullptr) {
      out.twist = *twist;
    }
    publisher_->publish(out);

    if (publish_tf_ && tf_broadcaster_) {
      geometry_msgs::msg::TransformStamped tf;
      tf.header.stamp = stamp;
      tf.header.frame_id = map_frame_;
      tf.child_frame_id = odom_frame_;
      tf.transform = tf2::toMsg(t_map_odom);
      tf_broadcaster_->sendTransform(tf);
    }
  }

  std::string output_topic_;
  std::string source_topic_;
  std::string source_type_;
  std::string odom_topic_;
  bool publish_tf_{true};
  std::string map_frame_;
  std::string odom_frame_;
  std::string base_frame_;

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
  rclcpp::spin(std::make_shared<nav_kit::LocHub>());
  rclcpp::shutdown();
  return 0;
}
