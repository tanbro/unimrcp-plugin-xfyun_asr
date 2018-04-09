/**
 * @brief 日志函数
 *
 * @file xfyun_asr_logging.h
 * @author liuxy
 * @date 2018-04-09
 */

#ifndef _XFYUN_ASR_LOGGING_H_
#define _XFYUN_ASR_LOGGING_H_

#include "apt_log.h"

#define LOGGER "\t[XFYUN_ASR-PLUGIN]\t"
#define LOG(level, fmt, ...) \
    apt_log(BDSR_LOG_MARK, level, LOGGER fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_DEBUG, LOGGER fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_INFO, LOGGER fmt, ##__VA_ARGS__)
#define LOG_NOTICE(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_NOTICE, LOGGER fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_WARNING, LOGGER fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_ERROR, LOGGER fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_CRITICAL, LOGGER fmt, ##__VA_ARGS__)
#define LOG_ALERT(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_ALERT, LOGGER fmt, ##__VA_ARGS__)
#define LOG_EMERGENCY(fmt, ...) \
    apt_log(BDSR_LOG_MARK, APT_PRIO_EMERGENCY, LOGGER fmt, ##__VA_ARGS__)

#endif
