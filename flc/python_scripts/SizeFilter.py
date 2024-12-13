import struct
import argparse

def is_within_range(value, min_value, max_value):
    return min_value <= value <= max_value

def reorder_bytes(value):
    return ((value & 0xFFFF) << 16) | ((value & 0xFFFF0000) >> 16)

def process_file(input_file, output_file, min_value, max_value, debug=False):
    min_value_bytes = min_value * 1024 * 1024 * 1024
    max_value_bytes = max_value * 1024 * 1024 * 1024

    with open(input_file, 'rb') as infile, open(output_file, 'wb') as outfile:
        while True:
            original_offset = infile.tell()

            data = infile.read(16)
            if len(data) < 16:
                if data and debug:
                    print(f"Warning: Skipping remaining data with insufficient length: {len(data)} bytes")
                break

            frame1 = data[:8]
            frame2 = data[8:]

            value1 = struct.unpack_from('<I', frame1, 4)[0]  
            value2 = struct.unpack_from('<I', frame2, 4)[0]  

            value1_reordered = reorder_bytes(value1)
            value2_reordered = reorder_bytes(value2)

            if debug:
                print(f"Original offset: {original_offset:08X} - Comparing values: {value1_reordered:08X} and {value2_reordered:08X}")

            if not is_within_range(value1_reordered, min_value_bytes, max_value_bytes):
                outfile.write(frame1)
                if debug:
                    print(f"Original offset: {original_offset:08X} - Frame1 written to output file")
            else:
                if debug:
                    print(f"Original offset: {original_offset:08X} - Skipping Frame1: {value1_reordered:08X} (within range)")

            if not is_within_range(value2_reordered, min_value_bytes, max_value_bytes):
                outfile.write(frame2)
                if debug:
                    print(f"Original offset: {original_offset:08X} - Frame2 written to output file")
            else:
                if debug:
                    print(f"Original offset: {original_offset:08X} - Skipping Frame2: {value2_reordered:08X} (within range)")

def main():
    parser = argparse.ArgumentParser(description="Filter records based on reordered 4-byte values with a specified range in GB.")
    parser.add_argument('input_file', help="Input binary file")
    parser.add_argument('output_file', help="Output binary file")
    parser.add_argument('--min', type=int, required=True, help="Minimum value in the range (in GB)")
    parser.add_argument('--max', type=int, required=True, help="Maximum value in the range (in GB)")
    parser.add_argument('--debug', action='store_true', help="Enable debug mode with detailed output")
    args = parser.parse_args()

    process_file(args.input_file, args.output_file, min_value=args.min, max_value=args.max, debug=args.debug)

if __name__ == "__main__":
    main()

