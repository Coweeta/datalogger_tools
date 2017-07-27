import time
import serial

class DataLoggerFault(Exception):
    pass

class DataLoggerInterface:
    """Wrapper to control Coweeta's Arduino based datalogger.

    """


    def __init__(self, device_name, debug=False):
        """Open interface to Gate-Sync box.

        deviceName would be something like "/dev/ttyUSB0"
        """
        self.ser = serial.Serial(device_name, 230400, xonxoff=1, rtscts=0, timeout=0.2)
        self.debug = debug
        self._download_file = None
        self._download_filename = None
        self._file_bytes_left = 0
        self.check_protocol_version()
        self._event_names = self._write_and_read("n")


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


    def get_event_names(self, trigger_mask=None):
        if trigger_mask is None:
            return self._event_names
        else:
            matching_events = []
            for i, name in enumerate(self._event_names):
                if trigger_mask & (1 << i):
                    matching_events.append(name)
            return matching_events


    def check_protocol_version(self):
        version_response = self._write_and_read("v")
        if len(version_response) != 1:
            raise DataLoggerFault("Device may not be a Coweeta data logger", version_response)
        version_string = str(version_response[0])
        if version_string[:3] != "COW":
            raise DataLoggerFault("Device may not be a Coweeta data logger", version_string)
        if version_string[3:] != "0.0":
            raise DataLoggerFault("Device uses newer protocol than this application supports", version_string)


    def wait_for_prompt(self):
        for attempt in range(50):
          got = self.ser.read()
          if got is not None:
              if got != ">":
                  raise Exception("didn't get prompt", got)
              return True
        return False


    def start_download_file(self, filename):
        self._download_filename = filename
        if self._file_bytes_left != 0:
            print("ERROR", self._file_bytes_left)   #TEMP!!! how to handle????

        self._flush()
        self.ser.write(bytes("G{}|".format(filename), 'utf-8'))
        size_line = self.ser.readline()
        print("###", size_line)
        self._file_bytes_left = int(size_line)
        self._download_file = open(filename, 'wb')


    def download_chunk(self):
        bytes = self.ser.read(min(5000, self._file_bytes_left))

        self._download_file.write(bytes)
        bytes_read = len(bytes)
        self._file_bytes_left -= bytes_read

        if self._file_bytes_left == 0:
            self._download_file.close()
            term = self.ser.readlines()
            if len(term) != 1 or term[0] != b">":
                print("ERROR", term)   #TEMP!!! how to handle????
            else:
                self._write_and_read("R{}|".format(self._download_filename))
        return (self._file_bytes_left, bytes_read)


    def abort_download(self):
        self.ser.write(bytes(" ", 'utf-8'))
        self._flush()
        self._download_file.close()
        self._file_bytes_left = 0


    def get_time_delta(self):
        lines = self._write_and_read("t")
        if len(lines) != 1:
            raise Exception("bad num lines", lines)
        their_time = int(lines[0])
        our_time = time.time()
        delta = their_time - our_time
        print("TDTDTD", delta)
        return delta


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



    def trigger_events(self, event_list):
        event_mask = 0
        for event_name in event_list:
            index = self._event_names.index(event_name)
            event_mask += 1 << index
        result = self._write_and_read("e{}".format(event_mask))
        if len(result) > 1:
            raise DataLoggerFault('bad log length', result)
        if len(result) == 0:
            return ''
        else:
            return result[0]


    def get_next_event(self):
        delay_str, event_str, enabled_str  = self._write_and_read("w")
        delay = int(delay_str)
        event_mask = int(event_str)
        next_event_names = []
        for i, name in enumerate(self._event_names):
            if event_mask & (1 << i):
                next_event_names.append(name)
        return delay, next_event_names


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






