# 重庆市忠县中学 初2029级11班 ClassIsland 集控静态配置仓库

本仓库用于托管 ClassIsland 客户端的静态集控配置文件，实现教室大屏课表、时间表及科目的统一分发与管理。

## 仓库信息
- **学校/班级**：重庆市忠县中学 初2029级11班
- **集控模式**：静态配置文件（Static Manifest，`ServerKind: 0`）
- **托管地址**：`https://raw.githubusercontent.com/wngmay/ZhongXianMiddleSchool-ClassislandControl/main/`
- **适用客户端**：ClassIsland

## 目录结构
```text
.
├── manifest.json               # 集控清单（客户端入口，含各配置 url 与版本号）
├── classplans.json             # 课表 + 课表群
├── timelayouts.json            # 时间表
├── subjects.json               # 科目
├── policy.json                 # 集控策略
├── sync-plugins.json           # 插件同步清单（远程内容同步插件的入口）
├── plugins/                    # 待分发的插件包（.cipx）
├── automation/
│   └── rules.json              # 待分发的自动化规则
├── cilent/
│   └── ManagementPreset.json   # 客户端导入用预设（学生机加入集控用，不参与下发）
├── tools/
│   ├── split_profile.cpp       # 档案拆分工具源码（C++）
│   └── gen-sync-manifest.py    # 生成 sync-plugins.json（扫描 plugins/ 并算 sha256）
└── README.md
```

## 当前配置内容
- **科目 `subjects.json`**：22 个 —— 语文、数学、英语、历史、政治、物理、化学、生物、地理、信息技术、体育、自习、通用技术、音乐、美术、选修课、社团、心理、早读、班会、周测、未知
- **时间表 `timelayouts.json`**：3 套 —— 平常时间表、临周时间表、周后时间表
- **课表 `classplans.json`**：7 份 —— 周一至周日，另含默认叠加课表与「全局课表群」
- **策略 `policy.json`**：`DisableSettingsEditing: false`（未限制客户端编辑设置）

## 更新配置（推荐流程）
1. 在 ClassIsland 客户端【档案编辑】中改好课表、时间表与科目，关闭应用；
2. 用拆分工具把档案 `Default.json`（客户端 `data\Profiles\Default.json`）拆成三份并覆盖本仓库文件：
   ```text
   tools\bin\split_profile.exe "D:\...\data\Profiles\Default.json" <本仓库目录>
   ```
   拆分规则：`Subjects` → `subjects.json`，`TimeLayouts` → `timelayouts.json`，`ClassPlans` + `ClassPlanGroups` → `classplans.json`；其余顶层字段原样保留，不属于该部分的集合字段置为 `{}`；
3. **把 `manifest.json` 中对应 `Source` 的 `Version` 加 1**——不加版本号客户端不会重新拉取；
4. 提交并推送到 `main` 分支，客户端重启后生效。

## 拆分工具（tools/split_profile.cpp）
纯 C++ 实现，仅按顶层字段切分，值按原始文本搬运，不重新序列化 JSON，因此不会改动字段内容。

用法：
```text
split_profile.exe <档案.json> [输出目录]
```

编译（Visual Studio 2026）：
```powershell
Import-Module "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\18\Community" -DevCmdArguments "-arch=x64" -SkipAutomaticLocation
cl /utf-8 /EHsc /std:c++17 /O2 /Fo:tools\bin\ /Fe:tools\bin\split_profile.exe tools\split_profile.cpp
```
`/utf-8` 必需（源码含中文注释，否则编译报 C2059/C2061）。产物在 `tools/bin/`，已被 `.gitignore` 忽略。

## 插件与自动化规则分发（sync-plugins.json）

静态集控本身只能分发课表/时间表/科目/策略，发不了插件和自动化规则。这部分由
[远程内容同步插件 RemoteContentSync](https://github.com/wngmay/ClassIsland-ControlPLUS) 补齐：
教室机每次启动 ClassIsland 时读本仓库的 `sync-plugins.json`，把新版插件包放进待安装目录、
把 `automation/rules.json` 写进配置目录，然后提示「将在下次重启时应用」。

**注意：必须重启两次** —— 第一次下载，第二次由主机完成安装/生效。

### 新增或更新插件

1. 把插件包放进 `plugins/`。来源二选一：
   - 从插件市场/Release 下载现成的 `.cipx`；
   - 打包教室里某台机器已装好的插件（脚本在 ControlPLUS 仓库）：

     ```powershell
     .\pack-installed-plugin.ps1 -All `
       -AppRoot "D:\...\data" -OutDir "E:\ClassIsland-Control\plugins"
     ```

2. 重新生成清单（扫描 `plugins/`，读出包内 `manifest.yml` 的 id/version，算 sha256，Version 自动 +1）：

   ```text
   python tools/gen-sync-manifest.py
   ```

3. 提交并推送到 `main`。

### 更新自动化规则

把客户端 `data\Config\Automations\Default.json` 覆盖到 `automation/rules.json`，
再跑一次 `python tools/gen-sync-manifest.py`（会自动给 Automation.Version +1）。

> 教室机上规则版本变化时，插件会把现有 `Default.json` 备份为 `.bak` 后覆盖写入。

### 客户端首装

插件无法自己装自己，教室机需要人工一次：把 `plugins/school.remote.content.sync-1.0.0.0.cipx`
复制到 `{AppRoot}\Cache\PluginPackages\`，重启 ClassIsland 两次即可。之后它会自己续上。

## 加入集控
1. 将 `cilent/ManagementPreset.json` 放到 ClassIsland 安装目录，启动应用后会自动加载该预设；
2. 或在应用内【集控】界面点击【浏览】手动选择该文件；
3. 填入 ID（建议用班级名，如 `初2029级11班`），连接并确认加入，应用会自动重启生效。

## 注意事项
- 改动任何配置文件内容后，必须给 `manifest.json` 中对应 `Source` 的 `Version` 加 1，否则客户端不会更新。
- 清单地址使用 `main` 分支；若仓库默认分支为 `master`，需一并替换 `manifest.json` 与 `cilent/ManagementPreset.json` 中的 `/main/`。
- `raw.githubusercontent.com` 有 CDN 缓存，推送后可能延迟几分钟生效；国内访问不畅时可改用镜像域名或 Gitee Pages。
- 课表 `Classes` 条数需与其引用时间表中 `TimeType: 0`（上课）的时间点数量一致。
- `CoreVersion` 当前为 `2.0.0.0`，需与客户端集控核心版本匹配。
- `sync-plugins.json` 的 `Version` 同样必须 +1，否则客户端不会重新比对插件版本（用 `gen-sync-manifest.py` 会自动处理）。
- `.cipx` 是 zip，包根必须含 `manifest.yml`；单个包请保持在 20 MB 以内（jsDelivr 限制）。
