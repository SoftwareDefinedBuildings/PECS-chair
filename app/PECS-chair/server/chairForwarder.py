import json
import msgpack
import requests
import rnq
import socket
import time

def handlemsg(received, addr):
    newmsg = {}
    newmsg['macaddr'] = hex(received[1])[-4:]
    newmsg['occupancy'] = received[2]
    newmsg['backh'] = received[3]
    newmsg['bottomh'] = received[4]
    newmsg['backf'] = received[5]
    newmsg['bottomf'] = received[6]
    newmsg['temperature'] = received[7]
    newmsg['humidity'] = received[8]
    newmsg['fromFS'] = True
    newmsg['myIP'] = addr[0]
    jsonData = json.dumps(newmsg)
    print "Received:", jsonData
    r = requests.post("http://localhost:38001", data=jsonData)
    print r.text
    return {"rv": "ok"}

listener = rnq.RNQServer(38003, handlemsg)

while True:
    time.sleep(60)
