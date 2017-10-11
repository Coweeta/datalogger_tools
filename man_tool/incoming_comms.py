import logging

logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

class ProtocolException(Exception):
    """The  data logger has reported an error with the communications.

    """


class IncomingComms:

    def __init__(self, serial_stream):
        self._s = serial_stream
        self._b = {
            '#': [],
            '!': [],
            ' ': []}

    def flush(self):
        others = {}
        stuff = self._s.readlines()
        for line in stuff:
            line = str(line, 'utf-8')
            cmd_char = line[0]
            body = line[1:-1]
            if cmd_char in self._b.keys():
                self._b[cmd_char].append(body)
            else:
                if cmd_char not in others.keys():
                    others[cmd_char] = []
                others[cmd_char].append(body)
        return others


    def get_file_text(self):
        return

    def get_log_dump(self):
        dump = self._b['!']
        self._b['!'] = []
        return dump

    #def handle_file_dump(self, line):
    #    self._f += line[1:]


    def read_stuff(self, expect_cmd_char, multiline=False):
        if multiline:
            result = []
        else:
            result = ''
        logging.debug("CHAR: '" + expect_cmd_char + "'")

        still_going = True
        while still_going:

            stuff = self._s.readlines()
            logging.debug("LINES: " + str(len(stuff)))
            # if self.remainder:
            #     stuff[0] = self.remainder + stuff[0]
            # if stuff[-1][-1] != '\n':
            #     self.remainder = stuff[-1]
            #     stuff = stuff[:-1]
            for line in stuff:
                line = str(line, 'utf-8')
                #TEMP!!! logging.debug("LINE1: " + line + str(still_going))
                cmd_char = line[0]
                body = line[1:-1]
                #TEMP!!! logging.debug("LINE2: '" + cmd_char + "' " + body)
                if cmd_char == expect_cmd_char:
                    #TEMP!!! logging.debug("LINE3: '" + cmd_char + "' " + body)
                    if not still_going:
                        raise ProtocolException("got extra", body)
                    if multiline:
                        if len(body) == 0:
                            still_going = False
                        else:
                            result.append(body)
                    else:
                        still_going = False
                        result = body
                elif cmd_char == 'X':
                    raise ProtocolException(body)
                else:
                    #TEMP!!! print((cmd_char * 10) + '\n')
                    self._b[cmd_char].append(body)

        logging.debug("LINE3: '" + str(self._b))

        return result



    def sizes(self):
        return {key:len(self._b[key]) for key in self._b}


    def dump(self, cmd_char):
        text = self._b[cmd_char]
        self._b[cmd_char] = []
        return text

