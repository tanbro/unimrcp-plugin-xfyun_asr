#include "xfyun_asr.h"

mrcp_engine_t* mrcp_plugin_create(apr_pool_t* pool) {
    LOG_NOTICE("[plugin_create] Version: %s. Compiler: %s. Build-Time: %s %s",
               MAKE_STR(VERSION_STRING), MAKE_STR(COMPILER_STRING) __TIME__,
               __DATE__);

    engine_object_t* engine_object =
        (engine_object_t*)apr_palloc(pool, sizeof(engine_object_t));
    apt_task_t* task;
    apt_task_vtable_t* vtable;
    apt_task_msg_pool_t* msg_pool;

    LOG_DEBUG("[plugin_create] create task message pool");
    msg_pool = apt_task_msg_pool_create_dynamic(sizeof(task_msg_t), pool);
    engine_object->task =
        apt_consumer_task_create(engine_object, msg_pool, pool);
    if (!engine_object->task) {
        LOG_ERROR("[plugin_create] create task message pool failed");
        return NULL;
    }
    task = apt_consumer_task_base_get(engine_object->task);
    apt_task_name_set(task, TASK_NAME);
    vtable = apt_task_vtable_get(task);
    if (vtable) {
        vtable->process_msg = task_msg_process;
    } else {
        LOG_ERROR("[plugin_create] cannot find task vtable");
        return NULL;
    }

    /* create engine base */
    LOG_DEBUG("[plugin_create] create engine base");
    mrcp_engine_t* engine = mrcp_engine_create(
        MRCP_RECOGNIZER_RESOURCE,  // MRCP resource identifier
        engine_object,             // object to associate
        &engine_vtable,            // virtual methods table of engine
        pool                       // pool to allocate memory from
    );

    // 返回 engine
    return engine;
}

apt_bool_t plugin_destroy(mrcp_engine_t* engine) {
    LOG_NOTICE("[plugin_destroy]");

    engine_object_t* obj = (engine_object_t*)engine->obj;
    if (obj->task) {
        apt_task_t* task = apt_consumer_task_base_get(obj->task);
        apt_task_destroy(task);
        obj->task = NULL;
    }
    return TRUE;
}

apt_bool_t plugin_open(mrcp_engine_t* engine) {
    LOG_NOTICE("[plugin_open]");

    const char* login_params =
        "appid = 58cb86b4, work_dir = .";  // 登录参数，appid与msc库绑定,请勿随意改动

    int ret = MSPLogin(NULL, NULL, login_params);
    if (MSP_SUCCESS != ret) {
        LOG_ERROR("[plugin_open] MSPLogin failed, error %d", ret);
        return FALSE;
    }

    engine_object_t* obj = (engine_object_t*)engine->obj;
    if (obj->task) {
        apt_task_t* task = apt_consumer_task_base_get(obj->task);
        apt_task_start(task);
    }
    return mrcp_engine_open_respond(engine, TRUE);
}

apt_bool_t plugin_close(mrcp_engine_t* engine) {
    LOG_NOTICE("[plugin_close]");

    int ret = MSPLogout();
    if (MSP_SUCCESS != ret) {
        LOG_WARNING("[plugin_open] MSPLogout failed, error %d", ret);
    }

    engine_object_t* obj = (engine_object_t*)engine->obj;
    if (obj->task) {
        apt_task_t* task = apt_consumer_task_base_get(obj->task);
        apt_task_terminate(task, TRUE);
    }
    return mrcp_engine_close_respond(engine);
}

mrcp_engine_channel_t* channel_create(mrcp_engine_t* engine, apr_pool_t* pool) {
    LOG_INFO("[channel_create]");

    session_t* sess = (session_t*)apr_palloc(pool, sizeof(session_t));
    sess->obj = (engine_object_t*)engine->obj;

    // TODO: 初始化 Session 数据

    /// stream 设置
    mpf_stream_capabilities_t* capabilities;
    mpf_termination_t* termination;

    capabilities = mpf_sink_stream_capabilities_create(pool);
    mpf_codec_capabilities_add(&capabilities->codecs, MPF_SAMPLE_RATE_8000,
                               "LPCM");

    /* create media termination */
    termination = mrcp_engine_audio_termination_create(
        sess,            // object to associate
        &stream_vtable,  // virtual methods table of audio stream
        capabilities,    // stream capabilities
        pool             // pool to allocate memory from
    );

    /* create engine channel base */
    sess->channel = mrcp_engine_channel_create(
        engine,           // engine
        &channel_vtable,  // virtual methods table of engine channel
        sess,             // object to associate
        termination,      // associated media termination
        pool              // pool to allocate memory from
    );

    return sess->channel;
}

