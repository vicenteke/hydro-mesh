import os, sys, time, serial, argparse, requests

DOMAIN="http://150.162.62.3/data/hydro/put.php"

parser = argparse.ArgumentParser(description='EPOS Serial->IoT Gateway')
parser.add_argument('-d','--dev', help='EPOSMote III device descriptor file', default='/dev/ttyACM0')
parser.add_argument('-t','--timeout', help='Timeout for reading from mote', default=600)

args = vars(parser.parse_args())
DEV = args['dev']
TIMEOUT = int(args['timeout'])

def init_mote():
    global DEV
    global TIMEOUT

    ok = False
    while not ok:
        try:
            # time.sleep(3)
            print("Waiting for", DEV, "to appear")
            while not os.path.exists(DEV) or not os.access(DEV, os.W_OK):
                # print("Waiting for", DEV, "to appear")
                # if os.path.exists(DEV):
                #     print("exists")
                # else:
                #     print("not exists")
                # if os.access(DEV, os.W_OK):
                #     print("writeable")
                # else:
                #     print("now writeable")
                time.sleep(3)
            mote = serial.Serial(DEV, 115200, timeout = TIMEOUT, write_timeout = 10)
            mote.close()
            mote.open()
            ok = True
        except KeyboardInterrupt:
            raise
        except Exception as e:
            # print("Exception caught:", e, file=sys.stderr)
            ok = False
            time.sleep(3)

    # print("Mote open", file=sys.stderr)
    # if(SETUP):
    #     ts = int(time.time() * 1000000)
    #     str_ts = bytearray(8)
    #     pack_into('<Q', str_ts, 0, ts)
    #     try:
    #         mote.write(b'YYYX'+str_ts)
    #         print("epoch written", file=sys.stderr)
    #     except KeyboardInterrupt:
    #         raise
    #     except serial.serialutil.SerialTimeoutException:
    #         pass
    #
    # print("init_mote() done", file=sys.stderr)
    print("init_mote() done")
    return mote

mote = init_mote()
headers = {'Content-Type': 'application/json', 'Accept':'application/json'}

while True:
    buf = ['!']
    i = 0
    while buf[0] != '@':
        buf[0] = mote.read(1)

    buf[0] = mote.read(1)
    while buf[i] != '@':
        buf.append(mote.read(1))
        i += 1

    buf.pop()

    print("".join(buf))

    res = requests.post(DOMAIN, data="".join(buf), headers=headers)
    # res = requests.post(DOMAIN, data=json.dumps(buf), headers=headers)
    if res:
        print("Sent to", DOMAIN)
