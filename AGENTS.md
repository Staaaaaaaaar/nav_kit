# Repository Guidelines

## Project Structure & Module Organization

NavForge is a ROS 2 Humble navigation deployment framework built around Nav2 for
simulation and real mobile robots.

- `src/navforge/`: C++ nodes. Interfaces live under `src/interface/`; TF utilities live under `src/tf/`; shared headers belong in `include/navforge/`.
- `src/navforge_bringup/launch/`: Python launch files. `navforge.launch.py` selects a mode and includes module launch files.
- `src/navforge_config/`: robot profiles, mode composition, node parameters, RViz configurations, and local maps.
- `scripts/`: dependency installation, workspace builds, and map-saving helpers.

Keep algorithm parameters in `params/`, robot-specific frames in `profiles/`, and module selection in `modes/`. Map files under `src/navforge_config/maps/` are local data and are intentionally ignored.

## Build, Test, and Development Commands

```bash
./scripts/install_deps.sh                 # Install ROS 2/Nav2 dependencies
./scripts/build.sh                        # Build with colcon and symlink install
source install/setup.bash                 # Activate the built workspace
colcon test --packages-select navforge navforge_bringup navforge_config
colcon test-result --verbose              # Display test failures
```

Run a representative mode after building:

```bash
ros2 launch navforge_bringup navforge.launch.py \
  mode:=mapping use_sim_time:=true use_rviz:=false
```

Use `use_sim_time:=false` on real hardware. If a build cache was created from another path, rebuild with `./scripts/build.sh --cmake-clean-cache`.

## Coding Style & Naming Conventions

Use C++17-compatible ROS 2 code, two-space C++/CMake indentation, and four-space Python indentation. Follow existing ROS naming: `snake_case` for files, executables, parameters, and node names; `PascalCase` for C++ classes. Keep launch modules named `<module>.launch.py` and parameter files descriptive, such as `interface_nav.yaml`. Preserve compiler warnings (`-Wall -Wextra -Wpedantic`) and run `git diff --check` before committing.

## Testing Guidelines

There is currently no dedicated automated test suite. Every change must at least compile affected packages, parse modified YAML/Python launch files, and smoke-test the relevant launch mode. For TF changes, verify `/tf` or `/tf_static` and ensure each child frame has only one publisher. Add future tests through standard ament tooling and name them after the behavior under test.

## Commit & Pull Request Guidelines

History follows Conventional Commits, usually with scopes: `feat(tf): ...`, `feat(nav): ...`, `refactor(config): ...`, and `chore(maps): ...`. Keep commits focused and imperative. Pull requests should explain the affected mode, list validation commands, call out configuration or TF-tree changes, link related issues, and include RViz screenshots only when visualization behavior changes.
