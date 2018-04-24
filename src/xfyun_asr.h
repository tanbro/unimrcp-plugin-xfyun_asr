#ifndef _XFYUN_ASR_H_
#define _XFYUN_ASR_H_

///////////////////////////
// gcc/git 产生的版本信息
#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

#ifndef VERSION_STRING
#define VERSION_STRING 0
#endif

#ifndef COMPILER_STRING
#define COMPILER_STRING "c"
#endif
///////////////////////////

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

// unimrcp plugin requires includes
#include "apt_consumer_task.h"
#include "apt_dir_layout.h"
#include "apt_log.h"
#include "mpf_activity_detector.h"
#include "mrcp_recog_engine.h"

// 用到的 APR 功能
#include "apr.h"
#include "apr_file_info.h"
#include "apr_queue.h"
#include "apr_tables.h"
#include "apr_thread_cond.h"
#include "apr_thread_mutex.h"
#include "apr_thread_pool.h"

// 讯飞 MSC SDK
#include "msp_cmn.h"
#include "msp_errors.h"
#include "qisr.h"

// 其它依赖库的头文件
#include <libxml/parser.h>

#define TASK_NAME "XFYUN ASR Engine"

#define ERRSTR_SZ 256
#define IAT_SESSION_ID_LEN 64
#define IAT_BEGIN_PARAMS_LEN 2048
#define IAT_RESULT_STR_LEN 2048

/**
 * 每个识别会话的流媒体缓冲 QUEUE 中，放这个对象
 */
typedef struct _wav_que_obj_t {
    void* data;
    apr_size_t len;
} wav_que_obj_t;

/**
 * The resource engine plugin must declare its version number by using the
 * following helper macro.
 */
MRCP_PLUGIN_VERSION_DECLARE

/**
 * Declare this macro to use log routine of the server, plugin is loaded from.
 * Enable/add the corresponding entry in logger.xml to set a cutsom log source
 * priority. <source name="XFYUNASR-PLUGIN" priority="DEBUG" masking="NONE"/>
 */
MRCP_PLUGIN_LOG_SOURCE_IMPLEMENT(XFYUNASR_PLUGIN, "XFYUNASR-PLUGIN")

/** Use custom log source mark */
#define XFYUNASR_LOG_MARK APT_LOG_MARK_DECLARE(XFYUNASR_PLUGIN)

