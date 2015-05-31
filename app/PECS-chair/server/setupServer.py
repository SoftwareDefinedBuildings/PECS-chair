from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import ConfigParser
import json
import requests
import socket
import urlparse
from string import Template

ID_KEY = 'chairid'
APP_LINK = 'http://www.android.com'

PAGE_TEMPLATE = Template(Template('''<!DOCTYPE HTML>
<html lang="en-US">
    <head>
        <meta name="chairid" content="$chairid">
        <meta charset="UTF-8">
        <meta http-equiv="refresh" content="1;url=$app_link">
        <script type="text/javascript">
            window.location.href = "$app_link"
        </script>
        <title>PECS Chair Setup</title>
    </head>
    <body>
        If you are not redirected automatically, follow the <a href='$app_link'>link to download app</a>
    </body>
</html>
''').safe_substitute(app_link=APP_LINK))

def generate_page(chair_id):
    return PAGE_TEMPLATE.safe_substitute(chairid=chair_id)
    
class InitializationHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        try:
            path, tmp = self.path.split('?', 1)
            qs = urlparse.parse_qs(tmp)
            chair_id = qs[ID_KEY]
            chair_id = int(chair_id[0])

        except:
            print "sending 400: invalid"
            self.send_response(400)
            return
        if ID_KEY in qs:
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(generate_page(chair_id))
        else:
            print "sending 400: missing"
            self.send_response(400)
            return

serv = HTTPServer(('', 38002), InitializationHandler)
serv.serve_forever()
