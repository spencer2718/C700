/*
**	fft.c -- Fast Fourier Transform (FFT)
**	Decimation-in-time FFT (Cooley-Tukey algorithm)
**
**	Modified from the program published in "Encyclopedia of Latest Algorithms
**	in C" (by Haruhiko Okumura).
**
**	Revised by MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> (Sep 15, 2003)
**
**	[TAB4]
*/

#include <stdlib.h>
#include <math.h>

#include "fft_czt.h"

#undef PI
#define PI	3.1415926535897932384626433832795L	/* Pi */


/*
**	Create trigonometric function table.
*/
static void make_sintbl(int n, REAL sintbl[])
{
	/* If precision is not critical, double may be acceptable */
	long double r, d = (2 * PI) / n;
	int i, n2 = n / 2, n4 = n / 4, n8 = n / 8;

	for (i = 0; i <= n8; i++) {
		r = d * i;
		sintbl[i]      = sin(r);			/* sin(2*PI*i/n) */
		sintbl[n4 - i] = cos(r);
	}
	for (i = 0; i < n4; i++) {
		sintbl[n2 - i] =  sintbl[i];
		sintbl[i + n2] = -sintbl[i];
	}
}


/*
**	Create bit-reversal table.
*/
static void make_bitrev(int n, int bitrev[])
{
	int i = 0, j = 0, k;
	int n2 = n / 2;

	for (;;) {
		bitrev[i] = j;
		if (++i >= n) break;
		k = n2;
		while (k <= j) { j -= k; k /= 2; }
		j += k;
	}
}


/*
**	Create lookup table data for sample count n in the FFT computation structure.
**
**	fftp	= Pointer to FFT computation structure
**	n		= Number of sample points (must be a power of 2, >= 4)
**	return	= 0: success, 1: invalid n, 2: out of memory
*/
int fft_init(fft_struct *fftp, int n)
{
	int i;

	for (i = 4; i < n; i *= 2) ;
	if (i != n) return 1;

	fftp->samples = n;
	fftp->sintbl  = (REAL*)malloc((n / 4 * 3) * sizeof(REAL));
	fftp->bitrev  = (int*)malloc(n * sizeof(int));
	if (fftp->sintbl == NULL || fftp->bitrev == NULL) {
		fft_end(fftp);
		return 2;
	}
	make_sintbl(n, fftp->sintbl);
	make_bitrev(n, fftp->bitrev);

	return 0;
}


/*
**	Clear the lookup table data in the FFT computation structure and free its memory.
**
**	fftp	= Pointer to FFT computation structure
*/
void fft_end(fft_struct *fftp)
{
	if (fftp->sintbl != NULL) { free(fftp->sintbl); fftp->sintbl = NULL; }
	if (fftp->bitrev != NULL) { free(fftp->bitrev); fftp->bitrev = NULL; }

	fftp->samples = 0;
}


/*
**	Fast Fourier Transform (FFT) (Cooley-Tukey algorithm).
**	re[] is the real part, im[] is the imaginary part. Results overwrite re[] and im[].
**	If inv!=0 (=TRUE), perform inverse transform. fftp specifies the structure
**	containing computation data.
*/
void fft(fft_struct *fftp, int inv, REAL re[], REAL im[])
{
	REAL si, co, dr, di, t;
	int i, j, k, ik, h, d, k2, n, n4;

	n  = fftp->samples;
	n4 = n / 4;

	for (i = 0; i < n; i++) {		/* Bit reversal */
		j = fftp->bitrev[i];
		if (i < j) {
			t = re[i];  re[i] = re[j];  re[j] = t;
			t = im[i];  im[i] = im[j];  im[j] = t;
		}
	}
	for (k = 1; k < n; k = k2) {	/* Transform */
		h = 0;  k2 = k + k;  d = n / k2;
		for (j = 0; j < k; j++) {
			co = fftp->sintbl[h + n4];
			si = fftp->sintbl[h];
			if (inv) si = -si;
			for (i = j; i < n; i += k2) {
				ik = i + k;
				dr = si * im[ik] + co * re[ik];
				di = co * im[ik] - si * re[ik];
				re[ik] = re[i] - dr;  re[i] += dr;
				im[ik] = im[i] - di;  im[i] += di;
			}
			h += d;
		}
	}
	if (!inv) {						/* If not inverse transform, divide by n */
		t = 1.0 / n;			/* Multiply by reciprocal (division is slow) */
		for (i = 0; i < n; i++) {
			re[i] *= t;
			im[i] *= t;
		}
	}
}

