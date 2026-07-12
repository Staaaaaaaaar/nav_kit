import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

sys.path.insert(0, os.path.dirname(__file__))
from config_utils import get_input_topic, get_input_type, load_params  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    params_file = LaunchConfiguration("params_file").perform(context)
    profile_context_yaml = LaunchConfiguration("profile_context_yaml").perform(context)

    profile_context = yaml.safe_load(profile_context_yaml) if profile_context_yaml else {}
    params = load_params(params_file)
    nodes = []

    lidar_type = get_input_type(profile_context, "lidar", "")
    if lidar_type != "pointcloud2":
        return nodes

    if "pointcloud_to_laserscan" not in params:
        return nodes

    node_params = params["pointcloud_to_laserscan"].get("ros__parameters", {})
    lidar_topic = get_input_topic(profile_context, "lidar", "/lidar/points")
    scan_topic = profile_context.get("topics", {}).get("scan", "/scan")
    frames = profile_context.get("frames", {})
    if "target_frame" not in node_params:
        node_params["target_frame"] = frames.get("base", "base_link")

    nodes.append(
        Node(
            package="pointcloud_to_laserscan",
            executable="pointcloud_to_laserscan_node",
            name="pointcloud_to_laserscan",
            output="screen",
            remappings=[
                ("cloud_in", lidar_topic),
                ("scan", scan_topic),
            ],
            parameters=[node_params],
        )
    )

    return nodes


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="laserscan.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
