import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node

sys.path.insert(0, os.path.dirname(__file__))
from config_utils import load_params  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    params_file = LaunchConfiguration("params_file").perform(context)
    profile_context_yaml = LaunchConfiguration("profile_context_yaml").perform(context)
    map_path = LaunchConfiguration("map_path").perform(context)
    use_rviz = LaunchConfiguration("use_rviz")
    rviz_config_file = LaunchConfiguration("rviz_config").perform(context) or "slam_mapping.rviz"

    profile_context = yaml.safe_load(profile_context_yaml) if profile_context_yaml else {}
    params = load_params(params_file)
    nodes = []
    rviz_config = os.path.join(
        get_package_share_directory("nav_kit_config"),
        "rviz",
        rviz_config_file,
    )

    if "slam_toolbox" in params:
        node_params = params["slam_toolbox"].get("ros__parameters", {})
        frames = profile_context.get("frames", {})
        scan_topic = node_params.pop("scan_topic", "/scan")
        pose_topic = node_params.pop("output_topic", "/loc/slam")
        node_params.setdefault("map_frame", frames.get("map", "map"))
        node_params.setdefault("odom_frame", frames.get("odom", "odom"))
        node_params.setdefault("base_frame", frames.get("base", "base_link"))
        nodes.append(
            Node(
                package="slam_toolbox",
                executable="async_slam_toolbox_node",
                name="slam_toolbox",
                output="screen",
                parameters=[node_params],
                remappings=[
                    ("scan", scan_topic),
                    ("pose", pose_topic),
                ],
            )
        )

    if map_path:
        print(f"[slam] map save directory hint: {map_path}")

    nodes.append(
        Node(
            package="rviz2",
            executable="rviz2",
            name="rviz2_slam",
            output="screen",
            arguments=["-d", rviz_config],
            condition=IfCondition(use_rviz),
        )
    )

    return nodes


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="slam_mapping.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            DeclareLaunchArgument("map_path", default_value=""),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            DeclareLaunchArgument("rviz_config", default_value="slam_mapping.rviz"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
