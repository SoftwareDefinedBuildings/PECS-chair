import json
import msgpack
import requests
import rnq
import socket
import time

ack_sender = rnq.RNQClient(10000)

def ack_printer(*args):
    if len(args) > 0 and args[0] is not None:
        print "Successfully sent ACK"
    else:
        print "Could not send ACK"

def handlemsg(received, addr):
    print "Raw:", received
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
    ack_id = None
    if len(received) > 9:
        newmsg['timestamp'] = received[9]
        ack_id = received[10]
    jsonData = json.dumps(newmsg)
    print "Received:", jsonData
    try:
        r = requests.post("http://localhost:38001", data=jsonData)
    except requests.ConnectionError as e:
        print e,
        return {'rv': e}
    print r.text
    if ack_id is not None and r.text == 'success':
        print "Sending ACK:", ack_id
        ack_sender.sendMessage({"ack": ack_id, "toIP": "ff02:{0}".format(received[1])}, (addr[0], 20000), 100, 0.1, None, ack_printer)
    return {"rv": "ok"}

listener = rnq.RNQServer(38003, handlemsg)

while True:
    time.sleep(60)
