import tkinter as tk
import tkinter.ttk as ttk

class VerticalScrolledFrame(tk.Frame):
    """A pure Tkinter scrollable frame that actually works!

    * Use the 'interior' attribute to place widgets inside the scrollable frame
    * Construct and pack/place/grid normally
    * This frame only allows vertical scrolling

    Taken from http://tkinter.unpythonic.net/wiki/VerticalScrolledFrame

    """
    def __init__(self, parent, *args, **kw):
        tk.Frame.__init__(self, parent, *args, **kw)

        # create a canvas object and a vertical scrollbar for scrolling it
        vscrollbar = tk.Scrollbar(self, orient=tk.VERTICAL)
        vscrollbar.pack(fill=tk.Y, side=tk.RIGHT, expand=tk.FALSE)
        canvas = tk.Canvas(self, bd=0, highlightthickness=0,
                        yscrollcommand=vscrollbar.set)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=tk.TRUE)
        vscrollbar.config(command=canvas.yview)

        # reset the view
        canvas.xview_moveto(0)
        canvas.yview_moveto(0)

        # create a frame inside the canvas which will be scrolled with it
        self.interior = interior = tk.Frame(canvas)
        interior_id = canvas.create_window(0, 0, window=interior,
                                           anchor=tk.NW)

        # track changes to the canvas and frame width and sync them,
        # also updating the scrollbar
        def _configure_interior(event):
            # update the scrollbars to match the size of the inner frame
            size = (interior.winfo_reqwidth(), interior.winfo_reqheight())
            canvas.config(scrollregion="0 0 %s %s" % size)
            if interior.winfo_reqwidth() != canvas.winfo_width():
                # update the canvas's width to fit the inner frame
                canvas.config(width=interior.winfo_reqwidth())
        interior.bind('<Configure>', _configure_interior)

        def _configure_canvas(event):
            if interior.winfo_reqwidth() != canvas.winfo_width():
                # update the inner frame's width to fill the canvas
                canvas.itemconfigure(interior_id, width=canvas.winfo_width())
        canvas.bind('<Configure>', _configure_canvas)



if __name__ == "__main__":

    class SampleApp(tk.Tk):
        def __init__(self, *args, **kwargs):
            root = tk.Tk.__init__(self, *args, **kwargs)


            self.frame = VerticalScrolledFrame(root)
            self.button = tk.Button(text="Hello", command=self.bob)
            self.frame.pack(expand=True, fill='both')
            self.label = tk.Label(text="Shrink the window to activate the scrollbar.")
            self.label.pack()
            self.button.pack()
            buttons = []
            for i in range(10):
                f = tk.Frame(self.frame.interior)
                tk.Button(f, text="Left " + str(i)).pack(side='left',expand=True, fill='x')
                tk.Button(f, text="Button " + str(i)).pack(side='left',expand=True, fill='x')
                tk.Button(f, text="Right " + str(i)).pack(side='left',expand=True, fill='x')
                buttons.append(f)
                buttons[-1].pack(expand=True, fill='x')

        def bob(self):
            tk.Button(self.frame.interior, text="Button").pack()


    app = SampleApp()
    app.mainloop()