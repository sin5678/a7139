/*******************************************************************************
  Copyright (C), RHTECH. Co., Ltd.
  Author:          侯敏
  Date:            2013/12/3
  Version:         1.0.1
  Description:     Application log library
  History:
      <author>     <time>        <version>     <desc>
      侯敏          2013/10/25    1.0.0         初始版本
      侯敏          2013/12/3     1.0.1         添加注释
      qjn           2014/3/24     1.1.0         standard function and variable
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>
#include "applog.h"

static int applog_level = LOG_ERR;
static char *applog_appname;

/*******************************************************************************
* Function Name  : applog_open(const char *app_name)
* Description    : init app log, set the default log level
* Input params   : app_name is for used to web console
* Output params  : none
* Return         : none
*******************************************************************************/
void applog_open(const char *app_name, int loglevel, char *applog_update_cmd)
{
    char cmd_string[1024];

    assert(loglevel > LOG_EMERG && loglevel <= LOG_DEBUG);

    applog_level = loglevel;
	openlog(app_name, LOG_PID, 0);

    applog_appname = strdup(app_name);

    snprintf(cmd_string, 1024, "%s add %s \"%s\"", APPLOG_SCRIPT, applog_appname, applog_update_cmd);
	system(cmd_string);
}


/*******************************************************************************
* Function Name  : applog_close()
* Description    : close the app log, applog_close is optional
* Input params   : none
* Output params  : none
* Return         : none
*******************************************************************************/
void applog_close(void)
{
    char cmd_string[1024];
	closelog();

    snprintf(cmd_string, 1024, "%s del %s", APPLOG_SCRIPT, applog_appname);
	system(cmd_string);
}
/*******************************************************************************
* Function Name  : applog_level_set()
* Description    : update the app log level
* Input params   : none
* Output params  : none
* Return         : none
*******************************************************************************/
void applog_level_set(int loglevel)
{
    assert(loglevel > LOG_EMERG && loglevel <= LOG_DEBUG);

    applog_level = loglevel;
}

/*******************************************************************************
* Function Name  : applog_level_get()
* Description    : get the app log level
* Input params   : none
* Output params  : none
* Return         : int
*******************************************************************************/
int applog_level_get(void)
{
    return applog_level;
}

/*******************************************************************************
* Function Name  : applog_needed(int priority, int *facility)
* Description    : from priority, decide how to output the log
* Input params   : priority
* Output params  : facility
* Return         : 1: need output the applog, and return the facility
*                  0: not need output the applog
*******************************************************************************/
static int applog_needed(int priority, int *facility)
{
    assert(priority > LOG_EMERG && priority <= LOG_DEBUG);

    /* check if need log */
    if (applog_level < priority) {
        return 0;                       /* don't need log */
    }

    /* in debug level */
	if (applog_level == LOG_DEBUG) {
		if (priority == LOG_DEBUG) {
			*facility = LOG_LOCAL1;     /* out to debug log file */
		} else {
			*facility = LOG_LOCAL2;     /* out to user log file */
		}
	}
	else {
		*facility = LOG_LOCAL0;         /* out to user log file */
	}

	return 1;
}

/*******************************************************************************
* Function Name  : applog_print(int priority, const char *fmt, ...)
* Description    : output a log
* Input params   : priority, log content
* Output params  : none
* Return         : none
*******************************************************************************/
void applog_print(int priority, const char *fmt, ...)
{
	va_list ap;
	int facility;

	if (!applog_needed(priority, &facility)) {
		return;
	}

	va_start(ap, fmt);
	vsyslog(facility | priority, fmt, ap);
	va_end(ap);
}

/*******************************************************************************
* Function Name  : applog_print_data
* Description    : print hex format data to log
* Input params   : priority, data, len, fmt
* Output params  : none
* Return         : none
*******************************************************************************/
void applog_print_data(int priority, const char *data, int len, const char *fmt, ...)
{
	va_list ap;
	int facility;
    int string_len, i;
	char string_data[MAX_LOG_SIZE];

	if (len <= 0) {
		return;
    }

	if (!applog_needed(priority, &facility)) {
		return;
    }

    bzero(string_data, MAX_LOG_SIZE);

	va_start(ap, fmt);
	string_len = vsnprintf(string_data, MAX_LOG_SIZE, fmt, ap);
	va_end(ap);

    for (i = 0; i < len; i++) {
        string_len += snprintf(string_data + string_len,
                MAX_LOG_SIZE - string_len, "%02x ", data[i]);
    }

    syslog(facility | priority, "%s", string_data);
}


