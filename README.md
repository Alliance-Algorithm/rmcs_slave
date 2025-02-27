# RMCS Slave

[无下位机控制系统 RMCS（RoboMaster Control System）](https://github.com/Alliance-Algorithm/RMCS) 的下位机固件。

与上位机通过 USB 2.0 FS 端口连接，即可以 BULK 模式实时转发单片机各接口 (CAN/SPI/UART等) 的所有数据至上位机。

不使用操作系统以最大化转发效率，使用无锁同步原语 (LL/SC) 以保证中断安全。

基于 STM32 HAL 和 C++20，部分性能要求的 HAL 代码改为直接寄存器操作。

支持在 Linux 和 Windows 系统上使用。若使用原生Windows（非WSL），则上位机仅能使用 [librmcs](https://github.com/Alliance-Algorithm/librmcs) 。

支持的硬件：

- RoboMaster C型开发板：当前分支
- 达妙 STM32H7 开发板：开发中

支持的接口：

- CAN: 支持 (Classical 1Mbps CAN)
- UART: 支持 (100000 baudrate for DBUS, 115200 for others)
- SPI: 仅 BMI088 (2000Hz Gyroscope & 1600Hz Accelerometer)
- I²C: 暂不支持
- GPIO: 暂不支持

如果有更多的硬件和接口支持需求，可以向我们提 Issue 或 Pull Request。

正在研发队列：

- GPIO 支持
- 自定义的 UART 波特率

## Build

### Github

可以在 [Release](https://github.com/Alliance-Algorithm/rmcs_slave/releases/latest) 中下载构建好的二进制文件。

### Windows

#### 1. 安装 xmake latest

下载并安装 [xmake-v2.9.8.win64.exe](https://github.com/xmake-io/xmake/releases/latest) 或更高版本。

#### 2. 安装 arm-gnu-toolchain latest

下载并安装 [arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi.exe](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 或更高版本。

**注意**：默认安装路径带空格，带空格的路径会导致 clangd 无法正常工作，请手动选择一个不带空格的安装路径，并记录该路径。

#### 3. 克隆 & 配置

在合适的目录下打开终端，克隆仓库并进入：

```ps
git clone https://github.com/Alliance-Algorithm/rmcs_slave.git
```

```ps
cd rmcs_slave
```

接下来配置编译器路径：

```ps
xmake f --sdk="<arm-gnu-toolchain-path>"
```
请将 `<arm-gnu-toolchain-path>` 更改为安装时输入的路径。

例如，若安装时路径为 `C:\arm-none-eabi`，则使用命令：

```ps
xmake f --sdk="C:\arm-none-eabi"
```

路径不需要带 `bin` 目录。

#### 4. 构建

使用命令

```ps
xmake
```

即可触发构建。构建产物位于 `build\cross\arm\release\application.elf`。

#### 5. Commands

使用 `xmake -r` 可强制重新构建。

使用 `xmake -v` 可显示构建过程中的细节，如构建指令、资源占用等。

### Linux

#### 1. 安装 xmake latest

使用命令

```bash
curl -fsSL https://xmake.io/shget.text | bash
```

或

```bash
wget https://xmake.io/shget.text -O - | bash
```

#### 2. 安装 arm-gnu-toolchain latest

ubuntu 可使用 apt 安装

```bash
sudo apt install gcc-arm-none-eabi
```

其它发行版用户可自行查询包管理器。

安装完成后，使用 `arm-none-eabi-gcc -v` 检查 gcc 版本是否大于等于 13.2。

旧版本编译器对 C++20 的支持可能不完备，从而导致编译失败。

#### 3. 克隆 & 配置

重启终端或重新登录，确保环境变量的变更生效。

在合适的目录克隆仓库并进入：

```ps
git clone https://github.com/Alliance-Algorithm/rmcs_slave.git
```

```ps
cd rmcs_slave
```

#### 4. 构建

使用命令

```ps
xmake
```

即可触发构建。构建产物位于 `build/cross/arm/release/application.elf`。

若出现报错 `error: cross toolchain not found!`，请参照 `Build -> Windows` 条目手动配置编译器路径。

#### 5. Commands

使用 `xmake -r` 可强制重新构建。

使用 `xmake -v` 可显示构建过程中的细节，如构建指令、资源占用等。