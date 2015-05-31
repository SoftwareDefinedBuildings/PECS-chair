import rnq

rnqc = rnq.RNQClient(60001)
rnqs = rnq.RNQServer(60002)

def myprint(x):
    print x
