import serial
import struct
import sys

def parse_ubx(port="/dev/ttyUSB0", baud=230400):
    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"Connected to {port}. Extracting coordinates...")

        while True:
            char1 = ser.read(1)
            if char1 == b'\xb5':
                char2 = ser.read(1)
                if char2 == b'\x62':
                    header = ser.read(4)
                    if len(header) < 4: continue
                    
                    msg_class, msg_id, length = struct.unpack("<BBH", header)
                    payload = ser.read(length)
                    ser.read(2) # Consume checksum
                    
                    # NAV-PVT Message (Class 0x01, ID 0x07)
                    if msg_class == 0x01 and msg_id == 0x07 and length >= 92:
                        # iTOW(0), year(4), month(6), day(7), hour(8), min(9), sec(10), valid(11), tAcc(12), fNano(16), fixType(20)
                        # Lon is at offset 24, Lat at offset 28 (both 4 bytes, signed i32)
                        lon_raw, lat_raw = struct.unpack("<ii", payload[24:32])
                        fix_type = payload[20] # 0=No fix, 3=3D fix
                        
                        lon = lon_raw / 1e7
                        lat = lat_raw / 1e7
                        
                        fix_status = {0:"No Fix", 2:"2D Fix", 3:"3D Fix", 4:"GNSS+Dead Reckoning"}.get(fix_type, "Unknown")
                        
                        num_sv = payload[23] # Number of satellites used in Nav Solution
                        print(f"[{fix_status}] Satellites: {num_sv} | Lat: {lat:.7f}, Lon: {lon:.7f}")              


    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        if 'ser' in locals(): ser.close()

if __name__ == "__main__":
    device = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    parse_ubx(device)
