#!/usr/bin/env bash
# Save an occupancy map via slam_toolbox (requires mapping mode running).
set -eo pipefail

cd "$(dirname "$0")/.."
REPO_ROOT="$(pwd)"

if [ -f install/setup.bash ]; then
  # shellcheck disable=SC1091
  set +u
  source install/setup.bash
elif [ -f /opt/ros/humble/setup.bash ]; then
  # shellcheck disable=SC1091
  set +u
  source /opt/ros/humble/setup.bash
fi

usage() {
  cat <<'EOF'
Usage: ./scripts/save_map.sh [OPTIONS] [MAP_NAME] [OUTPUT_DIR]

Save the current slam_toolbox occupancy map. By default, the active profile is
read from topic_relay and the map is saved as:
  src/navforge_config/maps/<profile>/map.yaml

Options:
  -p, --profile PROFILE    Select the profile directory explicitly
  -o, --output-dir DIR    Override the output directory
  -n, --map-name NAME     Override the map basename (default: map)
  -h, --help              Show this help

The positional MAP_NAME and OUTPUT_DIR arguments are retained for compatibility.
EOF
}

PROFILE=""
MAP_NAME=""
MAP_DIR=""
POSITIONAL=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    -p|--profile)
      [ "$#" -ge 2 ] || { echo "Missing value for $1" >&2; exit 2; }
      PROFILE="$2"
      shift 2
      ;;
    -o|--output-dir)
      [ "$#" -ge 2 ] || { echo "Missing value for $1" >&2; exit 2; }
      MAP_DIR="$2"
      shift 2
      ;;
    -n|--map-name)
      [ "$#" -ge 2 ] || { echo "Missing value for $1" >&2; exit 2; }
      MAP_NAME="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      POSITIONAL+=("$@")
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
    *)
      POSITIONAL+=("$1")
      shift
      ;;
  esac
done

if [ "${#POSITIONAL[@]}" -gt 2 ]; then
  echo "Expected at most MAP_NAME and OUTPUT_DIR positional arguments" >&2
  exit 2
fi

MAP_NAME="${MAP_NAME:-${POSITIONAL[0]:-map}}"
MAP_DIR="${MAP_DIR:-${POSITIONAL[1]:-}}"
MAP_NAME="${MAP_NAME%.yaml}"

if [ -z "$MAP_NAME" ] || [[ "$MAP_NAME" == */* ]]; then
  echo "Map name must be a non-empty basename without directory separators" >&2
  exit 2
fi

if [ -z "$MAP_DIR" ]; then
  if [ -z "$PROFILE" ]; then
    PROFILE_OUTPUT="$(ros2 param get /topic_relay_odom navforge_profile 2>/dev/null || true)"
    case "$PROFILE_OUTPUT" in
      "String value is: "*) PROFILE="${PROFILE_OUTPUT#String value is: }" ;;
    esac
  fi
  PROFILE="${PROFILE:-quadrover}"
  MAP_DIR="${REPO_ROOT}/src/navforge_config/maps/${PROFILE}"
fi

mkdir -p "$MAP_DIR"
SAVE_PATH="${MAP_DIR}/${MAP_NAME}"

echo "Saving map to ${SAVE_PATH} ..."
ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap "name: {data: '${SAVE_PATH}'}"

echo "Done. Expected files:"
echo "  ${SAVE_PATH}.yaml"
echo "  ${SAVE_PATH}.pgm"
