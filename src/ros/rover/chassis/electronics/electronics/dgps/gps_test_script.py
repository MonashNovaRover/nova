#!/usr/bin/env python3

from serial import Serial
from pynmeagps import NMEAReader
import time


def build_query_extended_nmea():
    payload = bytes([0x64, 0x00])
    cs = 0
    for b in payload:
        cs ^= b
    return bytes([0xA0, 0xA1, 0x00, 0x02]) + payload + bytes([cs, 0x0D, 0x0A])


def build_enable_psti036(psti030_rate=0, psti036_rate=1, save_to_flash=False):
    attrs = 0x01 if save_to_flash else 0x00
    payload = bytes([
        0x64,
        0x01,
        0x01,           # GGA rate
        0x01,           # GSA rate
        0x01,           # GSV rate
        0x01,           # GLL rate
        0x01,           # RMC rate
        0x01,           # VTG rate
        0x01,           # ZDA rate
        psti030_rate,
        psti036_rate,
        attrs,
    ])
    cs = 0
    for b in payload:
        cs ^= b
    return bytes([0xA0, 0xA1, 0x00, len(payload)]) + payload + bytes([cs, 0x0D, 0x0A])


def parse_psti(raw_bytes):
    """Parse PSTI sentences from SkyTraq modules."""
    try:
        sentence = raw_bytes.decode('ascii', errors='ignore').strip()
        if not sentence.startswith('$PSTI'):
            return None
        if '*' in sentence:
            sentence = sentence[:sentence.index('*')]
        fields = sentence.split(',')
        msg_type = fields[1] if len(fields) > 1 else '?'

        if msg_type == '030':
            # $PSTI,030,hhmmss,fix_status,lat,N/S,lon,E/W,alt,E,N,U,date,da_status,baseline_len,baseline_course
            # Index:  0    1       2           3    4   5   6   7  8  9 10  11  12       13          14             15
            def ff(i):
                return float(fields[i]) if len(fields) > i and fields[i] else None

            fix_status = fields[3] if len(fields) > 3 else '?'
            da_status  = fields[13] if len(fields) > 13 else '?'
            baseline   = ff(14)
            return {
                'type':       '030',
                'time':       fields[2] if len(fields) > 2 else None,
                'fix_status': fix_status,
                'da_status':  da_status,   # dual-antenna status: A=valid, V=invalid
                'baseline':   baseline,    # metres — should match physical antenna separation
                'e':          ff(9),       # baseline east component
                'n':          ff(10),      # baseline north component
                'u':          ff(11),      # baseline up component
                'valid':      da_status == 'A' and baseline is not None and baseline > 0.01,
            }

        elif msg_type == '032':
            # $PSTI,032,hhmmss,ddmmyy,status,heading,pitch,roll,baseline,...
            status = fields[4] if len(fields) > 4 else '?'
            def ff(i):
                return float(fields[i]) if len(fields) > i and fields[i] else None
            return {
                'type':     '032',
                'time':     fields[2] if len(fields) > 2 else None,
                'status':   status,
                'heading':  ff(5),
                'pitch':    ff(6),
                'roll':     ff(7),
                'baseline': ff(8),
                'valid':    status == 'A',
            }

        elif msg_type == '036':
            # $PSTI,036,hhmmss,status,heading,pitch,roll,baseline,...
            status = fields[3] if len(fields) > 3 else '?'
            def ff(i):
                return float(fields[i]) if len(fields) > i and fields[i] else None
            return {
                'type':     '036',
                'status':   status,
                'heading':  ff(4),
                'pitch':    ff(5),
                'roll':     ff(6),
                'baseline': ff(7),
                'valid':    status == 'A',
            }

        else:
            return {'type': msg_type, 'raw': sentence}

    except Exception as e:
        return {'type': 'error', 'error': str(e), 'raw': raw_bytes}


