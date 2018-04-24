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

    // 初始化 Session
    session_t* sess = (session_t*)apr_palloc(pool, sizeof(session_t));
    sess->obj = (engine_object_t*)engine->obj;
    sess->iat_session_id = NULL;
    sess->iat_begin_params = (char*)apr_pcalloc(pool, IAT_BEGIN_PARAMS_LEN);
    sess->iat_result = (char*)apr_pcalloc(pool, IAT_RESULT_STR_LEN);

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

    // 准备这一块音频流，写到处理缓冲
    // 假定都是 PCM S8 LE
    wav_que_obj_t* wav_obj =
        (wav_que_obj_t*)apr_pcalloc(channel->pool, sizeof(wav_que_obj_t));
    wav_obj->data = frame->codec_frame.buffer;
    wav_obj->len = frame->codec_frame.size;
    apr_queue_push(sess->wav_queue, wav_obj);

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

void on_channel_close(session_t* sess) {}

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
    const char* session_begin_params =
        "sub = iat, domain = iat, language = zh_cn, accent = mandarin, "
        "sample_rate = 8000, result_type = plain, result_encoding = utf8";
    strncpy(sess->iat_begin_params, session_begin_params, IAT_BEGIN_PARAMS_LEN);

    // 启动会话处理线程
    apr_status_t status = APR_SUCCESS;
    char errstr[ERRSTR_SZ] = {0};
    apr_thread_pool_push(thread_pool, recog_thread_func, (void*)sess, 0, NULL);
    if (APR_SUCCESS != status) {
        apr_strerror(status, errstr, ERRSTR_SZ);
        LOG_ERROR(
            "[on_recog_start] [%s] apr_thread_pool_push failed. error [%d] %s",
            channel->id.buf, status, errstr);
        return FALSE;
    }

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

    // TODO: 如果还在识别中，就得停止。如果不是在识别，当心强行停止出问题！

    // 打断流媒体缓冲处理队列
    apr_status_t status = APR_SUCCESS;
    char errstr[ERRSTR_SZ] = {0};
    status = apr_queue_term(sess->wav_queue);
    if (APR_SUCCESS != status) {
        apr_strerror(status, errstr, ERRSTR_SZ);
        LOG_ERROR("[on_recog_stop] [%s] apr_queue_term failed. error [%d] %s",
                  channel->id.buf, status, errstr);
        return FALSE;
    }
    return TRUE;
}

