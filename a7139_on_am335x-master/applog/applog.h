/*******************************************************************************
  Copyright (C), RHTECH. Co., Ltd.
  Author:          侯敏
  Date:            2013/12/3
  Version:         1.0.1
  Description:     Application log library header file
  History:
      <author>     <time>        <version>     <desc>
      侯敏          2013/10/25    1.0.0         初始版本
      侯敏          2013/12/3     1.0.1         添加注释
      qjn           2014/3/24     1.1.0         standard function and variable
*******************************************************************************/

#ifndef __APPLOG_HEADER__
#define __APPLOG_HEADER__

#include <syslog.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOG_SIZE        1024
#define APPLOG_SCRIPT       "/etc/init.d/syslogd"

void applog_open(const char *app_name, int loglevel, char *applog_update_cmd);
void applog_close(void);
void applog_level_set(int loglevel);
int  applog_level_get(void);
void applog_print(int priority, const char *fmt, ...);
void applog_print_data(int priority, const char *data, int len, const char *fmt, ...);

#define open_app_log(m)
#define close_app_log()
#define print_app_log(fmt, args...)
#define print_app_data(level, data, len, fmt, ...)
#define update_app_debug_mode()

#ifdef __cplusplus
}
#endif


#endif
