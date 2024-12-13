import struct

# Define the maximum address limit (4GB)
max_address =  4 * 1024 * 1024 * 1024   # 4GB
current_address = 0

# Open the binary file for writing
with open("output_binary_file.bin", "wb") as bin_file:
    while current_address < max_address:
        # First entry (8 bytes)
        entry_1 = struct.pack("<I", 0x000040)  # Size
        entry_1 += struct.pack("<I", current_address)  # Base address

        # Increment the base address for the second entry by 4KB
        next_address = current_address + 0x1000
        # Second entry (8 bytes)
        entry_2 = struct.pack("<I", 0x000040)  # Size
        entry_2 += struct.pack("<I", next_address)  # Base address

        bin_file.write(entry_1)
        bin_file.write(entry_2)

        # Print the formatted output
#        print(f"{current_address:08X}: {entry_1.hex(' ')} {entry_2.hex(' ')}")

        # Increment the address for the next set of entries by 8KB (0x2000)
        current_address += 0x2000  # Increment by 8KB for the next line

print("Binary file generated successfully.")

