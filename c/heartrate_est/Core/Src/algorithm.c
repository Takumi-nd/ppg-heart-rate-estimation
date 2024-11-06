/*
 * algorithm.c
 *
 *  Created on: Jun 22, 2024
 *      Author: DELL
 */
#include <stdint.h>
#include <math.h>
#include "algorithm.h"

void filter(uint32_t signal[], float output[], uint8_t signal_len, float prev_input[], float prev_out[]){
	float b[5] = {0.01658193f, 0.0f, -0.03316386f, 0.0f, 0.01658193f};
	float a[5] = {1.0f, -3.58623981f, 4.8462898f, -2.93042722f, 0.67045791f};
	float sum;
	uint8_t i, j;

	for (i = 0; i < signal_len; i++) {
		sum = 0.0f;

		for (j = 4; j > 0; j--) {
			prev_input[j] = prev_input[j - 1];
		}
		prev_input[0] = (float)signal[i] * -1.0f;

		for (j = 0; j < 5; j++) {
			sum += b[j] * prev_input[j];
		}

		for (j = 1; j < 5; j++) {
			sum -= a[j] * prev_out[j - 1];
		}

		sum /= a[0];
		output[i] = sum;

		for (j = 4; j > 0; j--) {
			prev_out[j] = prev_out[j - 1];
		}
		prev_out[0] = output[i];
	}
}

//void pad_array(float *q, uint8_t n, float *q_padded, uint8_t pad_left, uint8_t pad_right) {
//	uint8_t i = 0;
//    for (i = 0; i < pad_left; i++) {
//        q_padded[i] = q[0];
//    }
//
//    for (i = 0; i < n; i++) {
//        q_padded[pad_left + i] = q[i];
//    }
//
//    for (i = 0; i < pad_right; i++) {
//        q_padded[pad_left + n + i] = q[n - 1];
//    }
//}
//
//void moving_avg_and_dtr_sig(float signal_filt[], float out[], uint8_t signal_len, uint8_t w, uint8_t detrend_sig){
//	uint8_t padded_size = signal_len + w - 1;
//	uint8_t pad_left = w / 2;
//	uint8_t pad_right = w - 1 - pad_left;
//
//	float q_padded[padded_size];
//
//	pad_array(signal_filt, signal_len, q_padded, pad_left, pad_right);
//
//	uint8_t i, j;
//
//	for (i = 0; i < signal_len; i++) {
//		float sum = 0.0f;
//		for (j = 0; j < w; j++) {
//			sum += q_padded[i + j];
//		}
//		sum = sum / w;
//		if (detrend_sig == 0)
//			out[i] = sum;
//		else
//			out[i] = signal_filt[i] - sum;
//	}
//}

void moving_avg_and_dtr(float signal_filt[], float out[], uint8_t signal_len, uint8_t w, uint8_t dtr) {
    uint8_t half_window = (w - 1) / 2;

    for (int i = 0; i < signal_len; i++) {
        uint8_t start_index = (i - half_window) > 0 ? (i - half_window) : 0;
        uint8_t end_index = (i + half_window) < signal_len ? (i + half_window) : signal_len - 1;

        uint8_t count = 0;
        float sum = 0;
        for (uint8_t i = start_index; i<=end_index; i++ ) {
            sum += signal_filt[i];
            count++;
        }
        sum /= (float)count;

        if (dtr != 0)
        	out[i] = signal_filt[i] - sum;
        else
        	out[i] = sum;
    }
}

void peak_detection(float signal[], uint8_t signal_len, uint16_t peak_list[], uint8_t *num_peaks, uint8_t seg_count, uint8_t w1, uint8_t w2, float beta) {
	float Z[signal_len];
	float y[signal_len];
	float ma_peak[signal_len];
	float ma_beat[signal_len];
	uint8_t block_of_interest[signal_len];
	uint8_t BOI_idx[signal_len];
	uint8_t BOI_width_idx[signal_len];
	uint8_t i, j;

	for (i = 0; i < signal_len; i++) {
		Z[i] = (signal[i] > 0) ? signal[i] : 0.0f;
		y[i] = Z[i] * Z[i];
	}

	moving_avg_and_dtr(y, ma_peak, signal_len, w1, 0);
	moving_avg_and_dtr(y, ma_beat, signal_len, w2, 0);

	float z_mean = 0.0f;
	for (i = 0; i < signal_len; i++) {
		z_mean += y[i];
	}
	z_mean /= (float)signal_len;

	float alpha = beta * z_mean;

	float thr1[signal_len];
	for (i = 0; i < signal_len; i++) {
		thr1[i] = ma_beat[i] + alpha;
	}

	for (i = 0; i < signal_len; i++) {
		block_of_interest[i] = (ma_peak[i] > thr1[i]) ? 1 : 0;
	}

	uint8_t thr2 = w1;
	uint8_t num_boi = 0;

    for (i = 0; i < signal_len; i++) {
        if (block_of_interest[i] > 0) {
            BOI_idx[num_boi++] = i;
        }
    }

    uint8_t BOI_count = 0;
    for (i = 0; i < num_boi - 1; i++) {
        if (BOI_idx[i+1] - BOI_idx[i] > 1) {
            BOI_width_idx[BOI_count++] = i;
        }
    }
    BOI_width_idx[BOI_count++] = num_boi - 1;

    for (i = 0; i < BOI_count; i++) {
        uint8_t left_idx = (i == 0) ? 0 : BOI_width_idx[i - 1] + 1;
        uint8_t right_idx = BOI_width_idx[i];
        uint8_t BOI_width = right_idx - left_idx;

        if (BOI_width >= thr2) {
            float max_val = 0.0;
            int peak_idx = left_idx;
            for (j = BOI_idx[left_idx]; j <= BOI_idx[right_idx]; j++) {
                if (y[j] > max_val) {
                    max_val = y[j];
                    peak_idx = j;
                }
            }
            peak_list[(*num_peaks)++] = peak_idx + (seg_count * OFFSET);
        }
    }
}

