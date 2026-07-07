#!/usr/bin/env bash
set -eo pipefail

cd "$(dirname "$0")/.."

if [ -f /opt/ros/humble/setup.bash ]; then
  source /opt/ros/humble/setup.bash
else
  echo "ROS 2 Humble not found at /opt/ros/humble/setup.bash" >&2
  exit 1
fi

colcon build --symlink-install "$@"

echo "Done. Source: source install/setup.bash"
