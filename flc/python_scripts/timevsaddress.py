#import pandas as pd
#import matplotlib.pyplot as plt
#data = pd.read_csv('embedded_data.csv')
#data['Address'] = data['Address'].apply(lambda x: int(x, 16))
#plt.plot(data['Read Time (ms)'], data['Address'], marker='o')
#plt.xlabel('Read Time (ms)')
#plt.ylabel('Address')
#plt.title('Read Time vs Address')
#plt.grid(True)
#plt.show()

#  import pandas as pd
# import matplotlib.pyplot as plt
# data = pd.read_csv('csv_12_08_24_kfifo_1920.csv')
# #data = data[pd.to_numeric(data['Read Time (ms)'], errors='coerce').notnull()]
# #data['Read Time (ms)'] = data['Read Time (ms)'].astype(float)
# data['Address'] = data['Address'].apply(lambda x: int(x, 16))
# plt.figure(figsize=(10, 6))
# plt.plot(data['Serial No'], data['Address'], marker='o', linestyle='-', color='b')
# plt.gca().get_yaxis().set_major_formatter(plt.FuncFormatter(lambda x, _: f'0x{int(x):08X}'))
# #plt.gca().get_xaxis().set_major_formatter(plt.FuncFormatter(lambda x, _: f'{x:.2f}'))
# plt.xlabel('Serial No')
# plt.ylabel('Address (Hex)')
# plt.title('Read Time vs Address')
# plt.grid(True)
# plt.show()
#
# import pandas as pd
# import matplotlib.pyplot as plt
# 
# # Load the CSV file
# data = pd.read_csv('csv_12_08_24_kfifo_1920.csv')
# 
# # Define a function to safely convert hex strings to integers
# def safe_hex_to_int(x):
#     if isinstance(x, str):
#         try:
#             return int(x, 16)
#         except ValueError:
#             return None
#     else:
#         return None
# 
# # Apply the function to the Address column
# data['Address'] = data['Address'].apply(safe_hex_to_int)
# 
# # Drop rows where Address is None (invalid hex values)
# data.dropna(subset=['Address'], inplace=True)
# 
# # Ensure Address is of integer type
# data['Address'] = data['Address'].astype(int)
# 
# # Plotting
# plt.figure(figsize=(10, 6))
# plt.plot(data['Serial No'], data['Address'], marker='o', linestyle='-', color='b')
# 
# # Format the y-axis as hexadecimal
# plt.gca().get_yaxis().set_major_formatter(plt.FuncFormatter(lambda x, _: f'0x{int(x):X}'))
# 
# plt.xlabel('Serial No')
# plt.ylabel('Address (Hex)')
# plt.title('Read Time vs Address')
# plt.grid(True)
# plt.show()
#
# import pandas as pd
# import matplotlib.pyplot as plt
# import matplotlib.ticker as ticker
#  
# # # Load the CSV file
# data = pd.read_csv('csv_12_08_24_kfifo_1920.csv')
# # 
# #  Define a function to safely convert hex strings to integers
# def safe_hex_to_int(x):
#      try:
#          # Check if the value is a valid hex string
#          if isinstance(x, str) and not x.startswith('0x'):
#              x = x.strip()
#              if 'E' in x or 'e' in x:
#                  return None
#              return int(x, 16)
#          return None
#      except ValueError:
#          return None
#  
#  # Apply the function to the Address column
# data['Address_Num'] = data['Address'].apply(safe_hex_to_int)
#  
#  # Drop rows with invalid Address_Num
# data.dropna(subset=['Address_Num'], inplace=True)
#  
#  # Ensure Address_Num is of integer type
# data['Address_Num'] = data['Address_Num'].astype(int)
#  
# # # Filter out addresses that are not within 32-bit range
# data = data[(data['Address_Num'] >= 0) & (data['Address_Num'] <= 0xFFFFFFFF)]
# # 
#  # Create a mapping from numerical addresses to 32-bit hexadecimal strings
# def format_32bit_hex(num):
#      return f'0x{num:08X}'
# # 
#  # Plotting
# plt.figure(figsize=(12, 8))
# plt.plot(data['Serial No'], data['Address_Num'], marker='o', linestyle='-', color='b')
# # 
#  # Set y-axis labels
# yticks = sorted(data['Address_Num'].unique())
# plt.gca().set_yticks(yticks)
# plt.gca().set_yticklabels([format_32bit_hex(val) for val in yticks])
# # 
#  # Format the y-axis to show 32-bit hexadecimal addresses
# def format_hex(y, pos):
#      return format_32bit_hex(int(y))
# # 
# plt.gca().yaxis.set_major_formatter(ticker.FuncFormatter(format_hex))
# # 
# plt.xlabel('Serial No')
# plt.ylabel('Address (32-bit Hex)')
# plt.title('Serial No vs Address')
# plt.grid(True)
# plt.show()
# #
# import pandas as pd
# import matplotlib.pyplot as plt
# import matplotlib.ticker as ticker
# 
# # Load the CSV file
# data = pd.read_csv('csv_12_08_24_kfifo_1920.csv')
# 
# # Define a function to safely convert hex strings to integers
# def safe_hex_to_int(x):
#     try:
#         if isinstance(x, str) and not x.startswith('0x'):
#             x = x.strip()
#             if 'E' in x or 'e' in x:
#                 return None
#             return int(x, 16)
#         return None
#     except ValueError:
#         return None
# 
# # Apply the function to the Address column
# data['Address_Num'] = data['Address'].apply(safe_hex_to_int)
# 
# # Drop rows with invalid Address_Num
# data.dropna(subset=['Address_Num'], inplace=True)
# 
# # Ensure Address_Num is of integer type
# data['Address_Num'] = data['Address_Num'].astype(int)
# 
# # Filter out addresses that are not within 32-bit range
# data = data[(data['Address_Num'] >= 0) & (data['Address_Num'] <= 0xFFFFFFFF)]
# 
# # Create a mapping from numerical addresses to 32-bit hexadecimal strings
# def format_32bit_hex(num):
#     return f'0x{num:08X}'
# 
# # Plotting
# plt.figure(figsize=(14, 8), dpi=100)  # Increase figure size and DPI for better quality
# 
# # Plot the data
# plt.plot(data['Serial No'], data['Address_Num'], marker='o', linestyle='-', color='b')
# 
# # Set y-axis limits and format
# yticks = sorted(data['Address_Num'].unique())
# # Limit y-ticks to avoid overcrowding
# plt.gca().set_yticks(yticks[::len(yticks)//10])  # Show fewer y-ticks
# plt.gca().set_yticklabels([format_32bit_hex(val) for val in plt.gca().get_yticks()], fontsize=10)
# 
# # Format the y-axis to show 32-bit hexadecimal addresses
# def format_hex(y, pos):
#     return format_32bit_hex(int(y))
# 
# plt.gca().yaxis.set_major_formatter(ticker.FuncFormatter(format_hex))
# 
# # Improve x and y labels and title
# plt.xlabel('Serial No', fontsize=12)
# plt.ylabel('Address (32-bit Hex)', fontsize=12)
# plt.title('Serial No vs Address', fontsize=14)
# 
# # Rotate y-axis labels to fit better
# plt.gca().tick_params(axis='y', labelrotation=45)
# 
# plt.grid(True)
# plt.tight_layout()  # Adjust layout to fit labels
# 
# plt.show()
#


