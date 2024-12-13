import struct

# Define the maximum address limit (6GB)
max_address = 6 * 1024 * 1024 * 1024  # 6GB
current_address = 0

# Open the binary file for writing
with open("output_binary_file_6gb.bin", "wb") as bin_file:
    while current_address < max_address:
        # First entry (8 bytes: 3 bytes for size + 5 bytes for address)
        entry_1 = struct.pack("<I", 0x0040)[:3]  # Size (3 bytes)
        entry_1 += struct.pack("<Q", current_address)[1:6]  # Base address (5 bytes)

        # Increment the base address for the second entry by 4KB
        next_address = current_address + 0x1000
        # Second entry (8 bytes: 3 bytes for size + 5 bytes for address)
        entry_2 = struct.pack("<I", 0x0040)[:3]  # Size (3 bytes)
        entry_2 += struct.pack("<Q", next_address)[1:6]  # Base address (5 bytes)

        # Write both entries to the file
        bin_file.write(entry_1)
        bin_file.write(entry_2)

        # Print the formatted output for debugging
        print(f"{current_address:08X}: {entry_1.hex()} {entry_2.hex()}")

        # Increment the address for the next set of entries by 8KB (0x2000)
        current_address += 0x2000  # Increment by 8KB for the next line

print("Binary file generated successfully.")

