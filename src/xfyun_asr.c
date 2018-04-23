#include "xfyun_asr.h"

mrcp_engine_t* mrcp_plugin_create(apr_pool_t* pool) {
    LOG_NOTICE("[plugin_create] Version: %s. Compiler: %s. Build-Time: %s %s",
               MAKE_STR(VERSION_STRING), MAKE_STR(COMPILER_STRING), __TIME__,
               __DATE__);

    apr_status_t status = APR_SUCCESS;
    char errstr[ERRSTR_SZ] = {0};

    // TODO: 线程池的大小设置
    apr_thread_pool_create(&thread_pool, get_nprocs(), get_nprocs() * 5, pool);
    if (APR_SUCCESS != status) {
        apr_strerror(status, errstr, ERRSTR_SZ);
        LOG_ERROR(
            "[mrcp_plugin_create] apr_thread_pool_create failed. error [%d] %s",
            status, errstr);
        return NULL;
    }

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

    apr_status_t status = APR_SUCCESS;
    char errstr[ERRSTR_SZ] = {0};

    // TODO: 线程池的大小设置
    apr_thread_pool_destroy(thread_pool);
    if (APR_SUCCESS != status) {
        apr_strerror(status, errstr, ERRSTR_SZ);
        LOG_ERROR("[apt_bool_t] apr_thread_pool_destroy failed. error [%d] %s",
                  status, errstr);
    }

    return TRUE;
}