import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# Load the CSV file
data = pd.read_csv('csv_12_08_24_kfifo_1920.csv')

# Define a function to safely convert hex strings to integers
def safe_hex_to_int(x):
    try:
        if isinstance(x, str) and not x.startswith('0x'):
            x = x.strip()
            if 'E' in x or 'e' in x:
                return None
            return int(x, 16)
        return None
    except ValueError:
        return None

# Define a function to format numbers as 32-bit hexadecimal strings
def format_32bit_hex(num):
    return f'0x{num:08X}'

# Define a function to convert addresses to binned ranges
def bin_address(x):
    bin_index = np.digitize(x, bin_edges) - 1
    if bin_index < 0 or bin_index >= len(bin_edges):
        return ''
    return f'{format_32bit_hex(bin_edges[bin_index])} - {format_32bit_hex(bin_edges[bin_index] + bin_size - 1)}'

# Apply the function to the Address column
data['Address_Num'] = data['Address'].apply(safe_hex_to_int)

# Drop rows with invalid Address_Num
data.dropna(subset=['Address_Num'], inplace=True)

# Ensure Address_Num is of integer type
data['Address_Num'] = data['Address_Num'].astype(int)

# Filter out addresses that are not within 32-bit range
data = data[(data['Address_Num'] >= 0) & (data['Address_Num'] <= 0xFFFFFFFF)]

# Determine the range of addresses
min_address = data['Address_Num'].min()
max_address = data['Address_Num'].max()

# Define bin edges (32-bit ranges from min to max address)
bin_size = 32
bin_edges = np.arange(min_address, max_address + bin_size, bin_size)

# Apply binning function to address column
data['Address_Bin'] = data['Address_Num'].apply(bin_address)

# Plotting
plt.figure(figsize=(14, 8), dpi=100)
# Plot the data
plt.plot(data['Serial No'], data['Address_Num'], marker='o', linestyle='-', color='b')

# Set y-axis limits and format
yticks = sorted(data['Address_Num'].unique())
plt.gca().set_yticks(bin_edges[::max(1, len(bin_edges)//10)])  # Show fewer y-ticks
plt.gca().set_yticklabels([bin_address(val) for val in plt.gca().get_yticks()], fontsize=10)

# Improve x and y labels and title
plt.xlabel('Serial No', fontsize=12)
plt.ylabel('Address Range (32-bit Hex)', fontsize=12)
plt.title('Serial No vs Address Range', fontsize=14)

# Rotate y-axis labels to fit better
plt.gca().tick_params(axis='y', labelrotation=45)

plt.grid(True)
plt.tight_layout()  # Adjust layout to fit labels

plt.show()

