# CONTRIBUTING

**‼ 注意**:

> - 目前只支持部分`Linux`发行版。
> - 这个项目使用 [CMake] 进行构建。

## 目录结构

    workspace
        ├── deps
        │   ├── unimrcp-1.5.0
        │   └── xfy_linux_iat
        └── xfyun_asr
            ├── CMakeLists.txt
            ├── README.md
            ├── script
            └── src

## 开发工具

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

## UniMRCP

1. 下载[UniMRCP]最新稳定版(目前是`1.5.0`)主程序源代码，然后解压到工作目录。

    ```sh
    cd path/of/workspace/deps
    wget https://www.unimrcp.org/project/release-view/unimrcp-1-5-0-tar-gz/download
    tar -xf unimrcp-1.5.0.tar.gz
    ```

1. 安装[UniMRCP]依赖库

    从 <https://www.unimrcp.org/project/component-view/dependencies> 下载[UniMRCP]主程序最新稳定版(目前是`1.5.0`)对应的依赖库。

    **注意**:

    > [UniMRCP]使用的依赖库是修改过的，**一定**要使用`UniMRCP`官方提供的与主程序对应的依赖包。

    解压缩依赖库，然后进入目录；根据说明文档，编译并安装依赖库:

    ```sh
    wget https://www.unimrcp.org/project/component-view/dependencies/unimrcp-deps-1-5-0-tar-gz
    tar -xf unimrcp-deps-1.5.0.tar.gz
    cd unimrcp-deps-1.5.0
    sudo ./build-dep-libs.sh
    ```

## 获取代码

1. git 下载

    ```sh
    cd path/of/workspace
    git clone git@bitbucket.org:hesong-core-team/unimrcp-plugin-xfyun_asr.git xfyun_asr
    ```

1. 其它方法获得代码文件，复制到工作目录下的`xfyun_asr`子目录

## 构建

这个项目使用 [CMake] 进行构建。

在项目目录新建一个名为`build`的子目录，专门用于构建。
然后在这个目录中使用[CMake]进行构建:

```sh
cd path/of/workspace/xfyun_asr
mkdir -p build
cd build
cmake ..
make
```

[CMake]: https://cmake.org/
[uniMRCP]: http://unimrcp.org/
[xfyun]: http://www.xfyun.cn/
[msc]: https://www.kancloud.cn/iflytek_sdk/iflytek_msc_novoice
