import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

sys.path.insert(0, os.path.dirname(__file__))
from config_utils import params_file_path  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    params_file = LaunchConfiguration("params_file").perform(context)
    profile_context_yaml = LaunchConfiguration("profile_context_yaml").perform(context)
    map_path = LaunchConfiguration("map_path").perform(context)

    profile_context = yaml.safe_load(profile_context_yaml) if profile_context_yaml else {}
    use_sim_time = str(profile_context.get("use_sim_time", False)).lower()

    if not map_path:
        raise RuntimeError("amcl module requires map_path (set mode map: or launch map:=...)")

    nav2_bringup_dir = get_package_share_directory("nav2_bringup")
    localization_launch = os.path.join(nav2_bringup_dir, "launch", "localization_launch.py")

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(localization_launch),
            launch_arguments={
                "map": map_path,
                "params_file": params_file_path(params_file),
                "use_sim_time": use_sim_time,
            }.items(),
        )
    ]


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="amcl_indoor.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            DeclareLaunchArgument("map_path", default_value=""),
            OpaqueFunction(function=_launch_setup),
        ]
    )
