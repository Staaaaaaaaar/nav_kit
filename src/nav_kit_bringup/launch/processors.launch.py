import os
import sys

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

sys.path.insert(0, os.path.dirname(__file__))
from config_utils import load_params  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    params_file = LaunchConfiguration("params_file").perform(context)
    topics_yaml = LaunchConfiguration("topics_yaml").perform(context)

    import yaml

    topics = yaml.safe_load(topics_yaml) if topics_yaml else {}
    params = load_params(params_file)
    nodes = []

    if "imu_integrator" in params:
        node_params = params["imu_integrator"].get("ros__parameters", {})
        if "input_topic" not in node_params and topics.get("imu"):
            node_params["input_topic"] = topics["imu"]
        nodes.append(
            Node(
                package="nav_kit",
                executable="imu_integrator",
                name="imu_integrator",
                output="screen",
                parameters=[node_params],
            )
        )

    if "pointcloud_to_laserscan" in params:
        node_params = params["pointcloud_to_laserscan"].get("ros__parameters", {})
        lidar_topic = topics.get("lidar", "/lidar/points")
        scan_topic = topics.get("scan", "/scan")
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
            DeclareLaunchArgument("params_file", default_value="processors.yaml"),
            DeclareLaunchArgument("topics_yaml", default_value="{}"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
