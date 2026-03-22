/*
**	czt.c -- Chirp z-Transform (CZT)
**	A fast DFT algorithm capable of computing with arbitrary data lengths
**
**	Based on the explanation in "Transistor Gijutsu" magazine, Feb 1993 issue, pp.363-368.
**
**	Public domain by MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> (Sep 15, 2003)
**
**	[TAB4]
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "fft_czt.h"

#undef PI
#define PI	3.1415926535897932384626433832795L	/* Pi */


/*
**	Create weight data and impulse response data
*/
static void make_cztdata(int n, int no, int nx, REAL wr[], REAL wi[],
                         REAL vr[], REAL vi[])
{
	/* If precision is not critical, double may be acceptable */
	long double r, d = PI / n;
	int i, j, n2 = n * 2;

	for (i = j = 0; i < n; i++, j = (j + (2 * i - 1)) % n2) {
		r = d * j;
		wr[i] =  cos(r);					/*  cos(i*i*PI/n) */
		wi[i] = -sin(r);					/* -sin(i*i*PI/n) */
	}

	vr[0] = nx;					/* nx: correction for FFT forward transform scaling */
	vi[0] = 0;
	for (i = 1; i < n; i++) {
		vr[i] = vr[nx - i] =  nx * wr[i];	/* nx * cos(i*i*PI/n) */
		vi[i] = vi[nx - i] = -nx * wi[i];	/* nx * sin(i*i*PI/n) */
	}
	for (i = no, j = nx - n; i <= j; i++, j--) {
		vr[j] = vr[i] = 0;					/* Fill remaining extended part with 0 */
		vi[j] = vi[i] = 0;
	}
}


/*
**	Create lookup table data for sample count n and output count no
**	in the CZT computation structure.
**
**	cztp	= Pointer to CZT computation structure
**	n		= Number of sample points
**	no		= Number of output data points
**	return	= 0: success, 1: invalid n, 2: out of memory
*/
int czt_init(czt_struct *cztp, int n, int no)
{
	int i, nx;

	if (n <= 1) return 1;
	if (no <= 1 || n < no) no = n;

	nx = n + no;		/* Extend (n + no) to the next power of 2 (nx) */
	for (i = 1; i < nx; i *= 2) ;
	nx = i;

	if (fft_init(&cztp->fft, nx) != 0) return 2;

	cztp->samples     = n;
	cztp->samples_out = no;
	cztp->samples_ex  = nx;
	cztp->wr = (REAL*)malloc(2 * n  * sizeof(REAL));
	cztp->vr = (REAL*)malloc(2 * nx * sizeof(REAL));
	cztp->tr = (REAL*)malloc(2 * nx * sizeof(REAL));
	if (cztp->wr == NULL || cztp->vr == NULL || cztp->tr == NULL) {
		czt_end(cztp);
		return 2;
	}
	cztp->wi = cztp->wr + n;
	cztp->vi = cztp->vr + nx;
	cztp->ti = cztp->tr + nx;

	make_cztdata(n, no, nx, cztp->wr, cztp->wi, cztp->vr, cztp->vi);
	fft(&cztp->fft, 0, cztp->vr, cztp->vi);

	return 0;
}


/*
**	Clear the lookup table data in the CZT computation structure and free its memory.
**
**	cztp	= Pointer to CZT computation structure
*/
void czt_end(czt_struct *cztp)
{
	if (cztp->wr != NULL) { free(cztp->wr); cztp->wr = NULL; }
	if (cztp->vr != NULL) { free(cztp->vr); cztp->vr = NULL; }
	if (cztp->tr != NULL) { free(cztp->tr); cztp->tr = NULL; }

	cztp->samples     = 0;
	cztp->samples_out = 0;
	cztp->samples_ex  = 0;

	fft_end(&cztp->fft);
}


