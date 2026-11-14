# NoVanillaMessages

NoVanillaMessages 是一个运行在 LeviLamina 服务端的轻量插件，用于隐藏原版聊天框里的「进服 / 退服 / 入睡 / 跳过夜晚 / 死亡」五类玩家公告，让聊天区只保留玩家真正关心的内容。客户端无需安装资源包，游戏流程不受影响。

## 功能特性

- **进服提示** — 隐藏「xx 加入了游戏」
- **退服提示** — 隐藏「xx 离开了游戏」
- **入睡提示** — 隐藏「xx 正在睡觉」
- **跳过夜晚提示** — 隐藏夜间被跳过时的公告
- **死亡提示** — 隐藏整个 `death.*` 公告家族（摔死、烧死、岩浆、溺水、被生物或玩家击杀等）
- **语言无关** — 任何语言的客户端都能生效
- **零客户端改动** — 纯服务端实现，不需要资源包，也不影响控制台日志

## 配置文件

首次加载后会在 `mods/NoVanillaMessages/config/config.json` 生成配置，修改后重启服务端生效。

### 配置说明

#### 主配置 - config.json

```json
{
    "version": 4,
    "hideJoinMessage": true,      // 隐藏进服提示
    "hideLeaveMessage": true,     // 隐藏退服提示
    "hideSleepMessage": true,     // 隐藏入睡提示
    "hideSkipNightMessage": true, // 隐藏跳过夜晚提示
    "hideDeathMessage": true,     // 隐藏死亡提示
    "debug": false                // 调试模式：输出排查日志
}
```

| 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `version` | int | `4` | 配置版本 |
| `hideJoinMessage` | bool | `true` | 隐藏「xx 加入了游戏」 |
| `hideLeaveMessage` | bool | `true` | 隐藏「xx 离开了游戏」 |
| `hideSleepMessage` | bool | `true` | 隐藏「xx 正在睡觉」 |
| `hideSkipNightMessage` | bool | `true` | 隐藏「跳过夜晚」的公告 |
| `hideDeathMessage` | bool | `true` | 隐藏整个 `death.*` 公告家族 |
| `debug` | bool | `false` | 调试模式 |

### 配置示例

#### 全部隐藏（默认）

```json
{
    "version": 4,
    "hideJoinMessage": true,
    "hideLeaveMessage": true,
    "hideSleepMessage": true,
    "hideSkipNightMessage": true,
    "hideDeathMessage": true,
    "debug": false
}
```

#### 只隐藏死亡与跳过夜晚提示，保留进服/退服/入睡

```json
{
    "version": 4,
    "hideJoinMessage": false,
    "hideLeaveMessage": false,
    "hideSleepMessage": false,
    "hideSkipNightMessage": true,
    "hideDeathMessage": true,
    "debug": false
}
```

## ⚠️ 版本兼容警告

插件版本必须与服务器上的 LeviLamina 版本一致，版本不匹配时插件会在控制台报错并拒绝启用：

```
Built for LeviLamina 26.60 but the server runs 26.51.5: the hooks would patch wrong
addresses, refusing to enable.
```

插件版本号与所适配的 LeviLamina 版本一致，请按服务器版本取用对应的包

## 安装（服务端）

### 使用 LIP

```
lip install github.com/FLAI-Studio/NoVanillaMessages
```

### 手动安装

从 Releases 下载 `NoVanillaMessages-server-windows-x64.zip` 并解压到服务端`./mods`