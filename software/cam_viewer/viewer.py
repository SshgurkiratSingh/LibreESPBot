import cv2
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import threading
import socket
import time

class ESP32CamViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32-CAM MJPEG Viewer")
        self.root.geometry("800x600")
        self.root.configure(bg="#2c2c2c")

        # Top Control Frame
        self.control_frame = tk.Frame(root, bg="#1e1e1e", pady=10)
        self.control_frame.pack(fill=tk.X, side=tk.TOP)

        tk.Label(self.control_frame, text="Camera URL:", bg="#1e1e1e", fg="white", font=("Arial", 12)).pack(side=tk.LEFT, padx=10)
        
        self.url_var = tk.StringVar(value="http://192.168.4.2/")
        self.url_entry = ttk.Entry(self.control_frame, textvariable=self.url_var, width=40, font=("Arial", 12))
        self.url_entry.pack(side=tk.LEFT, padx=10)

        self.btn_connect = ttk.Button(self.control_frame, text="Connect", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=10)

        # Video Display Label
        self.video_label = tk.Label(root, bg="black", text="NO SIGNAL", fg="red", font=("Arial", 24, "bold"))
        self.video_label.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # State variables
        self.cap = None
        self.is_running = False
        self.is_app_running = True
        self.thread = None
        
        # Start UDP Discovery
        self.discovery_thread = threading.Thread(target=self.udp_listener, daemon=True)
        self.discovery_thread.start()

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def udp_listener(self):
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        udp_socket.bind(("", 4210))
        udp_socket.settimeout(1.0)
        
        while self.is_app_running:
            try:
                data, addr = udp_socket.recvfrom(1024)
                message = data.decode('utf-8').strip()
                if message.startswith("ESP32-CAM:"):
                    ip = message.split(":")[1]
                    discovered_url = f"http://{ip}/"
                    
                    # Update URL field if it's currently using the default or something else
                    current_url = self.url_var.get()
                    if current_url != discovered_url:
                        self.url_var.set(discovered_url)
                        
            except socket.timeout:
                continue
            except Exception as e:
                print(f"UDP Error: {e}")
                time.sleep(1)

    def toggle_connection(self):
        if self.is_running:
            self.stop_stream()
        else:
            self.start_stream()

    def start_stream(self):
        self.url = self.url_var.get()
        self.is_running = True
        self.btn_connect.config(text="Disconnect")
        
        self.thread = threading.Thread(target=self.stream_worker, daemon=True)
        self.thread.start()

    def stop_stream(self):
        self.is_running = False
        if self.cap:
            self.cap.release()
        self.btn_connect.config(text="Connect")
        self.video_label.config(text="DISCONNECTED", image="")

    def stream_worker(self):
        while self.is_running:
            self.cap = cv2.VideoCapture(self.url)
            
            if not self.cap.isOpened():
                if self.is_running:
                    self.video_label.config(text="CONNECTION FAILED. RETRYING...", image="")
                    time.sleep(3)
                continue

            self.video_label.config(text="")
            
            while self.is_running:
                ret, frame = self.cap.read()
                if not ret:
                    # OpenCV read failed - could be a dropped connection or stuck stream
                    print("Stream dropped. Reconnecting...")
                    break
                
                # Convert OpenCV BGR format to RGB
                cv2image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                img = Image.fromarray(cv2image)
                
                # Resize image to fit label while maintaining aspect ratio
                label_w = self.video_label.winfo_width()
                label_h = self.video_label.winfo_height()
                
                if label_w > 1 and label_h > 1:
                    img.thumbnail((label_w, label_h), Image.Resampling.LANCZOS)
                    
                imgtk = ImageTk.PhotoImage(image=img)
                
                # Update GUI safely
                self.video_label.imgtk = imgtk
                self.video_label.configure(image=imgtk)
            
            if self.cap:
                self.cap.release()
                
            # If still meant to be running but the inner loop broke, wait 3 seconds and retry
            if self.is_running:
                self.video_label.config(text="STUCK/DISCONNECTED. RECONNECTING IN 3s...", image="")
                time.sleep(3)

    def on_close(self):
        self.is_app_running = False
        self.stop_stream()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    
    # Configure style
    style = ttk.Style()
    style.theme_use('clam')
    
    app = ESP32CamViewer(root)
    root.mainloop()
