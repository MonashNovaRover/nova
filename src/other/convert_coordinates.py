#!/home/nova/Builds/master/bin/python3

import argparse
import re
import sys

def parse_coordinate(coord_str, in_format):
    """Extracts numbers from the string and converts to a base Decimal Degree (DD) float."""
    coord_str = coord_str.strip().upper()
    
    # Determine the sign based on cardinal direction or negative sign
    sign = -1 if 'S' in coord_str or 'W' in coord_str or coord_str.startswith('-') else 1
    
    # Check if it's explicitly a Longitude based on the letter
    is_lat = False if 'E' in coord_str or 'W' in coord_str else True

    # Extract all floating-point or integer numbers from the string
    nums = [float(n) for n in re.findall(r"\d+\.?\d*", coord_str)]

    if not nums:
        raise ValueError(f"Could not extract numbers from: '{coord_str}'")

    if in_format == 'DMS' and len(nums) >= 3:
        dd = nums[0] + (nums[1] / 60.0) + (nums[2] / 3600.0)
    elif in_format == 'DDM' and len(nums) >= 2:
        dd = nums[0] + (nums[1] / 60.0)
    elif in_format == 'DD' and len(nums) >= 1:
        dd = nums[0]
    else:
        raise ValueError(f"Input '{coord_str}' doesn't have enough numbers for {in_format} format.")

    return dd * sign, is_lat

def format_coordinate(dd, out_format, is_lat=True):
    """Converts a Decimal Degree (DD) float back into the requested string format."""
    # Determine cardinal direction
    if is_lat:
        direction = "N" if dd >= 0 else "S"
    else:
        direction = "E" if dd >= 0 else "W"
        
    dd = abs(dd)

    if out_format == 'DD':
        return f"{dd:.6f}° {direction}"
        
    elif out_format == 'DDM':
        degrees = int(dd)
        minutes = (dd - degrees) * 60
        return f"{degrees}° {minutes:.4f}' {direction}"
        
    elif out_format == 'DMS':
        degrees = int(dd)
        minutes = int((dd - degrees) * 60)
        seconds = (dd - degrees - (minutes / 60.0)) * 3600
        return f"{degrees}° {minutes}' {seconds:.2f}\" {direction}"

def main():
    parser = argparse.ArgumentParser(
        description="CLI tool to convert coordinates between DD, DDM, and DMS.",
        epilog="Examples:\n"
               "  python coord_converter.py \"40 26 46 N\"\n"
               "  python coord_converter.py \"40° 26' 46 N\" \"79° 58' 56 W\"\n"
               "  python coord_converter.py -i DD -o DMS \"40.446111\" \"-79.982222\"",
        formatter_class=argparse.RawTextHelpFormatter
    )
    
    parser.add_argument("coordinates", nargs='+', help="The coordinate string(s) to convert. Enclose in quotes.")
    parser.add_argument("-i", "--input", choices=['DD', 'DDM', 'DMS'], default='DMS', help="Format of the input (Default: DMS)")
    parser.add_argument("-o", "--output", choices=['DD', 'DDM', 'DMS'], default='DD', help="Format of the output (Default: DD)")

    args = parser.parse_args()

    print(f"\nConverting {args.input} -> {args.output}\n" + "-"*30)

    for i, coord in enumerate(args.coordinates):
        try:
            # Parse the input into a standard Decimal Degree format
            base_dd, detected_is_lat = parse_coordinate(coord, args.input)
            
            # If the user didn't provide N/S/E/W, we assume the first coordinate is Lat and second is Lon
            if 'N' not in coord.upper() and 'S' not in coord.upper() and 'E' not in coord.upper() and 'W' not in coord.upper():
                is_lat = True if i == 0 else False
            else:
                is_lat = detected_is_lat
                
            # Format to the desired output
            result = format_coordinate(base_dd, args.output, is_lat)
            
            print(f"Input:  {coord}")
            print(f"Output: {result}\n")
            
        except ValueError as e:
            print(f"Error processing '{coord}': {e}\n")
            sys.exit(1)

if __name__ == '__main__':
    main()
