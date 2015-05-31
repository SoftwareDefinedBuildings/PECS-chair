import msgpack
import socket
from smap import actuate

PORT = 60001

class ChairHeaterActuator(actuate.BinaryActuator):
    def setup(self, opts):
        actuate.BinaryActuator.setup(self, opts)
        self.file = os.path.expanduser(opts['filename'])
        self.ip = opts['ip']
        self.dest_ip = opts['dest_ip']
        print('initialized')

    def get_state(self, request):
        with open(self.file, 'r') as fp:
            return int(fp.read())

    def set_state(self, request, state):
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        bytesToSend = msgpack.packb({"_id": 0, "toIP": self.ip, "heaters": "ON" if state else "OFF"})
        sock.sendTo(bytesToSend, (self.dest_ip, PORT))
        with open(self.file, 'w') as fp:
            fp.write(str(state))
        sock.close()
        return state