void* recog_thread_func(apr_thread_t* thread, void* arg) {
    int errcode = MSP_SUCCESS;
    session_t* sess = (session_t*)arg;
    mrcp_engine_channel_t* channel = sess->channel;

    LOG_DEBUG("[recog_thread_func] [%s]  >>>", channel->id.buf);

    // 开始【讯飞云】听写会话
    LOG_DEBUG("[recog_thread_func] [%s] QISRSessionBegin()", channel->id.buf);
    const char* iat_session_id =
        QISRSessionBegin(NULL, sess->iat_begin_params,
                         &errcode);  //听写不需要语法，第一个参数为NULL
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR(
            "[recog_thread_func] [%s] QISRSessionBegin failed! error code: "
            "%d",
            channel->id.buf, errcode);
        return FALSE;
    }
    if (!iat_session_id) {
        LOG_ERROR("[recog_thread_func] [%s] QISRSessionBegin failed!",
                  channel->id.buf);
        return FALSE;
    }
    sess->iat_session_id =
        (char*)apr_pcalloc(channel->pool, IAT_SESSION_ID_LEN);
    strncpy(sess->iat_session_id, iat_session_id, IAT_SESSION_ID_LEN);
    LOG_DEBUG("[recog_thread_func] [%s] QISRSessionBegin -> %s",
              channel->id.buf, sess->iat_session_id);

    ///

    // 调用【讯飞云】听写 API
    unsigned int total_len = 0;
    int ep_stat = MSP_EP_LOOKING_FOR_SPEECH;  //端点检测
    int rec_stat = MSP_REC_STATUS_SUCCESS;    //识别状态

    bool is_terminated = false;

    // 上传！
    while (true) {
        // POP 流媒体数据
        // 如果 queue 已经空，将 BLOCK!!!
        wav_que_obj_t* wav_obj = NULL;
        apr_status_t status = APR_SUCCESS;
        char errstr[ERRSTR_SZ] = {0};
        status = apr_queue_pop(sess->wav_queue, (void**)&wav_obj);
        if (APR_EOF == status) {
            // 被打断
            is_terminated = true;
            LOG_DEBUG("[recog_thread_func] [%s] terminated", channel->id.buf);
            break;
        }
        if (APR_SUCCESS != status) {
            apr_strerror(status, errstr, ERRSTR_SZ);
            LOG_ERROR(
                "[recog_thread_func] [%s] apr_queue_pop failed. error [%d] "
                "%s",
                channel->id.buf, status, errstr);
            break;
        }

        if (wav_obj) {
            // 上传到讯飞云，进行识别
            errcode =
                QISRAudioWrite(iat_session_id, wav_obj->data, wav_obj->len,
                               MSP_AUDIO_SAMPLE_CONTINUE, &ep_stat, &rec_stat);
            if (MSP_SUCCESS != errcode) {
                LOG_ERROR(
                    "[recog_thread_func] [%s] (%s) QISRAudioWrite failed! "
                    "error code: %d",
                    channel->id.buf, iat_session_id, errcode);
                break;
            }
        }

        // 如果已经有部分听写结果
        if (MSP_REC_STATUS_SUCCESS == rec_stat) {
            const char* rslt =
                QISRGetResult(iat_session_id, &rec_stat, 0, &errcode);
            if (MSP_SUCCESS != errcode) {
                LOG_ERROR(
                    "[recog_thread_func] [%s] (%s) QISRGetResult failed!"
                    " error code: %d",
                    channel->id.buf, iat_session_id, errcode);
                break;
            }
            if (NULL != rslt) {
                // 识别出来了部分结果。记录下来！
                LOG_DEBUG(
                    "[recog_thread_func] [%s] (%s) MSP_REC_STATUS_SUCCESS: %s",
                    channel->id.buf, iat_session_id, rslt);
                strncat(sess->iat_result, rslt,
                        IAT_RESULT_STR_LEN - strlen(sess->iat_result));
            }
        }

        if (MSP_EP_AFTER_SPEECH == ep_stat) {
            // 说完了。退出读缓冲的循环
            LOG_DEBUG("[recog_thread_func] [%s] (%s) MSP_EP_AFTER_SPEECH",
                      channel->id.buf, iat_session_id);
            break;
        }
    }

    // 上传一个空音频块，表示音频流结束
    LOG_DEBUG(
        "[recog_thread_func] [%s] (%s) QISRAudioWrite MSP_AUDIO_SAMPLE_LAST",
        channel->id.buf, iat_session_id);
    errcode = QISRAudioWrite(iat_session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST,
                             &ep_stat, &rec_stat);
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR(
            "[recog_thread_func] [%s] (%s) QISRAudioWrite failed for  "
            "MSP_AUDIO_SAMPLE_LAST! error code: %d",
            channel->id.buf, iat_session_id, errcode);
    }

    if (is_terminated) {
        // TODO: 是被打断的，应该如何处理？
        LOG_DEBUG("[recog_thread_func] [%s] (%s) 被打断，应该如何处理？",
                  channel->id.buf, iat_session_id);
    } else if (MSP_EP_AFTER_SPEECH == ep_stat) {
        // 说完了，但是还要继续接收结果
        // 继续接收结果，直到完成
        while (MSP_REC_STATUS_COMPLETE != rec_stat) {
            const char* rslt =
                QISRGetResult(iat_session_id, &rec_stat, 0, &errcode);
            if (MSP_SUCCESS != errcode) {
                LOG_ERROR(
                    "[recog_thread_func] [%s] (%s) QISRGetResult failed!"
                    " error code: %d",
                    channel->id.buf, iat_session_id, errcode);
                break;
            }
            if (NULL != rslt) {
                // 识别出来了最后一个部分结果。记录下来！
                LOG_DEBUG(
                    "[recog_thread_func] [%s] (%s) MSP_REC_STATUS_COMPLETE: %s",
                    channel->id.buf, iat_session_id, rslt);
                strncat(sess->iat_result, rslt,
                        IAT_RESULT_STR_LEN - strlen(sess->iat_result));
            }
            usleep(150 * 1000);  //防止频繁占用CPU
        }
    }

    // 结束【讯飞云】听写会话
    LOG_DEBUG("[recog_thread_func] [%s] QISRSessionBegin(%s)", channel->id.buf,
              iat_session_id);
    errcode = QISRSessionEnd(iat_session_id, NULL);
    if (MSP_SUCCESS != errcode) {
        LOG_ERROR(
            "[recog_thread_func] [%s] (%s) QISRSessionEnd failed! "
            "error code: %d",
            channel->id.buf, iat_session_id, errcode);
    }

    LOG_DEBUG("[recog_thread_func] [%s]  <<<", channel->id.buf);

    return NULL;
}