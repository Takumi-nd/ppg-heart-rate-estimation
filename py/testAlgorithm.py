from scipy.signal import butter, lfilter, freqz, cheby2, correlate
from scipy.stats import skew
from scipy import signal
import sys
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
np.set_printoptions(threshold=sys.maxsize)

def cheby2_bandpass(lowcut, highcut, fs, order):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = cheby2(order, 20, [low, high], btype='band')
    return b,a  

def cheby2_bandpass_filter(data, lowcut, highcut, fs, order):
    b,a = cheby2_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

def butter_bandpass(lowcut, highcut, fs, order):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b,a

def bw_bandpass_filter(data, lowcut, highcut, fs, order):
    b,a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

def mov_avg_and_detrend(data, w, dtr):
    size = len(data)
    result = np.zeros(size)
    half_window = (w - 1) // 2
    
    for i in range(size):
        start_index = max(i - half_window, 0)
        end_index = min(i + half_window + 1, size)
        
        mean = np.mean(data[start_index:end_index])
        
        if dtr != 0:
            result[i] = data[i] - mean
        else:
            result[i] = mean
    
    return result

def find_max(signal, start, end):
    return np.argmax(signal[start:end+1]) + start

def find_min(signal, start, end):
    return np.argmin(signal[start:end+1]) + start

def detect_peak_trough_adaptive_threshold(signal, moving_avg):
    signal_size = len(signal)
    peak = []
    trough = []

    start_idx = -1

    for i in range(signal_size - 1):
        if moving_avg[i] > signal[i] and moving_avg[i + 1] < signal[i + 1]:
            start_idx = i

        if moving_avg[i] < signal[i] and moving_avg[i + 1] > signal[i + 1] and start_idx != -1:
            end_idx = i

            if start_idx < end_idx:
                peak_idx = find_max(signal, start_idx, end_idx)
                peak.append(peak_idx)

                if len(peak) > 1:
                    trough_idx = find_min(signal, peak[-2], peak[-1])
                    trough.append(trough_idx)

                start_idx = -1

    if start_idx != -1:
        end_idx = signal_size - 1
        peak_idx = find_max(signal, start_idx, end_idx)
        peak.append(peak_idx)

        if len(peak) > 1:
            trough_idx = find_min(signal, peak[-2], peak[-1])
            trough.append(trough_idx)

    return peak, trough

def detect_peak_trough_default_scipy(s, ma):
    peak_finalist = signal.find_peaks(s, height=ma)[0]
    trough_finalist = []
    for idx in range(len(peak_finalist) - 1):
        region = s[peak_finalist[idx]:peak_finalist[idx + 1]]
        trough_finalist.append(np.argmin(region) + peak_finalist[idx])

    return peak_finalist, trough_finalist

