import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

sys.path.insert(0, os.path.join(get_package_share_directory("nav_kit_bringup"), "launch"))
from config_utils import load_mode, load_profile, resolve_map_path  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    profile_name = LaunchConfiguration("profile").perform(context)
    mode_name = LaunchConfiguration("mode").perform(context)
    map_override = LaunchConfiguration("map").perform(context)

    profile = load_profile(profile_name)
    mode = load_mode(mode_name)
    topics_yaml = yaml.dump(profile.get("topics", {}), default_flow_style=True)

    map_path = map_override or mode.get("map", "")
    if map_path:
        map_path = resolve_map_path(map_path)

    bringup_share = get_package_share_directory("nav_kit_bringup")
    actions = []

    for module_name, params_file in mode.get("modules", {}).items():
        launch_file = os.path.join(bringup_share, "launch", f"{module_name}.launch.py")
        if not os.path.isfile(launch_file):
            raise RuntimeError(f"Missing launch file for module '{module_name}': {launch_file}")

        actions.append(
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(launch_file),
                launch_arguments={
                    "params_file": params_file,
                    "topics_yaml": topics_yaml,
                }.items(),
            )
        )

    return actions


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("mode", default_value="phase1"),
            DeclareLaunchArgument("profile", default_value="quadrover_sim"),
            DeclareLaunchArgument("map", default_value=""),
            OpaqueFunction(function=_launch_setup),
        ]
    )
