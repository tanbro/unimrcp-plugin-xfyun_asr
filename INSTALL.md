# INSTALL

**‼ 注意**:

> 在安装`uniMRCP Server`之后，再安装本插件。

## 依赖库的安装

- Ubuntu 1604 LTS

所有依赖库都可以通过操作系统的包管理器`apt`安装：

```sh
sudo apt install libxml2
```

- CentOS 7

所有依赖库都可以通过操作系统的包管理器`yum`安装：

```sh
sudo yum install libxml2
```

## 讯飞云[msc]共享库

将讯飞云[msc]SDK的共享库文件`libmsc.so`复制到目标计算机的`/usr/local/lib`，并执行：

```sh
sudo ldconfig
```

**‼ 注意**:

> - 讯飞云[msc]SDK的共享库二进制文件包含有对应的讯飞云`APP`的`ID`。所以，本插件所使用的一个SDK共享库只能对应地使用`ID`相同的`APP`。
> - 确保`/usr/local/lib`在`ld`的搜索路径中。

### 配置文件

首先修改`uniMRCP`的配置(默认路径是`/opt/uniMRCP/conf/unimrcpserver.xml`)，加上本插件。
具体方法是`/unimrcpserver/properties/components/plugin-factory`结点下，加上以下`engine`子结点:

```xml
<engine id="Xfyun-Recog" name="xfyun-asr" enable="true">
</engine>
```

**‼ 注意**:

> 要将其它的语音识别插件的`enable`全部设置为`false`

然后在目标计算机上[uniMRPC]服务的`config`目录(默认路径是`/opt/unimrcp/conf`)，新建名为`plugin-xfyun-asr.xml`的`XML`格式配置文件。
其内容如下:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- 讯飞云在线听写转 uniMRCPServer SpeechRecognize 插件的配置 -->
<!-- 讯飞云在线听写配置 -->
<xfyun-asr>
    <!-- 登录通用接口配置 -->
    <MSPLogin>
        <params>
            <!-- SDK申请成功后获取到的appid。申请SDK请前往http://www.xfyun.cn/，此参数必须传入 -->
            <appid>sat2wqsdfwe</appid>
        </params>
    </MSPLogin>

    <!-- 插件的参数 -->
    <plugin>

        <!-- 线程池设置 -->
        <thread_pool>
            <!-- 初始线程数
            默认： cpu_count
            -->
            <init_threads>4</init_threads>
            <!-- 最大线程数
            默认： cpu_count * 5
            -->
            <max_threads>16</max_threads>
        </thread_pool>

    </plugin>

    <!-- 在线听写会话配置 -->
    <QISRSession>

        <!-- 在线听写的会话初始化参数 -->
        <params>

            <!-- 语言
            可取值：
            zh_cn：简体中文
            zh_tw：繁体中文
            en_us：英文
            默认值：zh_cn
            -->
            <language>zh_cn</language>

            <!-- 语言区域
            可取值：
            mandarin：普通话
            cantonese：粤语
            lmz：四川话
            默认值：mandarin
            -->
            <accent>mandarin</accent>

            <!-- 音频采样率
            可取值：16000，8000
            默认值：8000
            -->
            <sample_rate>8000</sample_rate>

            <!-- 添加标点符号
            0:无标点符号;1:有标点符号。默认为1
            -->
            <ptt>1</ptt>

            <!-- 音频编码格式和压缩等级
            编码算法：raw；speex；speex-wb；ico
            编码等级：raw：不进行压缩。speex系列：0-10；
            云端默认为speex-wb;7
            speex对应sample_rate=8000
            speex-wb对应sample_rate=16000
            ico对应sample_rate=16000
            默认值: raw
            -->
            <aue>raw</aue>

            <!-- VAD功能开关
            是否启用VAD
            默认为开启VAD
            0:不启用;1:启用。默认为1
            -->
            <vad_enable></vad_enable>

            <!-- 允许头部静音的最长时间
            0-10000毫秒。默认为 10000
            如果静音时长超过了此值，则认为用户此次无有效音频输入。此参数仅在打开VAD功能时有效。
            -->
            <vad_bos>10000</vad_bos>

            <!-- 允许尾部静音的最长时间
            0-10000毫秒。默认为 2000
            如果尾部静音时长超过了此值，则认为用户音频已经结束，此参数仅在打开VAD功能时有效。
            -->
            <vad_eos>2000</vad_eos>

        </params>

    </QISRSession>

</xfyun-asr>
```

如果要让这个插件输出日志，可以修改`uniMRCP`的日志配置文件(默认路径是`/opt/unimrcp/conf/log.xml`)，
在`/aptlogger/sources`结点中加上以下`XML`片段:

```xml
<source name="XFYUNASR-PLUGIN" priority="DEBUG" masking="NONE" />
```

[uniMRCP]: http://unimrcp.org/
[xfyun]: http://www.xfyun.cn/
[msc]: https://www.kancloud.cn/iflytek_sdk/iflytek_msc_novoice
