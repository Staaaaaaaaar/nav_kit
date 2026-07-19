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
    rviz_config_file = LaunchConfiguration("rviz_config").perform(context) or "nav_known_map.rviz"
    use_rviz_val = LaunchConfiguration("use_rviz").perform(context)
    use_rviz = LaunchConfiguration("use_rviz")

    profile_context = yaml.safe_load(profile_context_yaml) if profile_context_yaml else {}
    use_sim_time = str(profile_context.get("use_sim_time", False)).lower()

    nav2_bringup_dir = get_package_share_directory("nav2_bringup")
    navigation_launch = os.path.join(nav2_bringup_dir, "launch", "navigation_launch.py")
    rviz_config = os.path.join(
        get_package_share_directory("nav_kit_config"),
        "rviz",
        rviz_config_file,
    )
    if not os.path.isfile(rviz_config):
        raise RuntimeError(f"RViz config not found: {rviz_config}")

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
    if use_rviz_val.lower() not in ("true", "1", "yes"):
        print("[nav2] RViz disabled (use_rviz:=false)")
    else:
        print(f"[nav2] RViz config: {rviz_config}")
    return actions


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="nav2_indoor.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            DeclareLaunchArgument("map_path", default_value=""),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            DeclareLaunchArgument("rviz_config", default_value="nav_known_map.rviz"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
