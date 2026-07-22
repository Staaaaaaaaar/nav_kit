import os
import sys

import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

sys.path.insert(0, os.path.dirname(__file__))
from config_utils import load_params  # noqa: E402


def _launch_setup(context, *args, **kwargs):
    params_file = LaunchConfiguration("params_file").perform(context)
    profile_context_yaml = LaunchConfiguration("profile_context_yaml").perform(context)
    profile_context = yaml.safe_load(profile_context_yaml) if profile_context_yaml else {}
    params = load_params(params_file)
    nodes = []

    for node_name, node_block in params.items():
        if not node_name.startswith("topic_relay"):
            continue
        node_params = node_block.get("ros__parameters", {})
        node_params.setdefault("nav_kit_profile", profile_context.get("profile", ""))
        nodes.append(
            Node(
                package="nav_kit",
                executable="topic_relay",
                name=node_name,
                output="screen",
                parameters=[node_params],
            )
        )

    return nodes


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="interface_nav.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
