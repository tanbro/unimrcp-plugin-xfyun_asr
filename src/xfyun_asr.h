#ifndef _XFYUN_ASR_H_
#define _XFYUN_ASR_H_

#include <stdbool.h>
#include <string.h>

// unimrcp plugin requires includes
#include "apt_consumer_task.h"
#include "apt_dir_layout.h"
#include "apt_log.h"
#include "mpf_activity_detector.h"
#include "mrcp_recog_engine.h"

// 用到的 APR 功能
#include "apr.h"
#include "apr_file_info.h"
#include "apr_xml.h"

// 讯飞 MSC SDK
#include "msp_cmn.h"
#include "msp_errors.h"
#include "qisr.h"

#define TASK_NAME "XFYUN ASR Engine"

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
#define BDSR_LOG_MARK APT_LOG_MARK_DECLARE(XFYUNASR_PLUGIN)

/** Declaration of recognizer engine object to associate */
typedef struct _recog_plugin_engine_t {
    apt_consumer_task_t* task;
} recog_plugin_engine_t;

/** Declaration of recognizer channel */
typedef struct _session_t {
    /** Back pointer to engine */
    recog_plugin_engine_t* obj;
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
} session_t;

typedef enum {
    PLUGIN_MSG_CHANNEL_OPEN,
    PLUGIN_MSG_CHANNEL_CLOSE,
    PLUGIN_MSG_CHANNEL_PROCESS_REQUEST
} plugin_msg_type_e;
/** Declaration of demo recognizer task message */
typedef struct _plugin_msg_t {
    plugin_msg_type_e type;
    mrcp_engine_channel_t* channel;
    mrcp_message_t* request;
} plugin_msg_t;

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
static apt_bool_t bdsr_engine_destroy(mrcp_engine_t* engine);
/** Virtual open */
static apt_bool_t bdsr_engine_open(mrcp_engine_t* engine);
/** Virtual close */
static apt_bool_t bdsr_engine_close(mrcp_engine_t* engine);
/** Virtual channel create */
static mrcp_engine_channel_t* bdsr_channel_create(mrcp_engine_t* engine,
                                                  apr_pool_t* pool);
/** Table of MRCP engine virtual methods */
static const struct mrcp_engine_method_vtable_t bdsr_engine_vtable = {
    bdsr_engine_destroy, bdsr_engine_open, bdsr_engine_close,
    bdsr_channel_create};

#endif