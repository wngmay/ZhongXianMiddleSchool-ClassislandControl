# 重庆市忠县中学 初2029级11班 ClassIsland 集控静态配置仓库

本仓库用于托管 ClassIsland 客户端的静态集控配置文件，实现教室大屏课表、时间表及科目的统一分发与管理。

## 仓库信息
- **学校/班级**：重庆市忠县中学 初2029级11班
- **集控模式**：静态配置文件（Static Manifest）
- **适用客户端**：ClassIsland

## 目录结构
```text
.
├── manifest.json          # 集控清单主文件（客户端入口）
├── classplans.json        # 课表配置
├── timelayouts.json       # 时间表配置
├── subjects.json          # 科目配置
├── policy.json            # 集控策略（限制客户端功能）
├── ManagementPreset.json  # 客户端导入用预设（不下发，仅用于加入集控）
└── README.md              # 本说明文件
```

## 加入集控
1. 将 `ManagementPreset.json` 放到 ClassIsland 安装目录，启动应用后会自动加载该预设；
2. 或在应用内【集控】界面点击【浏览】手动选择该文件；
3. 填入 ID（建议用班级名，如 `初2029级11班`），连接并确认加入，应用会自动重启生效。

## 注意事项
- 改了任何配置文件的内容后，必须把 `manifest.json` 中对应 `Source` 的 `Version` 加 1，否则客户端不会重新拉取。
- 清单中的地址使用 `main` 分支，若仓库默认分支为 `master`，需一并替换。
- `raw.githubusercontent.com` 有 CDN 缓存，推送后可能需要几分钟才生效；国内访问不畅时可改用镜像域名或 Gitee Pages。
- 课表 `Classes` 条数需与时间表中 `TimeType: 0`（上课）的时间点数量一致，当前为 8 节。
