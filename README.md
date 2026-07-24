# NavForge

**ROS2 Navigation System Builder.**

NavForge is a ROS2 navigation deployment framework built around Nav2, providing
reusable configurations, dependencies and launch workflows for mobile robots.

NavForge 面向仿真与真实移动机器人提供可复用的 ROS 2 Humble 2D 导航部署能力。
框架通过 profile、mode 和 params 组合构建导航系统；自研节点负责话题与 frame
规范化，点云转激光、SLAM、AMCL 和 Nav2 等能力通过依赖包按需启用。

ROS 2 包名遵循小写命名约定：

- `navforge`：话题与 TF 规范化节点
- `navforge_bringup`：统一入口和模块化 launch 工作流
- `navforge_config`：机器人 profile、部署 mode、算法参数、地图与 RViz 配置

## 功能模式

| mode | 用途 | 定位 | 模块 |
|------|------|------|------|
| `mapping` | SLAM 建图 | slam_toolbox | static_tf + laserscan + interface + slam |
| `unknown_map_nav` | 未知地图导航 | slam_toolbox | static_tf + laserscan + interface + slam + nav2 |
| `known_map_nav` | 已知地图导航 | AMCL | static_tf + laserscan + interface + amcl + nav2 |

机器狗 L1 使用对应的 `mapping_l1`、`unknown_map_nav_l1`、
`known_map_nav_l1` mode；这些 mode 会自动选择 `profiles/l1.yaml` 和 `_l1` 参数集。

## 快速开始

```bash
./scripts/install_deps.sh
./scripts/build.sh
source install/setup.bash
```

先启动仿真或真机驱动，再 launch 对应 mode。

## 架构

```text
外部环境（仿真 / 真机）
    ├── 传感器 / 定位话题
    │       ↓ params 中写明的 input_topic
    │   laserscan + interface
    │       ↓ /scan、/odom 与动态 TF
    │   slam / amcl / nav2
    │
    └── static_tf（按配置补齐真机缺失的固定坐标变换）
```

### TF 分工

| 模式 | odom→base_link | map→odom |
|------|----------------|----------|
| `mapping` / `unknown_map_nav` | interface | slam_toolbox |
| `known_map_nav` | interface | AMCL |

`map→odom` 只能由一个模块发布：**slam_toolbox 与 AMCL 不可同时运行**。
传感器等固定 frame 由机器人驱动、`robot_state_publisher` 或 `static_tf` 发布，
同一变换只能保留一个发布源。

### 配置分层

| 层级 | 职责 |
|------|------|
| `profiles/*.yaml` | footprint、frames |
| `modes/*.yaml` | 默认 profile/时钟、模块列表、params、`rviz_config`、地图路径 |
| `params/*.yaml` | 话题、frame、算法参数 |
| launch `use_sim_time` | 可覆盖 mode 默认时钟；通用 mode 为 `true`，L1 为 `false` |

### params 一览

| 文件 | 用于 |
|------|------|
| `laserscan.yaml` | 点云→`/scan` |
| `interface_mapping.yaml` | 建图 / 未知地图导航 |
| `interface_nav.yaml` | 已知地图导航 |
| `slam_mapping.yaml` | slam_toolbox（建图与未知地图导航共用） |
| `amcl_indoor.yaml` | AMCL + map_server |
| `nav2_indoor.yaml` | Nav2 规划与控制 |
| `static_tf.yaml` | 真机缺失的静态 TF（可配置任意多条） |

L1 使用同名 `_l1` 参数文件。其中 `nav2_l1.yaml` 使用 MPPI Omni 控制器、
SmacPlanner2D、全向 AMCL 运动模型配套的室外 costmap；`slam_mapping_l1.yaml`
提高了扫描匹配密度并启用长距离轨迹所需的回环检测。

仿真与真机差异：修改 `interface_*.yaml` 中的 `input_topic`。

### 静态 TF 配置

所有 mode 默认都会启动 `static_tf` 模块。编辑 `params/static_tf.yaml` 的
`transforms` 数组即可补齐任意多个固定变换；未配置 `transforms` 时节点不会发布 TF。

