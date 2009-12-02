%module pypurple
%{
#include <glib.h>
void pypurple_init();
void set_msg_cb(PyObject*);
void set_connection_cb(PyObject*, PyObject*, PyObject*);
void set_buddy_cb(PyObject*, PyObject*, PyObject*);
void account_new(const char *, const char *, const char *);
void account_set_status(const char *, const char *, const char *);
void account_set_status_message(const char *, const char *, const char *);
void send_msg(const char *, const char *, const char *, const char *);
void set_eventloop_ops(PyObject *, PyObject *, PyObject *, PyObject *);
void timeout_invoke(int handle);
void io_invoke(int, int);
PyObject* account_get_blist(const char *name, const char *prpl);
void set_buddy_icon_cb(PyObject *_on_buddy_icon_changed);
PyObject *buddy_get_icon(const char *name, const char *prpl, const char *buddy);
%}
void pypurple_init();
void set_msg_cb(PyObject*);
void set_connection_cb(PyObject*, PyObject*, PyObject*);
void set_buddy_cb(PyObject*, PyObject*, PyObject*);
void account_new(const char *, const char *, const char *);
void account_set_status(const char *, const char *, const char *);
void account_set_status_message(const char *, const char *, const char *);
void send_msg(const char *, const char *, const char *, const char *);
void set_eventloop_ops(PyObject *, PyObject *, PyObject *, PyObject *);
void timeout_invoke(int handle);
void io_invoke(int, int);
PyObject* account_get_blist(const char *name, const char *prpl);
void set_buddy_icon_cb(PyObject *_on_buddy_icon_changed);
PyObject *buddy_get_icon(const char *name, const char *prpl, const char *buddy);

#include<glib.h>
