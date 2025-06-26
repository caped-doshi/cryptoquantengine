import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

def plot_equity_dd(time, equity,xlabel:str,ylabel:str,title:str,y_lim:float=0.0,log:bool=False):
    peak = np.maximum.accumulate(equity)
    drawdown = (equity - peak)
    drawdown_pct = drawdown / peak * 100

    fig, ax1 = plt.subplots(figsize=(16, 6))

    ax1.fill_between(time, equity, color='slateblue', alpha=0.3)

    ax1.plot(time, equity, color='black', linewidth=1.2, label='Equity')

    ax2 = ax1.twinx()
    ax2.fill_between(time, 0, drawdown_pct, where=(drawdown_pct < 0),
                     color='red', alpha=0.5, label='Drawdown (%)')

    ax1.set_ylabel("Equity", color='black')
    ax2.set_ylabel("Drawdown (%)",color='black')

    ax1.tick_params(axis='y', labelcolor='black')
    ax2.tick_params(axis='y', labelcolor='black')

    plt.title("Equity Curve with Drawdown (%) on Right Axis")
    ax1.grid(True)

    if log:
        ax1.set_yscale('log')
    if y_lim:
        ax1.set_ylim(bottom=y_lim)

    plt.tight_layout()
    plt.show()

def plot_equity(x, y, xlabel="x", ylabel="y", title="title", log=False):

    plt.figure(figsize=(12, 6))
    plt.plot(x, y, label="Equity")
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    
    if log:
        plt.yscale("log")
    
    plt.grid(True, which='major', linestyle='-', linewidth=0.75)
    plt.grid(True, which='minor', linestyle='--', linewidth=0.5)
    
    # Enable minor ticks (both x and y)
    plt.minorticks_on()
    
    plt.legend()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    df = pd.read_csv("equity.csv", names=["Time", "Equity"])
    df["Time"] = pd.to_datetime(df["Time"], unit='us')
    plot_equity(df.Time,df.Equity,xlabel="trade",ylabel="USD",title="equity over time",log=False)