from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import ConfigParser
import json

portmap = {}

parser = ConfigParser.RawConfigParser()
parser.read('chair.ini')
for sect in parser.sections():
    if parser.has_option(sect, 'macaddr') and parser.has_option(sect, 'port'):
        portmap[parser.get(sect, 'macaddr')] = parser.getint(sect, 'port')
    
class PortRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.set_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write('PECS Chair Port Server')

    def do_POST(self):
        doc_recvd = self.rfile.read(int(self.headers['Content-Length']))
        try:
            doc = json.loads(doc_recvd)
            macaddr = doc['macaddr']
            port = portmap.get(macaddr, "unknown MAC Address")
        except:
            self.send_response(400)
            return
        self.send_response(200)
        self.send_header('Content-type', 'text/json')
        self.end_headers()
        self.wfile.write(json.dumps({"port": port}))

serv = HTTPServer(('', 38000), PortRequestHandler)
serv.serve_forever()