```yaml
static_tf_publisher:
  ros__parameters:
    # parent child x y z roll pitch yaw
    # 平移单位：米；旋转单位：弧度
    transforms:
      - "base_link lidar_link 0.20 0.0 0.30 0.0 0.0 0.0"
      - "base_link imu_link 0.00 0.0 0.10 0.0 0.0 1.5708"
```

每个 child frame 只能配置一次。不要配置已由机器人驱动、
`robot_state_publisher`、SLAM 或 AMCL 发布的变换，否则 TF 会产生冲突。

### 地图文件

`src/navforge_config/maps/` 已 **gitignore**，本地存放，不入库。

```bash
./scripts/save_map.sh
# 默认保存：src/navforge_config/maps/<当前 profile>/map.yaml + map.pgm
```

| 用途 | 格式 | 默认路径 |
|------|------|----------|
| 建图保存 | `.yaml` + `.pgm` | `maps/<profile>/map` |
| 已知地图导航 | `.yaml` + `.pgm` | `maps/example/map` |
| L1 建图 / 已知地图 | 同上 | `maps/l1/map` |

建图类 mode 不配置保存路径。`save_map.sh` 会从正在运行的 `topic_relay_odom`
读取 profile；无法读取时回退为 `quadrover`。也可以显式指定 profile、目录和文件名：

```bash
./scripts/save_map.sh --profile l1
./scripts/save_map.sh --output-dir /data/maps --map-name campus
# 兼容旧位置参数：MAP_NAME OUTPUT_DIR
./scripts/save_map.sh campus /data/maps
```

## 启动命令

```bash
# 建图
ros2 launch navforge_bringup navforge.launch.py mode:=mapping

# 未知地图导航（SLAM 定位 + Nav2）
ros2 launch navforge_bringup navforge.launch.py mode:=unknown_map_nav

# 已知地图导航
ros2 launch navforge_bringup navforge.launch.py mode:=known_map_nav

# 指定已知地图
ros2 launch navforge_bringup navforge.launch.py mode:=known_map_nav map:=maps/example/map

# 真机
ros2 launch navforge_bringup navforge.launch.py mode:=unknown_map_nav use_sim_time:=false
```

可选参数：`use_rviz:=false`、`profile:=...`、`use_sim_time:=...`。未显式指定时
使用 mode 中的默认值；通用 mode 使用 `quadrover`/仿真时钟，L1 mode 使用
`l1`/系统时钟。

## L1 机器狗真机

### 部署前必须确认

默认配置假设机器狗驱动提供：

| 接口 | 默认值 | 要求 |
|------|--------|------|
| 地面过滤点云 | `/lidar/points_filtered` | `sensor_msgs/PointCloud2`，保留可碰撞物、去除地面 |
| 融合里程计 | `/odom/filtered` | `nav_msgs/Odometry`，建议 LiDAR/视觉惯性融合 |
| 速度指令 | `/cmd_vel` | 支持 `linear.x`、`linear.y`、`angular.z`，停止时三个分量必须为 `0` |
| TF | `map→odom→base_link` | 只能有一个发布源；传感器固定 TF 必须完整 |

按实际驱动修改 `laserscan_l1.yaml`、`interface_*_l1.yaml`，测量包含载荷后的
机身外廓并同步修改 `profiles/l1.yaml` 与 `nav2_l1.yaml` 中的 footprint。
在 `static_tf_l1.yaml` 中只补充驱动或 URDF 未发布的固定变换。
如果融合里程计自身已经发布 `odom→base_link`，应将 `interface_*_l1.yaml` 的
`publish_tf` 改为 `none`，避免重复 TF。

> 本仓库使用 2D costmap。室外坡地、台阶和负障碍不能仅凭投影后的 `/scan`
> 判断可通行性；送入本工具前必须完成地面分割/地形过滤，并保留独立急停与真机限速。

### 启动

