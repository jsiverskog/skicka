# -*- coding: utf-8 -*-

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import urlparse

PORT_NUMBER = 8000

class TestServer(BaseHTTPRequestHandler):
    '''
    A simple web server for testing HTTP requests
    returning various types of data.
    '''
    def do_GET(self):
        path = urlparse.urlparse(self.path).path
        
        self.send_response(200, 'OK')
        self.end_headers()
        print "PATH", path
        if path == "/json":
            self.wfile.write('{"key":"räksmörgås"}')
        elif path == "/xml":
            self.wfile.write('<root></root>')
        elif path == "/raw":
            self.wfile.write('Räksmörgås.')


try:
    server = HTTPServer(('', PORT_NUMBER), TestServer)
    print 'Started test HTTP server on port ' , PORT_NUMBER
    server.serve_forever()

except KeyboardInterrupt:
    print '^C received, shutting down the web server'
    server.socket.close()


