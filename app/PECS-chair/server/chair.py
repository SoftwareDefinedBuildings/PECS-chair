import json
from smap import actuate, driver, util
from chairActuator import ChairHeaterActuator
import time
from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.internet import reactor

# Consider data to be unknown if no FS data is received for this many seconds
INACTIVITY_GAP = 35

readings = {
    "backh": 0,
    "bottomh": 0,
    "backf": 0,
    "bottomf": 0,
    "occupancy": 0,
    "temperature": 0.0,
    "humidity": 0.0
}

translator = {
    "OFF": 0,
    "ON": 100,
    "LOW": 25,
    "MEDIUM": 50,
    "HIGH": 75,
    "MAX": 100
}

port = None

class ChairResource(Resource):
    isLeaf = True
    def __init__(self, *args):
        Resource.__init__(self, *args)
        self.lastAct = int(time.time())
        self.lasttruevaltime = 0
    def render_GET(self, request):
        doc = {"time": self.lastAct,
            "bottomh": readings["bottomh"],
            "backh": readings["backh"],
            "bottomf": readings["bottomf"],
            "backf": readings["backf"]}
        return json.dumps(doc)
    def render_POST(self, request):
        doc_recvd = request.content.read()
        print doc_recvd
        print "Got JSON at port", port
        doc = json.loads(doc_recvd)
        for key in doc:
            if key in readings:
                readings[key] = doc[key]
            elif key == "heaters":
                readings["bottomHeater"] = doc[key]
                readings["backHeater"] = doc[key]
            elif key == "fans":
                readings["bottomFan"] = doc[key]
                readings["backFan"] = doc[key]
        self.lastAct = int(time.time())
        if "fromFS" in doc and doc["fromFS"]:
            print "lasttruevaltime", port
            self.lasttruevaltime = self.lastAct
        return str(self.lastAct)

class PECSChairDriver(driver.SmapDriver):
    def setup(self, opts):
        self.state = readings.copy()
        self.macaddr = opts.get("macaddr")
        backh = self.add_timeseries('/backheater', '%', data_type='long')
        bottomh = self.add_timeseries('/bottomheater', '%', data_type='long')
        backf = self.add_timeseries('/backfan', '%', data_type='long')
        bottomf = self.add_timeseries('/bottomfan', '%', data_type='long')
        occ = self.add_timeseries('/occupancy', 'binary', data_type='long')
        temp = self.add_timeseries('/temperature', 'Celsius', data_type='double')
        hum = self.add_timeseries('/humidity', '%', data_type='double')

        archiver = opts.get('archiver')
        backh.add_actuator(ChairActuator(chair=self, key="backh", archiver=archiver))
        bottomh.add_actuator(ChairActuator(chair=self, key="bottomh", archiver=archiver))
        backf.add_actuator(ChairActuator(chair=self, key="backf", archiver=archiver))
        bottomf.add_actuator(ChairActuator(chair=self, key="bottomf", archiver=archiver))

        self.port = int(opts.get('port', 9001))

        global port
        port = self.port
        print "Setting up a chair driver with port", self.port

        self.resource = ChairResource()
        self.factory = Site(self.resource)

    def start(self):
        print "Starting a chair driver with port", self.port
        util.periodicSequentialCall(self.poll).start(15)
        reactor.listenTCP(self.port, self.factory)

    def poll(self):
        print "Polling a chair driver with port", self.port
        self.state = readings.copy()
        currTime = time.time()
        print currTime
        print self.resource.lasttruevaltime
        print currTime - self.resource.lasttruevaltime
        print INACTIVITY_GAP
        if currTime - self.resource.lasttruevaltime < INACTIVITY_GAP:
            print "Updating streams"
            self.add('/backheater', currTime, readings['backh'])
            self.add('/bottomheater', currTime, readings['bottomh'])
            self.add('/backfan', currTime, readings['backf'])
            self.add('/bottomfan', currTime, readings['bottomf'])
            self.add('/occupancy', currTime, 1 if readings['occupancy'] else 0)
            self.add('/temperature', currTime, readings['temperature'] / 100.0)
            self.add('/humidity', currTime, readings['humidity'] / 100.0)

class ChairActuator(actuate.ContinuousIntegerActuator):
    def __init__(self, **opts):
        datarange = (0, 100)
        actuate.SmapActuator.__init__(self, archiver_url=opts['archiver'])
        actuate.ContinuousIntegerActuator.__init__(self, datarange)
        self.chair = opts['chair']
        self.key = opts['key']

    def get_state(self, request):
        return self.chair.state[self.key]

    def set_state(self, request, state):
        print "GOT REQUEST:", self.key, state
        if not self.valid_state(state):
            print "WARNING: attempt to set to invalid state", state
            return self.chair.state[self.key]
        doc = {self.key: self.parse_state(state), "macaddr": self.chair.macaddr}
        requests.post("http://localhost:38001/", json.dumps(doc))
        print "Setting", self.key, "to", state
        return int(state)
