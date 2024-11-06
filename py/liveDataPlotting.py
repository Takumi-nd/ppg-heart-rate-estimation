import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

plt.style.use('fivethirtyeight')
filePath = 'records/raw_data.csv'

def plotting(i,filePath):
    data = pd.read_csv(filePath)
    plt.clf()

    plt.plot(data['ir1'],linewidth = '1.5', label = "ir1")
    plt.plot(data['ir2'],linewidth = '1.5', label = "ir2")
    plt.legend(loc="upper right")

    plt.tight_layout()

def animatePlot(filePath):
    anim = FuncAnimation(plt.figure(), plotting, fargs=(filePath,), interval=1000, frames=60)
    plt.show()
    
def staticPlot(filePath):
    data = pd.read_csv(filePath)
    plt.plot(data['ir1'],linewidth = '1.5', label = "ir1")
    plt.plot(data['ir2'],linewidth = '1.5', label = "ir2")
    plt.legend(loc="upper right")
    plt.show()
    
if __name__ == "__main__":
    choice = input("s for static, a for anim: ").lower()
    if choice == 's':
        staticPlot(filePath)
    elif choice == 'a':
        animatePlot(filePath)
        
    


    
