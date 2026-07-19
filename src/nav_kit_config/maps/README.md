# Maps

本地地图目录。`.pgm`、`.yaml`、`.data`、`.posegraph` 已在仓库 `.gitignore` 中忽略。

## 建图输出

`./scripts/save_map.sh` 默认写入 `maps/quadrover/indoor/`，生成 slam_toolbox 序列图：

- `*.posegraph`
- `*.data`

## 导航输入

`known_map_nav` 需要 Nav2 占用栅格：

- `my_map.yaml`（元数据 + `image:` 指向 pgm）
- `my_map.pgm`

在 mode 或 launch 中通过 `map:=maps/my_map` 指定（可省略 `.yaml` 后缀）。
