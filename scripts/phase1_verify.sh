#!/usr/bin/env bash
# Phase 1 acceptance checks (requires nav_kit + quadrover sim running).
set -eo pipefail

cd "$(dirname "$0")/.."

if [ -f install/setup.bash ]; then
  # shellcheck disable=SC1091
  source install/setup.bash
elif [ -f /opt/ros/humble/setup.bash ]; then
  # shellcheck disable=SC1091
  source /opt/ros/humble/setup.bash
fi

echo "=== Phase 1 verify ==="
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
  rate=$(timeout 6 ros2 topic hz "$topic" 2>&1 | awk '/average rate/ {print $3; exit}')
  if [ -n "$rate" ]; then
    echo "[OK] $topic rate ~${rate} Hz"
  else
    echo "[FAIL] $topic no data (wait longer or check upstream)"
    return 1
  fi
}

fail=0
check_topic /odom/wheel || fail=1
check_topic /lidar/points || fail=1
check_topic /odom || fail=1

echo ""
echo "Checking /scan (this command also activates pointcloud_to_laserscan)..."
check_hz /scan || fail=1

echo ""
if [ "$fail" -eq 0 ]; then
  echo "Phase 1 checks passed."
else
  echo "Some checks failed."
  exit 1
fi
