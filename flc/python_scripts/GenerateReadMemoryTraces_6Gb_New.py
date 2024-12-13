import struct

max_address = 6 * 1024 * 1024 * 1024  # 6GB
current_address = 0

with open("output_binary_file_6gb_new.bin", "wb") as bin_file:
    while current_address < max_address:
        entry_1 = struct.pack("<I", 0x000040)[:3]  # Size (3 bytes)
        entry_1 += struct.pack("<Q", current_address)[0:5]  # Base address (5 bytes)

        next_address = current_address + 0x1000
        entry_2 = struct.pack("<I", 0x000040)[:3]  # Size (3 bytes)
        entry_2 += struct.pack("<Q", next_address)[0:5]  # Base address (5 bytes)

        bin_file.write(entry_1)
        bin_file.write(entry_2)

        print(f"{current_address:08X}: {entry_1.hex()} {entry_2.hex()}")

        current_address += 0x2000  # Increment by 8KB for the next line

print("Binary file generated successfully.")

