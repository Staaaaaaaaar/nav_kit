# nav_kit

可配置的 ROS 2 Humble 导航工具箱（双 Hub：odom_hub + loc_hub）。

## 前提

- Ubuntu 22.04
- [ROS 2 Humble](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html)（`/opt/ros/humble`）
- 仿真或实车需单独提供传感器与 `/odom/wheel`（如 [quadrover_simulation](file:///home/sam/quadrover_simulation)）

## 依赖安装

```bash
./scripts/install_deps.sh          # Phase 1（默认）
./scripts/install_deps.sh --all    # 含后续 amcl / slam / rtk / nav2 规划依赖
```

## 依赖清单

### 构建工具

| apt 包 | 用途 |
|--------|------|
| `python3-colcon-common-extensions` | `colcon build` |
| `ros-humble-ament-cmake` | CMake 构建 |

### Phase 1 — `nav_kit` 节点

| apt 包 | 用途 |
|--------|------|
| `ros-humble-rclcpp` | C++ 节点运行时 |
| `ros-humble-nav-msgs` | `Odometry` 等 |
| `ros-humble-sensor-msgs` | `Imu` 等 |
| `ros-humble-geometry-msgs` | 位姿/四元数 |
| `ros-humble-tf2` | TF 数学 |
| `ros-humble-tf2-ros` | TF 广播 |
| `ros-humble-tf2-geometry-msgs` | 消息转换 |

### Phase 1 — `nav_kit_bringup` 启动与 processors

| apt 包 | 用途 |
|--------|------|
| `ros-humble-launch` | launch 系统 |
| `ros-humble-launch-ros` | ROS 节点 launch |
| `ros-humble-ament-index-python` | 查找 `nav_kit_config` 等资源 |
| `python3-yaml` | `config_utils.py` 读 YAML |
| `ros-humble-pointcloud-to-laserscan` | 3D 点云 → `/scan`（**Phase 1 启动必需**） |
| `ros-humble-laser-geometry` | pointcloud_to_laserscan 依赖 |
| `ros-humble-tf2-sensor-msgs` | pointcloud_to_laserscan 依赖 |

### 后续 Phase（`install_deps.sh --all`）

| apt 包 | 计划用途 |
|--------|----------|
| `ros-humble-navigation2` / `ros-humble-nav2-bringup` | Phase 3+ 室内/室外导航 |
| `ros-humble-slam-toolbox` | Phase 4 建图与定位 |
| `ros-humble-robot-localization` | Phase 5 RTK / EKF |
| `ros-humble-rviz2` | Phase 6 可视化 |

## 构建

```bash
./scripts/build.sh
source install/setup.bash
```

## Phase 1 启动（processors + odom_hub）

先启动仿真（或实车驱动），再：

```bash
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=phase1 profile:=quadrover_sim
```

验收：

- 话题：`/odom/imu`、`/scan`、`/odom`
- TF：`odom → base_link`

```bash
ros2 topic list | grep -E 'odom|scan'
ros2 run tf2_ros tf2_echo odom base_link
./scripts/phase1_verify.sh
```

### `/scan` 与 rqt 中看不到 `/lidar/points` 订阅

`pointcloud_to_laserscan`（上游包）采用**懒订阅**：只有当 **`/scan` 已有订阅者** 时，才会订阅 `/lidar/points`。仅打开 rqt_graph 而不订阅 `/scan` 时，图上不会显示对点云的订阅，这是正常现象。

触发转换并验收：

```bash
ros2 topic hz /scan          # 或 RViz 添加 LaserScan 显示 /scan
ros2 topic hz /lidar/points  # 此时应能看到 pointcloud_to_laserscan 在订阅
```

后续 AMCL / SLAM 启动后会订阅 `/scan`，无需额外处理。

### RViz 中 `/scan` 不显示

终端若出现 `incompatible QoS ... RELIABILITY_QOS_POLICY`，原因是：

| 端 | QoS |
|----|-----|
| `pointcloud_to_laserscan` 发布 `/scan` | **Best Effort**（传感器默认） |
| RViz LaserScan 手动添加时 | **Reliable**（RViz 默认） |

二者不匹配时，RViz **收不到任何数据**（即使 launch 日志显示已启动 pointcloud 订阅）。

**推荐**：使用已配好 QoS 的配置文件：

```bash
rviz2 -d $(ros2 pkg prefix nav_kit_config)/share/nav_kit_config/rviz/phase1.rviz
```

**或**在 RViz 中手动添加 LaserScan → Topic 设为 `/scan` → **Reliability Policy 改为 Best Effort**。Fixed Frame 用 `base_link`（或 `odom`）。

## 故障排除

### `ros-humble-pointcloud-to-laserscan` 安装 404

本地 apt 索引可能仍指向已下架的 deb 版本（例如 `...20260422...`），而官方仓库已发布新版本。重新运行：

```bash
./scripts/install_deps.sh
```

脚本会清除 ROS 源缓存、强制 `apt update`，若仍失败则从 `packages.ros.org` 拉取当前 deb 直接安装。
