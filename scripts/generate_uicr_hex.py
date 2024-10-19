#!/usr/bin/env python3

import sys
import re
from intelhex import IntelHex


def main():
    if len(sys.argv) != 3:
        print("Usage: generate_uicr_hex.py <value> <output_hex_file>")
        print("Value can be a 32-bit hex value (e.g., 0x01020300) or a version string (e.g., v1.2.3)")
        print("Example: generate_uicr_hex.py v1.2.3 uicr_data.hex")
        sys.exit(1)

    value_str = sys.argv[1]
    output_filename = sys.argv[2]

    # Function to parse version string
    def parse_version_string(version_str):
        match = re.match(r'^v(\d+)\.(\d+)\.(\d+)$', version_str)
        if not match:
            raise ValueError("Invalid version string format. Expected format 'vX.Y.Z'")
        major, minor, patch = match.groups()
        major = int(major)
        minor = int(minor)
        patch = int(patch)
        if not (0 <= major <= 255 and 0 <= minor <= 255 and 0 <= patch <= 255):
            raise ValueError("Version numbers must be between 0 and 255")
        # Pack into 0xMMmmpp00 format
        value = (major << 24) | (minor << 16) | (patch << 8)
        return value

    # Function to parse hex value
    def parse_hex_value(hex_str):
        try:
            value = int(hex_str, 16)
        except ValueError:
            raise ValueError("Invalid hex value. Please provide a valid 32-bit hexadecimal value.")
        # Ensure the value is a 32-bit unsigned integer
        if value < 0 or value > 0xFFFFFFFF:
            raise ValueError("Value must be a 32-bit unsigned integer (0x00000000 to 0xFFFFFFFF).")
        return value

    # Determine if value_str is a version string or hex value
    if value_str.startswith('v'):
        try:
            value = parse_version_string(value_str)
        except ValueError as e:
            print(f"Error: {e}")
            sys.exit(1)
    else:
        try:
            value = parse_hex_value(value_str)
        except ValueError as e:
            print(f"Error: {e}")
            sys.exit(1)

    # Define the UICR address
    uicr_address = 0x10001080

    # Create an IntelHex object
    ih = IntelHex()

    # Convert the value to little-endian byte order
    value_bytes = value.to_bytes(4, byteorder='little')

    # Write the value to the specified address
    ih.puts(uicr_address, value_bytes)

    # Write the Intel HEX file
    ih.write_hex_file(output_filename)

    print(f"Generated HEX file '{output_filename}' with value 0x{value:08X} at address 0x{uicr_address:08X}")


if __name__ == "__main__":
    main()
