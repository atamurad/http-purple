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

#include<stdio.h>
#include<glib.h>
#include<purple.h>
#include<Python.h>

#include "ioloop.h"

#define UI_ID "pypurple"

// EventLoopUIOps structure for libpurple
static PurpleEventLoopUiOps ioloop_eventloops = 
{
	ioloop_timeout_add,
	ioloop_timeout_remove,
	ioloop_input_add,
	ioloop_input_remove,
	NULL,
	NULL,
	/* padding */
	NULL, NULL,	NULL
};


PyObject *py_on_buddy_status_update;
PyObject *py_on_buddy_signed_on;
PyObject *py_on_buddy_signed_off;
PyObject *py_on_buddy_icon_changed;

void set_buddy_cb(PyObject *_on_buddy_signon, PyObject *_on_buddy_signoff, PyObject *_on_buddy_status)
{
  py_on_buddy_status_update = _on_buddy_status;
  py_on_buddy_signed_on = _on_buddy_signon;
  py_on_buddy_signed_off = _on_buddy_signoff;
}

void on_buddy_status_changed(PurpleBuddy *buddy, PurpleStatus *old_status, PurpleStatus *status)
{
  char *message = purple_status_get_attr_string(status, "message");

  printf("Buddy status changed %s message = %s \n", buddy->name, message);

  PyObject *res = PyEval_CallFunction(py_on_buddy_status_update, "(sssss)", 
      buddy->account->protocol_id, buddy->account->username,  buddy->name, purple_status_get_id(status), message );
  Py_XDECREF(res);

}

void on_buddy_signed_on(PurpleBuddy *buddy)
{
  printf("Buddy signed on %s\n", buddy->name);

  PyObject *res = PyEval_CallFunction(py_on_buddy_signed_on, "(sss)", 
      buddy->account->protocol_id, buddy->account->username,  buddy->name );
  Py_XDECREF(res);
}

void on_buddy_signed_off(PurpleBuddy *buddy)
{
  printf("Buddy signed off %s\n", buddy->name);

  PyObject *res = PyEval_CallFunction(py_on_buddy_signed_off, "(sss)", 
      buddy->account->protocol_id, buddy->account->username,  buddy->name );
  Py_XDECREF(res);
}

void set_buddy_icon_cb(PyObject *_on_buddy_icon_changed)
{
  py_on_buddy_icon_changed = _on_buddy_icon_changed;
}

void on_buddy_icon_changed(PurpleBuddy *buddy)
{
  printf("Buddy icon changed %s\n", buddy->name);

  PyObject *res = PyEval_CallFunction(py_on_buddy_icon_changed, "(sss)", 
      buddy->account->protocol_id, buddy->account->username,  buddy->name );
  Py_XDECREF(res);
}






// Connection Callbacks

PyObject *py_on_connected;
PyObject *py_on_disconnected;
PyObject *py_on_connection_error;

void set_connection_cb(PyObject *_on_connected, PyObject *_on_disconnected, PyObject *_on_connection_error)
{
  py_on_connection_error = _on_connection_error;
  py_on_connected = _on_connected;
  py_on_disconnected = _on_disconnected;
}

void on_connection_error(PurpleConnection *gc, PurpleConnectionError err, const gchar *desc)
{
	PurpleAccount *account = purple_connection_get_account(gc);
  printf("\nConnection error! %s %s\n", account->username, desc);

  PyObject *res = PyEval_CallFunction(py_on_connection_error, "(sss)", account->protocol_id, account->username, desc);
  Py_XDECREF(res);
}

void on_connected(PurpleConnection *gc, gpointer null)
{
	PurpleAccount *account = purple_connection_get_account(gc);
	printf("Account connected: %s %s\n", account->username, account->protocol_id);

  PyObject *res = PyEval_CallFunction(py_on_connected, "(ss)", account->protocol_id, account->username);
  Py_XDECREF(res);
}

void on_disconnected(PurpleConnection *gc, gpointer null)
{
	PurpleAccount *account = purple_connection_get_account(gc);
	printf("Account disconnected: %s %s\n", account->username, account->protocol_id);

  purple_accounts_remove(account);

  PyObject *res = PyEval_CallFunction(py_on_disconnected, "(ss)", account->protocol_id, account->username);
  Py_XDECREF(res);
}


// Message callbacks

PyObject *py_on_message;

void set_msg_cb(PyObject *func)
{
  py_on_message = func;
}

static void on_message(PurpleConversation *conv, const char *who, const char *alias,const char *message, PurpleMessageFlags flags, time_t mtime)
{
  // invoke for only received messages
  if( flags & PURPLE_MESSAGE_RECV ) {
    PyObject *res = PyEval_CallFunction(py_on_message, "(ssssi)", conv->account->protocol_id, conv->account->username,  who, message, mtime);
    Py_XDECREF(res);
  }

	printf("(%s) %s %s: %s %d\n", purple_conversation_get_name(conv),
			purple_utf8_strftime("(%H:%M:%S)", localtime(&mtime)),
			who, message, flags);
}


static PurpleConversationUiOps pypurple_conv_uiops = 
{
	NULL,                      /* create_conversation  */
	NULL,                      /* destroy_conversation */
	NULL,                      /* write_chat           */
	NULL,                      /* write_im             */
	on_message,                /* write_conv           */
	NULL,                      /* chat_add_users       */
	NULL,                      /* chat_rename_user     */
	NULL,                      /* chat_remove_users    */
	NULL,                      /* chat_update_user     */
	NULL,                      /* present              */
	NULL,                      /* has_focus            */
	NULL,                      /* custom_smiley_add    */
	NULL,                      /* custom_smiley_write  */
	NULL,                      /* custom_smiley_close  */
	NULL,                      /* send_confirm         */
	NULL,
	NULL,
	NULL,
	NULL
};


