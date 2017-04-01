#!/usr/bin/env python

import datetime
import tkinter as tk
import tkinter.ttk as ttk

import coweeta_icons


import serial.tools.list_ports

import vert_scroll_frame as vsf
import progress_dialog as prog

import interface

class AboutDialog(tk.Frame):
    """Just display a simple about box.

    """
    def __init__(self, parent):

        top = tk.Toplevel(parent)

        image = tk.PhotoImage(data=coweeta_icons.usfs)
        panel = tk.Label(top, image=image)
        panel.image = image
        panel.pack(side='left', padx=10, pady=10)

        image = tk.PhotoImage(data=coweeta_icons.salamandar)
        panel = tk.Label(top, image=image)
        panel.image = image
        panel.pack(side='right', padx=10, pady=10)

        text = "\n".join([
                "Datalogger Management Tool",
                "Coweeta Hydrologic Laboratory",
                "Southern Research Station",
                "USDA Forest Service"])

        tk.Label(top, text=text).pack(side='top', padx=10, pady=10)

        tk.Button(top, text="OK", command=top.destroy).pack(side='bottom', padx=10, pady=10)

        top.title("About Coweeta Log")


class GuiLoggerInterface(tk.Frame):
    """A Tk based user interface for the Logger controller.

    """

    def __init__(self, callbacks, parent=None):
        """Create Tk GUI

        *callbacks* is a dictionary of functions
        *parent* is #TEMP!!!
        """
        parent.option_add("*Font", "helvetica 16")

        tk.Frame.__init__(self, parent)
        self._callbacks = callbacks
        self._parent = parent
        self._device_name = None
        self._make_menus()

        self._left_panel =  tk.Frame(self, relief='ridge', borderwidth=3)
        self._right_panel =  tk.Frame(self, relief='ridge', borderwidth=3)

        self._connect_button = tk.Button(self._left_panel, text="Connect", command=self._connect_to_port)
        self._connect_button.pack()

        self._set_up_file_list_frame()
        self._file_frame.pack(side='bottom', expand=True, fill='x')

        self._set_up_time_error_frame()
        self._time_frame.pack(side='bottom', expand=True, fill='x')

        self._set_up_log_time()
        self._log_frame.pack(side='bottom', expand=True, fill='x')

        self._parent.wm_title("Coweeta Log")

        self._left_panel.pack(side='left', expand=True)
        self._right_panel.pack(side='right', expand=True)

        self.after(1000, self._periodic_call)

    def _set_up_time_error_frame(self):
        self._time_frame = tk.Frame(self._left_panel, relief='ridge', borderwidth=3)
        time_text = tk.Label(self._time_frame, text='Time error')
        self._time_error_label = tk.Label(self._time_frame, relief="sunken")
        resync_button = tk.Button(self._time_frame, text="Resync Logger", command=self._callbacks['resync'])

        time_text.pack(side='left', expand=True)
        self._time_error_label.pack(side='left', expand=True)
        resync_button.pack(side='left', expand=True)

        self.update_time_discrepancy(None)


    def update_time_discrepancy(self, time_error):
        """Report the difference between our time and the logger's.

        If passed None then the field is set to blank.
        """
        if time_error is None:
            self._time_error_label.configure(text='-', foreground='#BBBBBB')
        elif abs(time_error) > 10000:
            self._time_error_label.configure(text='TOO BIG', foreground='#FF0000')
        else:
            text = "{:0.3f}s".format(time_error)
            self._time_error_label.configure(text=text, foreground='#000000')


    def _invert_file_selection(self):
        for file_sel in self._check_vars.values():
            file_sel.set(not file_sel.get())


    def _clear_file_selection(self):
        for file_sel in self._check_vars.values():
            file_sel.set(False)


    def _fetch_files(self):
        filenames = []
        for filename in self._check_vars:
            print("ZZZ", filename, self._check_vars[filename].get())
            if self._check_vars[filename].get():
                filenames.append(filename)
        total_size, first_filename = self._callbacks['begin_fetch'](filenames)
        dialog = prog.ProgressDialog(self, total_size, first_filename, self._callbacks['step_fetch'], self._callbacks['halt_fetch'])

        self.wait_window(dialog.top)


    def _refresh_file_list(self):
        file_list, active = self._callbacks['get_file_list']()
        self.populate_file_list(file_list, active)


    def _set_up_file_list_frame(self):
        import vert_scroll_frame as vsf

        self._file_frame = tk.Frame(self._left_panel, relief='ridge', borderwidth=3)
        self._list_frame = vsf.VerticalScrolledFrame(self._file_frame, background="#00ff00")

        f = tk.Frame(self._file_frame)
        tk.Button(f, text='Refresh', command=self._refresh_file_list).pack(side='left')
        tk.Button(f, text='Toggle', command=self._invert_file_selection).pack(side='left')
        tk.Button(f, text='Clear', command=self._clear_file_selection).pack(side='left')
        tk.Button(f, text='Fetch', command=self._fetch_files).pack(side='right')

        self._list_frame.pack(side='top')
        f.pack(side='bottom')

        self._file_widgets = []
        self._check_vars = {}




    def populate_file_list(self, file_list, active):
        self._check_vars = {}
        for check_button, filename_widget, size_widget in self._file_widgets:
            check_button.grid_forget()
            filename_widget.grid_forget()
            size_widget.grid_forget()
            del(check_button)
            del(filename_widget)
            del(size_widget)

        for row in range(len(file_list)):
            if row % 2:
                background = "#FFFFFF"
            else:
                background = "#BBBBBB"

            filename, size = file_list[row]
            if size is None:
                foreground = "#0000FF"
                size_text = ''
                check_button = tk.Label(self._list_frame.interior, background=background, text='')
            else:
                size_text = str(size)
                if filename == active:
                    foreground = "#00FF00"
                    check_button = tk.Label(self._list_frame.interior, background=background, text='')
                else:
                    foreground = "#000000"
                    self._check_vars[filename] = tk.IntVar()
                    check_button = tk.Checkbutton(self._list_frame.interior, background=background, text='', variable=self._check_vars[filename])

            filename_widget = tk.Label(self._list_frame.interior, text=filename, background=background, foreground=foreground)
            size_widget = tk.Label(self._list_frame.interior, text=size_text, background=background)

            check_button.grid(row=row, column=0, sticky='nsew')
            filename_widget.grid(row=row, column=1, sticky='nsew')
            size_widget.grid(row=row, column=2, sticky='nsew')

            self._file_widgets.append((check_button, filename_widget, size_widget))

    def _periodic_call(self):
        self._callbacks['periodic']()
        self.after(1000, self._periodic_call)



    def _set_up_log_time(self):
        self._log_frame = tk.Frame(self._right_panel, relief='ridge', borderwidth=3)
        time_text = tk.Label(self._log_frame, text='Time to next log event')
        self.wait_time_label = tk.Label(self._log_frame, relief="sunken")
        self.next_events_label = tk.Label(self._log_frame, relief="sunken")
        log_now_button = tk.Button(self._log_frame, text="Trigger Log Now", command=self._trigger_event)

        time_text.pack(side='left')
        self.wait_time_label.pack(side='left')
        self.next_events_label.pack(side='left')
        self._trigger_frame = tk.Frame(self._log_frame, relief='ridge', borderwidth=3)
        log_now_button.pack(side='left')
        self._trigger_frame.pack(side='bottom')
        self._trigger_vars = {}
        self._trigger_widgets = {}
        self._log_text = tk.Text(self._right_panel)
        self._log_text.pack(side='bottom')

        self.update_log_time(None)


    def update_log_time_window(self, event_names):
        for var in self._trigger_vars.values():
            del(var)
        for widget in self._trigger_widgets.values():
            del(widget)

        for i, name in enumerate(event_names):
            self._trigger_vars[name] = tk.IntVar()
            self._trigger_widgets[name] = tk.Checkbutton(self._trigger_frame, text=name, variable=self._trigger_vars[name])
            self._trigger_widgets[name].pack(side='bottom')


    def update_log_time(self, time_delay, events=None):
        if time_delay is None:
            self.wait_time_label.configure(text='-', foreground='#BBBBBB')
            self.next_events_label.configure(text='-', foreground='#BBBBBB')
        else:
            text = str(int(time_delay))
            print('#TEMP!!! ', text)
            self.wait_time_label.configure(text=text, foreground='#000000')
            text = ", ".join(events)
            self.next_events_label.configure(text=text, foreground='#000000')


    def _trigger_event(self):
        event_list = []
        for name, state in self._trigger_vars.items():
            if state.get():
                event_list.append(name)
        print(event_list)
        result = self._callbacks['trigger_event'](event_list)
        print(result)
        self._log_text.insert(tk.END, result + "\n")

    def _make_menus(self):
        menu_bar = tk.Menu(self)

        file_menu = tk.Menu(self, tearoff=False)
        file_menu.add_command(label="Quit", command=self.quit)
        menu_bar.add_cascade(label="File", menu=file_menu)

        self._make_com_port_menu(menu_bar)

        help_menu = tk.Menu(menu_bar, tearoff=False)
        help_menu.add_command(label="About", command=self._help_about)
        menu_bar.add_cascade(label="Help", menu=help_menu)

        self._parent.config(menu=menu_bar)


    def _select_port(self, device):
        self._device_name = device


    def _help_about(self):
        AboutDialog(self)


    def _make_com_port_menu(self, parent_menu):
        com_ports = serial.tools.list_ports.comports()
        port_menu = tk.Menu(self, tearoff=False)
        for port in com_ports:
            port_menu.add_command(label=port.device, command=lambda: self._select_port(port.device))
        parent_menu.add_cascade(label="Comm Ports", menu=port_menu)


    def _connect_to_port(self):
        self._callbacks['connect'](self._device_name)

    def _disconnect_from_port(self):
        self._callbacks['connect'](self._device_name)




