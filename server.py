#!/usr/bin/env python
#
import tornado.httpserver
import tornado.httpclient
import tornado.ioloop
import tornado.options
import tornado.web
import tornado.escape
import simplejson as json
import time
import pypurple
import apikey_single as auth
import os.path

from tornado.options import define, options

define("port", default=8888, help="run on the given port", type=int)

# HTTP Callback #
class HTTPClient:
  def __init__(self,_apikey):
    self.client = tornado.httpclient.AsyncHTTPClient()
    self.apikey = _apikey

  def handle_response(self, response):
    print response;
    pass

  def http_call(self,args):
    url = auth.apikey_get(self.apikey, "callback_url")

    body = "&".join([ x+"="+tornado.escape.url_escape(str(args[x])) for x in args.keys()])
    req = tornado.httpclient.HTTPRequest(url, "POST", body=body)
    self.client.fetch(req, self.handle_response);
##

clients = {}
sessions = {}
connections = {}

def get_httpclient(apikey):
  global clients;

  if not apikey in clients:
    print "Creating new async http client for ", apikey
    clients[apikey] = HTTPClient(apikey);

  return clients[apikey]

def get_apikey(prpl, user):
  global sessions;
  print sessions
  return sessions[(prpl,user)]

def is_connected(prpl, user):
  return (prpl, user) in connections;

## Callback dispatch layer
def http_callback(prpl, user, args):
  prpl = prpl[5:]     # strip prpl- prefix
  apikey = get_apikey(prpl, user)

  args["prpl"] = prpl
  args["account"] = user
  args["apikey"] = apikey

  # get http client from pool
  client = get_httpclient(apikey)
  client.http_call(args)


# PyPurple stuff

def on_msg(prpl, user, frm, msg, timestamp):
  http_callback(prpl, user, {"event": "message", "from":frm, "message":msg, "timestamp":timestamp});

def on_connected(prpl, user):
  http_callback(prpl, user, {"event": "connected"});
  connections[(prpl[5:], user)] = True;
def on_disconnected(prpl, user):
  http_callback(prpl, user, {"event": "disconnected"});
  # remove it from sessions
  sessions.pop((prpl[5:],user));
  if is_connected(prpl[5:], user):
    connections.pop((prpl[5:], user));
def on_connection_error(prpl, user, error):
  http_callback(prpl, user, {"event": "connection_error", "reason": error});

def on_buddy_sign_on(prpl, user, buddy):
  http_callback(prpl, user, {"event": "buddy_sign_on", "buddy": buddy});
def on_buddy_sign_off(prpl, user, buddy):
  http_callback(prpl, user, {"event": "buddy_sign_off", "buddy": buddy});
def on_buddy_status(prpl, user, buddy, status, message):
  http_callback(prpl, user, {"event": "buddy_status", "buddy": buddy, "status": status, "message": message});
def on_buddy_icon(prpl, user, buddy):
  file = pypurple.buddy_get_icon(user, prpl, buddy)
  http_callback(prpl, user, {"event": "buddy_icon", "buddy": buddy, "url": file})


ioloop = tornado.ioloop.IOLoop.instance()

def timeout_add(interval, obj):
  return ioloop.add_timeout(time.time() + float(interval)/1000, lambda : pypurple.timeout_invoke(obj) )
def timeout_remove(obj):
  ioloop.remove_timeout(obj)
def input_add(fd, cond):
  ioloop.add_handler(fd, pypurple.io_invoke, cond)
def input_remove(fd):
  ioloop.remove_handler(fd)


pypurple.set_eventloop_ops( input_add, input_remove, timeout_add, timeout_remove ) 
pypurple.pypurple_init()

pypurple.set_msg_cb( on_msg )
pypurple.set_connection_cb( on_connected, on_disconnected, on_connection_error)
pypurple.set_buddy_cb( on_buddy_sign_on, on_buddy_sign_off, on_buddy_status)
pypurple.set_buddy_icon_cb( on_buddy_icon )

