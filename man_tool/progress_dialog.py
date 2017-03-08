import tkinter as tk
from tkinter import ttk
import time


class ProgressDialog(tk.Tk):

    def __init__(self, parent, download_callback, halt_callback):
        self.top = tk.Toplevel(parent)

        self._download_callback = download_callback
        self._halt_callback = halt_callback

        self.listbox = tk.Listbox(self.top, width=20, height=5)
        self._progress_bar = ttk.Progressbar(self.top, orient='horizontal',
                                           length=300, mode='determinate')
        self._my_button = tk.Button(self.top, text="Cancel", command=self._my_button_command)

        self.listbox.pack(padx=10, pady=10)
        self._progress_bar.pack(padx=10, pady=10)
        self._my_button.pack(padx=10, pady=10)

        self._state = 'running'
        self._percent = 0

        self._periodic_call()


    def _my_button_command(self):
        if self._state == 'running':
            self.listbox.insert('end', "CANCELING")
            self._state = 'cancelled'
        elif self._state == 'done':
            self.top.destroy()


    def _periodic_call(self):
        if self._state == 'running':
            done, percent, new_filename = self._download_callback()
            if percent != self._percent:
                self._progress_bar.step(percent - self._percent)
                self._percent = percent
            if new_filename is not None:
                self.listbox.insert('end', new_filename)
            if done:
                self._my_button.configure(text='OK')
                self._state = 'done'
                self.listbox.insert('end', "COMPLETE")
            else:
                self.top.after(100, self._periodic_call)
        elif self._state == 'cancelled':
            self._halt_callback()
            self.listbox.insert('end', "CANCELLED")
            self._my_button.configure(text='OK')
            self._state = 'done'
        else:
            raise Exception('bad state', self._state)



if __name__ == "__main__":


    class Bob(object):

        def __init__(self):
            self.x = 0

        def dl(self):
            time.sleep(0.05)
            if self.x % 34 == 0:
                fn = "BOB{:03}.CSV".format(self.x)
                print(fn)
            else:
                fn = None
            self.x += 1
            return (self.x == 500), int(self.x / 5), fn


        def halt(self):
            print("Halting at {}".format(self.x))


    b = Bob()
    root = tk.Tk()
    tk.Button(root, text="Hello!").pack()
    root.update()
    app = ProgressDialog(root, b.dl, b.halt)
    root.wait_window(app.top)