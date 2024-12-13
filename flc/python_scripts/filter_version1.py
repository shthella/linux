import struct
import argparse

def is_greater_than_3GB(value):
    # 3GB in bytes
    THRESHOLD = 2 * 1024 * 1024 * 1024  # 3221225472 bytes
    return value > THRESHOLD

def reorder_bytes(value):
    # Reorder bytes from something like 9ba30000 to a39b0000
    return ((value & 0xFFFF) << 16) | ((value & 0xFFFF0000) >> 16)

def process_file(input_file, output_file):
    with open(input_file, 'rb') as infile, open(output_file, 'wb') as outfile:
        while True:
            # Get the current offset in the original file
            original_offset = infile.tell()

            # Read 16 bytes of data
            data = infile.read(16)
            if len(data) < 16:
                # End of file or incomplete record
                if data:
                    print(f"Warning: Skipping remaining data with insufficient length: {len(data)} bytes")
                break

            # Extract the 4-byte fields for comparison from offsets 8 and 12
            value1 = struct.unpack_from('<I', data, 4)[0]  # 4 bytes at offset 8
            value2 = struct.unpack_from('<I', data, 12)[0]  # 4 bytes at offset 12

            # Reorder bytes within the 4-byte field
            value1_reordered = reorder_bytes(value1)
            value2_reordered = reorder_bytes(value2)

            # Print the values being compared with the correct original offset
            print(f"Original offset: {original_offset:08X} - Comparing values: {value1_reordered:08X} and {value2_reordered:08X}")

            # Check if either value is greater than 3GB
            if not (is_greater_than_3GB(value1_reordered) or is_greater_than_3GB(value2_reordered)):
                # Write the record to the output file if both values are not greater than 3GB
                outfile.write(data)
                print(f"Output offset: {original_offset:08X} - Written to output file")
            else:
                # Print the skipped values with the correct original offset
                if is_greater_than_3GB(value1_reordered):
                    print(f"Original offset: {original_offset:08X} - Skipping value: {value1_reordered:08X} (greater than 3GB)")
                if is_greater_than_3GB(value2_reordered):
                    print(f"Original offset: {original_offset:08X} - Skipping value: {value2_reordered:08X} (greater than 3GB)")

def main():
    parser = argparse.ArgumentParser(description="Filter records based on reordered 4-byte values starting from offsets 8 and 12.")
    parser.add_argument('input_file', help="Input binary file")
    parser.add_argument('output_file', help="Output binary file")
    args = parser.parse_args()

    process_file(args.input_file, args.output_file)

if __name__ == "__main__":
    main()

                           