#define LOGGER "\t[XFYUN_ASR-PLUGIN]\t"
#define LOG(level, fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, level, LOGGER fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_DEBUG, LOGGER fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_INFO, LOGGER fmt, ##__VA_ARGS__)
#define LOG_NOTICE(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_NOTICE, LOGGER fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_WARNING, LOGGER fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_ERROR, LOGGER fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_CRITICAL, LOGGER fmt, ##__VA_ARGS__)
#define LOG_ALERT(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_ALERT, LOGGER fmt, ##__VA_ARGS__)
#define LOG_EMERGENCY(fmt, ...) \
    apt_log(XFYUNASR_LOG_MARK, APT_PRIO_EMERGENCY, LOGGER fmt, ##__VA_ARGS__)

/** 插件的线程池 */
static apr_thread_pool_t* thread_pool = NULL;

/** Declaration of recognizer engine object to associate */
typedef struct _engine_object_t {
    apt_consumer_task_t* task;
} engine_object_t;

/** Declaration of recognizer channel */
typedef struct _session_t {
    /** Back pointer to engine */
    engine_object_t* obj;
    /** Engine channel base */
    mrcp_engine_channel_t* channel;
    /** Active (in-progress) recognition request */
    mrcp_message_t* recog_request;
    /** Pending stop response */
    mrcp_message_t* stop_response;
    /** Indicates whether input timers are started */
    apt_bool_t timers_started;
    /** Voice activity detector */
    mpf_activity_detector_t* detector;

    ////
    apr_thread_cond_t* started_cond;
    apr_thread_mutex_t* started_mutex;
    apr_thread_cond_t* stopped_cond;
    apr_thread_mutex_t* stopped_mutex;

    /** xfyun 听写 session id */
    char* iat_session_id;
    /** xfyun 听写 过程已经结束 */
    bool iat_complted;
    /** wav 识别缓冲 */
    apr_queue_t* wav_queue;
    /** xfyun 的 session 参数 */
    char* iat_begin_params;
    /** xfyun 语音听写结果*/
    char* iat_result;
} session_t;

typedef enum {
    CHANNEL_OPEN,
    CHANNEL_CLOSE,
    CHANNEL_PROCESS_REQUEST
} task_msg_type_e;
/** Declaration of demo recognizer task message */
typedef struct _task_msg_t {
    task_msg_type_e type;
    mrcp_engine_channel_t* channel;
    mrcp_message_t* request;
} task_msg_t;

/**
 * The plugin must implement the creator function of the resource engine, which
 * is the entry point for the run-time loadable library.
 *
 * Note that the function must be declared with the exact signature and the name
 * in order to be found and loaded dynamically at run-time.
 */
MRCP_PLUGIN_DECLARE(mrcp_engine_t*) mrcp_plugin_create(apr_pool_t* pool);

/**
 * The resource engine interface
 *
 */

/**
 * The resource engine is an aggregation of the resource channels. The resource
 * engine is created upon plugin creation and gets destroyed when the plugin is
 * unloaded. The following methods of the resource engine need to be implemented
 * in the plugin.
 */

/** Virtual destroy */
static apt_bool_t plugin_destroy(mrcp_engine_t* engine);
/** Virtual open */
static apt_bool_t plugin_open(mrcp_engine_t* engine);
/** Virtual close */
static apt_bool_t plugin_close(mrcp_engine_t* engine);
/** Virtual channel create */
static mrcp_engine_channel_t* channel_create(mrcp_engine_t* engine,
                                             apr_pool_t* pool);
/** Table of MRCP engine virtual methods */
static const struct mrcp_engine_method_vtable_t engine_vtable = {
    plugin_destroy, plugin_open, plugin_close, channel_create};

/**
 * The resource channel interface
 */

/**
 * The resource channel is created in the scope of the MRCP session and gets
 * destroyed with the session termination. The following methods of the resource
 * channel need to be implemented in the plugin.
 */

/** Virtual destroy */
static apt_bool_t channel_destroy(mrcp_engine_channel_t* channel);
/** Virtual open */
static apt_bool_t channel_open(mrcp_engine_channel_t* channel);
/** Virtual close */
static apt_bool_t channel_close(mrcp_engine_channel_t* channel);
/** Virtual process_request */
static apt_bool_t channel_process_request(mrcp_engine_channel_t* channel,
                                          mrcp_message_t* request);
/** Table of channel virtual methods */
static const struct mrcp_engine_channel_method_vtable_t channel_vtable = {
    channel_destroy, channel_open, channel_close, channel_process_request};

/**
 * The audio stream interface
 *
 */

/**
 * The audio stream interface needs to be implemented in order to process audio
 * data, for example, from the server to the ASR engine or, in reverse
 * direction, from the TTS engine to the server.
 */

/** Virtual destroy method */
static apt_bool_t stream_destroy(mpf_audio_stream_t* stream);
/** Virtual open transmitter method */
static apt_bool_t stream_open_rx(mpf_audio_stream_t* stream,
                                 mpf_codec_t* codec);
/** Virtual close transmitter method */
static apt_bool_t stream_close_rx(mpf_audio_stream_t* stream);
/** Virtual write frame method */
static apt_bool_t stream_write_frame(mpf_audio_stream_t* stream,
                                     const mpf_frame_t* frame);
/** Table of audio stream virtual methods */
static const mpf_audio_stream_vtable_t stream_vtable = {
    stream_destroy,      //
    NULL,                //
    NULL,                //
    NULL,                //
    stream_open_rx,      //
    stream_close_rx,     //
    stream_write_frame,  //
    NULL                 //
};

/**
 * 异步任务处理
 */
static apt_bool_t task_msg_signal(task_msg_type_e type,
                                  mrcp_engine_channel_t* channel,
                                  mrcp_message_t* request);
static apt_bool_t task_msg_process(apt_task_t* task, apt_task_msg_t* msg);

/**
 * 异步任务处理函数
 */
static apt_bool_t on_channel_open(session_t* sess);
static void on_channel_close(session_t* sess);
static void on_channel_request(session_t* sess, mrcp_message_t* request);

static apt_bool_t on_recog_start(session_t* sess,
                                 mrcp_message_t* request,
                                 mrcp_message_t* response);
static apt_bool_t on_recog_start_input_timers(session_t* sess,
                                              mrcp_message_t* request,
                                              mrcp_message_t* response);
static apt_bool_t on_recog_stop(session_t* sess,
                                mrcp_message_t* request,
                                mrcp_message_t* response);

static void* recog_thread_func(apr_thread_t*, void*);

static apt_bool_t term_session(session_t* sess);

/* Raise RECOGNITION-COMPLETE event */
static void emit_recog_result(session_t* sess,
                              mrcp_recog_completion_cause_e cause);

static bool generate_nlsml_result(session_t* sess, mrcp_message_t* message);

#endif