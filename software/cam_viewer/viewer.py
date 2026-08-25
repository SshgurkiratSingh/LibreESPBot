import cv2
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import threading

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
        
        self.url_var = tk.StringVar(value="http://192.168.4.2:81/stream")
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
        self.thread = None

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def toggle_connection(self):
        if self.is_running:
            self.stop_stream()
        else:
            self.start_stream()

    def start_stream(self):
        self.url = self.url_var.get()
        self.cap = cv2.VideoCapture(self.url)
        
        if not self.cap.isOpened():
            self.video_label.config(text="CONNECTION FAILED", image="")
            return

        self.is_running = True
        self.btn_connect.config(text="Disconnect")
        self.video_label.config(text="")
        
        self.thread = threading.Thread(target=self.update_frame, daemon=True)
        self.thread.start()

    def stop_stream(self):
        self.is_running = False
        if self.cap:
            self.cap.release()
        self.btn_connect.config(text="Connect")
        self.video_label.config(text="DISCONNECTED", image="")

    def update_frame(self):
        while self.is_running:
            ret, frame = self.cap.read()
            if not ret:
                continue
            
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

    def on_close(self):
        self.stop_stream()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    
    # Configure style
    style = ttk.Style()
    style.theme_use('clam')
    
    app = ESP32CamViewer(root)
    root.mainloop()
