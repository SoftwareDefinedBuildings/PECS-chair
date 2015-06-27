import msgpack
import random
import socket
import threading
import time

random.seed(time.time())

def empty(*args):
    return

def empty2(*args):
    return {}

# These don't support a close method. This isn't a problem, unless you plan to
# delete RNQClients or RNQServers. The general use case seems to be to just
# keep them open, so that isn't a problem for now.

class RNQClient(object):
    def __init__(self, port):
        self.socket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        self.socket.bind(('', port))
        def repeatedly_poll_socket():
            while True:
                data, addr = self.socket.recvfrom(1024)
                unpacked = msgpack.unpackb(data)
                if addr[:2] == self.currAddr[:2] and unpacked["_id"] == self.currID and self.pending:
                    self.pending = False
                    self.currSuccess(unpacked, addr)

        self.recvThread = threading.Thread(None, repeatedly_poll_socket)
        self.recvThread.daemon = True
        self.recvThread.start()

        self.currAddr = None
        self.pending = False
        self.ready = True
        self.pendingID = None

        self.currSuccess = empty

        self.queue = {}
        self.front = 1
        self.back = 1

        self.currID = int(random.getrandbits(16))

    def sendMessage(self, message, address, timesToTry=None, timeBetweenTries=None, eachTry=None, callback=None):
        callback = callback or empty
        eachTry = eachTry or empty
        self.queue[self.back] = {
            "msg": message,
            "addr": address,
            "callback": callback,
            "tcallback": eachTry,
            "times": timesToTry,
            "period": timeBetweenTries
        }
        self.back = self.back + 1
    
        self.processNextFromQueue()

    def processNextFromQueue(self):
        if self.ready and not self.pending:
            if self.front == self.back:
                return # nothing left to process in the queue
            
            # Dequeue request
            req = self.queue[self.front]
            del self.queue[self.front]
            self.front = self.front + 1

            self.currAddr = req["addr"]
            message = req["msg"]
            message["_id"] = self.currID
            msg = msgpack.packb(message)

            self.currSuccess = req["callback"]
            tryCallback = req["tcallback"]

            self.pending = True
            self.ready = False

            timesToTry = req["times"] or 1000
            timeBetween = req["period"] or 0.050

            def send_until_ack():
                try:
                    i = 0
                    while self.pending and i < timesToTry:
                        self.socket.sendto(msg, self.currAddr)
                        tryCallback()
                        time.sleep(timeBetween)
                        i = i + 1
                    if self.pending:
                        time.sleep(0.500) # wait a bit in case one of the last tries was heard
                    self.currSuccess = empty
                    if self.pending:
                        self.pending = False
                        req["callback"](None, None)
                finally:
                    self.pending = False
                    self.currID = int(random.getrandbits(16))
                    self.ready = True
                    self.processNextFromQueue()

            sendThread = threading.Thread(None, send_until_ack)
            sendThread.daemon = True
            sendThread.start()

    def cancelMessage(self):
        self.pending = False

    def empty(self):
        self.front = 1
        self.back = 1

class RNQServer(object):
    def __init__(self, port, responseGenerator=None):
        self.currIDs = {}
        responseGenerator = responseGenerator or empty2
        self.socket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        self.socket.bind(('', port))
        def repeatedly_poll_socket():
            while True:
                payload, addr = self.socket.recvfrom(1024)
                message = msgpack.unpackb(payload)
                id = message["_id"]
                if addr not in self.currIDs:
                    self.currIDs[addr] = {}
                if self.currIDs[addr].get("id", None) != id:
                    response = responseGenerator(message, addr)
                    response["_id"] = id
                    toReply = msgpack.packb(response)
                    self.currIDs[addr]["id"] = id
                    self.currIDs[addr]["reply"] = toReply
                else:
                    toReply = self.currIDs[addr]["reply"]
                self.socket.sendto(toReply, addr)
        self.recvThread = threading.Thread(None, repeatedly_poll_socket)
        self.recvThread.daemon = True
        self.recvThread.start()
