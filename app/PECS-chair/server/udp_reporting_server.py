import socket
import requests
import msgpack
import json
import ConfigParser

ipmap = {}

parser = ConfigParser.RawConfigParser()
parser.read('chair.ini')
for sect in parser.sections():
    if (parser.has_option(sect, 'macaddr') and
            parser.has_option(sect, 'rel_ip') and
            parser.has_option(sect, 'dest_ip') and
            parser.has_option(sect, 'port'):
        ipmap[parser.get(sect, 'macaddr')] = (parser.get(sect, 'rel_ip'),
                                              parser.get(sect, 'dest_ip'),
                                              parser.get(sect, 'port'))

sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.bind(("::", 39000))

while True:
    data, addr = sock.recvfrom(1024)
    print("From addr {} received {}".format(addr, data))
    unpacked = msgpack.unpackb(data)

    try:
        ips = ipmap[unpacked.pop('macaddr')]
    except:
        print("Error processing message from socket")
        continue
    encoded = json.JSONEncoder().encode(unpacked)
    res = requests.post("http://localhost:{0}/".format(ips[2]), json.dumps(encoded))
    if res.status_code != 200:
        print("Error updating smap")
