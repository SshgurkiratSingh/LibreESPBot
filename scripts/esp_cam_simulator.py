#!/usr/bin/env python3
import http.server
import socketserver
import time
import io
import math
import os
import glob
import threading
import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageDraw

class SimulatorState:
    def __init__(self):
        self.image_files = []
        self.frame_idx = 0
        self.use_generated = True

state = SimulatorState()

class ESPCamHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        # Suppress logging to keep console clean
        pass

    def do_GET(self):
        if self.path.startswith('/capture'):
            self.send_response(200)
            self.send_header('Content-Type', 'image/jpeg')
            self.end_headers()
            
            if state.use_generated or not state.image_files:
                self.serve_generated()
            else:
                self.serve_file()
        else:
            self.send_error(404)
            
    def serve_generated(self):
        state.frame_idx += 1
        img = Image.new('RGB', (640, 480), color=(30, 30, 30))
        draw = ImageDraw.Draw(img)
        cx, cy = 320, 240
        radius = 100
        t = state.frame_idx * 0.1
        x = cx + math.cos(t) * radius
        y = cy + math.sin(t) * radius
        draw.ellipse((x-20, y-20, x+20, y+20), fill=(200, 50, 50))
        draw.text((10, 10), "ESP32-CAM Simulator", fill=(255, 255, 255))
        draw.text((10, 30), f"Frame: {state.frame_idx} (No Folder Selected)", fill=(255, 255, 255))
        draw.text((10, 50), time.strftime("%H:%M:%S"), fill=(200, 200, 200))
        
        buf = io.BytesIO()
        img.save(buf, format='JPEG', quality=80)
        self.wfile.write(buf.getvalue())

    def serve_file(self):
        img_path = state.image_files[state.frame_idx % len(state.image_files)]
        state.frame_idx += 1
        try:
            with open(img_path, 'rb') as f:
                self.wfile.write(f.read())
        except Exception as e:
            print(f"Error serving {img_path}: {e}")
            self.serve_generated()

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32-CAM Simulator")
        self.root.geometry("450x320")
        self.root.resizable(False, False)
        
        self.httpd = None
        self.server_thread = None
        
        self.lbl_title = tk.Label(root, text="ESP32-CAM Stream Simulator", font=("Helvetica", 14, "bold"))
        self.lbl_title.pack(pady=10)
        
        # Server Port Frame
        frame_server = tk.Frame(root)
        frame_server.pack(pady=5)
        
        tk.Label(frame_server, text="Port:").grid(row=0, column=0, padx=5)
        self.entry_port = tk.Entry(frame_server, width=8)
        self.entry_port.insert(0, "80")
        self.entry_port.grid(row=0, column=1, padx=5)
        
        self.btn_server = tk.Button(frame_server, text="Start Server", command=self.toggle_server, width=15)
        self.btn_server.grid(row=0, column=2, padx=10)
        
        self.lbl_server_status = tk.Label(root, text="Server Stopped", fg="red", font=("Helvetica", 9, "italic"))
        self.lbl_server_status.pack(pady=2)

        # Separator
        tk.Frame(root, height=2, bd=1, relief=tk.SUNKEN).pack(fill=tk.X, padx=20, pady=10)

        # Mode Status
        self.lbl_status = tk.Label(root, text="Mode: Generated Animation", fg="blue", font=("Helvetica", 10))
        self.lbl_status.pack(pady=5)
        
        self.lbl_folder = tk.Label(root, text="No folder selected", wraplength=400, fg="gray")
        self.lbl_folder.pack(pady=5)
        
        frame_buttons = tk.Frame(root)
        frame_buttons.pack(pady=5)
        
        self.btn_select = tk.Button(frame_buttons, text="Select Data Folder", command=self.select_folder, width=20)
        self.btn_select.grid(row=0, column=0, padx=5)
        
        self.btn_reset = tk.Button(frame_buttons, text="Reset to Animation", command=self.reset_mode, width=20)
        self.btn_reset.grid(row=0, column=1, padx=5)
        
        # Handle window close
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

    def toggle_server(self):
        if self.httpd is None:
            # Try to start server
            try:
                port = int(self.entry_port.get())
                socketserver.TCPServer.allow_reuse_address = True
                self.httpd = socketserver.TCPServer(("", port), ESPCamHandler)
                
                self.server_thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)
                self.server_thread.start()
                
                self.btn_server.config(text="Stop Server")
                self.entry_port.config(state="disabled")
                self.lbl_server_status.config(text=f"Serving on: http://localhost:{port}/capture", fg="green")
                print(f"Server started on port {port}")
            except PermissionError:
                messagebox.showerror("Permission Error", f"Permission denied for port {port}. Try running with sudo or pick a higher port (e.g. 8080).")
                self.httpd = None
            except Exception as e:
                messagebox.showerror("Server Error", str(e))
                self.httpd = None
        else:
            # Stop server
            self.stop_server()
            self.btn_server.config(text="Start Server")
            self.entry_port.config(state="normal")
            self.lbl_server_status.config(text="Server Stopped", fg="red")
            print("Server stopped.")

    def stop_server(self):
        if self.httpd:
            self.httpd.shutdown()
            self.httpd.server_close()
            self.httpd = None
        if self.server_thread:
            self.server_thread.join(timeout=1.0)
            self.server_thread = None

    def select_folder(self):
        folder_path = filedialog.askdirectory(title="Select Folder with JPEG Frames")
        if folder_path:
            # Find all images (jpg, jpeg, png)
            files = []
            for ext in ('*.jpg', '*.JPG', '*.jpeg', '*.JPEG', '*.png', '*.PNG'):
                files.extend(glob.glob(os.path.join(folder_path, ext)))
            
            files.sort()
            
            if not files:
                messagebox.showwarning("No Images", f"No supported images found in:\n{folder_path}")
                return
            
            state.image_files = files
            state.frame_idx = 0
            state.use_generated = False
            
            self.lbl_status.config(text=f"Mode: Replaying {len(files)} Frames", fg="green")
            self.lbl_folder.config(text=folder_path, fg="black")
            print(f"Loaded {len(files)} frames from {folder_path}")

    def reset_mode(self):
        state.use_generated = True
        state.image_files = []
        self.lbl_status.config(text="Mode: Generated Animation", fg="blue")
        self.lbl_folder.config(text="No folder selected", fg="gray")
        print("Reset to generated animation mode.")

    def on_closing(self):
        self.stop_server()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()
