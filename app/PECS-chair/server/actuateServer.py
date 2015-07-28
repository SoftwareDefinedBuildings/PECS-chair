from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SocketServer import ThreadingMixIn
import ConfigParser
import json
import msgpack
import requests
import rnq
import socket
import sys
import time
import urlparse

settingMap = {
    "OFF": 0,
    "ON": 100,
    "LOW": 25,
    "MEDIUM": 50,
    "HIGH": 75,
    "MAX": 100
}

ipmap = {}

parser = ConfigParser.RawConfigParser()
parser.read('chair.ini')
for sect in parser.sections():
    if parser.has_option(sect, 'macaddr') and parser.has_option(sect, 'dest_ip') and parser.has_option(sect, 'port'):
        ipmap[parser.get(sect, 'macaddr')] = [parser.get(sect, 'dest_ip'), int(parser.get(sect, 'port'))]

FS_PORT = 60001

rnq_assign = {}
def get_rnqc(macaddr):
    if macaddr not in ipmap:
        print "Invalid macaddr"
    if macaddr in rnq_assign:
        return rnq_assign[macaddr]
    else:
        rnqc = rnq.RNQClient(ipmap[macaddr][1] - 1000)
        rnq_assign[macaddr] = rnqc
        return rnqc

def myprint(x):
    print x

timesynchronizer = rnq.RNQServer(38002, lambda msg, addr: {"time": time.time()})

class ActuationHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        print "Got GET request"
        try:
            path, tmp = self.path.split('?', 1)
            qs = urlparse.parse_qs(tmp)
            macaddr = qs['macaddr']
            ips = ipmap[macaddr[0].lower()]
        except:
            print "sending 400: invalid"
            self.send_response(400)
            return
        if 'macaddr' in qs:
            res = requests.get("http://localhost:{0}/".format(ips[1]))
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            print res.text
            self.wfile.write(res.text)
        else:
            print "sending 400: missing"
            self.send_response(400)
            return

    def do_POST(self):
        print "Got POST request"
        print self.headers
        doc_recvd = self.rfile.read(int(self.headers['Content-Length']))
        timestamp = None # So that I assign it before referencing it
        if doc_recvd == "{}":
            # Special notiation: get time on device
            print "Got time sync request"
            timestamp = time.time()
        else:
            print doc_recvd
            try:
                doc = json.loads(doc_recvd)
                macaddr = doc.pop('macaddr')
                ips = ipmap[macaddr]
            except:
                print "Invalid JSON or Mac Address"
                self.send_response(400)
                return
            print ips
            res = requests.post("http://localhost:{0}/".format(ips[1]), json.dumps(doc))
            if res.status_code != 200:
                self.send_response(404)
                self.end_headers()
                self.wfile.write("Could not update sMAP")
                return
            print "Successfully updated sMAP"
            if res.text in ('success', 'failure'):
                timestamp = res.text
            else:
                removeList = []
                timestamp = float(res.text)
                if 'myIP' in doc:
                    ips[0] = doc['myIP']
                if 'fromFS' not in doc or not doc['fromFS']:
                    with open("actuations/{0}".format(str(time.time())), 'w') as f:
                        f.write(doc_recvd)
                    for key in doc:
                        if key not in ["backh", "bottomh", "backf", "bottomf", "heaters", "fans"]:
                            removeList.append(key)
                    for key in removeList:
                        del doc[key]
                    if len(doc) != 0:
                        if "header" in doc:
                            del doc["header"]
                        print "Actuating chair"
                        print "IP", ips[0]
                        rnqc = get_rnqc(macaddr)
                        rnqc.cancelMessage() # stop trying to send the current actuation message
                        rnqc.empty() # pop pending actuations from queue
                        rnqc.sendMessage(doc, (ips[0], FS_PORT), 11, 1, lambda: myprint("trying"), lambda msg, addr: myprint(msg))
        self.send_response(200)
        self.send_header('Content-type', 'text/json')
        self.end_headers()
        self.wfile.write(str(timestamp))

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in separate threads to improve performance."""

serv = ThreadedHTTPServer(('', 38001), ActuationHandler)
serv.serve_forever()
