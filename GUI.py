import tkinter as tk
from tkinter import ttk, messagebox
import hashlib
import os
import serial
import threading

# Setup the serial connection (Modify COM port and baud rate as necessary)
try:
    ser = serial.Serial('COM4', 9600, timeout=1)  # Added timeout for reading
    print("Serial port opened successfully")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    ser = None

# Function to hash password
def md5_hash_password(password):
    md5_hash = hashlib.md5()
    md5_hash.update(password.encode())
    return md5_hash.hexdigest()

# Function to create user
def create_user(username, password, file_path):
    hashed_password = md5_hash_password(password)
    with open(file_path, 'a') as user_file:
        user_file.write(f"{username}:{hashed_password}\n")

# Function to handle registration
def register():
    username = register_username_entry.get()
    password = register_password_entry.get()
    file_path = 'credentials.txt'
    
    if not os.path.exists(file_path):
        with open(file_path, 'w') as user_file:
            pass
    
    with open(file_path, 'r+') as user_file:
        users = user_file.readlines()
        for user in users:
            existing_username, _ = user.strip().split(":")
            if existing_username == username:
                messagebox.showerror("Registration Error", "Username already exists")
                return
    
    create_user(username, password, file_path)
    messagebox.showinfo("Registration Success", "User registered successfully!")
    register_username_entry.delete(0, tk.END)
    register_password_entry.delete(0, tk.END)

# Function to handle login
def login():
    username = login_username_entry.get()
    password = login_password_entry.get()
    
    hashed_password = md5_hash_password(password)
    file_path = 'credentials.txt'
    
    with open(file_path, 'r') as user_file:
        users = user_file.readlines()
        for user in users:
            existing_username, existing_password = user.strip().split(":")
            if existing_username == username and existing_password == hashed_password:
                messagebox.showinfo("Login Success", "Logged in successfully!")
                create_mesg_tab(username)
                remove_auth_tabs()
                start_serial_read_thread()
                return
    
    messagebox.showerror("Login Error", "Invalid username or password")

# Function to remove the registration and login tabs
def remove_auth_tabs():
    tabControl.forget(registration_tab)
    tabControl.forget(login_tab)

# Function to read from serial port
def read_from_serial():
    while True:
        if ser and ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    mesg_text.config(state=tk.NORMAL)
                    mesg_text.insert(tk.END, f"Serial: {line}\n")
                    mesg_text.config(state=tk.DISABLED)
                    mesg_text.see(tk.END)
            except Exception as e:
                print(f"Error reading from serial port: {e}")

# Start serial read thread
def start_serial_read_thread():
    thread = threading.Thread(target=read_from_serial, daemon=True)
    thread.start()

# Initialize tkinter
root = tk.Tk()
root.title("TDP Project Authentication Screen")
root.geometry("800x600")

font = ("Helvetica", 14)
bg_color = "#e6e6fa"  # Background color to match the UI

# Create Tab Control
tabControl = ttk.Notebook(root)

# Create Tabs
registration_tab = tk.Frame(tabControl, bg='#f0f8ff')
login_tab = tk.Frame(tabControl, bg=bg_color)

tabControl.add(registration_tab, text='Registration')
tabControl.add(login_tab, text='Login')

tabControl.pack(expand=1, fill="both")

# Create container frames for centering
reg_container = tk.Frame(registration_tab, bg='#f0f8ff')
login_container = tk.Frame(login_tab, bg=bg_color)

reg_container.place(relx=0.5, rely=0.5, anchor=tk.CENTER)
login_container.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

# Registration Tab
tk.Label(reg_container, text="Register a New Account", font=("Helvetica", 18, "bold"), bg='#f0f8ff').pack(pady=20)
tk.Label(reg_container, text="Username:", font=font, bg='#f0f8ff').pack(pady=10)
register_username_entry = tk.Entry(reg_container, font=font)
register_username_entry.pack(pady=10, padx=10)

tk.Label(reg_container, text="Password:", font=font, bg='#f0f8ff').pack(pady=10)
register_password_entry = tk.Entry(reg_container, show="*", font=font)
register_password_entry.pack(pady=10, padx=10)

register_button = tk.Button(reg_container, text="Register", command=register, font=font, bg='#32cd32', fg='white')
register_button.pack(pady=20, padx=10)

# Login Tab
tk.Label(login_container, text="Login to Your Account", font=("Helvetica", 18, "bold"), bg=bg_color).pack(pady=20)
tk.Label(login_container, text="Username:", font=font, bg=bg_color).pack(pady=10)
login_username_entry = tk.Entry(login_container, font=font)
login_username_entry.pack(pady=10, padx=10)

tk.Label(login_container, text="Password:", font=font, bg=bg_color).pack(pady=10)
login_password_entry = tk.Entry(login_container, show="*", font=font)
login_password_entry.pack(pady=10, padx=10)

login_button = tk.Button(login_container, text="Login", command=login, font=font, bg='#1e90ff', fg='white')
login_button.pack(pady=20, padx=10)

# Function to create mesg tab
def create_mesg_tab(username):
    global mesg_text  # Make mesg_text global to access it in read_from_serial
    mesg_tab = ttk.Frame(tabControl)
    tabControl.add(mesg_tab, text='Messages')
    tabControl.select(mesg_tab)

    mesg_text = tk.Text(mesg_tab, height=15, width=50, state=tk.DISABLED, font=font)
    mesg_text.pack(pady=10, padx=10, fill=tk.X)

    receiver_id_frame = tk.Frame(mesg_tab, bg=bg_color)
    receiver_id_frame.pack(pady=10, padx=10, fill=tk.X)

    tk.Label(receiver_id_frame, text="Receiver ID:", font=font, bg=bg_color).pack(side=tk.LEFT, padx=(0, 10))
    receiver_id_entry = tk.Entry(receiver_id_frame, font=font, width=4)
    receiver_id_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)

    message_entry = tk.Entry(mesg_tab, font=font)
    message_entry.pack(pady=10, padx=10, fill=tk.X)
    message_entry.focus()

    send_button = tk.Button(mesg_tab, text="Send", command=lambda: send_message(mesg_text, message_entry, receiver_id_entry, username), font=font, padx=40, pady=5, bg='#ff4500', fg='white')
    send_button.pack(pady=10, padx=10, fill=tk.X)

# Function to send message
def send_message(mesg_text, message_entry, receiver_id_entry, username):
    message = message_entry.get()
    receiver_id = receiver_id_entry.get()
    if not receiver_id:
        messagebox.showerror("Error", "Receiver ID is required.")
        return
    if message:
        sender_device_id = 1  # Assuming the device ID of the sender is 1
        formatted_message = f"{receiver_id}:{sender_device_id}:{username}:{message}\n"
        mesg_text.config(state=tk.NORMAL)
        mesg_text.insert(tk.END, formatted_message)
        mesg_text.config(state=tk.DISABLED)
        message_entry.delete(0, tk.END)
        receiver_id_entry.delete(0, tk.END)
        # Sending the message to Arduino via serial
        try:
            ser.write(formatted_message.encode())
            print(f"Sent message: {formatted_message}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to send message: {e}")
            print(f"Failed to send message: {e}")

root.mainloop()