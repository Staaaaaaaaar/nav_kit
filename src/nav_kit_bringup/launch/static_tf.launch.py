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
    params = load_params(params_file)
    node_block = params.get("static_tf_publisher")
    if node_block is None:
        return []
    node_params = node_block.get("ros__parameters", {})

    return [
        Node(
            package="nav_kit",
            executable="static_tf_publisher",
            name="static_tf_publisher",
            output="screen",
            parameters=[node_params] if node_params else [],
        )
    ]


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("params_file", default_value="static_tf.yaml"),
            DeclareLaunchArgument("profile_context_yaml", default_value="{}"),
            OpaqueFunction(function=_launch_setup),
        ]
    )
