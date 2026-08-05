# 灵犀副屏（LinkScreen）

灵犀副屏是一套由 Windows 主机端和 HarmonyOS NEXT 平板端组成的远程副屏软件。它的目标是让 HarmonyOS 平板显示 Windows 的第二块桌面，并将触摸、键盘和手写笔操作回传给电脑。

项目当前处于设计与基础工程阶段。

## 产品目标

- 支持 Windows 11 x64 主机。
- 支持 HarmonyOS 6.1 平板。
- 在同一局域网内自动发现和安全配对设备。
- 支持低延迟 H.264 画面传输。
- 支持触摸、鼠标滚动和键盘操作回传。
- 最终通过 Windows 虚拟显示驱动提供真正的扩展桌面。
- 形成能够安装、测试和发布的完整应用。

## 开发路线

1. 建立 Windows 与 HarmonyOS 工程骨架。
2. 实现局域网设备发现、配对和控制通道。
3. 实现 Windows 屏幕采集、编码和发送。
4. 实现 HarmonyOS 硬件解码与全屏显示。
5. 实现触控、滚动和键盘事件回传。
6. 加入 Windows IddCx 虚拟显示驱动。
7. 完成安全、安装、签名、测试和上架工作。

## 文档

- [ADR-0001：技术栈与总体架构](docs/architecture/ADR-0001-technology-stack.md)
- [LinkScreen Wire Protocol v1](docs/protocol/wire-protocol-v1.md)

## 文档约定

重要技术决定使用 ADR（Architecture Decision Record，架构决策记录）保存。ADR 不用于描述每天做了什么，而是回答以下问题：

- 我们决定采用什么方案？
- 为什么采用它？
- 曾考虑哪些替代方案？
- 这个决定带来哪些收益和代价？

已经生效的 ADR 原则上不直接改写结论。方案发生重大变化时，应增加一份新 ADR，并将旧 ADR 标记为“已取代”，以保留决策历史。

## Windows 开发构建

在仓库根目录执行：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --preset windows-debug
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build --preset windows-debug
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --preset windows-debug
```

当前开发主机只监听 `127.0.0.1:47831`。分别在两个终端运行：

```powershell
.\build\windows-debug\native\windows-engine\Debug\linkscreen_host_cli.exe
```

```powershell
.\build\windows-debug\native\windows-engine\Debug\linkscreen_handshake_client.exe
```

主机完成一次 `Hello` / `HelloAck` 握手后退出。配对、认证和加密完成前，不允许将监听地址改为所有网络接口。
