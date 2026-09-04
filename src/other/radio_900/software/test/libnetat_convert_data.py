
import struct

def load_hex_dump(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()

    hex_bytes = []
    for line in lines:
        line = line.strip()
        if not line or line.startswith('[') or 'adc dump' in line.lower():
            continue
        hex_bytes.extend(line.split())

    return bytes(int(b, 16) for b in hex_bytes)

def parse_adc_data(byte_data):
    num_samples = len(byte_data) // 2
    samples = struct.unpack('<' + 'h' * num_samples, byte_data)
    return samples

def main():
    input_file = 'adc_dump.txt'
    samples = parse_adc_data(load_hex_dump(input_file))
    
    print("Decoded ADC samples (signed 16-bit):")
    for i, val in enumerate(samples):
        print(f"{i}: {val}")

if __name__ == '__main__':
    main()