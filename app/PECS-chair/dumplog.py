#!/usr/bin/python
import csv
import datetime
import stormloader.main
import stormloader.sl_api
import struct
import sys
from time import sleep

if '-f' in sys.argv[1:]:
    fast = True
else:
    fast = False

sl = stormloader.sl_api.StormLoader()

# First, ping the storm
sl.enter_bootload_mode()
try:
    sl.c_ping()
except sl_api.StormloaderException as e:
    print "Device did not respond:", e
    sys.exit(1)
    
foundserial = False
for i in xrange(16):
    serlist = sl.c_gattr(i, True)
    if serlist[0] == "serial":
        serlist = serlist[1]
        foundserial = True
        break
if not foundserial:
    print "Could not find serial number"
    exit()
assert len(serlist) == 2, "Bad serial number list: {0}".format(serlist)
sernum = (serlist[0] << 8) | serlist[1]
sernump = hex(sernum)[2:]
print "Detected serial number: {0}".format(sernump)

USERLAND_OFFSET = 0x80000 # Hardcoded offset from the Kernel
USERLAND_MAX = 0x4000000 # Addresses above these (after adding the offset) are invalid

USERLAND_LENGTH = USERLAND_MAX - USERLAND_OFFSET

CHUNK_SIZE = 7 # Number of pages to read at a time.

# These definitions are mostly taken from flash.h
PAGE_EXP = 8
PAGE_SIZE = 1 << PAGE_EXP # The size of a block

LOG_ENTRY_LEN = 14
LOG_START = 768

ENTRIES_PER_PAGE = PAGE_SIZE / LOG_ENTRY_LEN # Integer division

SP_OFFSET = 0
BO_OFFSET = 8
CP_OFFSET = 16

FIRST_VALID_ADDR = 768
FIRST_INVALID_ADDR = 7340032

assert USERLAND_OFFSET + FIRST_INVALID_ADDR < USERLAND_MAX, "Userland log bounds extend beyond space allowed by kernel"

print "Reading superblock"
superblock = sl.c_xrrange(USERLAND_OFFSET, 3 << PAGE_EXP)
sbcopies = tuple(superblock[i << PAGE_EXP:(i + 1) << PAGE_EXP] for i in xrange(3))

sps, bos, cps = ([0, 0, 0] for _ in xrange(3))
for i, sbcopy in enumerate(sbcopies):
    sps[i], _, bos[i], _, cps[i], _ = struct.unpack("<IIIIII", sbcopy[:24])

# Similar to read_sp_tail in flash.c
def read_sp_tail(sps):
    sp1, sp2, sp3 = sps
    if sp1 == sp2 and sp2 == sp3:
        return sp1
    else:
        print "Corrupt log pointer: read {0}, {1}, {2} (possibly due to interrupted superblock write)".format(*sps)
        if sp1 == sp2:
            return sp1
        elif sp2 == sp3:
            return sp2
        elif sp1 == sp3:
            return sp3
        
def check_bounds(sp):
    if sp < FIRST_VALID_ADDR or sp >= FIRST_INVALID_ADDR:
        print "Superblock appears to be irreparably corrupt"
        print "Double-check that this device contains a correctly formatted PECS log"
        exit()
        
sp = read_sp_tail(sps)
bo = read_sp_tail(bos)
cp = read_sp_tail(cps)
check_bounds(sp)
check_bounds(cp)

print "Superblock: sp = {0}, bo = {1}, cp = {2}".format(sp, bo, cp)
if ((sp % PAGE_SIZE) % LOG_ENTRY_LEN) != 0:
    print "SP appears to not be entry-aligned!"
    resp = None
    while resp not in ("yes", "no"):
        resp = raw_input("Continue? (yes/no): ")
    if resp == "no":
        exit()
# This is like get_log_size_tail
loglen = sp - LOG_START
page = loglen >> PAGE_EXP
page_offset = loglen - (page << PAGE_EXP)
index = page * ENTRIES_PER_PAGE + page_offset / LOG_ENTRY_LEN
# INDEX is the index of the next entry to be added to the log.
# That's the same as the length of the log.
print "Log contains {0} entries".format(index)