```bash
# 室外建图
ros2 launch navforge_bringup navforge.launch.py \
  mode:=mapping_l1 use_sim_time:=false

# 已探索区域内边建图边导航
ros2 launch navforge_bringup navforge.launch.py \
  mode:=unknown_map_nav_l1 use_sim_time:=false

# 保存地图上的高精度 AMCL 导航
ros2 launch navforge_bringup navforge.launch.py \
  mode:=known_map_nav_l1 use_sim_time:=false map:=maps/l1/map
```

首次上车应架空或低速测试横移方向和急停，再从空旷区域开始。默认速度上限为
前进 `0.4 m/s`、后退 `0.2 m/s`、横移 `0.2 m/s`、转动 `0.6 rad/s`；
目标容差为 `0.10 m / 0.10 rad`。L1 驱动接受的最小非零速度为前后
`0.05 m/s`、横移 `0.10 m/s`、转动 `0.02 rad/s`，对应配置在
`nav2_l1.yaml` 的 `VelocityDeadbandCritic.deadband_velocities` 和
`velocity_smoother.deadband_velocity`。

整体导航速度主要由 `FollowPath` 下的 `vx_max`、`vx_min`、`vy_max`、`wz_max`
决定；`velocity_smoother` 的 `max_velocity`、`min_velocity` 是发布到真机前的最终
硬限制。调整速度时应同步修改这两组值，并优先保持两处限制一致。
先校准里程计、TF、footprint 和时间同步，再调整 MPPI critic 权重；不要通过继续
缩小目标容差掩盖定位漂移。

### 建图

1. 启动 `mapping`，遥控覆盖环境
2. `./scripts/save_map.sh` 保存栅格地图；默认输出到当前 profile 的 `map.yaml` 与 `map.pgm`

### 未知地图导航

1. 启动 `unknown_map_nav`
2. 遥控移动一段，SLAM 建立初始 `/map` 与 `map→odom`
3. 在**已探索区域**发送 Nav2 Goal
4. 无需 2D Pose Estimate

### 已知地图导航

1. 准备 `maps/example/map.yaml` + `map.pgm`
2. 启动 `known_map_nav`
3. RViz **2D Pose Estimate** 设置初始位姿
4. 发送 Nav2 Goal

## 脚本

| 脚本 | 作用 |
|------|------|
| `install_deps.sh` | 安装 apt 依赖 |
| `build.sh` | colcon 编译 |
| `save_map.sh` | 保存栅格地图，支持自定义 profile、目录和文件名 |

## 常见问题

### `/scan` 无数据

`pointcloud_to_laserscan` 懒订阅：打开 RViz 或 `ros2 topic hz /scan` 触发。

### RViz LaserScan 不显示

`/scan` 为 Best Effort。使用 mode 对应的 `rviz_config`，或手动将 Reliability 改为 Best Effort。

### RViz 未弹出（unknown_map_nav）

修改 launch 后需重新 `colcon build` 并 `source install/setup.bash`。

### AMCL 不定位

必须先 **2D Pose Estimate**，等待 AMCL 发布 `map→odom` 后再发 Goal。

### Goal 到达精度

在 `nav2_indoor.yaml` 的 `general_goal_checker` 中调整 `xy_goal_tolerance`（米）与 `yaw_goal_tolerance`（弧度）。
L1 对应 `nav2_l1.yaml` 的 `precise_goal_checker`；实际精度上限首先取决于传感器标定、
时间同步、里程计和全局定位质量。

### `/loc/gazebo` 误用

`/loc/gazebo` 是 `map→base_link` 真值，不能作为 AMCL 的 odom 输入；已知地图导航应使用 `/odom/wheel`（见 `interface_nav.yaml`）。

## 仓库结构

```text
NavForge/
├── scripts/              # install / build / save_map
├── src/
│   ├── navforge/          # C++ nodes and shared headers
│   ├── navforge_bringup/  # launch entrypoint and module launch files
│   └── navforge_config/   # profiles / modes / params / maps / rviz
├── AGENTS.md
└── README.md
```
