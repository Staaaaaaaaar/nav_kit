#include <cmath>
#include <string>

#include "geometry_msgs/msg/quaternion.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace nav_kit
{

class ImuIntegrator : public rclcpp::Node
{
public:
  ImuIntegrator()
  : Node("imu_integrator")
  {
    input_topic_ = declare_parameter<std::string>("input_topic", "/imu/data");
    output_topic_ = declare_parameter<std::string>("output_topic", "/odom/imu");
    odom_frame_ = declare_parameter<std::string>("odom_frame", "odom");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_link");
    two_d_mode_ = declare_parameter<bool>("two_d_mode", true);

    publisher_ = create_publisher<nav_msgs::msg::Odometry>(output_topic_, 10);
    subscription_ = create_subscription<sensor_msgs::msg::Imu>(
      input_topic_, rclcpp::SensorDataQoS(),
      std::bind(&ImuIntegrator::imuCallback, this, std::placeholders::_1));

    RCLCPP_INFO(get_logger(), "imu_integrator: %s -> %s", input_topic_.c_str(), output_topic_.c_str());
  }

private:
  void imuCallback(const sensor_msgs::msg::Imu::SharedPtr msg)
  {
    if (!initialized_) {
      last_time_ = msg->header.stamp;
      initialized_ = true;
      return;
    }

    const double dt = (rclcpp::Time(msg->header.stamp) - last_time_).seconds();
    last_time_ = msg->header.stamp;
    if (dt <= 0.0 || dt > 1.0) {
      return;
    }

    if (two_d_mode_) {
      yaw_ += msg->angular_velocity.z * dt;
    } else {
      // Full 3D integration is out of scope for Phase 1.
      yaw_ += msg->angular_velocity.z * dt;
    }

    nav_msgs::msg::Odometry odom;
    odom.header.stamp = msg->header.stamp;
    odom.header.frame_id = odom_frame_;
    odom.child_frame_id = base_frame_;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, yaw_);
    odom.pose.pose.orientation = tf2::toMsg(q);
    odom.pose.pose.position.x = 0.0;
    odom.pose.pose.position.y = 0.0;
    odom.pose.pose.position.z = 0.0;

    odom.twist.twist.angular = msg->angular_velocity;
    if (two_d_mode_) {
      odom.twist.twist.angular.x = 0.0;
      odom.twist.twist.angular.y = 0.0;
    }

    publisher_->publish(odom);
  }

  std::string input_topic_;
  std::string output_topic_;
  std::string odom_frame_;
  std::string base_frame_;
  bool two_d_mode_{true};
  bool initialized_{false};
  rclcpp::Time last_time_;
  double yaw_{0.0};

  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr publisher_;
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr subscription_;
};

}  // namespace nav_kit

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<nav_kit::ImuIntegrator>());
  rclcpp::shutdown();
  return 0;
}