/*
**	Fast DFT using the CZT (Chirp z-Transform) algorithm.
**	re[] is the real part, im[] is the imaginary part. Results overwrite re[] and im[].
**	If inv!=0 (=TRUE), perform inverse transform. cztp specifies the structure
**	containing computation data.
*/
void czt(czt_struct *cztp, int inv, REAL re[], REAL im[])
{
	REAL xr, xi, yr, yi, t;
	int i, n, no, nx;

	n  = cztp->samples;
	no = cztp->samples_out;
	nx = cztp->samples_ex;

	for (i = 0; i < n; i++) {		/* Multiply input by weight data */
		yr = cztp->wr[i];
		yi = cztp->wi[i];
		if (inv) yi = -yi;
		cztp->tr[i] = re[i] * yr - im[i] * yi;
		cztp->ti[i] = im[i] * yr + re[i] * yi;
	}
	for (; i < nx; i++) {			/* Fill remaining extended part with 0 */
		cztp->tr[i] = 0;
		cztp->ti[i] = 0;
	}

	fft(&cztp->fft, 0, cztp->tr, cztp->ti);

	for (i = 0; i < nx; i++) {		/* Convolution operation */
		xr = cztp->tr[i];
		xi = cztp->ti[i];
		yr = cztp->vr[i];
		yi = cztp->vi[i];
		if (inv) yi = -yi;
		cztp->tr[i] = xr * yr - xi * yi;
		cztp->ti[i] = xi * yr + xr * yi;
	}

	fft(&cztp->fft, 1, cztp->tr, cztp->ti);

	for (i = 0; i < no; i++) {		/* Multiply output by weight data */
		yr = cztp->wr[i];
		yi = cztp->wi[i];
		if (inv) yi = -yi;
		re[i] = cztp->tr[i] * yr - cztp->ti[i] * yi;
		im[i] = cztp->ti[i] * yr + cztp->tr[i] * yi;
	}

	if (!inv) {						/* If not inverse transform, divide by n */
		t = 1.0 / n;			/* Multiply by reciprocal (division is slow) */
		for (i = 0; i < no; i++) {
			re[i] *= t;
			im[i] *= t;
		}
	}
}

// Estimate the fundamental frequency
int estimatebasefreq(short *src, int length)
{
	REAL	*real, *imag, *autoc;
	czt_struct cztd;
	int i, index=1, pitch, error, half=length/2;
	if (half > 530)
		half = 530;
	
	real = (REAL*)malloc(sizeof(REAL)*length);
	imag = (REAL*)malloc(sizeof(REAL)*length);
	autoc = (REAL*)malloc(sizeof(REAL)*length);
	
	for (i = 0; i < length; i++) {
		real[i] = src[i];
		imag[i] = 0;
	}
	error = czt_init(&cztd, length, length);
	if (error) {
		printf("error:%d\n",error);
	}
	czt(&cztd, 0, real, imag);
	
	// Convert to power spectrum
	for (i = 0; i < length; i++) {
		real[i] = real[i]*real[i] + imag[i]*imag[i];
		real[i] = pow(real[i], 1.0/3.0);
		imag[i] = 0;
	}
	czt(&cztd, 1, real, imag);		/* Inverse transform to compute autocorrelation */
	czt_end(&cztd);
	
	// Clip negative values
	for (i = 0; i < half; i++) {
		if (real[i] < 0.0)
			real[i] = 0.0;
		imag[i] = real[i];
	}
	for (i = 0; i < half; i++)
		if ((i % 2) == 0)
            real[i] -= imag[i/2];
		else
            real[i] -= ((imag[i/2] + imag[i/2 + 1]) / 2);
	// Clip negative values
	/*
	for (i = 0; i < half; i++) {
		if (real[i] < 0.0)
			real[i] = 0.0;
	}
	*/
	// Pitch estimation
	for (i = 1; i < half; i++) {
		if (real[i] > real[index]) {
			index = i;
		}
	}
	
	if ( index == 0) {
		index = 1;
	}
	pitch = length/index;
	
	free(real);
	free(imag);
	free(autoc);
	
	return pitch;
}

