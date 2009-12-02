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


#include "ioloop.h"

// #define DEBUG_EVENTLOOP 1

/* Python eventloop functions */
PyObject *py_input_add, 
         *py_input_remove, 
         *py_timeout_add,
         *py_timeout_remove;

GHashTable *timeouts = NULL;
GHashTable *inputs = NULL;
int timeouts_last = 0;

void ioloop_init()
{
    timeouts = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, NULL);
    inputs = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, NULL);
}

void set_eventloop_ops(PyObject * inp_add, PyObject * inp_remove,
		  PyObject * time_add, PyObject * time_remove)
{
    py_input_add = inp_add;
    py_input_remove = inp_remove;
    py_timeout_add = time_add;
    py_timeout_remove = time_remove;

    printf("Setting python eventloop ops.\n");
}

void timeout_invoke(int handle)
{

#ifdef DEBUG_EVENTLOOP
    printf("Invoking timeout callback for handle %d\n", handle);
#endif

    PyPurpleTimeoutClosure *closure = g_hash_table_lookup(timeouts, &handle);

    Py_DECREF(closure->timeout);
    closure->timeout = PyEval_CallFunction(py_timeout_add, "ii", closure->interval, closure->handle);

    gboolean res = closure->function(closure->data);

    if (res == FALSE) {
        ioloop_timeout_remove(handle);
    }
}

guint ioloop_timeout_add(guint interval, GSourceFunc function, gpointer data)
{
    PyPurpleTimeoutClosure *closure = g_new0(PyPurpleTimeoutClosure, 1);
    closure->function = function;
    closure->data = data;
    closure->interval = interval;
    closure->handle = ++timeouts_last;
    closure->timeout = PyEval_CallFunction(py_timeout_add, "ii", interval,  closure->handle);

#ifdef DEBUG_EVENTLOOP
    printf("ioloop_timeout_add %d handle = %d\n", interval,
	   closure->handle);
#endif

    g_hash_table_insert(timeouts, &closure->handle, closure);

    return closure->handle;
}

void ioloop_timeout_remove(int handle)
{
    PyPurpleTimeoutClosure *closure =	g_hash_table_lookup(timeouts, &handle);

    if (closure) {
        g_hash_table_remove(timeouts, &handle);

#ifdef DEBUG_EVENTLOOP
        printf("ioloop_timeout_remove handle=%d\n", closure->handle);
#endif

        PyObject *res = PyEval_CallFunction(py_timeout_remove, "(O)",
                    closure->timeout);
        Py_XDECREF(res);
        Py_DECREF(closure->timeout);

        g_free(closure);
    } else {
        printf("warning! removing timeout handle that doesn exist...\n\n");
    }
}

void closure_print(PyPurpleIOClosure * closure)
{
    printf("FD=%d read=%x write=%x cond=%d\n\n", closure->fd,
	   closure->function_read, closure->function_write, closure->cond);
}


void io_invoke(int handle, int events)
{
#ifdef DEBUG_EVENTLOOP
    fprintf(stderr, "HEAR HEAR!! ioloop input invoke, fd=%d events=%d\n ",
	    handle, events);
#endif

    PyPurpleIOClosure *closure = g_hash_table_lookup(inputs, &handle);
    assert(closure != NULL);

#ifdef DEBUG_EVENTLOOP
    closure_print(closure);
#endif

    PurpleInputCondition cond = 0;
    if (events & IOLOOP_READ) {
        cond |= PURPLE_INPUT_READ;
    }
    if (events & IOLOOP_WRITE) {
        cond |= PURPLE_INPUT_WRITE;
    }

    if (events & _EPOLLIN)
        assert(closure->function_read);
    if (events & _EPOLLOUT)
        assert(closure->function_write);

    // invoke handler function
    if ((events & IOLOOP_READ) && closure->function_read)
        closure->function_read( closure->data_read, closure->fd, cond );

    if ((events & IOLOOP_WRITE) && closure->function_write)
        closure->function_write( closure->data_write, closure->fd, cond );
}

void ioloop_update(PyPurpleIOClosure * closure)
{
    assert(closure);

    guint events = 0;
    if (closure->function_read)
        events |= IOLOOP_READ;
    if (closure->function_write)
        events |= IOLOOP_WRITE;

    if (closure->cond != events) {      
      /*  TODO: Use of ioloop's update_handler instead of removing and re-adding */
      if (closure->cond != 0) {
          // Remove old 
          PyObject *res = PyEval_CallFunction(py_input_remove, "(i)", closure->fd);
          Py_XDECREF(res);
      }

      if (events != 0) {
          // add new
          PyObject *res = PyEval_CallFunction(py_input_add, "ii", closure->fd, events);
          Py_XDECREF(res);
      }
    }

    closure->cond = events;
}

guint ioloop_input_add(int fd, PurpleInputCondition cond, PurpleInputFunction func, gpointer user_data)
{

#ifdef DEBUG_EVENTLOOP
    printf("ioloop input_add fd=%d cond=%d handle = %d\n", fd, cond, fd << 2 | cond);
#endif

    if (cond == (PURPLE_INPUT_READ | PURPLE_INPUT_WRITE)) {
        printf("Looks like purple wants to handle inp/out with same callback function...!!!! die. now.\n");
	      exit(1);
    }

    // is fd already being monitored?
    PyPurpleIOClosure *closure = g_hash_table_lookup(inputs, &fd);

    if (closure == NULL) {
        // create new closure
        closure = g_new0(PyPurpleIOClosure, 1);
        closure->fd = fd;
        closure->cond = 0;
        closure->function_read = NULL;
        closure->function_write = NULL;

        g_hash_table_insert(inputs, &closure->fd, closure);
    }

    if ((cond & PURPLE_INPUT_READ) == PURPLE_INPUT_READ) {
	      closure->function_read = func;
    	  closure->data_read = user_data;
    }
    if ((cond & PURPLE_INPUT_WRITE) == PURPLE_INPUT_WRITE) {
      	closure->function_write = func;
    	  closure->data_write = user_data;
    }

    ioloop_update(closure);

#ifdef DEBUG_EVENTLOOP
    closure_print(closure);
#endif

    return closure->fd << 2 | cond;
}

void ioloop_input_remove(int handle)
{
#ifdef DEBUG_EVENTLOOP
    printf("ioloop input_remove handle=%d fd=%d cond=%d\n",  handle, handle >> 2, handle & 0x03);
#endif

    PurpleInputCondition cond = handle & 0x03;
    handle = handle >> 2;

    PyPurpleIOClosure *closure = g_hash_table_lookup(inputs, &handle);

    assert(closure != NULL);

    if (cond == PURPLE_INPUT_READ)
      	closure->function_read = NULL;
    else
      	closure->function_write = NULL;

    ioloop_update(closure);

#ifdef DEBUG_EVENTLOOP
    closure_print(closure);
#endif

    // TODO: fix this later.. it may cause memory leak, but it's good for performance..
  /*   if( closure->function_read==NULL && closure->function_write==NULL) {
    g_hash_table_remove(inputs, &handle);
    g_free(closure);
  } */
}
