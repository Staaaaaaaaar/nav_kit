#!/usr/bin/env bash
# Install apt dependencies for nav_kit (ROS 2 Humble on Ubuntu 22.04).
set -eo pipefail

ROS_DISTRO="${ROS_DISTRO:-humble}"
PHASE="phase1"
ROS_APT_REPO="http://packages.ros.org/ros2/ubuntu"

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Install apt packages required by nav_kit.

Options:
  --phase1    Phase 1 only: processors + odom_hub (default)
  --all       Phase 1 plus planned deps for amcl / slam / rtk / nav2 / rviz
  -h, --help  Show this help

Requires ROS 2 ${ROS_DISTRO} at /opt/ros/${ROS_DISTRO}/setup.bash
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --phase1) PHASE="phase1"; shift ;;
    --all) PHASE="all"; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
  esac
done

if [[ ! -f "/opt/ros/${ROS_DISTRO}/setup.bash" ]]; then
  echo "Error: ROS 2 ${ROS_DISTRO} not found at /opt/ros/${ROS_DISTRO}/setup.bash" >&2
  echo "Install ROS 2 Humble first: https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html" >&2
  exit 1
fi

# shellcheck disable=SC1091
source "/opt/ros/${ROS_DISTRO}/setup.bash"

refresh_ros_apt_index() {
  echo "Refreshing ROS apt index (fixes stale 404 on pointcloud_to_laserscan)..."
  sudo rm -f /var/lib/apt/lists/*packages.ros.org_ros2_ubuntu*
  sudo apt update -o Acquire::AllowReleaseInfoChange=true
}

apt_package_installed() {
  dpkg-query -W -f='${Status}' "$1" 2>/dev/null | grep -q "install ok installed"
}

install_pointcloud_to_laserscan() {
  local pkg="ros-${ROS_DISTRO}-pointcloud-to-laserscan"

  if apt_package_installed "$pkg"; then
    echo "$pkg already installed."
    return 0
  fi

  echo "Installing $pkg..."
  if sudo apt install -y "$pkg"; then
    return 0
  fi

  echo "apt install failed (often stale local index -> 404). Fetching current deb from pool..."
  local packages_url="${ROS_APT_REPO}/dists/jammy/main/binary-amd64/Packages"
  local deb_path
  deb_path=$(
    curl -fsSL "$packages_url" | awk '
      /^Package: ros-humble-pointcloud-to-laserscan$/ { found=1; next }
      found && /^Package: / { exit }
      found && /^Filename: / { print $2; exit }
    '
  )

  if [[ -z "$deb_path" ]]; then
    echo "Error: could not resolve $pkg Filename from ${packages_url}" >&2
    return 1
  fi

  local deb_url="${ROS_APT_REPO}/${deb_path}"
  local tmp_deb
  tmp_deb="$(mktemp /tmp/${pkg}.XXXXXX.deb)"
  trap 'rm -f "$tmp_deb"' RETURN

  echo "Downloading ${deb_url}"
  curl -fsSL "$deb_url" -o "$tmp_deb"
  sudo apt install -y \
    ros-${ROS_DISTRO}-laser-geometry \
    ros-${ROS_DISTRO}-tf2-sensor-msgs \
    ros-${ROS_DISTRO}-message-filters \
    ros-${ROS_DISTRO}-rclcpp-components
  sudo dpkg -i "$tmp_deb" || sudo apt install -f -y
}

PACKAGES=(
  # build
  python3-colcon-common-extensions
  ros-${ROS_DISTRO}-ament-cmake
  # nav_kit (C++)
  ros-${ROS_DISTRO}-rclcpp
  ros-${ROS_DISTRO}-nav-msgs
  ros-${ROS_DISTRO}-sensor-msgs
  ros-${ROS_DISTRO}-geometry-msgs
  ros-${ROS_DISTRO}-tf2
  ros-${ROS_DISTRO}-tf2-ros
  ros-${ROS_DISTRO}-tf2-geometry-msgs
  # nav_kit_bringup (launch)
  ros-${ROS_DISTRO}-launch
  ros-${ROS_DISTRO}-launch-ros
  ros-${ROS_DISTRO}-ament-index-python
  python3-yaml
)

if [[ "$PHASE" == "all" ]]; then
  PACKAGES+=(
    ros-${ROS_DISTRO}-navigation2
    ros-${ROS_DISTRO}-nav2-bringup
    ros-${ROS_DISTRO}-slam-toolbox
    ros-${ROS_DISTRO}-robot-localization
    ros-${ROS_DISTRO}-rviz2
  )
fi

echo "Installing nav_kit dependencies (${PHASE}) for ROS ${ROS_DISTRO}..."
refresh_ros_apt_index
sudo apt install -y "${PACKAGES[@]}"
install_pointcloud_to_laserscan
echo "Done. Next: ./scripts/build.sh && source install/setup.bash"
