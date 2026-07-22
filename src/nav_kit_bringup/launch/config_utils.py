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
    return load_yaml(profile_path)


def load_mode(mode_name: str) -> dict[str, Any]:
    mode_path = os.path.join(config_share_dir(), "modes", f"{mode_name}.yaml")
    return load_yaml(mode_path)


def load_params(params_file: str) -> dict[str, Any]:
    params_path = os.path.join(config_share_dir(), "params", params_file)
    return load_yaml(params_path)


def params_file_path(params_file: str) -> str:
    return os.path.join(config_share_dir(), "params", params_file)


def profile_context_yaml(
    profile: dict[str, Any], use_sim_time: bool = False, profile_name: str = ""
) -> str:
    return yaml.dump(
        {
            "frames": profile.get("frames", {}),
            "profile": profile_name,
            "use_sim_time": use_sim_time,
        },
        default_flow_style=True,
    )


def resolve_map_path(map_value: str | None) -> str:
    if not map_value:
        return ""
    if os.path.isabs(map_value):
        return map_value
    return os.path.join(config_share_dir(), map_value)


def resolve_map_yaml(map_value: str | None) -> str:
    """Resolve mode map entry to an occupancy grid yaml file path."""
    path = resolve_map_path(map_value)
    if not path:
        return ""
    if path.endswith((".yaml", ".yml")):
        return path
    yaml_path = f"{path}.yaml"
    if os.path.isfile(yaml_path):
        return yaml_path
    if os.path.isfile(path):
        return path
    return yaml_path
