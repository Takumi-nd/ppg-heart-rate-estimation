import serial
import keyboard
import csv

pause = True
running = True
filePath = 'records/raw_data.csv'
def togglePause():
    global pause
    pause = not pause
    
def toggleRunning():
    global running
    running = not running

def initFile(filePath):
    with open(filePath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['ir1','ir2'])

# main program
def readSerial(filePath, port, baudrate):
    ser = serial.Serial(port, baudrate)
    initFile(filePath)   
    print("Enter p to run/pause, q to quit:")
    keyboard.add_hotkey('p', togglePause)
    keyboard.add_hotkey('q', toggleRunning)
    while running:
        if not pause:
            # read data from uart
            serial_data = ser.readline().decode('utf8').strip()
            print(serial_data)
            # writeCSV
            data_arr = serial_data.split(',')
            with open(filePath, 'a', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(data_arr)
            #time.sleep()
    if keyboard.is_pressed('q'):
        print("Close!!!")
        ser.close()
   
if __name__ == "__main__":
    readSerial(filePath,"COM3",115200)
 

