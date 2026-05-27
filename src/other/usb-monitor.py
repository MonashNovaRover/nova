#!/usr/bin/env python3

#requirement:  pip install textual

import subprocess
import time
import re
import threading
import os
import sys
import fcntl
from collections import defaultdict, deque

from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, DataTable, Button, Static, Label
from textual.containers import Horizontal, Vertical
from textual.reactive import reactive
from textual.coordinate import Coordinate

from rich.text import Text

# --- CONFIGURATION ---
CHECK_INTERVAL = 1.0  
HISTORY_LEN = 20 # How many seconds of graph history to show

# Standard Linux USB Reset ioctl command
USBDEVFS_RESET = 21780

# Sparkline characters (from empty to full block)
SPARKS = "  ▂▃▄▅▆▇█"

# Shared state between sniffer thread and UI
rx_counts = defaultdict(int)
tx_counts = defaultdict(int)
count_lock = threading.Lock()

def check_root_and_usbmon():
    if os.geteuid() != 0:
        print("ERROR: This script requires root privileges to sniff live USB traffic.")
        sys.exit(1)
        
    if not os.path.exists("/sys/kernel/debug/usb/usbmon/0u"):
        print("ERROR: usbmon is not loaded. Please run: sudo usb-monitor.py")
        sys.exit(1)

def sniff_usbmon():
    """Background thread that reads kernel packets."""
    try:
        process = subprocess.Popen(
            ["cat", "/sys/kernel/debug/usb/usbmon/0u"],
            stdout=subprocess.PIPE, text=True
        )
        for line in iter(process.stdout.readline, ''):
            parts = line.split()
            if len(parts) >= 6 and parts[2] == 'C': 
                address = parts[3] 
                if len(address) >= 2:
                    direction = address[1] 
                    addr_parts = address.split(':')
                    if len(addr_parts) >= 3:
                        bus, dev = addr_parts[1].zfill(3), addr_parts[2].zfill(3)
                        try:
                            length = int(parts[5])
                            with count_lock:
                                if direction == 'i': rx_counts[f"{bus}:{dev}"] += length
                                elif direction == 'o': tx_counts[f"{bus}:{dev}"] += length
                        except ValueError: pass
    except Exception: pass

def get_usb_devices():
    """Maps device topologies."""
    devices = {}
    try:
        lsusb_out = subprocess.check_output(["lsusb"], text=True)
        for line in lsusb_out.splitlines():
            match = re.match(r"Bus (\d+) Device (\d+): ID [\w:]+ (.*)", line)
            if match:
                bus, dev, name = match.groups()
                name = name.strip()
                devices[f"{bus}:{dev}"] = {"bus": bus, "dev": dev, "name": name}
    except Exception: pass
    return devices

def generate_sparkline(data_queue, max_val):
    """Converts a deque of bandwidth integers into a btop-style graph string."""
    if max_val == 0:
        return Text("".join([" "] * HISTORY_LEN), style="dim")
    
    graph = ""
    for val in data_queue:
        if val == 0:
            graph += " "
        else:
            # Map value to 1-8 index for sparkline chars
            ratio = val / max_val
            idx = max(1, int(ratio * (len(SPARKS) - 1)))
            graph += SPARKS[idx]
            
    return Text(graph, style="bold cyan")

def format_bytes(b_s):
    if b_s == 0: return "0 B/s"
    elif b_s < 1024: return f"{b_s} B/s"
    elif b_s < 1048576: return f"{b_s / 1024:.1f} KB/s"
    else: return f"{b_s / 1048576:.2f} MB/s"


