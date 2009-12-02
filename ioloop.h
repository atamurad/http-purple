/*  
    http-purple: HTTP API to access IM networks
    Copyright (C) 2009 Atamurad Hezretkuliyev

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include<Python.h>
#include<purple.h>

#ifndef __IOLOOP_H__
#define __IOLOOP_H__

//    # Constants from the epoll module
#define    _EPOLLIN  0x001
#define    _EPOLLPRI  0x002
#define    _EPOLLOUT  0x004
#define    _EPOLLERR  0x008
#define    _EPOLLHUP  0x010
#define    _EPOLLRDHUP  0x2000
#define   IOLOOP_ERROR  (_EPOLLERR | _EPOLLHUP | _EPOLLRDHUP)

#define IOLOOP_READ  ( _EPOLLIN | IOLOOP_ERROR)
#define IOLOOP_WRITE ( _EPOLLOUT | IOLOOP_ERROR)


struct _PyPurpleTimeoutClosure {
    GSourceFunc function;
    gpointer *data;
    PyObject *timeout;
    int interval;
    int handle;
};
typedef struct _PyPurpleTimeoutClosure PyPurpleTimeoutClosure;

struct _PyPurpleIOClosure {
    PurpleInputFunction function_read;
    PurpleInputFunction function_write;
    PurpleInputCondition cond;
    gpointer *data_read;
    gpointer *data_write;
    int fd;
};
typedef struct _PyPurpleIOClosure PyPurpleIOClosure;


void set_eventloop_ops(PyObject *inp_add, PyObject *inp_remove, PyObject *time_add, PyObject *time_remove);

void ioloop_init();

void timeout_invoke(int handle);
guint ioloop_timeout_add(guint interval, GSourceFunc function, gpointer data);
void ioloop_timeout_remove(int handle);

void io_invoke(int handle, int events);
void ioloop_input_remove(int handle);
guint ioloop_input_add(int fd, PurpleInputCondition cond, PurpleInputFunction func, gpointer user_data);


#endif