void account_new(const char *prpl, const char *user, const char *password)
{
  printf("Creating new account, %s prpl:%s\n", user, prpl);

  PurpleAccount *acc = purple_account_new(user, prpl);  
  purple_account_set_password(acc, password);
  purple_account_set_enabled(acc, UI_ID, TRUE);
  purple_accounts_add(acc);
}

void account_set_status(const char *name, const char *prpl, const char *status)
{
  printf("Setting status for %s@%s to %s\n", name, prpl, status);

  PurpleAccount *acc = purple_accounts_find(name, prpl);
  if( acc ) {
    purple_presence_set_status_active(purple_account_get_presence(acc), status, TRUE);
  } else 
    printf("\n\nAccount not found!!\n\n");
}

void account_set_status_message(const char *name, const char *prpl, const char *message)
{
  printf("Setting status message for %s@%s to %s\n", name, prpl, message);

  PurpleAccount *acc = purple_accounts_find(name, prpl);
  if( acc ) {
    PurpleStatus *status = purple_presence_get_active_status(purple_account_get_presence(acc));
    GList *list = NULL;
    list = g_list_append(list, "message");
    list = g_list_append(list, message);
    purple_status_set_active_with_attrs_list(status, TRUE, list);
    g_list_free(list);
  } else 
    printf("\n\nAccount not found!!\n\n");
}


PyObject *buddy_get_icon(const char *name, const char *prpl, const char *buddy)
{
    PurpleAccount *acc = purple_accounts_find(name, prpl);
    if( ! acc ) 
      return NULL;

    PurpleBuddyIcon *icon = purple_buddy_icons_find(acc, buddy);
    if( ! icon ) 
      return Py_BuildValue("s", "none");

   gpointer icon_data;
   size_t len;
   char *ext;
   icon_data = purple_buddy_icon_get_data(icon, &len);
   ext = purple_buddy_icon_get_extension(icon);

   char *hash = purple_util_get_image_filename(icon_data, len);

    // save to file
    char fname[255];
    sprintf(fname, "static/%s", hash);
    g_free(hash);

    g_file_set_contents(fname, icon_data, len, NULL);

    return Py_BuildValue("s", fname);
}


PyObject *account_get_blist(const char *name, const char *prpl)
{
  printf("Buddy list for %s\n", name);

  PurpleAccount *acc = purple_accounts_find(name, prpl);
  if( acc ) {
    GSList *blist = purple_find_buddies(acc, NULL);

    PyObject *list = PyList_New(0);

    GSList *i;
    for(i = blist; i; i = i->next) {
      PurpleBuddy *buddy = i->data;
      PurpleGroup *group = purple_buddy_get_group( buddy );
      
      char *group_name = NULL;
      if( group )
        group_name = group->name;

      PyList_Append(list, Py_BuildValue("(sss)", purple_buddy_get_name(buddy), purple_buddy_get_contact_alias(buddy) , group_name));
    }

    g_slist_free(blist);

    return list;

  } else {
    printf("\n\nAccount not found!!\n\n");
    return NULL;
  }

}

void send_msg(const char *name, const char *prpl, const char *to, const char *msg)
{
  printf("Sending message to %s, msg=%s\n", to, msg);

  PurpleAccount *acc = purple_accounts_find(name, prpl);
  if( !acc ) {
    printf("WARNING!! Account not found...");
    return;
  }
  PurpleConversation *conv;
  // Find conversation
  conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, to, acc);
  if(!conv) {
    printf("Conversation not found, creating new one...\n");
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, acc, to);
  } else {
     printf("Conversation found..\n");
  }

  // Send msg
  purple_conv_im_send(PURPLE_CONV_IM(conv), msg);
  
}

gboolean pypurple_init()
{
  /*  initialize main event loop */
  ioloop_init();

  /*  initialize purple */
	purple_util_set_user_dir("/dev/null");
	purple_debug_set_enabled(FALSE);

  purple_eventloop_set_ui_ops(&ioloop_eventloops);
	purple_conversations_set_ui_ops(&pypurple_conv_uiops);

  if( ! purple_core_init(UI_ID) )
    return FALSE;

  printf("PyPurple Initilized successfully.");

	GList *iter = purple_plugins_get_protocols();
  int i;
	for (i = 0; iter; iter = iter->next) {
		PurplePlugin *plugin = iter->data;
		PurplePluginInfo *info = plugin->info;
		if (info && info->name) {
			printf("\t%d: %s %s\n", i++, info->id, info->name);
		}
	}

  /* Create and load the buddylist. */
	purple_set_blist(purple_blist_new());
	purple_blist_load();

  static int handle;
  // Connections
	purple_signal_connect(purple_connections_get_handle(), "signed-on", &handle,PURPLE_CALLBACK(on_connected), NULL);
	purple_signal_connect(purple_connections_get_handle(), "signed-off", &handle,PURPLE_CALLBACK(on_disconnected), NULL);
	purple_signal_connect(purple_connections_get_handle(), "connection-error", &handle,PURPLE_CALLBACK(on_connection_error), NULL);
  // Buddy list
  purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &handle, PURPLE_CALLBACK(on_buddy_signed_on), NULL);
  purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &handle, PURPLE_CALLBACK(on_buddy_signed_off), NULL);
  purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &handle, PURPLE_CALLBACK(on_buddy_status_changed), NULL);
  purple_signal_connect(purple_blist_get_handle(), "buddy-icon-changed", &handle, PURPLE_CALLBACK(on_buddy_icon_changed), NULL);

  return TRUE;
}




