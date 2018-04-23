import os
import cmd
import time
import random
import readline
from hw import Hw


hw = Hw()


class Commands(cmd.Cmd):

    def __init__(self, se):
        self.se = se
        cmd.Cmd.__init__(self)
        self.prompt = '> '
        self.load_history()

    def wait_a_moment(self):
        time.sleep(0.2)

    def emptyline(self):
        pass

    def load_history(self):
        self.history_file = '/tmp/.%s-history' % __file__
        if os.path.exists(self.history_file):
            readline.read_history_file(self.history_file)
        readline.set_history_length(1000)

    def save_history(self):
        readline.write_history_file(self.history_file)

    def do_exit(self, line):
        self.save_history()
        return True

    def do_EOF(self, line):
        print
        self.save_history()
        return True

    def do_ping(self, line):
        print hw.ping()
        self.wait_a_moment()

    def do_dump(self, line):
        hw.dump_card_data()
        self.wait_a_moment()

    def do_get_uid(self, line):
        hw.get_card_uid()
        self.wait_a_moment()

    def do_use_auth_key(self, line):
        l = [int(i) for i in line.split() if i.isdigit()]
        if len(l) != 6:
            print 'error: should be 6 numbers between 0 to 255'
            return
        hw.use_auth_key(l)
        self.wait_a_moment()

    def do_unit_card(self, line):
        if not line.isdigit():
            print 'error: card no is not specified'
            return
        card_no = int(line)
        hw.write_unit_card(card_no)
        self.wait_a_moment()


def main():
    # se = hw.open_serial_port()
    # se = hw.se
    # hw.run_reader_thread()
    Commands(None).cmdloop()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print