apt_bool_t plugin_open(mrcp_engine_t* engine) {
    LOG_NOTICE("[plugin_open]");

    const char* login_params =
        "appid = 5acb316c, work_dir = .";  // 登录参数，appid与msc库绑定,请勿随意改动

    LOG_INFO("[plugin_open] MSPLogin(%s)", login_params);
    int errcode = MSPLogin(NULL, NULL, login_params);
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR("[plugin_open] MSPLogin(%s) failed, error %d", login_params,
                  errcode);
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
        LOG_ERROR("[plugin_open] MSPLogout failed, error %d", ret);
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
    sess->iat_session_id = NULL;

    apr_status_t status = APR_SUCCESS;
    char errstr[ERRSTR_SZ] = {0};

    // TODO: 最大缓冲数量到底要设置为多少？
    status = apr_queue_create(&sess->wav_queue, 8192, pool);

    if (APR_SUCCESS != status) {
        apr_strerror(status, errstr, ERRSTR_SZ);
        LOG_ERROR("[channel_create] apr_queue_create failed. error [%d] %s",
                  status, errstr);
        return NULL;
    }

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

/** Callback is called from MPF engine context to write/send new frame */
apt_bool_t stream_write_frame(mpf_audio_stream_t* stream,
                              const mpf_frame_t* frame) {
    session_t* sess = (session_t*)stream->obj;
    mrcp_engine_channel_t* channel = sess->channel;
    const char* iat_session_id = sess->iat_session_id;

    if (sess->stop_response) {
        /* send asynchronous response to STOP request */
        LOG_DEBUG("[stream_write_frame] [%s] stream_wirte_frame: stop_response",
                  channel->id.buf);
        mrcp_engine_channel_message_send(channel, sess->stop_response);
        sess->stop_response = NULL;
        sess->recog_request = NULL;
        return TRUE;
    }

    if (sess->recog_request) {
        mpf_detector_event_e det_event =
            mpf_activity_detector_process(sess->detector, frame);
        switch (det_event) {
            case MPF_DETECTOR_EVENT_ACTIVITY:
                LOG_DEBUG(
                    "[stream_write_frame] [%s] MPF: voice ACTIVITY event "
                    "occurred. " APT_SIDRES_FMT,
                    MRCP_MESSAGE_SIDRES(sess->recog_request), channel->id.buf);
                // TODO: 输入开始
                // _event_start_of_input(session);
                break;
            case MPF_DETECTOR_EVENT_INACTIVITY:
                LOG_DEBUG(
                    "[stream_write_frame] [%s] MPF: voice IN-ACTIVITY event "
                    "occurred. " APT_SIDRES_FMT,
                    MRCP_MESSAGE_SIDRES(sess->recog_request), channel->id.buf);
                // TODO: 输入完成
                // _event_recognition_complete(
                //     sess, RECOGNIZER_COMPLETION_CAUSE_SUCCESS);
                break;
            case MPF_DETECTOR_EVENT_NOINPUT:
                LOG_DEBUG(
                    "[stream_write_frame] [%s] MPF: NO-INPUT event "
                    "occurred. " APT_SIDRES_FMT,
                    MRCP_MESSAGE_SIDRES(sess->recog_request), channel->id.buf);
                // TODO: 输入超时
                // if (sess->timers_started) {
                // TODO: 输入超时
                // _event_recognition_complete(
                //     bdsr_channel,
                //     RECOGNIZER_COMPLETION_CAUSE_NO_INPUT_TIMEOUT);
                // }
                break;
            default:
                break;
        }
    }

    if (sess->recog_request) {
        if ((frame->type & MEDIA_FRAME_TYPE_EVENT) == MEDIA_FRAME_TYPE_EVENT) {
            if (frame->marker == MPF_MARKER_START_OF_EVENT) {
                LOG_DEBUG(
                    "[stream_write_frame] [%s] MRCP MPF: Detected Start of "
                    "Event " APT_SIDRES_FMT " id:%d",
                    MRCP_MESSAGE_SIDRES(sess->recog_request), channel->id.buf,
                    frame->event_frame.event_id);
            } else if (frame->marker == MPF_MARKER_END_OF_EVENT) {
                LOG_DEBUG(
                    "[stream_write_frame] [%s] MRCP MPF: Detected End of "
                    "Event " APT_SIDRES_FMT " id:%d duration:%d ts",
                    MRCP_MESSAGE_SIDRES(sess->recog_request), channel->id.buf,
                    frame->event_frame.event_id, frame->event_frame.duration);
            }
        }
    }

    // 准备这一块音频流
    // 假定都是 8k sample-rate, 16 bit/sec 的 LPCM

    char* dst_buf = frame->codec_frame.buffer;
    unsigned dst_buf_sz = frame->codec_frame.size;

    // 调用【讯飞云】听写 API
    unsigned int total_len = 0;
    int aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;  //音频状态
    int ep_stat = MSP_EP_LOOKING_FOR_SPEECH;   //端点检测
    int rec_stat = MSP_REC_STATUS_SUCCESS;     //识别状态
    int errcode = MSP_SUCCESS;

    // 上传！

    errcode = QISRAudioWrite(iat_session_id, dst_buf, dst_buf_sz, aud_stat,
                             &ep_stat, &rec_stat);
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR(
            "[stream_write_frame] [%s] QISRAudioWrite(%s) failed! error code: "
            "%d",
            channel->id.buf, iat_session_id, errcode);
        return FALSE;
    }

    // 如果已经有部分听写结果
    if (MSP_REC_STATUS_SUCCESS == rec_stat) {
        const char* rslt =
            QISRGetResult(iat_session_id, &rec_stat, 0, &errcode);
        if (MSP_SUCCESS != errcode) {
            LOG_ERROR(
                "[stream_write_frame] [%s] QISRGetResult(%s) failed!"
                " error code: %d",
                channel->id.buf, iat_session_id, errcode);
            return FALSE;
        }
        if (NULL != rslt) {
            // 识别出来了部分结果
            // TODO: 记录下来！
            LOG_DEBUG(
                "[stream_write_frame] [%s] QISRGetResult(%s) 部分识别结果: %s",
                channel->id.buf, iat_session_id, errcode, rslt);
        }
    }

    // 如果说完了 (Speech 开始了，然后这里结束)
    if (MSP_EP_AFTER_SPEECH == ep_stat) {
        // 一次识别结束。上传一个空音频块，表示音频流结束
        LOG_DEBUG("[stream_write_frame] [%s] (%s) 此次语音结束",
                  channel->id.buf, iat_session_id);
        errcode = QISRAudioWrite(iat_session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST,
                                 &ep_stat, &rec_stat);
        if (MSP_SUCCESS != errcode) {
            LOG_ERROR(
                "[stream_write_frame] [%s] QISRAudioWrite(%s) failed for "
                "MSP_AUDIO_SAMPLE_LAST!"
                " error code: %d",
                channel->id.buf, iat_session_id, errcode);
            return FALSE;
        }

        // TODO:
        // 启动一个过程——继续接收剩下的识别结果，直到接收完毕！考虑在线程池中执行。
    }

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
    return TRUE;
}

