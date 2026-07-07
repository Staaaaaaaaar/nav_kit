#include <memory>
#include <string>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/transform_broadcaster.h"

namespace nav_kit
{

class OdomHub : public rclcpp::Node
{
public:
  OdomHub()
  : Node("odom_hub")
  {
    output_topic_ = declare_parameter<std::string>("output_topic", "/odom");
    publish_tf_ = declare_parameter<bool>("publish_tf", true);
    odom_frame_ = declare_parameter<std::string>("odom_frame", "odom");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_link");
    source_topic_ = declare_parameter<std::string>("source_topic", "");
    const auto legacy_primary = declare_parameter<std::string>("primary_source", "");
    if (source_topic_.empty()) {
      source_topic_ = legacy_primary.empty() ? "/odom/wheel" : legacy_primary;
    }

    publisher_ = create_publisher<nav_msgs::msg::Odometry>(output_topic_, 10);
    if (publish_tf_) {
      tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    }

    source_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
      source_topic_, rclcpp::QoS(20),
      std::bind(&OdomHub::sourceCallback, this, std::placeholders::_1));

    RCLCPP_INFO(
      get_logger(), "odom_hub: source=%s -> %s",
      source_topic_.c_str(), output_topic_.c_str());
  }

private:
  void sourceCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    nav_msgs::msg::Odometry out = *msg;
    out.header.frame_id = odom_frame_;
    out.child_frame_id = base_frame_;
    publisher_->publish(out);

    if (publish_tf_ && tf_broadcaster_) {
      geometry_msgs::msg::TransformStamped tf;
      tf.header.stamp = out.header.stamp;
      tf.header.frame_id = odom_frame_;
      tf.child_frame_id = base_frame_;
      tf.transform.translation.x = out.pose.pose.position.x;
      tf.transform.translation.y = out.pose.pose.position.y;
      tf.transform.translation.z = out.pose.pose.position.z;
      tf.transform.rotation = out.pose.pose.orientation;
      tf_broadcaster_->sendTransform(tf);
    }
  }

  std::string output_topic_;
  bool publish_tf_{true};
  std::string odom_frame_;
  std::string base_frame_;
  std::string source_topic_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr source_subscription_;

  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr publisher_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

}  // namespace nav_kit

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<nav_kit::OdomHub>());
  rclcpp::shutdown();
  return 0;
}
