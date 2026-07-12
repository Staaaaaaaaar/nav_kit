"""Load nav_kit_config YAML files for launch files."""

from __future__ import annotations

import os
from typing import Any

import yaml
from ament_index_python.packages import get_package_share_directory


def config_share_dir() -> str:
    return get_package_share_directory("nav_kit_config")


def load_yaml(path: str) -> dict[str, Any]:
    with open(path, "r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle)
    return data or {}


def load_profile(profile_name: str) -> dict[str, Any]:
    profile_path = os.path.join(config_share_dir(), "profiles", f"{profile_name}.yaml")
    profile = load_yaml(profile_path)
    robot_name = profile.get("robot", "quadrover")
    robot_path = os.path.join(config_share_dir(), "robots", f"{robot_name}.yaml")
    robot = load_yaml(robot_path)
    merged = {**robot, **profile}
    return merged


def load_mode(mode_name: str) -> dict[str, Any]:
    mode_path = os.path.join(config_share_dir(), "modes", f"{mode_name}.yaml")
    return load_yaml(mode_path)


def load_params(params_file: str) -> dict[str, Any]:
    params_path = os.path.join(config_share_dir(), "params", params_file)
    return load_yaml(params_path)


def profile_context_yaml(profile: dict[str, Any]) -> str:
    return yaml.dump(
        {
            "inputs": profile.get("inputs", {}),
            "frames": profile.get("frames", {}),
            "topics": profile.get("topics", {}),
        },
        default_flow_style=True,
    )


def _input_spec(inputs: dict[str, Any], input_key: str) -> dict[str, Any]:
    spec = inputs.get(input_key, {})
    if isinstance(spec, str):
        return {"topic": spec}
    return spec if isinstance(spec, dict) else {}


def apply_interface_context(
    relay_params: dict[str, Any],
    profile_context: dict[str, Any],
) -> dict[str, Any]:
    """Inject profile inputs and robot frames/topics into topic_relay parameters."""
    inputs = profile_context.get("inputs", {})
    frames = profile_context.get("frames", {})
    topics = profile_context.get("topics", {})
    merged = dict(relay_params)

    input_key = merged.get("input_key")
    if input_key and not merged.get("input_topic"):
        spec = _input_spec(inputs, input_key)
        if spec.get("topic"):
            merged["input_topic"] = spec["topic"]

    output_key = merged.get("output_topic_key")
    if output_key and not merged.get("output_topic"):
        if topics.get(output_key):
            merged["output_topic"] = topics[output_key]

    parent_key = merged.pop("parent_frame_key", None)
    child_key = merged.pop("child_frame_key", None)
    if parent_key:
        merged.setdefault("parent_frame", frames.get(parent_key, parent_key))
    if child_key:
        merged.setdefault("child_frame", frames.get(child_key, child_key))

    merged.setdefault("map_frame", frames.get("map", "map"))
    merged.setdefault("odom_frame", frames.get("odom", "odom"))

    if merged.get("publish_tf") == "map_odom" and not merged.get("odom_topic"):
        merged["odom_topic"] = topics.get("odom", "/odom")

    return merged


def get_input_topic(profile_context: dict[str, Any], input_key: str, default: str = "") -> str:
    spec = _input_spec(profile_context.get("inputs", {}), input_key)
    return spec.get("topic", default)


def get_input_type(profile_context: dict[str, Any], input_key: str, default: str = "") -> str:
    spec = _input_spec(profile_context.get("inputs", {}), input_key)
    return spec.get("type", default)


def resolve_map_path(map_value: str | None) -> str:
    if not map_value:
        return ""
    if os.path.isabs(map_value):
        return map_value
    return os.path.join(config_share_dir(), map_value)
