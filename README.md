# nav_kit

可配置的 ROS 2 Humble **2D 导航**工具箱。自研层仅负责**话题与 frame 规范化**；点云转激光、SLAM、AMCL、Nav2 等通过依赖包按需启用。里程计由外部环境或独立节点提供，nav_kit 不做传感器融合。

## 功能模式

| mode | 用途 | 模块 |
|------|------|------|
| `mapping` | SLAM 建图 | laserscan + interface + slam |
| `known_map_nav` | 已知地图导航 | laserscan + interface + amcl + nav2 |

## 前提

- Ubuntu 22.04
- [ROS 2 Humble](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html)
- 仿真：[quadrover_simulation](file:///home/sam/quadrover_simulation)（轮速里程计 `/odom/wheel`、真值 `/loc/gazebo`、点云 `/lidar/points`）
- 真机：独立里程计节点（如 `/odom/estimator`）+ 点云

## 快速开始

```bash
./scripts/install_deps.sh
./scripts/build.sh
source install/setup.bash
```

## 架构

```text
外部环境（仿真 / 真机）
    ↓ params 中写明的 input_topic
laserscan（pointcloud_to_laserscan → /scan）
    ↓
interface（topic_relay → /odom + TF）
    ↓
slam / amcl / nav2
```

### TF 分工

| 模式 | odom→base_link | map→odom |
|------|----------------|----------|
| `mapping` | interface | slam_toolbox |
| `known_map_nav` | interface | AMCL |

`map` 与 `odom` 帧不可由两个模块同时发布，建图与有图导航不要同时运行。

### 配置分层

| 层级 | 路径 | 职责 |
|------|------|------|
| profile | `profiles/quadrover.yaml` | footprint、frames |
| mode | `modes/*.yaml` | 启停模块、params 文件、默认地图 |
| params | `params/*.yaml` | 话题、frame、算法参数 |
| launch | `use_sim_time` | 仿真 `true`（默认），真机 `false` |

### profile 示例（`profiles/quadrover.yaml`）

```yaml
footprint: [[0.3, 0.2], [0.3, -0.2], [-0.3, -0.2], [-0.3, 0.2]]
frames:
  map: map
  odom: odom
  base: base_link
```

### params 文件

| 文件 | 模块 | 内容 |
|------|------|------|
| `laserscan.yaml` | laserscan | 点云→激光 |
| `interface_mapping.yaml` | interface | 建图：odom 中继 + `/loc` 中继 |
| `interface_nav.yaml` | interface | 导航：odom 中继 + `odom→base` TF |
| `slam_mapping.yaml` | slam | slam_toolbox |
| `amcl_indoor.yaml` | amcl | AMCL + map_server |
| `nav2_indoor.yaml` | nav2 | 规划、控制、代价地图 |

仿真与真机差异：修改 `interface_*.yaml` 中的 `input_topic`（仿真默认 `/odom/wheel`）。

### 地图文件

用户地图保存在 `src/nav_kit_config/maps/`（`*.pgm` / `*.yaml` 已 gitignore，仅本地保留）。

- 建图：`./scripts/save_map.sh` 保存 slam_toolbox 序列图（`.posegraph` / `.data`）
- 导航：AMCL / map_server 需要 Nav2 格式占用栅格（`.yaml` + `.pgm`），需从建图结果导出或单独保存

默认 `known_map_nav` 地图路径：`maps/my_map`（本地需自备 `my_map.yaml` + `my_map.pgm`）。

## 启动

先启动仿真或真机，再：

```bash
# SLAM 建图（仿真，use_sim_time 默认 true）
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=mapping

# 已知地图导航
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=known_map_nav

# 指定地图
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=known_map_nav map:=maps/my_map

# 真机
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=known_map_nav use_sim_time:=false
```

`profile` 默认为 `quadrover`，一般无需指定。关闭 RViz：`use_rviz:=false`

### 建图流程

1. 启动仿真 + `mapping`
2. 遥控机器人覆盖环境
3. `./scripts/mapping_verify.sh` 检查话题
4. `./scripts/save_map.sh` 保存序列图

### 导航流程

1. 准备 Nav2 格式地图（`.yaml` + `.pgm`）于 `maps/` 下
2. 启动仿真 + `known_map_nav`
3. RViz **2D Pose Estimate** 设置初始位姿，等待 AMCL 粒子收敛
4. **Nav2 Goal** 发送目标点
5. `./scripts/nav_verify.sh` 检查基础话题与 Nav2 状态

**注意：** `/loc/gazebo` 是 `map→base_link` 真值，不能代替 `/odom/wheel` 直接接入 AMCL 的 odom 输入，否则 scan 与 map 会不匹配。

## 验收脚本

```bash
./scripts/mapping_verify.sh   # 建图模式
./scripts/nav_verify.sh       # 导航模式
./scripts/save_map.sh         # 保存 SLAM 序列图
```

## 常见问题

### `/scan` 无数据

`pointcloud_to_laserscan` 懒订阅：仅当 `/scan` 有订阅者时才读点云。打开 RViz 或 `ros2 topic hz /scan` 即可触发。

### RViz 中 LaserScan 不显示

`/scan` 为 **Best Effort**。使用 `rviz/slam_mapping.rviz` 或 `rviz/nav_known_map.rviz`，或将 LaserScan Reliability 设为 Best Effort。

### AMCL 不定位 / Nav2 不 ready

必须先 **2D Pose Estimate**。未设初始位姿时 AMCL 不发布 `map→odom`，global_costmap 无法激活，planner 会卡在 Activating。

```bash
ros2 topic echo /amcl_pose --once
ros2 run tf2_ros tf2_echo map odom
ros2 lifecycle get /bt_navigator   # 期望 active [3]
```

### 依赖安装 404

```bash
./scripts/install_deps.sh
```

## 仓库结构

```text
nav_kit/
├── scripts/           # 构建、依赖、验收、存图
├── src/
│   ├── nav_kit/       # topic_relay（C++）
│   ├── nav_kit_bringup/   # launch
│   └── nav_kit_config/    # profiles / modes / params / maps / rviz
└── README.md
```
