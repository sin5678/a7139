//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/12/25
// Version:         1.0.0
// Description:     the buffer lib for c code
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/25  1.0.0       create this file
//
// TODO:
//      2013/12/31          add the common and debug
//*******************************************************************************
#ifndef __BUFFER_H__
#define __BUFFER_H__

#define BUF_MAX_INCR (4*1024)
#define BUF_MAX_SIZE (4*1024)

struct buf {

	char *data;
	int len;        /* the used size */
	int pos;
	int size;       /* the memory size */

};

typedef struct buf buffer;

buffer *buf_new(int size);
buffer *buf_new_max();
buffer *buf_new_data(int size, char *data);
void buf_free(buffer *buf);
int buf_incrsize(buffer *buf, int incr);
int buf_incrlen(buffer *buf, int incr);
int buf_incrpos(buffer *buf, int incr);
int buf_is_used(buffer *buf);
int buf_is_empty(buffer *buf);
int buf_read(int fd, buffer *buf);
int buf_write(int fd, buffer *buf);
int buf_clean(buffer *buf);
char buf_getchar(buffer *buf);
int buf_append(buffer *buf, char *data, int len);
void buf_copy(buffer *old, buffer *new);
void buf_dump(buffer *buf);
char *buf_data(buffer *buf);
int buf_len(buffer *buf);
int buf_space(buffer *buf);

#endif /* _BUFFER_H_ */
