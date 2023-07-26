# 讯飞 MSC 语音听写 uniMRCPServer Plugin

本项目旨在为 [uniMRCP][] 提供基于讯飞开放平台([xfyun][])语音听写([msc][])功能的语音识别插件。

它使用 [科大讯飞MSC（Mobile Speech Client，移动语音终端）Linux版SDK](https://www.kancloud.cn/iflytek_sdk/iflytek_msc_novoice)

该项目的的初始目的是为[广州和声][]的CTI服务程序 IPSC 提供第三方 [uniMRCP][] speech-recognition 插件。在 IPSC 已经接入 Azure, GCP, AWS, Baidu 等多个 speech-recognition 服务后，这个项目已经不再被 IPSC 使用。

学习者可以利用这个项目了解:

1. 使用 C 而不是 C++ 编写简单高效的 [uniMRCP][] speech-recognition 插件
1. 使用 CMake 管理 C 简单的项目

版权和许可信息详见 [LICENSE](LICENSE.txt) 文件。

> **Note:**
>
> 讯飞开放平台[msc][] Linux SDK 限制了**一个并发**，这个项目**并未**提供多并发的机制我们采用多进程方法。
> 在 IPSC 的实际商用版本中，我们采取的方式是：插件启动若干执行进程，每个执行进程进行一个识别的并发。插件和执行程序之间通过 IPC 进行交互控制。

[uniMRCP]: http://unimrcp.org/
[xfyun]: http://www.xfyun.cn/
[msc]: https://www.kancloud.cn/iflytek_sdk/iflytek_msc_novoice
[广州和声]: https://www.hesong.net/
