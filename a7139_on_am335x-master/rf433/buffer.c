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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "buffer.h"

/*****************************************************************************
* Function Name  : buf_new
* Description    : new buffer and alloc memory
* Input          : int
* Output         : None
* Return         : buffer*
*****************************************************************************/
buffer *buf_new(int size)
{
	buffer* buf;

	if (size > BUF_MAX_SIZE) {
        return NULL;
	}

	buf = (buffer*)malloc(sizeof(buffer));

    if (buf == NULL)
        return NULL;

	if (size > 0) {
		buf->data = (char*)malloc(size);
        if (buf->data == NULL) {
            free(buf);
            return NULL;
        }
	} else {
		buf->data = NULL;
	}

	buf->size = size;
	buf->pos = 0;
	buf->len = 0;

	return buf;

}

/*****************************************************************************
* Function Name  : buf_new_max
* Description    : new buffer and use max size alloc memory
* Input          : void
* Output         : None
* Return         : buffer*
*****************************************************************************/
buffer *buf_new_max(void)
{
    return buf_new(BUF_MAX_SIZE);
}

/*****************************************************************************
* Function Name  : buf_new_data
* Description    : new buffer and padding data to it
* Input          : int, char*
* Output         : None
* Return         : buffer*
*****************************************************************************/
buffer *buf_new_data(int size, char *data)
{
    buffer *newbuf;

    newbuf = buf_new(size);
    if (newbuf == NULL) {
        return NULL;
    }

    newbuf->data = malloc(size);
    if (newbuf->data == NULL) {
        free(newbuf);
        return NULL;
    }

    memcpy(newbuf->data, data, size);
    buf_incrlen(newbuf, size);

    return newbuf;
}

/*****************************************************************************
* Function Name  : buf_free
* Description    : delete buffer and free his memory
* Input          : buffer*
* Output         : None
* Return         : void
*****************************************************************************/
void buf_free(buffer *buf)
{
    if (!buf) {
        return;
    }

    if (buf->data) {
	    free(buf->data);
	}

	free(buf);
}

/*****************************************************************************
* Function Name  : buf_incrsize
* Description    : increase buffer size
* Input          : buffer*, int
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int buf_incrsize(buffer *buf, int incr)
{
	if (buf->size + incr > BUF_MAX_INCR) {
        return -1;
	}

    if (buf->data) {
        buf->data = realloc(buf->data, buf->size + incr);
    } else {
        buf->data = malloc(incr);
    }

    if (buf->data == NULL) {
        return -1;
    }

    return 0;
}

/*****************************************************************************
* Function Name  : buf_incrlen
* Description    : increase buffer length
* Input          : buffer*, int
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int buf_incrlen(buffer *buf, int incr)
{
	if (incr > BUF_MAX_INCR || buf->len + incr > buf->size) {
        return -1;
	}

	buf->len += incr;
    return 0;

}

/*****************************************************************************
* Function Name  : buf_incrpos
* Description    : move buffer position
* Input          : buffer*, int
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int buf_incrpos(buffer *buf,  int incr)
{
	if (incr > BUF_MAX_INCR || buf->pos + incr > buf->len
			|| buf->pos + incr < 0) {
        return -1;
	}

	buf->pos += incr;
    return 0;
}

/*****************************************************************************
* Function Name  : buf_append
* Description    : append data to buffer
* Input          : buffer*, char*, int
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int buf_append(buffer *buf, char *data, int len)
{
	if (len > BUF_MAX_INCR || buf->len + len > buf->size) {
        return -1;
	}

    memcpy(buf->data + buf->pos, data, len);
    buf_incrlen(buf, len);

    return 0;
}

/*****************************************************************************
* Function Name  : buf_is_used
* Description    : check the buffer is already used
* Input          : buffer*,
* Output         : None
* Return         : int(0:no, 1:yes)
*****************************************************************************/
int buf_is_used(buffer *buf)
{
    if (buf == NULL) {
        return 0;
    }

    return (buf->len - buf->pos) ? 1 : 0;
}

/*****************************************************************************
* Function Name  : buf_is_empty
* Description    : check the buffer is empty
* Input          : buffer*,
* Output         : None
* Return         : int(0:no, 1:yes)
*****************************************************************************/
int buf_is_empty(buffer *buf)
{
    if (buf == NULL) {
        return 0;
    }

    return (buf->len == buf->pos) ? 1 : 0;
}

/*****************************************************************************
* Function Name  : buf_read
* Description    : read data from fd to buffer
* Input          : int, buffer*,
* Output         : None
* Return         : int(read length)
*****************************************************************************/
int buf_read(int fd, buffer *buf)
{
    int ret;

    ret = read(fd, buf->data + buf->len, buf->size - buf->len);

    if (ret > 0) {
        buf_incrlen(buf, ret);
    }

    return ret;
}

/*****************************************************************************
* Function Name  : buf_write
* Description    : write data to fd from buffer
* Input          : int, buffer*,
* Output         : None
* Return         : int(write length)
*****************************************************************************/
int buf_write(int fd, buffer *buf)
{
    int ret;

    ret = write(fd, buf->data + buf->pos, buf->len - buf->pos);

    if (ret > 0) {
        buf_incrpos(buf, ret);
    }

    return ret;
}

/*****************************************************************************
* Function Name  : buf_clean
* Description    : clean buffer data
* Input          : buffer*,
* Output         : None
* Return         : int(0:ok)
*****************************************************************************/
int buf_clean(buffer *buf)
{
    buf->pos = buf->len = 0;
    bzero(buf->data, buf->size);

    return 0;
}

/*****************************************************************************
* Function Name  : buf_getchar
* Description    : get char from buffer
* Input          : buffer*,
* Output         : None
* Return         : char(data)
*****************************************************************************/
char buf_getchar(buffer *buf)
{
    if (buf->pos >= buf->len) {
        return '\0';
    }

    return buf->data[buf->pos++];
}

/*****************************************************************************
* Function Name  : buf_data
* Description    : get char pointer from buffer
* Input          : buffer*,
* Output         : None
* Return         : char*(pointer)
*****************************************************************************/
char *buf_data(buffer *buf)
{
    if (!buf) {
        return NULL;
    }

    return buf->data + buf->pos;
}

/*****************************************************************************
* Function Name  : buf_len
* Description    : get buffer data length
* Input          : buffer*,
* Output         : None
* Return         : int
*****************************************************************************/
int buf_len(buffer *buf)
{
    return buf->len - buf->pos;
}

/*****************************************************************************
* Function Name  : buf_space
* Description    : get buffer leave space length
* Input          : buffer*,
* Output         : None
* Return         : int
*****************************************************************************/
int buf_space(buffer *buf)
{
    return buf->size - buf->len;
}


/*****************************************************************************
* Function Name  : buf_copy
* Description    : copy buffer to new buffer
* Input          : buffer*, buffer*
* Output         : None
* Return         : void
*****************************************************************************/
void buf_copy(buffer *old, buffer *new)
{
    memcpy(new->data, old->data, new->size);
    new->len = old->len;
    new->pos = old->pos;
    new->size = old->size;
}

/*****************************************************************************
* Function Name  : buf_dump
* Description    : display all data in buffer use hex
* Input          : buffer*
* Output         : display
* Return         : void
*****************************************************************************/
void buf_dump(buffer *buf)
{
    int i;
    char *tmp;

    tmp = buf->data + buf->pos;

    for (i = 0; i < buf->len; i++)
    {
        if (i % 16 == 0)
            fprintf(stderr, "\n");
        fprintf(stderr, "%02x ", tmp[i]);
    }

    fprintf(stderr, "\n\n");
}

