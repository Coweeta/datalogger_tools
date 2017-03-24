import time
import serial

class DataLoggerInterface:
    """Wrapper to control Coweeta's Arduino based datalogger.

    """


    def __init__(self, device_name, debug=False):
        """Open interface to Gate-Sync box.

        deviceName would be something like "/dev/ttyUSB0"
        """
        self.ser = serial.Serial(device_name, 115200, xonxoff=1, rtscts=0, timeout=0.1)
        self.debug = debug


    def __del__(self):
        """close connection on object destruction"""
        self.ser.close()


    def _flush(self):
        """Clear all existing output from the logger."""
        out = self.ser.readlines()
        if self.debug:
            print("flush got '{}'".format(out))


    def _write_and_read(self, text):
        """private method to send command and handle output"""
        self._flush()
        self.ser.write(bytes(text, 'utf-8'))
        lines = []
        while True:
            out = self.ser.readlines()
            if self.debug:
                print("'{0}' gave '{1}'".format(text, out))
            for byte_line in out:
                line = str(byte_line, 'utf-8').strip()
                if line == ">":
                    return lines
                lines.append(line)



    def wait_for_prompt(self):
        for attempt in range(50):
          got = self.ser.read()
          if got is not None:
              if got != ">":
                  raise Exception("didn't get prompt", got)
              return True
        return False


    def download_file(self, file_num, filename, and_delete=False):
        lines = self._write_and_read("G{}".format(file_num))
        with open(filename, 'w') as file:
            file.writelines([line + "\n" for line in lines])
        if and_delete:
            self._write_and_read("R{}".format(file_num))


    def sync_time(self):
        lines = self._write_and_read("t")
        if len(lines) != 1:
            raise Exception("bad num lines", lines)
        their_time = int(lines[0])
        our_time = time.time()
        delta = their_time - our_time
        if abs(delta) > 2:
            self._write_and_read("s{}".format(int(our_time)))
            return (True, delta)
        else:
            return (False, delta)


    def new_active_file(self, file_num):
        self._write_and_read("N{}".format(file_num))


    def get_active_file_num(self):
        lines = self._write_and_read("A")
        if len(lines) != 1:
            raise Exception("bad num lines", lines)
        return int(lines[0])


    def list_files(self):
        lines = self._write_and_read("L")
        file_list = []
        for line in lines:
            if "\t" in line:
                parts = line.split("\t")
                file_list.append((parts[0], int(parts[1])))
            else:
                file_list.append((line, None))
        return file_list

    def trigger_event(self, event_number):
        event_mask = 1 <<  event_number:
        self._write_and_read("e{}".format(event_mask))


    def close(self):
        self.ser.close()



# L{}".format(filenum))
#         self._writeRead("t")
#
#
#     def sendSync(self):
#         self._writeRead('S')
#
#
#     def gatePeriod(self, period):
#         """Sets the gate signal period.
#
#         Period given in milliseconds.
#         Keeps the signal at 50% duty cycle.
#         """
#         half = int(5 * period)
#         if half < 1:
#             raise Exception("bad period")
#         self._writeRead('{0}L{1}H'.format(half, half))
#
#
#     def holdGate(self, state):
#         """Clamps the gate signal high or low
#
#         state: True/1 to hold high, False/0 to hold low
#         """
#         if state:
#             self._writeRead('0H')
#         else:
#             self._writeRead('0L')
#
#
#     def setSyncTiming(self, before=20, during=20, after=20):
#         """Sets up the 3 durations in a sync pulse.
#
#         All times given in ms.
#         """
#         b, d, a = (int(x * 10) for x in [before, during, after])
#         if min(b, d, a) < 1:
#             raise Exception("bad duration(s)")
#         cmd = '{0}B{1}D{2}A'.format(b, d, a)
#         self._writeRead(cmd)
#
#     def toggleGate(self):
#         self._writeRead('T')
#
#
#     def getSettings(self):
#         """Returns the help output.
#
#         As a list of strings.
#         """
#         self.ser.write('?')
#         return self.ser.readlines()






