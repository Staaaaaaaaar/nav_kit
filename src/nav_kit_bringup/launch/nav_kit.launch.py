import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import SetParameter
from ament_index_python.packages import get_package_share_directory

sys.path.insert(0, os.path.join(get_package_share_directory("nav_kit_bringup"), "launch"))
from config_utils import load_mode, load_profile, profile_context_yaml, resolve_map_yaml  # noqa: E402


def _parse_bool(value: str) -> bool:
    return value.lower() in ("true", "1", "yes")


def _launch_setup(context, *args, **kwargs):
    profile_name = LaunchConfiguration("profile").perform(context)
    mode_name = LaunchConfiguration("mode").perform(context)
    map_override = LaunchConfiguration("map").perform(context)
    use_rviz = LaunchConfiguration("use_rviz")
    use_sim_time = _parse_bool(LaunchConfiguration("use_sim_time").perform(context))

    profile = load_profile(profile_name)
    mode = load_mode(mode_name)
    context_yaml = profile_context_yaml(profile, use_sim_time=use_sim_time)

    map_path = resolve_map_yaml(map_override or mode.get("map", ""))

    bringup_share = get_package_share_directory("nav_kit_bringup")
    actions = []

    if use_sim_time:
        actions.append(SetParameter(name="use_sim_time", value=True))

    for module_name, params_file in mode.get("modules", {}).items():
        launch_file = os.path.join(bringup_share, "launch", f"{module_name}.launch.py")
        if not os.path.isfile(launch_file):
            raise RuntimeError(f"Missing launch file for module '{module_name}': {launch_file}")

        launch_arguments = {
            "params_file": params_file,
            "profile_context_yaml": context_yaml,
        }
        if map_path:
            launch_arguments["map_path"] = map_path
        if module_name in ("slam", "nav2"):
            launch_arguments["use_rviz"] = use_rviz

        actions.append(
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(launch_file),
                launch_arguments=launch_arguments.items(),
            )
        )

    return actions


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("mode", default_value="mapping"),
            DeclareLaunchArgument("profile", default_value="quadrover"),
            DeclareLaunchArgument("map", default_value=""),
            DeclareLaunchArgument("use_sim_time", default_value="true"),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
