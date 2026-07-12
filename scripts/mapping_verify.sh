#!/usr/bin/env bash
# Mapping mode acceptance checks (requires nav_kit + sim + mapping mode running).
set -eo pipefail

cd "$(dirname "$0")/.."

if [ -f install/setup.bash ]; then
  # shellcheck disable=SC1091
  source install/setup.bash
elif [ -f /opt/ros/humble/setup.bash ]; then
  # shellcheck disable=SC1091
  source /opt/ros/humble/setup.bash
fi

echo "=== Mapping verify ==="
echo "Note: pointcloud_to_laserscan only subscribes /lidar/points when /scan has subscribers."
echo ""

check_topic() {
  local topic=$1
  if ros2 topic info "$topic" 2>/dev/null | grep -q "Publisher count: [1-9]"; then
    echo "[OK] $topic has publisher"
  else
    echo "[FAIL] $topic has no publisher"
    return 1
  fi
}

check_hz() {
  local topic=$1
  local rate
  rate=$(timeout 8 ros2 topic hz "$topic" 2>&1 | awk '/average rate/ {print $3; exit}')
  if [ -n "$rate" ]; then
    echo "[OK] $topic rate ~${rate} Hz"
  else
    echo "[FAIL] $topic no data (drive the robot or wait longer)"
    return 1
  fi
}

fail=0
check_topic /odom || fail=1
check_topic /loc/slam || fail=1
check_topic /loc || fail=1

echo ""
echo "Checking /scan (also activates pointcloud_to_laserscan)..."
check_hz /scan || fail=1

echo ""
echo "Checking /map (drive robot to build map)..."
check_hz /map || fail=1

echo ""
if [ "$fail" -eq 0 ]; then
  echo "Mapping checks passed."
  echo "Save map: ./scripts/save_map.sh"
else
  echo "Some checks failed."
  exit 1
fi