if __name__ == "__main__":

    class Bob(object):

        def __init__(self):
            self.x = 0
            self.fn = None
            self.step = None
            self.control = None
            self._current_fetch = None
            self._fetch_filenames = []
            self._downloading = False


        def _update_file_list(self):
            file_list, active_file = self.get_file_list()
            window.populate_file_list(file_list, active_file)


        def connect(self, device):
            print("connect to {}".format(device))
            self.control = interface.DataLoggerInterface(device_name=device, debug=True)
            self._update_file_list()
            time_delta = self.control.get_time_delta()
            window.update_time_discrepancy(time_delta)
            event_names = self.control.get_event_names()
            window.update_log_time_window(event_names)

        def disconnect(self):
            print("disconnect")
            self.control = None
            window.populate_file_list([], '')
            window.update_time_discrepancy(None)
            window.update_log_time(None)


        def begin_fetch(self, filenames):
            self._downloading = True
            self._fetch_filenames = filenames
            self._current_fetch = 0
            all_file_sizes = dict(self.control.list_files())
            download_file_sizes = [all_file_sizes[filename] for filename in filenames]
            download_bytes_total = sum(download_file_sizes)
            print("begin", filenames, download_file_sizes)
            first_filename = self._fetch_filenames[self._current_fetch]
            self.control.start_download_file(first_filename)
            return download_bytes_total, first_filename

        def step_fetch(self):
            all_done = False
            next_file = None
            (bytes_left, bytes_read) = self.control.download_chunk()
            print('STEP #TEMP!!! ', bytes_left, bytes_read)

            if bytes_left == 0:
                self._current_fetch += 1
                if self._current_fetch == len(self._fetch_filenames):
                    all_done = True
                    self._downloading = False
                    self._update_file_list()
                else:
                    next_file = self._fetch_filenames[self._current_fetch]
                    self.control.start_download_file(next_file)

            return all_done, bytes_read, next_file


        def halt_fetch(self):
            print("Halting at {}".format(self._current_fetch))
            self._downloading = False
            self.control.abort_download()
            self._update_file_list()


        def sync_time(self):
            self.control.sync_time()
            time_delta = self.control.get_time_delta()
            window.update_time_discrepancy(time_delta)


        def get_file_list(self):
            file_list = self.control.list_files()
            active_num = self.control.get_active_file_num()
            active_file = "LOGGER{:02}.CSV".format(active_num)

            return file_list, active_file

        def periodic(self):
            if self.control is not None and not self._downloading:
                delay, event_names = self.control.get_next_event()
                window.update_log_time(delay, event_names)

        def log_now(self, event_names):
            return self.control.trigger_events(event_names)

    bob = Bob()  #TEMP!!! rename

    callbacks = {
        "connect": bob.connect,
        "disconnect": bob.disconnect,
        'get_file_list': bob.get_file_list,
        "resync": bob.sync_time,
        "trigger_event": bob.log_now,
        'begin_fetch': bob.begin_fetch,
        'step_fetch': bob.step_fetch,
        'halt_fetch': bob.halt_fetch,
        'periodic': bob.periodic}

    window = GuiLoggerInterface(callbacks, tk.Tk())
    window.pack()


    window.mainloop()