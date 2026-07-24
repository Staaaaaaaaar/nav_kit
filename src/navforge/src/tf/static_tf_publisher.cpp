#include <cmath>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_ros/static_transform_broadcaster.h"

namespace navforge
{

class StaticTfPublisher : public rclcpp::Node
{
public:
  StaticTfPublisher()
  : Node("static_tf_publisher"), broadcaster_(this)
  {
    const auto specifications = declare_parameter<std::vector<std::string>>(
      "transforms", std::vector<std::string>{});

    std::vector<geometry_msgs::msg::TransformStamped> transforms;
    transforms.reserve(specifications.size());
    std::unordered_set<std::string> child_frames;

    for (std::size_t index = 0; index < specifications.size(); ++index) {
      auto transform = parse_transform(specifications[index], index);
      if (!child_frames.insert(transform.child_frame_id).second) {
        throw std::invalid_argument(
                "Duplicate static TF child frame '" + transform.child_frame_id + "'");
      }
      transforms.push_back(std::move(transform));
    }

    if (transforms.empty()) {
      RCLCPP_INFO(get_logger(), "No static TF configured; nothing will be published");
      return;
    }

    broadcaster_.sendTransform(transforms);
    for (const auto & transform : transforms) {
      RCLCPP_INFO(
        get_logger(), "Published static TF: %s -> %s",
        transform.header.frame_id.c_str(), transform.child_frame_id.c_str());
    }
  }

private:
  geometry_msgs::msg::TransformStamped parse_transform(
    const std::string & specification, std::size_t index)
  {
    std::istringstream stream(specification);
    std::string parent_frame;
    std::string child_frame;
    double x;
    double y;
    double z;
    double roll;
    double pitch;
    double yaw;
    std::string trailing_value;

    if (!(stream >> parent_frame >> child_frame >> x >> y >> z >> roll >> pitch >> yaw) ||
      (stream >> trailing_value))
    {
      throw std::invalid_argument(
              "Invalid transforms[" + std::to_string(index) + "]: expected "
              "'parent_frame child_frame x y z roll pitch yaw', got '" +
              specification + "'");
    }

    if (parent_frame == child_frame) {
      throw std::invalid_argument(
              "Invalid transforms[" + std::to_string(index) +
              "]: parent_frame and child_frame must differ");
    }
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
      !std::isfinite(roll) || !std::isfinite(pitch) || !std::isfinite(yaw))
    {
      throw std::invalid_argument(
              "Invalid transforms[" + std::to_string(index) +
              "]: translation and rotation must be finite numbers");
    }

    tf2::Quaternion rotation;
    rotation.setRPY(roll, pitch, yaw);
    rotation.normalize();

    geometry_msgs::msg::TransformStamped transform;
    transform.header.stamp = now();
    transform.header.frame_id = parent_frame;
    transform.child_frame_id = child_frame;
    transform.transform.translation.x = x;
    transform.transform.translation.y = y;
    transform.transform.translation.z = z;
    transform.transform.rotation.x = rotation.x();
    transform.transform.rotation.y = rotation.y();
    transform.transform.rotation.z = rotation.z();
    transform.transform.rotation.w = rotation.w();
    return transform;
  }

  tf2_ros::StaticTransformBroadcaster broadcaster_;
};

}  // namespace navforge

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<navforge::StaticTfPublisher>());
  } catch (const std::exception & error) {
    RCLCPP_FATAL(rclcpp::get_logger("static_tf_publisher"), "%s", error.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
