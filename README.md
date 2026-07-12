# nav_kit

可配置的 ROS 2 Humble **2D 导航**工具箱。自研层仅负责**话题与 frame 规范化**；点云转激光等转换通过依赖包按需启用。里程计由外部环境或独立节点提供，nav_kit 不做传感器融合。

## 前提

- Ubuntu 22.04
- [ROS 2 Humble](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html)（`/opt/ros/humble`）
- 仿真：[quadrover_simulation](file:///home/sam/quadrover_simulation) 提供轮速里程计 + 点云
- 真机：IMU + 点云，由**独立里程计节点**订阅传感器后发布 `/odom/estimator`（话题名可在 profile 中配置）

## 依赖安装

```bash
./scripts/install_deps.sh
```

## 构建

```bash
./scripts/build.sh
source install/setup.bash
```

## 架构

```text
外部环境（仿真 / 真机驱动 / 里程计节点）
    ↓ profiles.inputs（话题映射）
[可选] laserscan（pointcloud_to_laserscan，2D /scan）
    ↓
interface（topic_relay：frame 重写 + TF）
    ↓ 规范接口 /odom /scan /loc
slam / amcl / nav2（2D 导航依赖包）
```

### 配置分层

| 层级 | 职责 |
|------|------|
| `robots/*.yaml` | footprint、标准 frame、规范输出话题 |
| `profiles/*.yaml` | 外部环境 inputs（仿真/真机差异） |
| `modes/*.yaml` | 启停模块 + params 文件 |
| `params/*.yaml` | 依赖包与 topic_relay 行为 |

### 仿真 vs 真机 profile

| 输入 | `quadrover_sim` | `quadrover_real` |
|------|-----------------|------------------|
| IMU | `/imu/data` | `/imu/data` |
| 激光 | `/lidar/points`（pointcloud2 → laserscan） | `/lidar/points`（同上） |
| 里程计 | `/odom/wheel`（仿真轮速） | `/odom/estimator`（外部节点输出） |
| 全局定位 | `/loc/gazebo`（phase2） | 暂未配置 |

### 模块说明

| 模块 | 类型 | 作用 |
|------|------|------|
| `laserscan` | 依赖包 | 3D 点云 → 2D `/scan`（`inputs.lidar.type: pointcloud2` 时启用） |
| `interface` | 自研 | `topic_relay`：中继 + 统一 frame + TF |
| `slam` | 依赖包 | `slam_toolbox` 2D 建图 |

### 标准内部接口（2D 导航）

| 话题 | 类型 | frame |
|------|------|-------|
| `/odom` | Odometry | `odom` → `base_link` |
| `/scan` | LaserScan | `base_link` |
| `/loc` | Odometry | `map` → `base_link` |

## 启动

先启动仿真或真机驱动（及里程计节点），再：

```bash
# 仿真 Phase 1
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=phase1 profile:=quadrover_sim

# 真机 Phase 1（需外部里程计节点已发布 /odom/estimator）
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=phase1 profile:=quadrover_real
```

验收：

```bash
ros2 topic list | grep -E 'odom|scan'
ros2 run tf2_ros tf2_echo odom base_link
./scripts/phase1_verify.sh
```

### `/scan` 懒订阅

`pointcloud_to_laserscan` 仅当 `/scan` 有订阅者时才订阅点云。运行 `ros2 topic hz /scan` 或打开 RViz 即可触发。

### RViz QoS

`/scan` 为 **Best Effort**。使用 `rviz/phase1.rviz`，或手动将 LaserScan Reliability 改为 Best Effort。

## 故障排除

### `ros-humble-pointcloud-to-laserscan` 安装 404

```bash
./scripts/install_deps.sh
```
