#!/usr/bin/python
import pyqrcode
import sys

verbose = False
if len(sys.argv) >= 2:
    if sys.argv[1] in ("-v", "--verbose"):
        verbose = True
    else:
        print "Unknown option {0}".format(sys.argv[1])
        print "Usage: -v or --verbose for verbose output"
        sys.exit(0)

SERVER = "shell.storm.pm"
PORT = 37998

serial_number = raw_input("Enter serial number: ")
blmacaddr = raw_input("Enter bluetooth mac address: ")
blname = raw_input("Enter chair name: ")

url = "http://{0}:{1}?chairid={2}&name={3}&wifimac={4}"

print "Generating QR Code..."
qrcode = pyqrcode.create(url.format(SERVER, PORT, serial_number, blmacaddr, blname))
if verbose:
    print qrcode.terminal()

print "Writing to file..."
with open('{0}_{1}_{2}.png'.format(serial_number, blmacaddr, blname), 'w') as f:
    qrcode.png(f, scale=6)

print "Done"

