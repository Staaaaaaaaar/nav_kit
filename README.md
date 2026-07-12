# nav_kit

可配置的 ROS 2 Humble **2D 导航**工具箱。自研层仅负责**话题与 frame 规范化**；点云转激光等转换通过依赖包按需启用。里程计由外部环境或独立节点提供，nav_kit 不做传感器融合。

## 前提

- Ubuntu 22.04
- [ROS 2 Humble](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html)（`/opt/ros/humble`）
- 仿真：[quadrover_simulation](file:///home/sam/quadrover_simulation) 提供轮速里程计 + 点云
- 真机：IMU + 点云，由**独立里程计节点**发布 `/odom/estimator`

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
    ↓ params 中写明的 input_topic
[可选] laserscan（pointcloud_to_laserscan，2D /scan）
    ↓
interface（topic_relay：frame 重写 + TF）
    ↓ 规范接口 /odom /scan /loc
slam / amcl / nav2（2D 导航依赖包）
```

### 配置分层

| 层级 | 职责 |
|------|------|
| `profiles/*.yaml` | footprint、frames、`use_sim_time` |
| `modes/*.yaml` | 启停模块 + 选用 params 文件 |
| `params/*.yaml` | 输入/输出话题、frame 名、模块行为 |

### profile 示例

```yaml
use_sim_time: true
footprint: [[0.3, 0.2], [0.3, -0.2], [-0.3, -0.2], [-0.3, 0.2]]
frames:
  map: map
  odom: odom
  base: base_link
```

### params 示例

```yaml
topic_relay_odom:
  ros__parameters:
    input_topic: /odom/wheel
    msg_type: odometry
    output_topic: /odom
    parent_frame: odom
    child_frame: base_link
    publish_tf: odom_base
```

仿真与真机的外部话题差异通过**不同 params 文件**区分（如 `interface_phase1.yaml` vs `interface_phase1_real.yaml`），由 mode 选用。

### 模块说明

| 模块 | 类型 | 作用 |
|------|------|------|
| `laserscan` | 依赖包 | 3D 点云 → 2D `/scan` |
| `interface` | 自研 | `topic_relay`：中继 + 统一 frame + TF |
| `slam` | 依赖包 | `slam_toolbox` 2D 建图 |

## 启动

先启动仿真或真机驱动（及里程计节点），再：

```bash
# 仿真 Phase 1
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=phase1 profile:=quadrover_sim

# 真机 Phase 1
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=phase1_real profile:=quadrover_real

# SLAM 建图
ros2 launch nav_kit_bringup nav_kit.launch.py mode:=mapping profile:=quadrover_sim
```

关闭 RViz：`use_rviz:=false`

验收：

```bash
./scripts/phase1_verify.sh
./scripts/mapping_verify.sh    # mapping 模式
./scripts/save_map.sh          # 保存地图
```

### `/scan` 懒订阅

`pointcloud_to_laserscan` 仅当 `/scan` 有订阅者时才订阅点云。运行 `ros2 topic hz /scan` 或打开 RViz 即可触发。

### RViz QoS

`/scan` 为 **Best Effort**。使用 `rviz/phase1.rviz` 或 `rviz/slam_mapping.rviz`，或手动将 LaserScan Reliability 改为 Best Effort。

## 故障排除

### `ros-humble-pointcloud-to-laserscan` 安装 404

```bash
./scripts/install_deps.sh
```
