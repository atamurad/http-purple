#!/usr/bin/python

import redis
r = redis.Redis()

def is_valid_key(apikey):
  global r
  return r.exists('api_settings_'+apikey+'_email')

def apikey_get_all(apikey):
  global r
  keys = r.keys('api_settings_'+apikey+'_*')
  info = {}
  for key in keys:
    value = r.get(key)
    info[key[len('api_settings_'+apikey+'_'):]] = value
  return info

def apikey_get(apikey, key):
  global r
  return r.get('api_settings_'+apikey+'_'+key)

