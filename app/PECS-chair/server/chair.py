import json
from smap import actuate, driver, util
from chairActuator import ChairHeaterActuator
import time
from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.internet import reactor

# Consider data to be unknown if no FS data is received for this many seconds
INACTIVITY_GAP = 35

def append_to_driver(driver):
    driver.readings = {
        "backh": 0,
        "bottomh": 0,
        "backf": 0,
        "bottomf": 0,
        "occupancy": 0,
        "temperature": 0.0,
        "humidity": 0.0
    }

    driver.settings = {
        "backh": 0,
        "bottomh": 0,
        "backf": 0,
        "bottomf": 0
    }

class ChairResource(Resource):
    isLeaf = True
    def __init__(self, driverInst, *args):
        Resource.__init__(self, *args)
        self.driver = driverInst
        self.lastAct = time.time()
        self.lasttruevaltime = 0
        self.lasthistvaltime = 1
    def render_GET(self, request):
        settings = self.driver.settings
        doc = {"time": self.lastAct,
            "bottomh": settings["bottomh"],
            "backh": settings["backh"],
            "bottomf": settings["bottomf"],
            "backf": settings["backf"]}
        return json.dumps(doc)
    def render_POST(self, request):
        doc_recvd = request.content.read()
        print doc_recvd
        print "Got JSON at port", self.driver.port
        sdriver = self.driver
        doc = json.loads(doc_recvd)
        if "timestamp" in doc:
            if self.driver is None:
                print 'driver is none?!'
                # Make sure an ACK doesn't get sent back!
                return 'failure'
            print "HANDLING HISTORICAL POINT"
            ptTime = doc["timestamp"]
            print "ptTime:", ptTime
            print "boundary:", self.lasthistvaltime
            if ptTime <= self.lasthistvaltime:
                print 'pttime',ptTime,'before bound',self.lasthistvaltime
                return 'success'
            else:
                print "ADDING HISTORICAL POINT TO SMAP"
                sdriver.add("/backheater_hist", ptTime, doc["backh"])
                sdriver.add("/bottomheater_hist", ptTime, doc["bottomh"])
                sdriver.add("/backfan_hist", ptTime, doc["backf"])
                sdriver.add("/bottomfan_hist", ptTime, doc["bottomf"])
                sdriver.add("/occupancy_hist", ptTime, 1 if doc["occupancy"] else 0)
                sdriver.add("/temperature_hist", ptTime, doc["temperature"] / 100.0)
                sdriver.add("/humidity_hist", ptTime, doc["humidity"] / 100.0)
                self.lasthistvaltime = ptTime
                return 'success'
        else:
            isReading = "fromFS" in doc and doc["fromFS"]
            for key in doc:
                if key in self.driver.settings:
                    self.driver.settings[key] = doc[key]
                if isReading and key in self.driver.readings:
                    self.driver.readings[key] = doc[key]
            self.lastAct = time.time()
            if "fromFS" in doc and doc["fromFS"]:
                print "lasttruevaltime", self.driver.port
                self.lasttruevaltime = self.lastAct
        print 'returning something else'
        return str(self.lastAct)

class PECSChairDriver(driver.SmapDriver):
    def setup(self, opts):
        append_to_driver(self)
        self.state = self.readings.copy()
        self.macaddr = opts.get("macaddr")
        self.backh = self.add_timeseries('/backheater', '%', data_type='long')
        self.bottomh = self.add_timeseries('/bottomheater', '%', data_type='long')
        self.backf = self.add_timeseries('/backfan', '%', data_type='long')
        self.bottomf = self.add_timeseries('/bottomfan', '%', data_type='long')
        self.occ = self.add_timeseries('/occupancy', 'binary', data_type='long')
        self.temp = self.add_timeseries('/temperature', 'Celsius', data_type='double')
        self.hum = self.add_timeseries('/humidity', '%', data_type='double')
        
        self.backh_hist = self.add_timeseries('/backheater_hist', '%', data_type='long')
        self.bottomh_hist = self.add_timeseries('/bottomheater_hist', '%', data_type='long')
        self.backf_hist = self.add_timeseries('/backfan_hist', '%', data_type='long')
        self.bottomf_hist = self.add_timeseries('/bottomfan_hist', '%', data_type='long')
        self.occ_hist = self.add_timeseries('/occupancy_hist', 'binary', data_type='long')
        self.temp_hist = self.add_timeseries('/temperature_hist', 'Celsius', data_type='double')
        self.hum_hist = self.add_timeseries('/humidity_hist', '%', data_type='double')

        archiver = opts.get('archiver')
        self.backh.add_actuator(ChairActuator(chair=self, key="backh", archiver=archiver))
        self.bottomh.add_actuator(ChairActuator(chair=self, key="bottomh", archiver=archiver))
        self.backf.add_actuator(ChairActuator(chair=self, key="backf", archiver=archiver))
        self.bottomf.add_actuator(ChairActuator(chair=self, key="bottomf", archiver=archiver))

        self.port = int(opts.get('port', 9001))

        print "Setting up a chair driver with port", self.port

        self.resource = ChairResource(self)
        self.factory = Site(self.resource)

    def start(self):
        print "Starting a chair driver with port", self.port
        util.periodicSequentialCall(self.poll).start(15)
        reactor.listenTCP(self.port, self.factory)

    def poll(self):
        print "Polling a chair driver with port", self.port
        self.state = self.readings.copy()
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
