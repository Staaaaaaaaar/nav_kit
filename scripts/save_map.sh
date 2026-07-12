#!/usr/bin/env bash
# Save SLAM map via slam_toolbox (requires mapping mode running).
set -eo pipefail

cd "$(dirname "$0")/.."

if [ -f install/setup.bash ]; then
  # shellcheck disable=SC1091
  set +u
  source install/setup.bash
elif [ -f /opt/ros/humble/setup.bash ]; then
  # shellcheck disable=SC1091
  set +u
  source /opt/ros/humble/setup.bash
fi

MAP_NAME="${1:-quadrover_indoor}"
MAP_DIR="${2:-$(ros2 pkg prefix nav_kit_config)/share/nav_kit_config/maps/quadrover/indoor}"

mkdir -p "$MAP_DIR"
SAVE_PATH="${MAP_DIR}/${MAP_NAME}"

echo "Saving map to ${SAVE_PATH} ..."
ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap "name: {data: '${SAVE_PATH}'}"

echo "Done. Expected files:"
echo "  ${SAVE_PATH}.posegraph"
echo "  ${SAVE_PATH}.data"
