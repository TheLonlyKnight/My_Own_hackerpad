#!/usr/bin/env python3
"""
Simple Tkinter GUI to control the hackerpad device over serial.

Features:
- List serial ports and connect/disconnect
- Map a keypad key to a text action (MAP <key> <text>)
- Send an image file (converted to 128x32 1-bit buffer) to the device (IMG <len> + bytes)
- Show device responses in a log area

Dependencies: pyserial, Pillow

Run: python gui_app.py --port COM5
"""
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import serial
import serial.tools.list_ports
from PIL import Image, ImageTk
import io
import time


def list_ports():
    return [p.device for p in serial.tools.list_ports.comports()]


def image_to_ssd1306_buffer(path, width=128, height=32):
    im = Image.open(path).convert('L')
    im = im.resize((width, height), Image.LANCZOS)
    bw = im.point(lambda p: 0 if p < 128 else 1, '1')
    pages = height // 8
    buf = bytearray(width * pages)
    for x in range(width):
        for page in range(pages):
            byte = 0
            for bit in range(8):
                y = page * 8 + bit
                pix = bw.getpixel((x, y))
                if pix:
                    byte |= (1 << bit)
            buf[x + page * width] = byte
    return bytes(buf)


class SerialThread(threading.Thread):
    def __init__(self, ser, on_line, stop_event):
        super().__init__(daemon=True)
        self.ser = ser
        self.on_line = on_line
        self.stop_event = stop_event

    def run(self):
        try:
            while not self.stop_event.is_set():
                try:
                    line = self.ser.readline()
                except Exception:
                    break
                if not line:
                    continue
                try:
                    text = line.decode('utf-8', errors='ignore').strip()
                except Exception:
                    text = repr(line)
                self.on_line(text)
        finally:
            pass


class App:
    def __init__(self, root):
        self.root = root
        root.title('Hackerpad Serial GUI')

        frm = ttk.Frame(root, padding=8)
        frm.grid(sticky='nsew')

        # Port selection
        ttk.Label(frm, text='Port:').grid(column=0, row=0, sticky='w')
        self.port_cb = ttk.Combobox(frm, values=list_ports(), width=20)
        self.port_cb.grid(column=1, row=0, sticky='w')
        self.refresh_btn = ttk.Button(frm, text='Refresh', command=self.refresh_ports)
        self.refresh_btn.grid(column=2, row=0, sticky='w')
        self.connect_btn = ttk.Button(frm, text='Connect', command=self.toggle_connect)
        self.connect_btn.grid(column=3, row=0, sticky='w')

        # Map key
        ttk.Label(frm, text='Map Key:').grid(column=0, row=1, sticky='w')
        self.map_key = ttk.Entry(frm, width=4)
        self.map_key.grid(column=1, row=1, sticky='w')
        self.map_text = ttk.Entry(frm, width=40)
        self.map_text.grid(column=2, row=1, columnspan=2, sticky='w')
        self.map_btn = ttk.Button(frm, text='Map', command=self.send_map)
        self.map_btn.grid(column=4, row=1, sticky='w')

        # Image send
        self.img_btn = ttk.Button(frm, text='Send Image', command=self.send_image)
        self.img_btn.grid(column=0, row=2, sticky='w')
        self.preview_label = ttk.Label(frm)
        self.preview_label.grid(column=1, row=2, columnspan=3, rowspan=2, sticky='w')

        # Log
        ttk.Label(frm, text='Log:').grid(column=0, row=4, sticky='nw')
        self.log = tk.Text(frm, width=60, height=12)
        self.log.grid(column=0, row=5, columnspan=5, sticky='nsew')

        for i in range(5):
            frm.columnconfigure(i, weight=1)
        frm.rowconfigure(5, weight=1)

        self.ser = None
        self.thread = None
        self.stop_event = threading.Event()

    def refresh_ports(self):
        self.port_cb['values'] = list_ports()

    def toggle_connect(self):
        if self.ser and self.ser.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_cb.get()
        if not port:
            ports = list_ports()
            if not ports:
                messagebox.showerror('Error', 'No serial ports found')
                return
            port = ports[0]
            self.port_cb.set(port)
        try:
            self.ser = serial.Serial(port, 115200, timeout=1)
        except Exception as e:
            messagebox.showerror('Error', f'Failed to open {port}: {e}')
            return
        self.stop_event.clear()
        self.thread = SerialThread(self.ser, self.on_line, self.stop_event)
        self.thread.start()
        self.connect_btn.config(text='Disconnect')
        self.log_append(f'Connected to {port}')

    def disconnect(self):
        self.stop_event.set()
        if self.thread:
            self.thread.join(timeout=1)
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
        self.connect_btn.config(text='Connect')
        self.log_append('Disconnected')

    def on_line(self, line):
        # Called from serial thread
        self.root.after(0, lambda: self.log_append('Device: ' + line))

    def log_append(self, text):
        self.log.insert('end', text + '\n')
        self.log.see('end')

    def send_map(self):
        if not self.ser or not self.ser.is_open:
            messagebox.showerror('Error', 'Not connected')
            return
        key = self.map_key.get()
        text = self.map_text.get()
        if not key or len(key) != 1:
            messagebox.showerror('Error', 'Map key must be a single character')
            return
        cmd = f"MAP {key} {text}\n"
        try:
            self.ser.write(cmd.encode('utf-8'))
            self.ser.flush()
            self.log_append(f'Sent: {cmd.strip()}')
        except Exception as e:
            self.log_append(f'Error sending MAP: {e}')

    def send_image(self):
        if not self.ser or not self.ser.is_open:
            messagebox.showerror('Error', 'Not connected')
            return
        path = filedialog.askopenfilename(filetypes=[('Images', '*.png;*.jpg;*.bmp;*.gif')])
        if not path:
            return
        try:
            buf = image_to_ssd1306_buffer(path)
        except Exception as e:
            messagebox.showerror('Error', f'Failed to process image: {e}')
            return
        header = f"IMG {len(buf)}\n"
        try:
            self.ser.write(header.encode('ascii'))
            self.ser.flush()
            # tiny pause
            time.sleep(0.01)
            self.ser.write(buf)
            self.ser.flush()
            self.log_append(f'Sent image ({len(buf)} bytes)')
        except Exception as e:
            self.log_append(f'Error sending image: {e}')
            return
        # show preview
        im = Image.open(path).convert('RGB')
        im.thumbnail((256, 64))
        photo = ImageTk.PhotoImage(im)
        self.preview_label.config(image=photo)
        self.preview_label.image = photo


def main():
    root = tk.Tk()
    app = App(root)
    root.protocol('WM_DELETE_WINDOW', lambda: (app.disconnect(), root.destroy()))
    root.mainloop()


if __name__ == '__main__':
    main()