void on_channel_close(session_t* sess) {
    mrcp_engine_channel_t* channel = sess->channel;
    int errcode = QISRSessionEnd(sess->iat_session_id, NULL);
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR(
            "[on_channel_close] [%s] QISRSessionEnd(%s) failed! error code: %d",
            channel->id.buf, sess->iat_session_id, errcode);
    }
}

void on_channel_request(session_t* sess, mrcp_message_t* request) {
    mrcp_engine_channel_t* channel = sess->channel;
    apt_bool_t processed = FALSE;
    mrcp_message_t* response = mrcp_response_create(request, request->pool);
    switch (request->start_line.method_id) {
        case RECOGNIZER_SET_PARAMS:
            break;
        case RECOGNIZER_GET_PARAMS:
            break;
        case RECOGNIZER_DEFINE_GRAMMAR:
            break;
        case RECOGNIZER_RECOGNIZE:
            processed = on_recog_start(sess, request, response);
            break;
        case RECOGNIZER_GET_RESULT:
            break;
        case RECOGNIZER_START_INPUT_TIMERS:
            processed = on_recog_start_input_timers(sess, request, response);
            break;
        case RECOGNIZER_STOP:
            processed = on_recog_stop(sess, request, response);
            break;
        default:
            break;
    }
    if (processed == FALSE) {
        /* send asynchronous response for not handled request */
        mrcp_engine_channel_message_send(channel, response);
    }
}

apt_bool_t on_recog_start(session_t* sess,
                          mrcp_message_t* request,
                          mrcp_message_t* response) {
    mrcp_engine_channel_t* channel = sess->channel;
    LOG_DEBUG("[on_recog_start] [%s]", channel->id.buf);

    // 开始一个语音会话
    int errcode = MSP_SUCCESS;
    const char* session_begin_params =
        "sub = iat, domain = iat, language = zh_cn, accent = mandarin, "
        "sample_rate = 8000, result_type = plain, result_encoding = utf8";
    sess->iat_session_id =
        QISRSessionBegin(NULL, session_begin_params,
                         &errcode);  //听写不需要语法，第一个参数为NULL
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR(
            "[on_recog_start] [%s] QISRSessionBegin failed! error code: "
            "%d",
            channel->id.buf, errcode);
        return FALSE;
    }
    LOG_DEBUG("[on_recog_start] [%s] QISRSessionBegin -> %s", channel->id.buf,
              sess->iat_session_id);
    return TRUE;
}

apt_bool_t on_recog_start_input_timers(session_t* sess,
                                       mrcp_message_t* request,
                                       mrcp_message_t* response) {
    mrcp_engine_channel_t* channel = sess->channel;
    LOG_DEBUG("[on_recog_start_input_timers] [%s]", channel->id.buf);
    sess->timers_started = TRUE;
    return TRUE;
}

apt_bool_t on_recog_stop(session_t* sess,
                         mrcp_message_t* request,
                         mrcp_message_t* response) {
    mrcp_engine_channel_t* channel = sess->channel;
    LOG_DEBUG("[on_recog_stop] [%s]", channel->id.buf);

    /* store STOP request, make sure there is no more activity and only then
     * send the response */
    sess->stop_response = response;

    // TODO: 如果还在识别中，就得停止

    return TRUE;
}
