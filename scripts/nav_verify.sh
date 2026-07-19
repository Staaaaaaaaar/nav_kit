#!/usr/bin/env bash
# Known-map navigation checks (requires sim + known_map_nav mode running).
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

echo "=== Known map nav verify ==="

check_topic() {
  local topic=$1
  if ros2 topic info "$topic" 2>/dev/null | grep -q "Publisher count: [1-9]"; then
    echo "[OK] $topic has publisher"
  else
    echo "[FAIL] $topic has no publisher"
    return 1
  fi
}

fail=0
check_topic /map || fail=1
check_topic /odom || fail=1
check_topic /scan || fail=1
check_topic /amcl_pose || fail=1

echo ""
echo "Checking Nav2 lifecycle (bt_navigator should be active after AMCL initializes)..."
if ros2 lifecycle get /bt_navigator 2>/dev/null | grep -q "active"; then
  echo "[OK] bt_navigator is active"
else
  echo "[WARN] bt_navigator not active yet (set initial pose in RViz if needed)"
fi

echo ""
echo "Checking map→odom TF (requires initial pose in RViz)..."
if timeout 3 ros2 run tf2_ros tf2_echo map odom 2>/dev/null | grep -q "At time"; then
  echo "[OK] map→odom transform available"
else
  echo "[WARN] map→odom not available (set 2D Pose Estimate in RViz)"
fi

echo ""
if [ "$fail" -eq 0 ]; then
  echo "Basic nav checks passed. Set initial pose, then send a goal in RViz."
else
  echo "Some checks failed."
  exit 1
fi
