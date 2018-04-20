# 讯飞 MSC 语音听写 uniMRCPServer Plugin

本项目旨在为[uniMRCP]提供基于[讯飞开放平台](http://www.xfyun.cn/)语音听写功能的语音识别插件。

它使用 [科大讯飞MSC（Mobile Speech Client，移动语音终端）Linux版SDK](https://www.kancloud.cn/iflytek_sdk/iflytek_msc_novoice)

## CONTRIBUTING

**‼ 注意**:

> - 目前只支持部分`Linux`发行版。
> - 这个项目使用 [CMake](https://cmake.org) 进行构建。

### 目录结构

    workspace
    ├── deps
    │   ├── unimrcp-1.5.0
    │   └── xfyun-msc-Linux_iat1166_5acb316c
    └── xfyun_asr
        ├── build
        ├── CMakeLists.txt
        ├── README.md
        ├── INSTALL.md
        ├── CHANGELOG.md
        ├── script
        └── src

### 开发工具

安装开发工具集：

- Ubuntu 1604 LTS

    ```sh
    sudo apt install build-essential autoconf automake libtool pkg-config cmake
    ```

- CentOS 7

    ```sh
    sudo yum groupinstall "development tools"
    sudo yum install cmake
    ```

### UniMRCP

1. 获取[UniMRCP]主程序源代码

    下载[UniMRCP](www.unimrcp.org)最新稳定版(目前是`1.5.0`)主程序源代码，解压缩:

    ```sh
    cd path/of/workspace/deps
    wget https://www.unimrcp.org/project/release-view/unimrcp-1-5-0-tar-gz/download
    tar -xf unimrcp-1.5.0.tar.gz
    ```
2. 安装[UniMRCP]依赖库

    从 <https://www.unimrcp.org/project/component-view/dependencies> 下载 [UniMRCP](www.unimrcp.org) 主程序最新稳定版(目前是`1.5.0`)对应的依赖库。

    **注意**:

    > [UniMRCP](www.unimrcp.org) 使用的依赖库是修改过的，**一定**要使用`UniMRCP`官方提供的与主程序对应的依赖包。

    解压缩依赖库，然后进入目录；根据说明文档，编译并安装依赖库:

    ```sh
    wget https://www.unimrcp.org/project/component-view/dependencies/unimrcp-deps-1-5-0-tar-gz
    tar -xf unimrcp-deps-1.5.0.tar.gz
    cd unimrcp-deps-1.5.0
    sudo ./build-dep-libs.sh
    ```

[uniMRCP]: http://unimrcp.org/