//void hr_vo2_cal(uint16_t peak_list[], uint8_t residual_ppi[], uint8_t num_peaks, uint8_t *residual_ppi_len, uint8_t *hr_rest, uint8_t *vo2) {
//    if (num_peaks < 2) {
//        return;
//    }
//    *hr_rest = 0;
//    *vo2 = 0;
//    uint8_t N = 10;
//    uint8_t ppi_list[10];
//    uint8_t ppi_len = 0;
//    float di[10];
//    float mean_ppi = 0.0f;
//    float stddev = 0.0f;
//    float Erd = 0.0f;
//    uint16_t sum_ppi = 0;
//    uint16_t valid_sum = 0;
//    uint8_t valid_ppi = 0;
//    uint8_t i;
//
//    // load residual ppi
//    for(i = 0; i < *residual_ppi_len; i++) {
//    	ppi_list[i] = residual_ppi[i];
//    	sum_ppi += ppi_list[i];
//    	residual_ppi[i] = 0;
//    }
//    ppi_len = *residual_ppi_len;
//    *residual_ppi_len = 0;
//
//    // store new ppi
//    for (i = 1; i < num_peaks; i++) {
//    	ppi_len = ppi_len + 1;
//
//    	if (ppi_len >= 10) {
//    		residual_ppi[(*residual_ppi_len)++] = peak_list[i] - peak_list[i - 1];
//    	} else {
//    		ppi_list[ppi_len] = peak_list[i] - peak_list[i - 1];
//    		sum_ppi += ppi_list[ppi_len];
//    	}
//    }
//
//    mean_ppi = (float)sum_ppi / N;
//
//    float sum_di = 0.0f;
//    for (i = 0; i < N; i++) {
//        di[i] = (float)ppi_list[i] - mean_ppi;
//        sum_di += di[i] * di[i];
//    }
//
//    stddev = sqrt(sum_di / (N - 1));
//    Erd = 3 * (stddev / sqrt(N));
//
//    int instance_hr = 0;
//    for (i = 0; i < N; i++) {
//        if (fabs(di[i]) < Erd && fabs(di[i]) < MAX_DEVIATION) {
////        	valid_sum += ppi_list[i];
//        	instance_hr += (uint8_t)(((FS * 60) / ppi_list[i]));
//            valid_ppi++;
//        }
//    }
//
//    if (valid_ppi == 0) {
//        return;
//    }
//
//    valid_sum = valid_sum / valid_ppi;
////    *hr_rest = (uint8_t)(((FS * 60) / valid_sum));
//    *hr_rest = instance_hr / valid_ppi;
//    *vo2 = (uint8_t)(15 * (208 - (0.7 * AGE)) / *hr_rest);
//
//    for (i = 0; i < num_peaks; i++) {
//    	peak_list[i] = 0;
//    }
//}

void hr_vo2_cal(uint16_t peak_list[], uint8_t num_peaks, uint8_t *hr_rest, uint8_t *vo2) {
    if (num_peaks <= 1) {
        return;
    }
    *hr_rest = 0;
    *vo2 = 0;
    uint8_t N = num_peaks - 1;
    uint8_t ppi_list[N];
    float di[N];
    float mean_ppi = 0.0f;
    float stddev = 0.0f;
    float Erd = 0.0f;
    uint16_t sum_ppi = 0;
    // uint8_t valid_sum = 0;
    uint8_t valid_ppi = 0;
    uint8_t i;

    for (i = 1; i < num_peaks; i++) {
        ppi_list[i - 1] = peak_list[i] - peak_list[i - 1];
        sum_ppi += ppi_list[i - 1];
    }

    mean_ppi = (float)sum_ppi / N;

    float sum_di = 0.0f;
    for (i = 0; i < N; i++) {
        di[i] = (float)ppi_list[i] - mean_ppi;
        sum_di += di[i] * di[i];
    }

    stddev = sqrt(sum_di / (N - 1));
    Erd = 3 * (stddev / sqrt(N));

    int instance_hr = 0;
    for (i = 0; i < N; i++) {
        if (fabs(di[i]) < Erd && fabs(di[i]) < MAX_DEVIATION) {
        	instance_hr += (uint8_t)(((FS * 60) / ppi_list[i]));
            valid_ppi++;
        }
    }

    if (valid_ppi == 0) {
        return;
    }

    *hr_rest = instance_hr / valid_ppi;
    *vo2 = (uint8_t)(15 * (208 - (0.7 *AGE)) / *hr_rest);

    for (i = 0; i < num_peaks; i++) {
    	peak_list[i] = 0;
    }
}