# Enf of PyPurple


class Application():
  def login(self, prpl, account, password):
    print "Login "+prpl+" "+account
    pypurple.account_new(str("prpl-"+prpl), str(account), str(password))
    return {"result": "Success"}

  def logout(self, prpl, account):
    print "Logout "+prpl+" "+account
    
    if not is_connected(prpl, account):
      return result_error("Account not connected")

    pypurple.account_set_status(str(account), str("prpl-"+prpl), "offline")
    return {"result": "Success"}
  
  def get_buddies(self, prpl, account):
    print "Get buddies "+prpl+" "+account

    # account connected?
    if not is_connected(prpl, account):
      return result_error("Account not connected")

    blist = pypurple.account_get_blist(str(account), str("prpl-"+prpl))
    return {"result": "Success", "buddylist": blist}

  def change_status(self, prpl, account, status, message):
    print "Change status "+prpl+" "+account

    if not is_connected(prpl, account):
      return result_error("Account not connected")

    pypurple.account_set_status(str(account), str("prpl-"+prpl), str(status))
    pypurple.account_set_status_message(str(account), str("prpl-"+prpl), str(message))
    return {"result": "Success"}

  def send_im(self, prpl, account, to, message):
    print "Send IM "+prpl+" "+account

    if not is_connected(prpl, account):
      return result_error("Account not connected")

    pypurple.send_msg(str(account), str("prpl-"+prpl), str(to), str(message))
    return {"result": "Success"}
  
app = Application()

def result_error(error_msg):
  return {"result": "Error", "error_msg": error_msg}

class ApiHandler(tornado.web.RequestHandler):
  def post(self):
    method = self.get_argument('method', "invalid");
    apikey = self.get_argument('apikey', "invalid");
    prpl = str(self.get_argument('prpl', "invalid"));
    account = str(self.get_argument('account', "invalid"));

    # Dispatcher
    if not auth.is_valid_key(apikey):
      self.write( json.dumps(result_error("Invalid argument: APIKEY")) );
      return

    if not method in ["login", "logout", "send_im", "get_buddies", "change_status", "stats"]:
      self.write( json.dumps(result_error("Invalid argument: METHOD")) );
      return

    if not prpl in ["yahoo", "msn", "jabber", "aim"]:
      self.write( json.dumps( result_error("Invalid argument: PRPL")));
      return

    if account=="invalid":
      self.write( json.dumps( result_error("Invalid argument: ACCOUNT")));
      return


    global sessions
    global connections
    # Session check
    if (prpl,account) in sessions:
      if not sessions[(prpl, account)] == apikey:
        self.write( json.dumps(result_error("Account already in use")) )
        return
    if method=="login":
      sessions[(prpl,account)] = apikey
    
    global app

    if method=="login":
      ret = app.login(prpl, account, self.get_argument('password', ''));
    if method=="logout":
      ret = app.logout(prpl, account);
    if method=="get_buddies":
      ret = app.get_buddies(prpl, account);
    if method=="change_status":
      ret = app.change_status(prpl, account, self.get_argument('status'), self.get_argument('message', ''));
    if method=="send_im":
      ret = app.send_im(prpl, account, self.get_argument('to'), self.get_argument('message'));

    if method=="stats":
      print "Sessions: "
      print sessions
      print "Connections: "
      print connections
      ret = {"result": "Success", "connections" : len(connections), "sessions" : len(sessions)};

    self.write( json.dumps(ret) )


def main():
    tornado.options.parse_command_line()
    settings = dict(static_path=os.path.join(os.path.dirname(__file__), "static"))
    application = tornado.web.Application([
        (r"/api", ApiHandler)
    ], **settings)
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(options.port)
    tornado.ioloop.IOLoop.instance().start()


print ""
if __name__ == "__main__":
    main()