apt_bool_t channel_destroy(mrcp_engine_channel_t* channel) {
    LOG_INFO("[channel_destroy] [%s]", channel->id.buf);

    // TODO: 通道结束，处理最终识别结果

    return TRUE;
}

/** Open engine channel (asynchronous response MUST be sent)*/
apt_bool_t channel_open(mrcp_engine_channel_t* channel) {
    LOG_INFO("[channel_open] [%s]", channel->id.buf);
    return task_msg_signal(CHANNEL_OPEN, channel, NULL);
}

/** Close engine channel (asynchronous response MUST be sent)*/
apt_bool_t channel_close(mrcp_engine_channel_t* channel) {
    LOG_INFO("[channel_close] [%s]", channel->id.buf);
    return task_msg_signal(CHANNEL_CLOSE, channel, NULL);
}

/** process engine channel request (asynchronous response MUST be sent)*/
apt_bool_t channel_process_request(mrcp_engine_channel_t* channel,
                                   mrcp_message_t* request) {
    LOG_INFO("[channel_process_request] [%s]", channel->id.buf);
    return task_msg_signal(CHANNEL_PROCESS_REQUEST, channel, request);
}

apt_bool_t stream_destroy(mpf_audio_stream_t* stream) {
    return TRUE;
}

apt_bool_t stream_open_rx(mpf_audio_stream_t* stream, mpf_codec_t* codec) {
    return TRUE;
}

apt_bool_t stream_close_rx(mpf_audio_stream_t* stream) {
    return TRUE;
}

apt_bool_t stream_write_frame(mpf_audio_stream_t* stream,
                              const mpf_frame_t* frame) {
    return TRUE;
}

apt_bool_t task_msg_signal(task_msg_type_e type,
                           mrcp_engine_channel_t* channel,
                           mrcp_message_t* request) {
    apt_bool_t status = FALSE;
    session_t* sess = (session_t*)channel->method_obj;
    engine_object_t* engine_object = (engine_object_t*)sess->obj;
    apt_task_t* task = apt_consumer_task_base_get(engine_object->task);
    apt_task_msg_t* msg = apt_task_msg_get(task);
    if (msg) {
        msg->type = TASK_MSG_USER;
        task_msg_t* task_msg = (task_msg_t*)msg->data;
        task_msg->type = type;
        task_msg->channel = channel;
        task_msg->request = request;
        status = apt_task_msg_signal(task, msg);
    }
    return status;
}

apt_bool_t task_msg_process(apt_task_t* task, apt_task_msg_t* msg) {
    /// 异步任务处理
    /// 该函数的执行已经处于线程池中了
    task_msg_t* task_msg = (task_msg_t*)msg->data;
    session_t* sess = (session_t*)task_msg->channel->method_obj;
    mrcp_engine_channel_t* channel = task_msg->channel;

    switch (task_msg->type) {
        case CHANNEL_OPEN: {
            apt_bool_t status = on_channel_open(sess);
            mrcp_engine_channel_open_respond(channel, status);
        } break;

        case CHANNEL_CLOSE: {
            on_channel_close(sess);
            mrcp_engine_channel_close_respond(channel);
        } break;

        case CHANNEL_PROCESS_REQUEST: {
            on_channel_request(sess, task_msg->request);
        } break;

        default:
            break;
    }

    return TRUE;
}

apt_bool_t on_channel_open(session_t* sess) {
    // TODO:
    return TRUE;
}

void on_channel_close(session_t* sess) {}

void on_channel_request(session_t* sess, mrcp_message_t* request) {}
