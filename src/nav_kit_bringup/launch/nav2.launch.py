import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node

sys.path.insert(0, os.path.dirname(__file__))
from config_utils import params_file_path  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    params_file = LaunchConfiguration("params_file").perform(context)
    profile_context_yaml = LaunchConfiguration("profile_context_yaml").perform(context)
    use_rviz = LaunchConfiguration("use_rviz")

    profile_context = yaml.safe_load(profile_context_yaml) if profile_context_yaml else {}
    use_sim_time = str(profile_context.get("use_sim_time", False)).lower()

    nav2_bringup_dir = get_package_share_directory("nav2_bringup")
    navigation_launch = os.path.join(nav2_bringup_dir, "launch", "navigation_launch.py")
    rviz_config = os.path.join(
        get_package_share_directory("nav_kit_config"),
        "rviz",
        "nav_known_map.rviz",
    )

    actions = [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(navigation_launch),
            launch_arguments={
                "params_file": params_file_path(params_file),
                "use_sim_time": use_sim_time,
            }.items(),
        ),
        Node(
            package="rviz2",
            executable="rviz2",
            name="rviz2_nav",
            output="screen",
            arguments=["-d", rviz_config],
            condition=IfCondition(use_rviz),
        ),
    ]
    return actions


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="nav2_indoor.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            DeclareLaunchArgument("map_path", default_value=""),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
