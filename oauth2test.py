#login
#curl -I

#granting access redirects to
#http://www.stuffmatic.com/?code=fe57277eae295fe3352d&state=abc123

#exchange code for an access token
#curl -I https://github.com/login/oauth/access_token?client_id=6c585bdf3d7aa7fc0182

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import requests

PORT_NUMBER = 8000

def printResponse(r):
    print "----------HEADER-----------"
    for h in r.headers:
        print h, ":", r.headers[h]
    print "----------BODY-----------"
    print r.text


class TestServer(BaseHTTPRequestHandler):
	
    def do_GET(self):
        BaseHTTPRequestHandler.do_GET(self)
        print "LOOLZ"

try:
    #Create a web server and define the handler to manage the
    #incoming request
    print dir(HTTPServer)
    server = HTTPServer(('', PORT_NUMBER), TestServer)
    print 'Started httpserver on port ' , PORT_NUMBER
    server.start()
    loginUrl = "https://github.com/login/oauth/authorize?client_id=6c585bdf3d7aa7fc0182&state=abc123"

    print 'sending request'
    r = requests.get(loginUrl)
    printResponse(r)
    

except KeyboardInterrupt:
    print '^C received, shutting down the web server'
    server.socket.close()