def get_moving_average(q, w):
    q_padded = np.pad(q, (w // 2, w - 1 - w // 2), mode='edge')
    convole = np.convolve(q_padded, np.ones(w) / w, 'valid')
    return convole

def detect_peak_trough_moving_average_threshold(s):
    peak_finalist = []
    through_finalist = []

    Z = np.array([np.max([0, z]) for z in s])
    y = Z**2

    w1 = 8
    w2 = 54
    ma_peak = mov_avg_and_detrend(y, w1, 0)
    ma_beat = mov_avg_and_detrend(y, w2, 0)
    z_mean = np.mean(y)
    beta = 0.1
    alpha = beta * z_mean + ma_beat
    thr1 = ma_beat + alpha
    block_of_interest = np.zeros(len(thr1))
    for i in range(len(ma_peak)):
        if ma_peak[i] > thr1[i]:
            block_of_interest[i] = 0.1

    thr2 = w1
    BOI_idx = np.where(block_of_interest > 0)[0]
    BOI_diff = np.diff(BOI_idx)
    BOI_width_idx = np.where(BOI_diff > 1)[0]
    BOI_width_idx = np.append(BOI_width_idx, len(BOI_idx) - 1)

    for i, val in enumerate(np.arange(len(BOI_width_idx))):
        if i == 0:
            BOI_width = BOI_width_idx[i]
        else:
            BOI_width = BOI_width_idx[i] - BOI_width_idx[i - 1]

        if BOI_width >= thr2:
            if i == 0:
                left_idx = 0
            else:
                left_idx = BOI_width_idx[i - 1] + 1
                         
            right_idx = BOI_width_idx[i]
            region = y[BOI_idx[left_idx] : BOI_idx[right_idx] + 1]
            peak_finalist.append(BOI_idx[left_idx] + np.argmax(region))

    return peak_finalist
    

def run():
    # csv raw data
    data = pd.read_csv("records/raw_data.csv")
    
    ir1 = data["ir1"].to_list()
    ir2 = data["ir2"].to_list()
    
    ir1filt = data["ir1filt"].to_list()
    ir2filt = data["ir2filt"].to_list()

    
    for i in range(len(ir1)):
        ir1[i] *= -1
        ir2[i] *= -1

     
    # Sample rate and desired cutoff frequencies (in Hz).
    fs = 100
    lowcut = 0.5
    highcut = 5.0
    ns = len(data) 
    win75= int((0.75 * fs) * 2 + 1)
    win55= int((0.55 * fs) * 2 + 1)
    win25= int((0.25 * fs) * 2 + 1)
    time = np.arange(ns)/fs
 
    y1 = bw_bandpass_filter(ir1, lowcut, highcut, fs, order=2)
    y2 = bw_bandpass_filter(ir2, lowcut, highcut, fs, order=2)

    ir1_detr = mov_avg_and_detrend(y1[200:], win25, 1)
    ir2_detr = mov_avg_and_detrend(y2[200:], win25, 1)
    
    # numpy_correlation = np.corrcoef(y1, ir1filt)[0, 1]
    # print('Signal Correlation:', numpy_correlation)
    
    ma1 = mov_avg_and_detrend(ir1_detr, win75, 0)
    ma2 = mov_avg_and_detrend(ir2_detr, win75, 0)
    
    peak_t1 = detect_peak_trough_moving_average_threshold(ir1_detr)
    peak_t2 = detect_peak_trough_moving_average_threshold(ir2_detr)
    print(peak_t2)
    
    # impulse response
    b,a = butter_bandpass(lowcut, highcut, fs, order=2)
    print(a)
    print(b)
    w, h = freqz(b,a)
    
    # map peak_list với tín hiệu
    peak_t1_values = [ir1_detr[idx] for idx in peak_t1]
    peak_t2_values = [ir2_detr[idx] for idx in peak_t2]
    
    # vẽ tín hiệu gốc
    plt.figure()
    plt.plot(ir1,linewidth = '1.5', label = "PPG1")
    plt.plot(ir2,linewidth = '1.5', label = "PPG2")
    plt.title("raw PPG")
    plt.grid()
    plt.legend(loc="upper right")
    
    # vẽ y tín hiệu sau lọc
    plt.figure()
    plt.plot(y1[200:], linewidth = '2.0', label = "filter_software")
    plt.plot(ir1filt[200:], linestyle = '--', label = "filter_C")
    plt.title("tín hiệu sau lọc ppg1")
    plt.grid()
    plt.legend(loc="upper right")
    
    plt.figure()
    plt.plot(y2[200:],linewidth = '1.5', label = "PPG2")
    plt.plot(ir2filt[200:], linestyle = '--', label = "filter_C")
    plt.title("tín hiệu sau lọc ppg2")
    plt.grid()
    plt.legend(loc="upper right")
    
    # vẽ tín hiệu detrend và đỉnh
    plt.figure()
    plt.plot(ir1_detr,linewidth = '1.5', label = "PPG1")
    plt.scatter(peak_t1, peak_t1_values, color = 'b')
    plt.title("baseline wander ppg1")
    plt.grid()
    plt.legend(loc="upper right")
    
    plt.figure()
    plt.plot(ir2_detr,linewidth = '1.5', label = "PPG2")
    plt.scatter(peak_t2, peak_t2_values, color = 'g')
    plt.title("baseline wander ppg2")
    plt.grid()
    plt.legend(loc="upper right")

    # vẽ đáp ứng tần số bộ lọc
    plt.figure()
    plt.plot((w/np.pi)*(fs/2), abs(h), linewidth=2)
    plt.grid()
    plt.ylabel("Biên độ")
    plt.xlabel("Tần số (rad/s)")
    plt.title("đáp ứng tần số bộ lọc")


    plt.show()

if __name__ == "__main__":    
    run()
