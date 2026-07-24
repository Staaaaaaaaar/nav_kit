#ifndef NAVFORGE__COMMON__UTILS_HPP_
#define NAVFORGE__COMMON__UTILS_HPP_

#include <string>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2/LinearMath/Transform.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace navforge
{
namespace utils
{

inline void normalize_odometry(
  nav_msgs::msg::Odometry & odom,
  const std::string & parent_frame,
  const std::string & child_frame)
{
  odom.header.frame_id = parent_frame;
  odom.child_frame_id = child_frame;
}

inline geometry_msgs::msg::TransformStamped odometry_to_transform(
  const nav_msgs::msg::Odometry & odom)
{
  geometry_msgs::msg::TransformStamped tf;
  tf.header.stamp = odom.header.stamp;
  tf.header.frame_id = odom.header.frame_id;
  tf.child_frame_id = odom.child_frame_id;
  tf.transform.translation.x = odom.pose.pose.position.x;
  tf.transform.translation.y = odom.pose.pose.position.y;
  tf.transform.translation.z = odom.pose.pose.position.z;
  tf.transform.rotation = odom.pose.pose.orientation;
  return tf;
}

inline tf2::Transform pose_to_transform(const geometry_msgs::msg::Pose & pose)
{
  tf2::Transform tf;
  tf.setOrigin(tf2::Vector3(pose.position.x, pose.position.y, pose.position.z));
  tf2::Quaternion q;
  tf2::fromMsg(pose.orientation, q);
  tf.setRotation(q);
  return tf;
}

}  // namespace utils
}  // namespace navforge

#endif  // NAVFORGE__COMMON__UTILS_HPP_
