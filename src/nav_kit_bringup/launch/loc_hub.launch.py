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

    if "loc_hub" in params:
        node_params = params["loc_hub"].get("ros__parameters", {})
        if "source_topic" not in node_params:
            if topics.get("loc"):
                node_params["source_topic"] = topics["loc"]
        nodes.append(
            Node(
                package="nav_kit",
                executable="loc_hub",
                name="loc_hub",
                output="screen",
                parameters=[node_params],
            )
        )

    return nodes


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="loc_hub.yaml"),
            DeclareLaunchArgument("topics_yaml", default_value="{}"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