class LogEntry(object):
    def __init__(self, string, userlandaddr):
        self.userlandaddr = userlandaddr
        self.timestamp, self.backh, self.bottomh, self.backf, self.bottomf, \
        self.temp, self.humidity, bits, _ = struct.unpack("<iBBBBhhBB", string)
        self.temp /= 100.0
        self.humidity /= 100.0
        self.occupancy = (bits & 0x80) != 0
        self.reboot = (bits & 0x40) != 0
        self.knowntime = (bits & 0x20) != 0
        self.inferredtime = False
        self.attemptinfertime = False
        self.attemptinfersent = False
        
    def handleReboot(self):
        assert self.reboot, "Cannot perform reboot inference on non-reboot log entry"
        self.inferredtime = (self.timestamp != 0)
        self.sent = False
        self.attemptinfertime = True
        self.attemptinfersent = True
    
    def inferAbsoluteTime(self, offset):
        assert not self.attemptinfertime, "Inferring time twice for the same log entry"
        self.attemptinfertime = True
        if offset == 0:
            return
        if not self.knowntime:
            self.timestamp += offset
            self.inferredtime = True
            
    def inferSentStatus(self, cp):
        assert not self.attemptinfersent, "Inferring sent status twice for the same log entry"
        if self.userlandaddr < cp and (self.knowntime or self.inferredtime):
            self.sent = True
        else:
            self.sent = False
        self.attemptinfersent = True
        
    def todict(self):
        assert self.attemptinfertime and self.attemptinfersent, "Must infer time and sent status before converting to a dictionary"
        dct = {
            "Flash Address": self.userlandaddr,
            "Reboot": self.reboot,
            "Recorded Abs. Time": self.knowntime,
            "Inferred Abs. Time": self.inferredtime,
            "Timestamp": self.timestamp
        }
        if self.knowntime or self.inferredtime:
            dct["UTC Datetime"] = datetime.datetime.utcfromtimestamp(self.timestamp)
        if self.reboot:
            return dct
            
        dct["Back Heater"] = self.backh
        dct["Bottom Heater"] = self.bottomh
        dct["Back Fan"] = self.backf
        dct["Bottom Fan"] = self.bottomf
        dct["Occupancy"] = self.occupancy
        dct["Temperature"] = self.temp
        dct["Rel. Humidity"] = self.humidity
        
        return dct

resp = None
while resp not in ("yes", "no"):        
    resp = raw_input("Write CSV file {0}.csv? (yes/no) ".format(sernump))
if resp == "no":
    exit()

csvfile = open("{0}.csv".format(sernump), 'w')
fieldnames = ["Flash Address", "Reboot", "Recorded Abs. Time", "Inferred Abs. Time", "Timestamp", "UTC Datetime",
              "Back Heater", "Bottom Heater", "Back Fan", "Bottom Fan", "Occupancy", "Temperature", "Rel. Humidity"]
csvwriter = csv.DictWriter(csvfile, fieldnames=fieldnames)
csvwriter.writeheader()
        
print "Reading log entries"
# Iterate over the log
addr = LOG_START # userland address
rebootoffset = 0 # time offset in the last seen reboot
entriesread = 0
while addr < sp:
    toread = min(CHUNK_SIZE << PAGE_EXP, sp - addr)
    if not fast:
        sleep(0.1) # Add some delay. Otherwise we see more failures in a VM
    try:
        chunk = sl.c_xrrange(USERLAND_OFFSET + addr, toread)
    except stormloader.sl_api.CommsTimeoutException:
        print "Read failed. Retrying..."
        continue
    pageaddr = addr
    for pageid in xrange(CHUNK_SIZE):
        page = chunk[pageid << PAGE_EXP:min((pageid + 1) << PAGE_EXP, len(chunk))]
        entries_in_page = len(page) / LOG_ENTRY_LEN
        for entryid in xrange(entries_in_page):
            entriesread += 1
            logent = LogEntry(page[entryid * LOG_ENTRY_LEN:(entryid + 1) * LOG_ENTRY_LEN], (pageaddr + LOG_ENTRY_LEN * entryid))
            if logent.reboot:
                rebootoffset = logent.timestamp
                logent.handleReboot()
            else:
                logent.inferAbsoluteTime(rebootoffset)
                logent.inferSentStatus(cp)
            csvwriter.writerow(logent.todict())
        pageaddr += PAGE_SIZE
    print "Read {0} out of {1} entries".format(entriesread, index)
    addr += toread
    
csvfile.close()
print "Finished writing CSV file"
