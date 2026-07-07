#include <map>
#include <memory>
#include <string>
#include <vector>

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
    primary_source_ = declare_parameter<std::string>("primary_source", "/odom/wheel");
    sources_ = declare_parameter<std::vector<std::string>>(
      "sources", {"/odom/wheel", "/odom/imu"});

    publisher_ = create_publisher<nav_msgs::msg::Odometry>(output_topic_, 10);
    if (publish_tf_) {
      tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    }

    for (const auto & topic : sources_) {
      auto sub = create_subscription<nav_msgs::msg::Odometry>(
        topic, rclcpp::QoS(10),
        [this, topic](const nav_msgs::msg::Odometry::SharedPtr msg) {
          sourceCallback(topic, msg);
        });
      subscriptions_.emplace(topic, sub);
    }

    RCLCPP_INFO(
      get_logger(), "odom_hub: sources=%zu primary=%s -> %s",
      sources_.size(), primary_source_.c_str(), output_topic_.c_str());
  }

private:
  void sourceCallback(const std::string & topic, const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    latest_[topic] = *msg;

    const nav_msgs::msg::Odometry * selected = nullptr;
    if (latest_.count(primary_source_) > 0) {
      selected = &latest_.at(primary_source_);
    } else if (!latest_.empty()) {
      selected = &latest_.begin()->second;
    }

    if (selected == nullptr) {
      return;
    }

    nav_msgs::msg::Odometry out = *selected;
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
  std::string primary_source_;
  std::vector<std::string> sources_;

  std::map<std::string, nav_msgs::msg::Odometry> latest_;
  std::map<std::string, rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr> subscriptions_;

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
