/*
 * algorithm.h
 *
 *  Created on: Jun 22, 2024
 *      Author: DELL
 */

#ifndef INC_ALGORITHM_H_
#define INC_ALGORITHM_H_

#include <stdint.h>

#define FS					100			// sampling frequency
#define TS					2			// sampling time
#define BUFFER_SIZE			TS*FS		// sampling time * sampling frequency
#define WINDOW_SIZE			0.25*FS
#define MAX_DEVIATION		10
#define AGE 				22
#define OFFSET				TS*FS
#define W1_PPG1				11
#define W1_PPG2				8
#define W2_PPG1				69
#define W2_PPG2				54
#define BETA_PPG1			0.1
#define BETA_PPG2			0.1

void filter(uint32_t signal[], float output[], uint8_t signalLen, float prev_input[], float prev_out[]);
//void pad_array(float *q, uint8_t n, float *q_padded, uint8_t pad_left, uint8_t pad_right);
//void moving_avg_and_dtr_sig(float signal_filt[], float out[], uint8_t signal_len, uint8_t w, uint8_t detrend_sig);
float calculateMean(float* data, int start, int end);
void moving_avg_and_dtr(float signal_filt[], float out[], uint8_t signal_len, uint8_t w, uint8_t dtr);
void peak_detection(float signal[], uint8_t signal_len, uint16_t peak_list[], uint8_t *num_peaks, uint8_t seg_count, uint8_t w1, uint8_t w2, float beta);
void hr_vo2_cal(uint16_t peak_list[], uint8_t num_peaks, uint8_t *hr_rest, uint8_t *vo2);

#endif /* INC_ALGORITHM_H_ */
