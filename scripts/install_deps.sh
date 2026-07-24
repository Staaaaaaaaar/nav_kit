#!/usr/bin/env bash
# Install apt dependencies for NavForge (ROS 2 Humble on Ubuntu 22.04).
set -euo pipefail

ROS_DISTRO="${ROS_DISTRO:-humble}"

if [[ ! -f "/opt/ros/${ROS_DISTRO}/setup.bash" ]]; then
  echo "Error: ROS 2 ${ROS_DISTRO} not found at /opt/ros/${ROS_DISTRO}/setup.bash" >&2
  echo "Install ROS 2 first: https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html" >&2
  exit 1
fi

PACKAGES=(
  python3-colcon-common-extensions
  python3-yaml
  ros-${ROS_DISTRO}-ament-cmake
  ros-${ROS_DISTRO}-ament-index-python
  ros-${ROS_DISTRO}-launch
  ros-${ROS_DISTRO}-launch-ros
  ros-${ROS_DISTRO}-geometry-msgs
  ros-${ROS_DISTRO}-nav-msgs
  ros-${ROS_DISTRO}-rclcpp
  ros-${ROS_DISTRO}-sensor-msgs
  ros-${ROS_DISTRO}-tf2
  ros-${ROS_DISTRO}-tf2-geometry-msgs
  ros-${ROS_DISTRO}-tf2-ros
  ros-${ROS_DISTRO}-pointcloud-to-laserscan
  ros-${ROS_DISTRO}-slam-toolbox
  ros-${ROS_DISTRO}-rviz2
  ros-${ROS_DISTRO}-navigation2
  ros-${ROS_DISTRO}-nav2-bringup
)

echo "Installing all NavForge dependencies for ROS ${ROS_DISTRO}..."
sudo apt update
sudo apt install -y "${PACKAGES[@]}"
echo "Done. Next: ./scripts/build.sh && source install/setup.bash"
