/*
**	fft_czt.h -- header file for fft.c and czt.c
**
**	Public domain by MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> (Sep 15, 2003)
**
**	[TAB4]
*/

#ifndef FFT_CZT_H
#define FFT_CZT_H

	/* Definition of real data type REAL (float or double) */
	/* If changing this type to long double, the trigonometric functions sin()/cos()
	 * used in czt.c/fft.c must be changed to their long double versions
	 * (sinl/cosl on C99-compatible compilers) */
typedef double REAL;

	/* FFT computation lookup table structure */
typedef struct {
	int  samples;		/* Number of sample points (must be a power of 2, >= 4) */
	int  *bitrev;		/* Bit-reversal table - element count is (samples) */
	REAL *sintbl;		/* Trigonometric function table - element count is (samples*3/4) */
} fft_struct;

	/* CZT computation lookup table structure */
typedef struct {
	fft_struct fft;		/* Sub-contracted FFT computation */
	int  samples;		/* Number of sample points */
	int  samples_out;	/* Number of output sample points */
	int  samples_ex;	/* Smallest power of 2 such that
						 * (samples + samples_out) <= samples_ex */
	REAL *wr, *wi;		/* Weight data - element count is (samples) */
	REAL *vr, *vi;		/* Impulse response data - element count is (samples_ex) */
	REAL *tr, *ti;		/* Working area - element count is (samples_ex) */
} czt_struct;

	/* fft.c */
int fft_init(fft_struct *fftp, int n);
void fft_end(fft_struct *fftp);
void fft(fft_struct *fftp, int inv, REAL re[], REAL im[]);

	/* czt.c */
int czt_init(czt_struct *cztp, int n, int no);
void czt_end(czt_struct *cztp);
void czt(czt_struct *cztp, int inv, REAL re[], REAL im[]);

int estimatebasefreq(short *src, int length);

#endif	/* FFT_CZT_H */
