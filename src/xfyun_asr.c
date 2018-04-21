#include "xfyun_asr.h"

mrcp_engine_t* mrcp_plugin_create(apr_pool_t* pool) {
    LOG_NOTICE(
        "[mrcp_plugin_create] Version: %s. Compiler: %s. Build-Time: %s %s",
        MAKE_STR(VERSION_STRING), MAKE_STR(COMPILER_STRING) __TIME__, __DATE__);

    engine_object_t* engine_object =
        (engine_object_t*)apr_palloc(pool, sizeof(engine_object_t));
    apt_task_t* task;
    apt_task_vtable_t* vtable;
    apt_task_msg_pool_t* msg_pool;

    LOG_DEBUG("[mrcp_plugin_create] create task message pool");
    msg_pool = apt_task_msg_pool_create_dynamic(sizeof(task_msg_t), pool);
    engine_object->task =
        apt_consumer_task_create(engine_object, msg_pool, pool);
    if (!engine_object->task) {
        LOG_ERROR("[mrcp_plugin_create] create task message pool failed");
        return NULL;
    }
    task = apt_consumer_task_base_get(engine_object->task);
    apt_task_name_set(task, TASK_NAME);
    vtable = apt_task_vtable_get(task);
    if (vtable) {
        vtable->process_msg = TASK_NAME;
    }

    /* create engine base */
    LOG_DEBUG("[mrcp_plugin_create] create engine base");
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
    // TODO:
    return false;
}

apt_bool_t plugin_open(mrcp_engine_t* engine) {
    // todo:
    return false;
}

apt_bool_t plugin_close(mrcp_engine_t* engine) {
    // todo:
    return false;
}

mrcp_engine_channel_t* channel_create(mrcp_engine_t* engine, apr_pool_t* pool) {
    // todo:
    return NULL;
}