class UsbTopApp(App):
    CSS = """
    DataTable { height: 1fr; border: solid blue; }
    #controls { height: 3; dock: bottom; padding: 0 1; }
    #device_label { padding-top: 1; width: 1fr; content-align: center middle; text-style: bold; color: white;}
    Button { margin-right: 2; }
    """

    selected_device = reactive(None)

    def __init__(self):
        super().__init__()
        self.device_history = defaultdict(lambda: deque([0]*HISTORY_LEN, maxlen=HISTORY_LEN))
        self.max_bw_seen = 1024 # Baseline for graph scaling

    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        yield DataTable(id="usb_table", cursor_type="row")
        
        with Horizontal(id="controls"):
            yield Button("RESET / KILL LINK", id="btn_kill", variant="error", disabled=True)
            yield Label("Select a device to view controls...", id="device_label")
            
        yield Footer()

    def on_mount(self) -> None:
        table = self.query_one(DataTable)
        table.add_columns("Bus:Dev", "Device Name", "TX Graph", "TX Rate", "RX Graph", "RX Rate")
        self.update_timer = self.set_interval(CHECK_INTERVAL, self.update_data)

    def on_data_table_row_selected(self, event: DataTable.RowSelected) -> None:
        table = self.query_one(DataTable)
        row_key = event.row_key
        # Retrieve the Bus:Dev string from the first column of the selected row
        bus_dev = table.get_cell_at(Coordinate(table.get_row_index(row_key), 0))
        dev_name = table.get_cell_at(Coordinate(table.get_row_index(row_key), 1))
        
        self.selected_device = bus_dev
        self.query_one("#device_label", Label).update(f"Target: [magenta]{dev_name}[/magenta] ({bus_dev})")
        self.query_one("#btn_kill", Button).disabled = False

    def on_button_pressed(self, event: Button.Pressed) -> None:
        if event.button.id == "btn_kill" and self.selected_device:
            self.reset_usb_device(self.selected_device)

    def reset_usb_device(self, bus_dev):
        """Sends the raw ioctl command to the kernel to drop the device."""
        bus, dev = bus_dev.split(":")
        device_path = f"/dev/bus/usb/{bus}/{dev}"
        
        try:
            with open(device_path, 'w') as f:
                fcntl.ioctl(f, USBDEVFS_RESET, 0)
            self.notify(f"Reset signal sent to {bus_dev}", title="Device Killed", severity="warning")
            self.selected_device = None
            self.query_one("#device_label", Label).update("Select a device to view controls...")
            self.query_one("#btn_kill", Button).disabled = True
        except Exception as e:
            self.notify(f"Failed to reset: {e}", title="Error", severity="error")

    def update_data(self) -> None:
        global rx_counts, tx_counts
        
        with count_lock:
            current_rx = dict(rx_counts)
            current_tx = dict(tx_counts)
            rx_counts.clear()
            tx_counts.clear()

        devices = get_usb_devices()
        table = self.query_one(DataTable)
        
        # Clear missing devices and update existing
        current_keys = set([f"{d['bus']}:{d['dev']}" for d in devices.values()])
        
        # Update our history deques and find the max bandwidth for graph scaling
        for bus_dev in current_keys:
            rx = int(current_rx.get(bus_dev, 0) / CHECK_INTERVAL)
            tx = int(current_tx.get(bus_dev, 0) / CHECK_INTERVAL)
            total = rx + tx
            if total > self.max_bw_seen:
                self.max_bw_seen = total
                
            self.device_history[f"{bus_dev}_rx"].append(rx)
            self.device_history[f"{bus_dev}_tx"].append(tx)

        table.clear()
        
        # Build the rows
        for bus_dev, info in devices.items():
            tx_q = self.device_history[f"{bus_dev}_tx"]
            rx_q = self.device_history[f"{bus_dev}_rx"]
            
            tx_graph = generate_sparkline(tx_q, self.max_bw_seen)
            rx_graph = generate_sparkline(rx_q, self.max_bw_seen)
            
            table.add_row(
                bus_dev,
                info['name'],
                tx_graph,
                Text(format_bytes(tx_q[-1]), style="green"),
                rx_graph,
                Text(format_bytes(rx_q[-1]), style="yellow"),
                key=bus_dev
            )

if __name__ == "__main__":
    check_root_and_usbmon()
    sniffer = threading.Thread(target=sniff_usbmon, daemon=True)
    sniffer.start()
    
    app = UsbTopApp()
    app.run()