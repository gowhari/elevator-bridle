import time
import crypto
import random
import serial
import termios
import threading
import serial.tools.list_ports


magic_bytes = [0x65, 0xd6, 0xbe, 0x42]
hw_cmd = {
    'ping': 1,
    'dump_card_data': 2,
    'dump_card_data_by_lib': 3,
    'get_card_uid': 4,
    'write_card_block': 5,
    'use_auth_key': 6,
    'set_card_auth_keys': 7,
    'fill_card_random': 8,
    'read_card_block': 10,
}


class Error(Exception):
    pass


def checksum(l):
    c = 0
    for i, v in enumerate(l):
        c ^= (v + i) & 0xff
    return c & 0xff


class Hw(object):

    def __init__(self, baudrate=115200):
        self.se = self.open_serial_port(baudrate)


    def open_serial_port(self, baudrate):
        ports = [i.device for i in serial.tools.list_ports.comports()]
        assert len(ports) == 1, ports
        port = ports[0]
        f = open(port)
        attrs = termios.tcgetattr(f)
        attrs[2] = attrs[2] & ~termios.HUPCL
        termios.tcsetattr(f, termios.TCSAFLUSH, attrs)
        f.close()
        se = serial.Serial()
        se.baudrate = baudrate
        se.port = port
        se.timeout = 0
        se.open()
        return se


    def check_error(self, s):
        if s.startswith('error:'):
            raise Error(s.lstrip('error:').strip())
        if s == '':
            raise Error('no data received')


    def read_raw_serial(self):
        return self.se.read(self.se.in_waiting)


    def clear_read_buffer(self):
        self.read_raw_serial()


    def read(self, timeout=0):
        t1 = time.time()
        s = self.read_raw_serial()
        while s == '' and timeout != 0 and (time.time() < t1 + timeout):
            time.sleep(0.1)
            s = self.read_raw_serial()
        s = s.replace('\r\n', '\n')
        if len(s) > 0 and s[-1] == '\n':
            s = s[:-1]
        return s


    def ping(self):
        self.clear_read_buffer()
        val = random.randint(0, 254)
        cmd = hw_cmd['ping']
        self.se.write('%c%c' % (cmd, val))
        ret = self.read(2)
        return ret == str(val + 1)


    def dump_card_data(self):
        cmd = hw_cmd['dump_card_data']
        self.se.write('%c' % cmd)
        res = ''
        s = self.read(11)
        if s == '' or s == 'error: no card found':
            return None
        while s != '':
            res += s
            s = self.read(1)
        print len(res)
        if len(res) != 1152:
            print 'invalid data len'
            print res.encode('hex')
            return
        # print [res]
        for i in range(64):
            s = res[18 * i:18 * (i + 1)]
            block = ord(s[0])
            success = ord(s[1])
            if success:
                print '%02d  %s' % (block, ' '.join(['%02x' % ord(i) for i in s[2:]]))
            else:
                print '%02d  %s' % (block, ' '.join(16 * ['--']))


    def get_card_uid(self):
        self.clear_read_buffer()
        cmd = hw_cmd['get_card_uid']
        self.se.write('%c' % cmd)
        res = self.read(11)
        self.check_error(res)
        if len(res) != 4:
            raise Error('invalid card uid', res)
        return res.encode('hex')


    def use_auth_key(self, key):
        self.clear_read_buffer()
        if type(key) != list and len(key) != 6:
            raise ValueError('must be list of 6 integers', key)
        key = key[:]
        key.append(checksum(key))
        s = ''.join(map(chr, key))
        cmd = hw_cmd['use_auth_key']
        self.se.write('%c%s' % (cmd, s))
        res = self.read(1)
        self.check_error(res)
        if res != 'success':
            raise Error('invalid result', res)


    def set_card_auth_keys(self, key):
        self.clear_read_buffer()
        if type(key) != list and len(key) != 6:
            raise ValueError('must be list of 6 integers', key)
        key = key[:]
        key.append(checksum(key))
        s = ''.join(map(chr, key))
        cmd = hw_cmd['set_card_auth_keys']
        self.se.write('%c%s' % (cmd, s))
        res = self.read(11)
        self.check_error(res)
        if res != 'success':
            raise Error('invalid result', res)


    def read_card_block(self, block):
        self.clear_read_buffer()
        cmd = hw_cmd['read_card_block']
        self.se.write('%c%c' % (cmd, block))
        s = self.read(11)
        self.check_error(s)
        res = ''
        while s != '':
            res += s
            s = self.read(1)
        if len(res) != 16:
            raise Error('invalid result', res)
        return res

    def write_card_block(self, block, data):
        self.clear_read_buffer()
        data = data[:]
        data.insert(0, block)
        data.append(checksum(data))
        s = ''.join(map(chr, data))
        cmd = hw_cmd['write_card_block']
        self.se.write('%c%s' % (cmd, s))
        res = self.read(11)
        self.check_error(res)
        if res != 'success':
            raise Error('invalid result', res)


    def fill_card_random(self):
        self.clear_read_buffer()
        cmd = hw_cmd['fill_card_random']
        self.se.write('%c' % (cmd))
        res = self.read(11)
        self.check_error(res)
        if res != 'success':
            raise Error('invalid result', res)


    def mk_first_block(self, card_type, card_no):
        l = [card_type, card_no >> 8, card_no & 0xff]
        l.extend(magic_bytes)
        blk = crypto.encrypt(l) + [random.randint(0, 255)]
        return blk


    def write_unit_card(self, card_no):
        card_type = 1
        block = 1
        l = self.mk_first_block(card_type, card_no)
        self.write_card_block(block, l)


    def read_serial_stream(self):
        while True:
            s = self.se.read()
            if s == '':
                continue
            time.sleep(0.1)
            s += self.se.read(self.se.in_waiting)
            s = s.replace('\r\n', '\n')
            if s[-1] == '\n':
                s = s[:-1]
            print s


    def run_reader_thread(self):
        t = threading.Thread(target=self.read_serial_stream, args=())
        t.daemon = True
        t.start()
        return t


# se = open_serial_port(115200)
