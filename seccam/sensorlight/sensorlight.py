#!/usr/bin/env python
# -*- coding: utf-8 -*-

import BaseHTTPServer
import serial
from SocketServer import ThreadingMixIn
from urlparse import parse_qs
import threading
import time

class APHttpServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
    pass

class APHttpServerFactory(object):
    @classmethod
    def createHttpServer(cls, portNumber, apHooks):
        handler = APHttpServeRequestHandler
        handler.apHooks = apHooks
        return APHttpServer(('',portNumber), handler)

class APHttpServeRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def do_GET(self):
        isHandled = False
        if self.is_rpc():
            isHandled = self.handle_rpc()

        if not isHandled:
            self.send_error(404, '')

    def do_POST(self):
        self.send_error(404, '')

    do_HEAD = do_GET

    def is_rpc(self):
        return self.path.startswith('/rpc')

    def handle_rpc(self):
        GET_RAW='/rpc/get_raw'
        GET_INFO='/rpc/get_info'
        SET_MODE='/rpc/set_mode'


        if self.path.startswith(GET_RAW):
            result = "get_raw no implemented"
            self._make_ok_response(result)
            return True

        elif self.path.startswith(GET_INFO):
            result = "get_info no implemented"
            self._make_ok_response(result)
            return True

        elif self.path.startswith(SET_MODE):
            argsDict = self._get_query_string_args()
            if 'mode' in argsDict:
                mode = int(argsDict['mode'][0])
            else:
                mode = 2
            result = self.apHooks.set_mode(mode)
            self._make_ok_response(result)
            return True

        else: return False

    def _make_ok_response(self, responseJson):
        self.send_response(200)
        self.send_header('Content-type', 'text/html; charset=utf-8')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", 0)
        self.end_headers()
        if self.command == 'GET':
            self.wfile.write(responseJson)

    def _get_query_string_args(self):
        if '?' in self.path:
            return parse_qs(self.path[(self.path.index('?')+1):])
        else:
            return {}


class APHooks(object):
    def __init__(self):
        #self.ser = serial.Serial('/dev/ttyUSB0', 57600)
        self.ser = serial.Serial('/dev/cu.usbserial-FTWI1JBN', 57600)
    def set_mode(self, mode):
        self.ser.write('#m %d$'%mode)
        return 'done'

    def get_info(self):
        self.ser.write('#i% $')
        return 'no implemented'

    def get_raw(self):
        self.ser.write('#r% $')
        return 'no implemented'


if __name__ == '__main__':
    global gHttpServer
    global ah 
    ah = APHooks()
    server = APHttpServerFactory.createHttpServer(8090, ah)
    thr = threading.Thread(None, server.serve_forever)
    thr.daemon = False
    thr.start()
    gHttpServer = server

    try:
        while True:
            time.sleep(5)
    except KeyboardInterrupt:
        print '\nbreak'

