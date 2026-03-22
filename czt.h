/*
**	fft_czt.h -- header file for fft.c and czt.c
**
**	Public domain by MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> (Sep 15, 2003)
**
**	[TAB4]
*/

#ifndef FFT_CZT_H
#define FFT_CZT_H

#include <Accelerate/Accelerate.h>

	/* Struct for holding lookup tables used in CZT computation */
typedef struct {
	FFTSetup		fftsetup;		/* For subordinate FFT computation */
	bool	no_czt;
	int		m;
	int		samples;		/* Number of sample points */
	int		samples_out;	/* Number of output sample points */
	int		samples_ex;	/* Smallest power of 2 such that
						 * (samples + samples_out) <= samples_ex */
	DSPSplitComplex w;		/* Weight data - element count is (samples) */
	DSPSplitComplex v;		/* Impulse response data - element count is (samples_ex) */
	DSPSplitComplex t;		/* Working buffer - element count is (samples_ex) */
} czt_struct;

	/* czt.c */
int czt_init(czt_struct *cztp, int n, int no);
void czt_end(czt_struct *cztp);
void czt(czt_struct *cztp, int inv, DSPSplitComplex *input);

int estimatebasefreq(short *src, int length);

#endif	/* FFT_CZT_H */
