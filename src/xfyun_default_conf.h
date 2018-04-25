#ifndef _XFYUN_DEFAULT_CONF_H_
#define _XFYUN_DEFAULT_CONF_H_

const char* DEFAULT_CONF_MSPLOGIN_PARAMS[][2] = {
    /**
     * SDK申请成功后获取到的appid
     *
     * 申请SDK请前往http://www.xfyun.cn/，此参数必须传入
     */
    {"appid", ""},     //
    {"work_dir", "."}  //
};

const char* DEFAULT_CONF_PLUGIN_THREADPOOL[][2] = {
    /**
     * 初始线程数
     *
     * default cpu_count
     */
    {"init_threads", ""},  //
    /**
     * 最大线程数
     *
     * 默认： cpu_count * 5
     */
    {"max_threads", ""}  //
};

const char* DEFAULT_CONF_QISRSession_PARAMS[][2] = {
    /**
     * 语言
     *
     * 可取值：
     * zh_cn：简体中文
     * zh_tw：繁体中文
     * en_us：英文
     * 默认值： zh_cn
     */
    {"language", "zh_cn"},  //
    /**
     * 语言区域
     *
     * 可取值：
     * mandarin：普通话
     * cantonese：粤语
     * lmz：四川话
     * 默认值： mandarin
     */
    {"accent", "mandarin"},  //
    /**
     * 音频采样率
     *
     * 可取值：16000，8000
     * 默认值： 8000
     */
    {"sample_rate", "8000"},  //
    /**
     * 添加标点符号
     *
     * 0:无标点符号;1:有标点符号。默认为1
     */
    {"ptt", "1"},  //
    /**
     * 频编码格式和压缩等级
     *
     * 编码算法：raw；speex；speex-wb；ico
     * 编码等级：raw：不进行压缩。speex系列：0-10；
     * xfyun 默认为speex-wb;7
     * speex对应sample_rate=8000
     * speex-wb对应sample_rate=16000
     * ico对应sample_rate=16000
     * 默认值: raw
     */
    {"aue", "raw"},  //
    /**
     * VAD功能开关
     *
     * 是否启用VAD
     * 默认为开启VAD
     * 0:不启用;1:启用。默认为1
     */
    {"vad_enable", "1"},  //
    /**
     * 允许头部静音的最长时间
     *
     * 0-10000毫秒。默认为 10000
     * 如果静音时长超过了此值，则认为用户此次无有效音频输入。此参数仅在打开VAD功能时有效。
     */
    {"vad_bos", "10000"},  //
    /**
     * 允许尾部静音的最长时间
     *
     * 0-10000毫秒。默认为 2000
     * 如果尾部静音时长超过了此值，则认为用户音频已经结束，此参数仅在打开VAD功能时有效。
     */
    {"vad_eos", "2000"}  //
};

#endif
