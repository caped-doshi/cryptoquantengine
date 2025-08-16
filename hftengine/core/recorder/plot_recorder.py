import sys
import pandas as pd
import matplotlib.pyplot as plt

# Check Python version
if sys.version_info < (3, 7):
    print("Python 3.7 or higher is required.")
    sys.exit(1)

# Check required packages
required = ['matplotlib', 'pandas']
missing = []
for pkg in required:
    try:
        __import__(pkg)
    except ImportError:
        missing.append(pkg)

if missing:
    print(f"Missing required packages: {', '.join(missing)}")
    print("Install them with: pip install " + ' '.join(missing))
    sys.exit(1)

if len(sys.argv) < 3:
    print("Usage: python plot_recorder.py <csv_filename> <asset_id>")
    sys.exit(1)

csv_filename = sys.argv[1]
asset_id = sys.argv[2]

# Load data
df = pd.read_csv(csv_filename)

# Convert timestamp to a readable format if needed (optional)
df['timestamp'] = pd.to_datetime(df['timestamp'], unit='us')

fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(14, 8))

# --- Top plot: Equity ---
color_equity = 'tab:blue'
color_mid = 'tab:gray'

ax1.plot(df['timestamp'], df['equity'], label='Equity')
ax1.set_ylabel('Equity')
ax1.tick_params(axis='y')
ax1.legend(loc='upper left')
ax1.grid(True)

# Right axis for mid price (background)
ax1b = ax1.twinx()
ax1b.plot(df['timestamp'], df['mid_price'], color=color_mid, alpha=0.3, label='Price')
ax1b.set_ylabel('Price')
ax1b.tick_params(axis='y')
ax1b.legend(loc='upper right')

# --- Bottom plot: Position ---
ax2.plot(df['timestamp'], df['position'], label='Position')
ax2.set_ylabel('Position')
ax2.tick_params(axis='y')
ax2.legend(loc='upper left')
ax2.grid(True)

# Right axis for mid price (background)
ax2b = ax2.twinx()
ax2b.plot(df['timestamp'], df['mid_price'], color=color_mid, alpha=0.3, label='Price')
ax2b.set_ylabel('Price')
ax2b.tick_params(axis='y')
ax2b.legend(loc='upper right')

plt.xlabel('Timestamp')
plt.suptitle(f'Asset {asset_id} Equity and Position Over Time')
plt.tight_layout()
plt.show()