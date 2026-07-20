# nav_kit

可配置的 ROS 2 Humble **2D 导航**工具箱。自研层仅负责**话题与 frame 规范化**；点云转激光、SLAM、AMCL、Nav2 等通过依赖包按需启用。

## 功能模式

| mode | 用途 | 定位 | 模块 |
|------|------|------|------|
| `mapping` | SLAM 建图 | slam_toolbox | static_tf + laserscan + interface + slam |
| `unknown_map_nav` | 未知地图导航 | slam_toolbox | static_tf + laserscan + interface + slam + nav2 |
| `known_map_nav` | 已知地图导航 | AMCL | static_tf + laserscan + interface + amcl + nav2 |

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
| `profiles/quadrover.yaml` | footprint、frames |
| `modes/*.yaml` | 模块列表、params、`rviz_config`、默认地图路径 |
| `params/*.yaml` | 话题、frame、算法参数 |
| launch `use_sim_time` | 仿真 `true`（默认），真机 `false` |

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

`src/nav_kit_config/maps/` 已 **gitignore**，本地存放，不入库。

```bash
mkdir -p src/nav_kit_config/maps/example
# 已知地图导航：map.yaml + map.pgm
# 建图保存：maps/slam/*.posegraph / *.data
```

| 用途 | 格式 | 默认路径 |
|------|------|----------|
| SLAM 序列图 | `.posegraph` + `.data` | `maps/slam/` |
| 已知地图导航 | `.yaml` + `.pgm` | `maps/example/map` |

## 启动命令

```bash
# 建图
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=mapping

# 未知地图导航（SLAM 定位 + Nav2）
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=unknown_map_nav

# 已知地图导航
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=known_map_nav

# 指定已知地图
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=known_map_nav map:=maps/example/map

# 真机
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=unknown_map_nav use_sim_time:=false
```

可选参数：`use_rviz:=false`、`profile:=quadrover`（默认）

### 建图

1. 启动 `mapping`，遥控覆盖环境
2. `./scripts/save_map.sh` 保存 SLAM 序列图

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
| `save_map.sh` | 保存 SLAM 序列图 |

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

### `/loc/gazebo` 误用

`/loc/gazebo` 是 `map→base_link` 真值，不能作为 AMCL 的 odom 输入；已知地图导航应使用 `/odom/wheel`（见 `interface_nav.yaml`）。

## 仓库结构

```text
nav_kit/
├── scripts/              # install / build / save_map
├── src/
│   ├── nav_kit/          # topic_relay / static_tf_publisher
│   ├── nav_kit_bringup/  # launch
│   └── nav_kit_config/   # profiles / modes / params / rviz
└── README.md
```
