#!/usr/bin/env python3

import subprocess
import time
from datetime import datetime

from rich.console import Console
from rich.live import Live
from rich.table import Table
from rich.panel import Panel
from rich.layout import Layout
from rich.text import Text

console = Console()

CHECK_INTERVAL = 2
PING_TIMEOUT = 1

RADIOS = {
    "2.4GHz Remote Link": "10.0.1.10",
    "900MHz Remote Link": "10.0.1.140",
    "2.4GHz Directional Local": "10.0.1.120",
    "2.4GHz Omni Backup": "10.0.1.11",
    "900MHz Local": "10.0.1.130",
}

ROVER = {
    "Rover Orin": "10.0.0.12"
}

last_seen = {}


def ping(host):
    """
    Returns:
        (success: bool, latency_ms: str)
    """

    try:
        result = subprocess.run(
            [
                "ping",
                "-c",
                "1",
                "-W",
                str(PING_TIMEOUT),
                host,
            ],
            capture_output=True,
            text=True,
        )

        if result.returncode == 0:
            output = result.stdout

            latency = "?"

            if "time=" in output:
                latency = output.split("time=")[1].split()[0] + " ms"

            last_seen[host] = datetime.now()
            return True, latency

        return False, "timeout"

    except Exception:
        return False, "error"



def status_text(ok):
    if ok:
        return "[bold green]UP[/bold green]"
    return "[bold red]DOWN[/bold red]"



def format_last_seen(host):
    if host not in last_seen:
        return "Never"

    delta = datetime.now() - last_seen[host]
    secs = int(delta.total_seconds())

    if secs < 5:
        return "Just now"

    return f"{secs}s ago"



def build_dashboard():
    table = Table(title="PRP Radio Network Status", expand=True)

    table.add_column("Component", style="cyan", no_wrap=True)
    table.add_column("IP", style="magenta")
    table.add_column("Status", justify="center")
    table.add_column("Latency", justify="right")
    table.add_column("Last Seen", justify="right")

    results = {}

    for name, ip in RADIOS.items():
        ok, latency = ping(ip)

        results[name] = ok

        table.add_row(
            name,
            ip,
            status_text(ok),
            latency,
            format_last_seen(ip),
        )

    # PRP summary logic
    link_24 = results["2.4GHz Remote Link"]
    link_900 = results["900MHz Remote Link"]

    if link_24 and link_900:
        prp_state = Text("FULL REDUNDANCY", style="bold green")
        prp_detail = "Both wireless links operational"

    elif link_24 or link_900:
        prp_state = Text("DEGRADED", style="bold yellow")

        if link_24:
            prp_detail = "Only 2.4GHz link operational"
        else:
            prp_detail = "Only 900MHz link operational"

    else:
        prp_state = Text("LINK FAILURE", style="bold red")
        prp_detail = "No wireless links reachable"

    summary = Table.grid(expand=True)
    summary.add_column(justify="center")

    summary.add_row(prp_state)
    summary.add_row(prp_detail)
    summary.add_row("")
    summary.add_row(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    # Rover status section
    rover_table = Table(title="Rover Status", expand=True)

    rover_table.add_column("Device", style="cyan")
    rover_table.add_column("IP", style="magenta")
    rover_table.add_column("Status", justify="center")
    rover_table.add_column("Latency", justify="right")
    rover_table.add_column("Last Seen", justify="right")

    for rover_name, rover_ip in ROVER.items():
        rover_ok, rover_latency = ping(rover_ip)

        rover_table.add_row(
            rover_name,
            rover_ip,
            status_text(rover_ok),
            rover_latency,
            format_last_seen(rover_ip),
        )

    layout = Layout()
    layout.split_column(
        Layout(Panel(summary, title="PRP Status", border_style="blue"), size=8),
        Layout(table),
        Layout(rover_table, size=7),
    )

    return layout



def main():
    with Live(build_dashboard(), refresh_per_second=2, screen=True) as live:
        while True:
            live.update(build_dashboard())
            time.sleep(CHECK_INTERVAL)


if __name__ == "__main__":
    main()