class GPSRoverStandalone:
    def __init__(self, port='COM7', baudrate=115200):
        self.port = port
        self.baudrate = baudrate

        self.latitude   = None
        self.longitude  = None
        self.altitude   = None
        self.fix_valid  = False
        self.heading    = None
        self.pitch      = None
        self.roll       = None
        self.baseline   = None
        self.nav_status = None

        self.ser = Serial()
        self._open_port()

        self.reader = NMEAReader(
            self.ser,
            validate=0x01,
            nmeaonly=False,
        )

    def _open_port(self):
        print("🔌 Opening serial port...")
        self.ser.port = self.port
        self.ser.baudrate = self.baudrate
        self.ser.open()
        print(f"✅ Connected to {self.ser.port} @ {self.ser.baudrate}")

    def _wait_for_sentence_boundary(self):
        self.ser.read_until(b'\n')

    def send_query(self):
        self._wait_for_sentence_boundary()
        cmd = build_query_extended_nmea()
        self.ser.write(cmd)
        print(f"📡 Sent config query: {cmd.hex()}")
        time.sleep(0.3)

    def send_heading_enable(self, save_to_flash=False):
        self._wait_for_sentence_boundary()
        cmd = build_enable_psti036(psti030_rate=1, psti036_rate=1, save_to_flash=save_to_flash)
        self.ser.write(cmd)
        print(f"📡 Sent heading enable: {cmd.hex()}")
        time.sleep(0.5)

    def parse_nmea(self):
        try:
            msg_raw, msg_parsed = self.reader.read()
        except Exception as e:
            if 'checksum' not in str(e).lower():
                print(f"❌ Read error: {e}")
            return

        # --- Intercept PSTI sentences before pynmeagps touches them ---
        if msg_raw and b'$PSTI' in msg_raw:
            result = parse_psti(msg_raw)
            if not result:
                return

            if result['type'] == '030':
                # Always print PSTI,030 — this is our key diagnostic
                da  = result['da_status']
                bl  = result['baseline']
                bl_str = f"{bl:.3f}m" if bl is not None else "n/a"
                e, n, u = result['e'], result['n'], result['u']
                vec_str = f"E={e:.3f} N={n:.3f} U={u:.3f}" if e is not None else "n/a"

                flag = ""
                if bl is not None and bl < 0.01:
                    flag = " \n⚠️  BASELINE IS ZERO — antennas may be on same port or one disconnected"
                elif bl is not None and bl > 0.01:
                    flag = "  ✅ Baseline looks healthy"

                print(f"📐 PSTI,030  fix={result['fix_status']}  da_status={da}  baseline={bl_str}  vector=({vec_str}){flag}")

            elif result['type'] in ('032', '036'):
                if result['valid']:
                    self.heading  = result['heading']
                    self.pitch    = result['pitch']
                    self.roll     = result['roll']
                    self.baseline = result['baseline']
                print(f"🧭 PSTI,{result['type']} — Dual Antenna Heading")
                print(f"   Raw     : {msg_raw.strip()}")
                print(f"   Status  : {result['status']}  ({'valid' if result['valid'] else 'waiting for RTK fix'})")
                if result['heading'] is not None:
                    print(f"   Heading : {result['heading']}°")
                    print(f"   Pitch   : {result['pitch']}°")
                    print(f"   Roll    : {result['roll']}°")
                    print(f"   Baseline: {result['baseline']} m")
                print("-" * 50)

            return

        if msg_parsed is None:
            return

        try:
            if msg_parsed.msgID == 'RMC':
                self.nav_status = getattr(msg_parsed, 'navStatus', None)
                if msg_parsed.status == 'A':
                    self.latitude  = msg_parsed.lat
                    self.longitude = msg_parsed.lon
                    self.fix_valid = True
                else:
                    self.fix_valid = False

            elif msg_parsed.msgID == 'GGA':
                if msg_parsed.quality > 0:
                    self.latitude  = msg_parsed.lat
                    self.longitude = msg_parsed.lon
                    self.altitude  = msg_parsed.alt
                    self.fix_valid = True

            if msg_parsed.msgID == 'GGA':
                print("📍 Position update:")
                if self.latitude is not None:
                    print(f"   Lat/Lon : {self.latitude:.7f}, {self.longitude:.7f}")
                    print(f"   Alt     : {self.altitude} m")
                else:
                    print("   Lat/Lon : no fix yet")
                print(f"   Fix     : quality={msg_parsed.quality}, sats={msg_parsed.numSV}, HDOP={msg_parsed.HDOP}")
                print(f"   NavStat : {self.nav_status}")
                if self.heading is not None:
                    print(f"   Heading : {self.heading:.2f}°  Pitch: {self.pitch:.2f}°  Roll: {self.roll:.2f}°  Base: {self.baseline:.3f}m")
                else:
                    print("   Heading : not yet available (waiting for PSTI,036)")
                print("-" * 50)

        except Exception as e:
            print(f"❌ Parse error: {e} — {msg_parsed}")

    def run(self, enable_heading=True, query_first=True):
        print("🚀 Starting GNSS reader...")

        if query_first:
            print("\n--- Querying current config ---")
            self.send_query()
            for _ in range(10):
                self.parse_nmea()

        if enable_heading:
            print("\n--- Enabling PSTI,030 + PSTI,036 ---")
            self.send_heading_enable(save_to_flash=False)

        print("\n--- Reading ---")
        try:
            while True:
                self.parse_nmea()
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\n🛑 Stopping...")
            self.ser.close()


if __name__ == "__main__":
    gps = GPSRoverStandalone(port='COM7', baudrate=115200)
    gps.run(enable_heading=True, query_first=True)