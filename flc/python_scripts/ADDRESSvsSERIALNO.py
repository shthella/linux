import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

data = pd.read_csv('csv_12_08_24_kfifo_1920.csv')

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

def format_32bit_hex(num):
    return f'0x{num:08X}'

def bin_address(x):
    bin_index = np.digitize(x, bin_edges) - 1
    if bin_index < 0 or bin_index >= len(bin_edges):
        return ''
    return f'{format_32bit_hex(bin_edges[bin_index])} - {format_32bit_hex(bin_edges[bin_index] + bin_size - 1)}'

data['Address_Num'] = data['Address'].apply(safe_hex_to_int)

data.dropna(subset=['Address_Num'], inplace=True)

data['Address_Num'] = data['Address_Num'].astype(int)

data = data[(data['Address_Num'] >= 0) & (data['Address_Num'] <= 0xFFFFFFFF)]

min_address = data['Address_Num'].min()
max_address = data['Address_Num'].max()

bin_size = 32
bin_edges = np.arange(min_address, max_address + bin_size, bin_size)

data['Address_Bin'] = data['Address_Num'].apply(bin_address)

plt.figure(figsize=(14, 8), dpi=100)
plt.plot(data['Serial No'], data['Address_Num'], marker='o', linestyle='-', color='b')

yticks = sorted(data['Address_Num'].unique())
plt.gca().set_yticks(bin_edges[::max(1, len(bin_edges)//10)])  # Show fewer y-ticks
plt.gca().set_yticklabels([bin_address(val) for val in plt.gca().get_yticks()], fontsize=10)

plt.xlabel('Serial No', fontsize=12)
plt.ylabel('Address Range (32-bit Hex)', fontsize=12)
plt.title('Serial No vs Address Range', fontsize=14)

plt.gca().tick_params(axis='y', labelrotation=45)

plt.grid(True)
plt.tight_layout()  # Adjust layout to fit labels

plt.show()

