import tkinter as tk
import serial
import serial.tools.list_ports
from functools import partial
import threading

# ────────────────────────────────────────
# Serial Port
# ────────────────────────────────────────
def find_serial_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "USB" in p.description or "Serial" in p.description or "UART" in p.description:
            return p.device
    return None

# Connect
ser = None
port = find_serial_port()
if port:
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"[INFO] Connected to {port}")
    except Exception as e:
        print("[ERROR] Connect Failed:", e)

# ────────────────────────────────────────
# Request and Response
# ────────────────────────────────────────
def send_command_serial(command):
    try:
        if ser and ser.is_open:
            ser.write((command + '\n').encode())  # 명령 전송
            response = ser.readline().decode().strip()
            if response.startswith("SPEED:"):
                return ("Speed set", response.split(":")[1])
            elif response:
                return ("Response", response)
            else:
                return ("Request Success", "")
        else:
            return ("Serial is not connected", "")
    except Exception as e:
        return ("Request Failed", str(e))

# ────────────────────────────────────────
# GUI
# ────────────────────────────────────────
def create_gui():
    root = tk.Tk()
    root.title("Water Robot Control GUI")
    root.geometry("520x440")

    # Focus
    root.focus_set()

    status = tk.StringVar()
    speed_text = tk.StringVar()
    status.set(f"Connected Port: {port if ser else 'None'}")
    speed_text.set("Current Speed: N/A")

    def on_command(command):
        result, detail = send_command_serial(command)
        if command in ['+', '-'] and detail:
            speed_text.set(f"Speed: {detail}ms")
        status.set(f"Request: {command}\nResult: {result} {detail}")

    # command mapping
    key_map = {
        'Q':'Q','W':'W','E':'E',
        'A':'A','S':'S','D':'D',
        'Z':'Z','X':'X','C':'C',
        'PLUS':'+','MINUS':'-',
        'G':'G','1':'1','2':'2',
        'UP':'W','LEFT':'A','DOWN':'S','RIGHT':'D'
    }

    # On down key
    def on_key_press(event):
        cmd = key_map.get(event.keysym.upper())
        if cmd:
            on_command(cmd)

    # On up key
    def on_key_release(event):
        if event.keysym.upper() in key_map:
            on_command('S')

    # Binding
    root.bind_all('<KeyPress>',   on_key_press)
    root.bind_all('<KeyRelease>', on_key_release)

    # ───── Create Button Function ─────
    def make_button(frame, text, cmd, row, col):
        btn = tk.Button(frame, text=text, width=8, height=2,
                        command=partial(on_command, cmd))
        btn.grid(row=row, column=col, padx=5, pady=5)

    # ───── Control Button ─────
    frame = tk.Frame(root)
    frame.pack(pady=10)
    directions = [
        ("↖", "Q", 0, 0), ("↑", "W", 0, 1), ("↗", "E", 0, 2),
        ("⟲", "A", 1, 0), ("■", "S", 1, 1), ("⟳", "D", 1, 2),
        ("↙", "Z", 2, 0), ("↓", "X", 2, 1), ("↘", "C", 2, 2),
    ]
    for label, cmd, r, c in directions:
        make_button(frame, label, cmd, r, c)

    # ───── Initialize Button ─────
    g_frame = tk.Frame(root)
    g_frame.pack()
    tk.Label(g_frame, text="Initialize:").grid(row=0, column=0)
    tk.Button(g_frame, text="Action G", width=10, height=2, command=partial(on_command, "G")).grid(row=0, column=1, padx=5)

    # ───── Relay Button ─────
    relay_frame = tk.Frame(root)
    relay_frame.pack(pady=5)
    tk.Label(relay_frame, text="Relay Control:").grid(row=0, column=0)
    tk.Button(relay_frame, text="Relay 1", width=10, command=partial(on_command, "1")).grid(row=0, column=1)
    tk.Button(relay_frame, text="Relay 2", width=10, command=partial(on_command, "2")).grid(row=0, column=2)

    # ───── Speed Button ─────
    speed_frame = tk.Frame(root)
    speed_frame.pack(pady=5)
    tk.Label(speed_frame, text="Speed Control:").grid(row=0, column=0)
    tk.Button(speed_frame, text="▲", width=5, command=partial(on_command, "+")).grid(row=0, column=1)
    tk.Button(speed_frame, text="▼", width=5, command=partial(on_command, "-")).grid(row=0, column=2)

    left_speed_frame = tk.Frame(root)
    left_speed_frame.pack(pady=5)
    tk.Label(left_speed_frame, text="Left Speed Control:").grid(row=0, column=0)
    tk.Button(left_speed_frame, text="▲", width=5, command=partial(on_command, "+++")).grid(row=0, column=1)
    tk.Button(left_speed_frame, text="▼", width=5, command=partial(on_command, "---")).grid(row=0, column=2)

    right_speed_frame = tk.Frame(root)
    right_speed_frame.pack(pady=5)
    tk.Label(right_speed_frame, text="Right Speed Control:").grid(row=0, column=0)
    tk.Button(right_speed_frame, text="▲", width=5, command=partial(on_command, "++")).grid(row=0, column=1)
    tk.Button(right_speed_frame, text="▼", width=5, command=partial(on_command, "--")).grid(row=0, column=2)

    # ───── State Text ─────
    tk.Label(root, textvariable=speed_text, fg="green", font=("Arial", 12)).pack(pady=5)
    tk.Label(root, textvariable=status,    fg="blue",  font=("Arial", 11)).pack(pady=5)

    root.mainloop()

# ────────────────────────────────────────
# Execution
# ────────────────────────────────────────
create_gui()
