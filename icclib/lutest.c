
/* 
 * International Color Consortium Format Library (icclib)
 * Lookup test code, and profile creation examples.
 *
 * Author:  Graeme W. Gill
 * Date:    00/6/18
 * Version: 2.03
 *
 * Copyright 1998 - 2002 Graeme W. Gill
 * Please refer to Licence.txt file for details.
 */

/* TTBD:
 *
 */

/*

	This file is intended to serve two purposes. One is to do some
	basic regression testing of the lookup function of the icc
	library, by creating profiles with known mapping characteristics,
	and then checking that lookup displays those characteristics.
	Given the huge possible number of combinations of color spaces,
	profile variations, number of input and output channels, intents etc.
	the tests done here are far from exaustive.

	The other purpose is as a very basic source code example of how
	to create various styles of ICC profiles from mapping information,
	since extracting this information from the source code can take some
	doing. The example code here is still not perfect, since
	it doesn't cover the finer points of how to handle various intents,
	gamut compression etc. Such things are really the domain of a
	CMS, and would be dependent on the exact nature of the
	profile representation that the ICC file is being created from.
	(See examples in the Argyll CMS for these sorts of details.)

    (Note XYZ scaling to 1.0, not 100.0, as per ICC)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <time.h>
#if defined(__IBMC__) && defined(_M_IX86)
#include <float.h>
#endif
#include "icc.h"

void error(char *fmt, ...), warning(char *fmt, ...);

/* Some debuging aids */
#define STOPONERROR	/* stop if errors are excessive */
#undef TESTLIN1		/* test linear in curves */
#undef TESTLIN2		/* test linear clut (fails with Lab) */
#undef TESTLIN3		/* test linear out curves */

/* These two assist the accuracy of our BToA Lut tests, using our simplistic test functions */
/* They probably shouldn't be used on any real profile. */
#define REVLUTSCALE1		/* Define this for pre-clut gamut bounding box scaling */
#define REVLUTSCALE2		/* Define this for post-clut gamut quantization scaling */

/*
 * We start with a mathematically defined transfer characteristic, that
 * we create profiles from, and then check that the lookup function
 * matches the mathematical characteristic we started with.
 *
 * Use a matrix style 3D to 3D XYZ transfer as the core, to be
 * compatible with a matrix and Lut style profile.
 */

/* - - - - - - - - - - - - - */
/* Some support functions */

/* Clip value to range 0.0 to 1.0 */
void clip(double inout[3]) {
	int i;
	for (i = 0; i < 3; i++) {
		if (inout[i] < 0.0)
			inout[i] = 0.0;
		else if (inout[i] > 1.0)
			inout[i] = 1.0;
	}
}

/* Protected power function */
double ppow(double num, double p) {
	if (num < 0.0)
		return -pow(-num, p);
	else
		return pow(num, p);
}

#define D50_X 0.9642
#define D50_Y 1.0000
#define D50_Z 0.8249

#define D50_BX ( 0.8951 * D50_X +  0.2664 * D50_Y + -0.1614 * D50_Z)
#define D50_BY (-0.7502 * D50_X +  1.7135 * D50_Y +  0.0367 * D50_Z)
#define D50_BZ ( 0.0389 * D50_X + -0.0685 * D50_Y +  1.0296 * D50_Z)

#define ABS_X 0.83
#define ABS_Y 0.95
#define ABS_Z 1.05

#define ABS_BX ( 0.8951 * ABS_X +  0.2664 * ABS_Y + -0.1614 * ABS_Z)
#define ABS_BY (-0.7502 * ABS_X +  1.7135 * ABS_Y +  0.0367 * ABS_Z)
#define ABS_BZ ( 0.0389 * ABS_X + -0.0685 * ABS_Y +  1.0296 * ABS_Z)

/* Convert from normalized to relative XYZ */
static void to_rel(double out[3], double in[3]) {
	/* Scale to relative white and black points */
	out[0] = D50_X * in[0];
    out[1] = D50_Y * in[1];
    out[2] = D50_Z * in[2];
}

/* Convert from relative to normalized XYZ */
static void from_rel(double out[3], double in[3]) {
	/* Remove white and black points scale */
	out[0] = in[0]/D50_X;
	out[1] = in[1]/D50_Y;
	out[2] = in[2]/D50_Z;
}

/* Convert from relative to absolute XYZ */
static void rel_to_abs(double out[3], double in[3]) {
	double tt[3];

	/* Multiply by bradford */
	tt[0] =  0.8951 * in[0] +  0.2664 * in[1] + -0.1614 * in[2];
	tt[1] = -0.7502 * in[0] +  1.7135 * in[1] +  0.0367 * in[2];
	tt[2] =  0.0389 * in[0] + -0.0685 * in[1] +  1.0296 * in[2];

	/* Scale from D50 to absolute white point */
	tt[0] = tt[0] * ABS_BX/D50_BX;
    tt[1] = tt[1] * ABS_BY/D50_BY;
    tt[2] = tt[2] * ABS_BZ/D50_BZ;

	/* Inverse bradford */
	out[0] =  0.986993 * tt[0] + -0.147054 * tt[1] + 0.159963 * tt[2];
	out[1] =  0.432305 * tt[0] + 0.518360  * tt[1] + 0.049291 * tt[2];
	out[2] = -0.008529 * tt[0] + 0.040043  * tt[1] + 0.968487 * tt[2];
}

/* Convert from normalized to absolute XYZ */
static void to_abs(double out[3], double in[3]) {

	to_rel(out, in);			/* Convert to relative */
	rel_to_abs(out, out);		/* Convert to absolute */

}

/* Convert from absolute to relative XYZ */
static void abs_to_rel(double out[3], double in[3]) {
	double tt[3];

	/* Multiply by bradford */
	tt[0] =  0.8951 * in[0] +  0.2664 * in[1] + -0.1614 * in[2];
	tt[1] = -0.7502 * in[0] +  1.7135 * in[1] +  0.0367 * in[2];
	tt[2] =  0.0389 * in[0] + -0.0685 * in[1] +  1.0296 * in[2];

	/* Scale from absolute white point to D50 */
	tt[0] = tt[0] * D50_BX/ABS_BX;
    tt[1] = tt[1] * D50_BY/ABS_BY;
    tt[2] = tt[2] * D50_BZ/ABS_BZ;

	/* Inverse bradford */
	out[0] =  0.986993 * tt[0] + -0.147054 * tt[1] + 0.159963 * tt[2];
	out[1] =  0.432305 * tt[0] +  0.518360 * tt[1] + 0.049291 * tt[2];
	out[2] = -0.008529 * tt[0] +  0.040043 * tt[1] + 0.968487 * tt[2];
}

/* Convert from absolute to normalized XYZ */
static void from_abs(double out[3], double in[3]) {
	
	abs_to_rel(out, in);		/* Convert to relative */
	from_rel(out, out);			/* Convert to normalised */
}

/* CIE XYZ to perceptual Lab with ICC D50 white point */
static void
XYZ2Lab(double *out, double *in) {
	double X = in[0], Y = in[1], Z = in[2];
	double x,y,z,fx,fy,fz;
	double L;

	x = X/D50_X;
	if (x > 0.008856451586)
		fx = pow(x,1.0/3.0);
	else
		fx = 7.787036979 * x + 16.0/116.0;

	y = Y/D50_Y;
	if (y > 0.008856451586) {
		fy = pow(y,1.0/3.0);
		L = 116.0 * fy - 16.0;
	} else {
		fy = 7.787036979 * y + 16.0/116.0;
		L = 903.2963058 * y;
	}

	z = Z/D50_Z;
	if (z > 0.008856451586)
		fz = pow(z,1.0/3.0);
	else
		fz = 7.787036979 * z + 16.0/116.0;

	out[0] = L;
	out[1] = 500.0 * (fx - fy);
	out[2] = 200.0 * (fy - fz);
}

/* Perceptual Lab with ICC D50 white point to CIE XYZ */
static void
Lab2XYZ(double *out, double *in) {
	double L = in[0], a = in[1], b = in[2];
	double x,y,z,fx,fy,fz;

	if (L > 8.0) {
		fy = (L + 16.0)/116.0;
		y = pow(fy,3.0);
	} else {
		y = L/903.2963058;
		fy = 7.787036979 * y + 16.0/116.0;
	}

	fx = a/500.0 + fy;
	if (fx > 24.0/116.0)
		x = pow(fx,3.0);
	else
		x = (fx - 16.0/116.0)/7.787036979;

	fz = fy - b/200.0;
	if (fz > 24.0/116.0)
		z = pow(fz,3.0);
	else
		z = (fz - 16.0/116.0)/7.787036979;

	out[0] = x * D50_X;
	out[1] = y * D50_Y;
	out[2] = z * D50_Z;
}

/* - - - - - - - - - - - - - */

/* Return maximum difference */
static double maxdiff(double in1[3], double in2[3]) {
	double tt, rv = 0.0;
	if ((tt = fabs(in1[0] - in2[0])) > rv)
		rv = tt;
	if ((tt = fabs(in1[1] - in2[1])) > rv)
		rv = tt;
	if ((tt = fabs(in1[2] - in2[2])) > rv)
		rv = tt;
	return rv;
}

/* Return absolute difference */
static double absdiff(double in1[3], double in2[3]) {
	double tt, rv = 0.0;
	tt = in1[0] - in2[0];
	rv += tt * tt;
	tt = in1[1] - in2[1];
	rv += tt * tt;
	tt = in1[2] - in2[2];
	rv += tt * tt;
	return sqrt(rv);
}

/* - - - - - - - - - - - - - - - - - */
/* Overall Monochrome XYZ device model is */
/* Gray -> GrayY -> XYZ */
/* Where GrayY is assumed to directly Scale Y */

/* Gray -> GrayY */
static double Gray_GrayY(double in) {
#ifdef TESTLIN1
	return in;
#else
	return ppow(in,1.6);
#endif
}

/* GrayY -> Gray */
static double GrayY_Gray(double in) {
#ifdef TESTLIN1
	return in;
#else
	return ppow(in,1.0/1.6);
#endif
}

/* Gray -> XYZ */
static void Gray_XYZ(double out[3], double in) {
	double temp[3];
	temp[0] = temp[1] = temp[2] = Gray_GrayY(in);

	/* Scale to relative white and black points */
	to_rel(out, temp);
}

/* XYZ -> Device gray space */
static double XYZ_Gray(double in[3]) {
	double temp[3];

	/* Remove relative white and black points scale */
	from_rel(temp, in);

	/* Do calculation just from Y value */
	return GrayY_Gray(temp[1]);
}

/* Gray -> XYZ absolute */
static void aGray_XYZ(double out[3], double in) {
	double temp[3];
	temp[0] = temp[1] = temp[2] = Gray_GrayY(in);

	/* Scale to absolute white and black points */
	to_abs(out, temp);

}

/* XYZ -> Gray absolute */
static double aXYZ_Gray(double in[3]) {
	double tt, temp[3];

	/* Remove absolute white and black points scale */
	abs_to_rel(temp, in);		/* Convert to relative */

	from_rel(temp, temp);			/* Convert to normalised */

	/* Do calculation just from Y value */
	tt = GrayY_Gray(temp[1]);

	return tt;
}

/* - - - - - - - - - - - */
/* Overall Monochrome Lab device model is */
/* Gray -> GrayL -> Lab */
/* Where GrayL is assumed to directly scale L */


/* Gray -> GrayL */
static double Gray_GrayL(double in) {
#ifdef TESTLIN1
	return in;				/* normalized L */
#else
	return  ppow(in,1.6);
#endif
}

/* GrayL -> Gray */
static double GrayL_Gray(double in) {
#ifdef TESTLIN1
	return in;
#else
	return ppow(in,1.0/1.6);	/* Y */
#endif
}

/* Gray -> Lab */
static void Gray_Lab(double out[3], double in) {
	double tt, wp[3];

	wp[0] = D50_X;
	wp[1] = D50_Y;
	wp[2] = D50_Z;
	XYZ2Lab(wp, wp);		/* Lab white point */

	tt = Gray_GrayL(in);	/* Raw L value */

	/* Scale to relative Lab white point */
	out[0] = wp[0] * tt;
	out[1] = wp[1] * tt;
	out[2] = wp[2] * tt;
}

/* Lab -> Gray */
static double Lab_Gray(double in[3]) {
	double tt, wp[3];

	wp[0] = D50_X;
	wp[1] = D50_Y;
	wp[2] = D50_Z;
	XYZ2Lab(wp, wp);		/* Lab white point */

	/* Scale from relative Lab white point */
	tt = in[0]/wp[0];

	return GrayL_Gray(tt);	/* Raw L value */

}

/* Gray -> Lab absolute */
static void aGray_Lab(double out[3], double in) {

	/* Generate relative Lab */
	Gray_Lab(out, in);

	Lab2XYZ(out, out);
	rel_to_abs(out, out);	
	XYZ2Lab(out, out);
}

/* Lab -> Gray  absolute */
static double aLab_Gray(double in[3]) {
	double tt[3];

	Lab2XYZ(tt, in);
	abs_to_rel(tt, tt);	
	XYZ2Lab(tt, tt);
	
	return Lab_Gray(tt);
}

/* - - - - - - - - - - - - - - - - - */
/* RGB -> XYZ test transfer curves */
/* Note that overall model is: */
/* RBG -> RGB' -> XYZ' -> XYZ */

/* Device space linearization */
/* RGB -> RGB' */
static void RGB_RGBp(void *cntx, double out[3], double in[3]) {
#ifdef TESTLIN1
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	out[0] = ppow(in[0],1.6);
	out[1] = ppow(in[1],1.5);
	out[2] = ppow(in[2],1.4);
#endif
}

/* RGB' -> RGB */
static void RGBp_RGB(void *cntx, double out[3], double in[3]) {
#ifdef TESTLIN1
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	out[0] = ppow(in[0],1.0/1.6);
	out[1] = ppow(in[1],1.0/1.5);
	out[2] = ppow(in[2],1.0/1.4);
#endif
}

#ifdef TESTLIN2
static double matrix[3][3] = {
	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 },
};
#else
static double matrix[3][3] = {
	{ 0.4361, 0.3851, 0.1431 },
	{ 0.2225, 0.7169, 0.0606 },
	{ 0.0139, 0.0971, 0.7141 },
};
#endif

/* 3x3 matrix conversion */
/* RGB' -> XYZ' */
static void RGBp_XYZp(void *cntx, double out[3], double in[3]) {
	double o0,o1,o2;

#ifdef TESTLIN2
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	o0 = matrix[0][0] * in[0] + matrix[0][1] * in[1] + matrix[0][2] * in[2];
	o1 = matrix[1][0] * in[0] + matrix[1][1] * in[1] + matrix[1][2] * in[2];
	o2 = matrix[2][0] * in[0] + matrix[2][1] * in[1] + matrix[2][2] * in[2];

	out[0] = o0;
	out[1] = o1;
	out[2] = o2;
#endif
}

/* XYZ' -> RGB' */
static void XYZp_RGBp(void *cntx, double out[3], double in[3]) {
	double o0,o1,o2;

#ifdef TESTLIN2
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	o0 =  3.13360257102309000 * in[0] + -1.61682140135654430 * in[1] + -0.49074240441282400 * in[2];
	o1 = -0.97865031588250000 * in[0] +  1.91606100412532800 * in[1] +  0.03351290204844009 * in[2];
	o2 =  0.07207655781398956 * in[0] + -0.22906554547222160 * in[1] +  1.40535949675456500 * in[2];

	out[0] = o0;
	out[1] = o1;
	out[2] = o2;
#endif
}

/* Output linearization curves */
/* XYZ' -> XYZ */
static void XYZp_XYZ(void *cntx, double out[3], double in[3]) {
#ifdef TESTLIN3
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	out[0] = ppow(in[0],0.9);
	out[1] = ppow(in[1],0.8);
	out[2] = ppow(in[2],1.1);
#endif
}

/* XYZ -> XYZ' */
static void XYZ_XYZp(void *cntx, double out[3], double in[3]) {
#ifdef TESTLIN3
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	out[0] = ppow(in[0],1.0/0.9);
	out[1] = ppow(in[1],1.0/0.8);
	out[2] = ppow(in[2],1.0/1.1);
#endif
}

/* RGB -> XYZ' */
static void RGB_XYZp(void *cntx, double out[3], double in[3]) {
	RGB_RGBp(cntx, out, in);
	RGBp_XYZp(cntx, out, out);
}

/* RGB -> XYZ', absolute (for matrix profile test) */
static void aRGB_XYZp(void *cntx, double out[3], double in[3]) {
	RGB_RGBp(cntx, out, in);
	RGBp_XYZp(cntx, out, out);
	from_rel(out, out);
	to_abs(out, out);
}

/* RGB -> XYZ */
static void RGB_XYZ(void *cntx, double out[3], double in[3]) {
	RGB_RGBp(cntx, out, in);
	RGBp_XYZp(cntx, out, out);
	XYZp_XYZ(cntx, out, out);
}

/* XYZ -> RGB */
static void XYZ_RGB(void *cntx, double out[3], double in[3]) {
	XYZ_XYZp(cntx, out, in);
	XYZp_RGBp(cntx, out, out);
	RGBp_RGB(cntx, out, out);
}

/* RGB -> XYZ, absolute */
static void aRGB_XYZ(void *cntx, double out[3], double in[3]) {
	RGB_XYZ(cntx, out, in);
	from_rel(out, out);
	to_abs(out, out);
}

/* XYZ -> RGB, absolute */
static void aXYZ_RGB(void *cntx, double out[3], double in[3]) {
	from_abs(out, in);
	to_rel(out, out);
	XYZ_RGB(cntx, out, out);
}

/* XYZ -> RGB, gamut constrained */
static void cXYZ_RGB(void *cntx, double out[3], double in[3]) {
	XYZ_RGB(cntx, out, in);
	clip(out);
}

/* XYZ' -> distance to gamut boundary */
static void XYZp_BDIST(void *cntx, double out[1], double in[3]) {
	double gdst;				/* Gamut error */
	int m, mini, outg;
	double tt, mind;
	double pcs[3];				/* PCS value of input */
	double dev[3];				/* Device value */
	double pgb[3];				/* PCS gamut boundary point */
	double dgb[3];				/* device gamut boundary point */

	/* Do XYZ' -> XYZ */
	XYZp_XYZ(cntx, pcs, in);

	/* Do XYZ -> RGB transform */
	XYZ_RGB(NULL, dev, pcs);

	/* Compute nearest point on gamut boundary, */
	/* and whether it is in or out of gamut. */
	/* This should be nearest in PCS space, but */
	/* we'll cheat and find the nearest point in */
	/* device space, and then compute the distance in PCS space. */

	for (m = 0; m < 3; m++)
		dgb[m] = dev[m];

	for (mind = 10000.0, outg = 0, m = 0; m < 3; m++) {
		if (dev[m] < 0.0) {		/* Clip any coordinates outside device limits */
			dgb[m] = 0.0;
			outg = 1;			/* Out of gamut */
		} else if (dev[m] > 1.0) {
			dgb[m] = 1.0;
			outg = 1;			/* Out of gamut */
		} else {				/* Note closest cood to boundary if within limits */
			if (dev[m] < 0.5)
				tt = 0.5 - dev[m];
			else 	/* >= 0.5 */
				tt = dev[m] - 0.5;
			if (tt < mind) {
				mind = tt;
				mini = m;
			}
		}
	}
	if (!outg) {				/* If point is within gamut, set to closest point */
		if (dev[mini] < 0.5)
			dgb[mini] = 0.0;
		else
			dgb[mini] = 1.0;
	}
	
	/* Do RGB -> XYZ transform on nearest gamut boundary point */
	RGB_XYZ(NULL, pgb, dgb);

	/* Distance to nearest gamut point in PCS (XYZ) space */
	gdst = absdiff(pcs, pgb);
	if (!outg)	/* If within gamut */
		gdst = -gdst;

	/* Distance in PCS space will be roughly -0.866 -> 0.866 */
	/* Convert so that 0.5 is on boundary, and then clip. */
	gdst += 0.5;
	if (gdst < 0.0)
		gdst = 0.0;
	else if (gdst > 1.0)
		gdst = 1.0;

	out[0] = gdst;
}

/* The output table is usually a special for the gamut table, returning */
/* a value of 0 for all inputs <= 0.5, and then outputing between */
/* 0.0 and 1.0 for the input range 0.5 to 1.0. This is so a graduated */
/* "gamut boundary distance" number from the multi-d lut can be */
/* translated into the ICC "0.0 if in gamut, > 0.0 if not" number. */
static void BDIST_GAMMUT(void *cntx, double out[1], double in[1]) {
	double iv, ov;
	iv = in[0];
	if (iv <= 0.5)
		ov = 0.0;
	else
		ov = (iv - 0.5) * 2.0;
	out[0] = ov;
}

/* - - - - - - - - - - - - - */
/* Lab versions for Lut profile, built on top of XYZ model */
/* The overall model is: */
/* RBG -> RGB' -> Lab' -> Lab */

/* 3x3 matrix conversion */
/* RGB' -> Lab' */
static void RGBp_Labp(void *cntx, double out[3], double in[3]) {
	RGBp_XYZp(cntx, out, in);
	XYZ2Lab(out, out);
}

/* Lab' -> RGB' */
static void Labp_RGBp(void *cntx, double out[3], double in[3]) {
	Lab2XYZ(out, in);
	XYZp_RGBp(cntx, out, out);
}

/* Lab' -> Lab */
/* (We are using linear) */
static void Labp_Lab(void *cntx, double out[3], double in[3]) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/* Lab -> Lab' */
static void Lab_Labp(void *cntx, double out[3], double in[3]) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/* RGB -> Lab */
static void RGB_Lab(void *cntx, double out[3], double in[3]) {
	RGB_RGBp(cntx, out, in);
	RGBp_Labp(cntx, out, out);
	Labp_Lab(cntx, out, out);
}

/* Lab -> RGB */
static void Lab_RGB(void *cntx, double out[3], double in[3]) {
	Lab_Labp(cntx, out, in);
	Labp_RGBp(cntx, out, out);
	RGBp_RGB(cntx, out, out);
}

/* RGB -> Lab, absolute */
static void aRGB_Lab(void *cntx, double out[3], double in[3]) {
	RGB_Lab(cntx, out, in);
	Lab2XYZ(out, out);
	from_rel(out, out);
	to_abs(out, out);
	XYZ2Lab(out, out);
}

/* Lab -> RGB, absolute */
static void aLab_RGB(void *cntx, double out[3], double in[3]) {
	Lab2XYZ(out, in);
	from_abs(out, out);
	to_rel(out, out);
	XYZ2Lab(out, out);
	Lab_RGB(cntx, out, out);
}

/* Lab -> RGB, gamut constrained */
static void cLab_RGB(void *cntx, double out[3], double in[3]) {
	Lab_RGB(cntx, out, in);
	clip(out);
}

/* Lab' -> distance to gamut boundary */
static void Labp_BDIST(void *cntx, double out[1], double in[3]) {
	double gdst;				/* Gamut error */
	int m, mini, outg;
	double tt, mind;
	double pcs[3];				/* PCS value of input */
	double dev[3];				/* Device value */
	double pgb[3];				/* PCS gamut boundary point */
	double dgb[3];				/* device gamut boundary point */

	/* Do Lab' -> Lab */
	Labp_Lab(cntx, pcs, in);

	/* Do Lab -> RGB transform */
	Lab_RGB(cntx, dev, pcs);

	/* Compute nearest point on gamut boundary, */
	/* and whether it is in or out of gamut. */
	/* This should be nearest in PCS space, but */
	/* we'll cheat and find the nearest point in */
	/* device space, and then compute the distance in PCS space. */

	for (m = 0; m < 3; m++)
		dgb[m] = dev[m];

	for (mind = 10000.0, outg = 0, m = 0; m < 3; m++) {
		if (dev[m] < 0.0) {		/* Clip any coordinates outside device limits */
			dgb[m] = 0.0;
			outg = 1;			/* Out of gamut */
		} else if (dev[m] > 1.0) {
			dgb[m] = 1.0;
			outg = 1;			/* Out of gamut */
		} else {				/* Note closest cood to boundary if within limits */
			if (dev[m] < 0.5)
				tt = 0.5 - dev[m];
			else 	/* >= 0.5 */
				tt = dev[m] - 0.5;
			if (tt < mind) {
				mind = tt;
				mini = m;
			}
		}
	}
	if (!outg) {				/* If point is within gamut, set to closest point */
		if (dev[mini] < 0.5)
			dgb[mini] = 0.0;
		else
			dgb[mini] = 1.0;
	}
	
	/* Do RGB -> Lab transform on nearest gamut boundary point */
	RGB_Lab(NULL, pgb, dgb);

	/* Distance to nearest gamut point in PCS (Lab) space */
	gdst = absdiff(pcs, pgb);
	if (!outg)	/* If within gamut */
		gdst = -gdst;

	/* Distance in PCS space will be roughly -86 -> 86 */
	/* Convert so that 0.5 is on boundary, and then clip. */
	gdst /= 100.0;
	gdst += 0.5;
	if (gdst < 0.0)
		gdst = 0.0;
	else if (gdst > 1.0)
		gdst = 1.0;

	out[0] = gdst;
}

/* - - - - - - - - - - - - - */

#define TRES 10
#define MON_POINTS 8101		/* Number of test points in monochrome tests */

int
main(
	int argc,
	char *argv[]
) {
	char *file_name = "xxxx.icm";
	icmFile *wr_fp, *rd_fp;
	icc *wr_icco, *rd_icco;		/* Keep object separate */
	int rv = 0;

	/* Check variables */
	int co[3];
	double in[3], out[3], check[3], temp[3];

	{

#if defined(__IBMC__) && defined(_M_IX86)
		_control87(EM_UNDERFLOW, EM_UNDERFLOW);
#endif
		printf("Starting lookup function test - V2.03\n");

		/* Do a check that our reference function is reversable */
		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);

					/* Do device -> XYZ transform */
					RGB_XYZ(NULL, out, in);
			
					/* Do XYZ -> Device transform */
					XYZ_RGB(NULL, check, out);

					/* Check the result */
					mxd = maxdiff(in, check);
					if (mxd > 0.00001)
#ifdef STOPONERROR
						error ("Excessive error %f > 0.00001",mxd);
#else
						warning ("Excessive error %f > 0.00001",mxd);
#endif /* STOPONERROR */
				}
			}
		}
		printf("Self check complete\n");
	}

	/* ---------------------------------------- */
	/* Create a monochrome XYZ profile to test      */
	/* ---------------------------------------- */

	/* Open up the file for writing */
	if ((wr_fp = new_icmFileStd_name(file_name,"w")) == NULL)
		error ("Write: Can't open file '%s'",file_name);

	if ((wr_icco = new_icc()) == NULL)
		error ("Write: Creation of ICC object failed");

	/* Add all the tags required */

	/* The header: */
	{
		icmHeader *wh = wr_icco->header;

		/* Values that must be set before writing */
		wh->deviceClass     = icSigDisplayClass;	/* Could use Output or Input too */
    	wh->colorSpace      = icSigGrayData;		/* It's a gray space */
    	wh->pcs             = icSigXYZData;			/* Test XYZ monochrome profile */
    	wh->renderingIntent = icRelativeColorimetric;

		/* Values that should be set before writing */
		wh->manufacturer = str2tag("tst2");
    	wh->model        = str2tag("test");
	}
	/* Profile Description Tag: */
	{
		icmTextDescription *wo;
		char *dst = "This is a test monochrome XYZ style Display Profile";
		if ((wo = (icmTextDescription *)wr_icco->add_tag(
		           wr_icco, icSigProfileDescriptionTag,	icSigTextDescriptionType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = strlen(dst)+1; 	/* Allocated and used size of desc, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->desc, dst);		/* Copy the string in */
	}
	/* Copyright Tag: */
	{
		icmText *wo;
		char *crt = "Copyright 1998 Graeme Gill";
		if ((wo = (icmText *)wr_icco->add_tag(
		           wr_icco, icSigCopyrightTag,	icSigTextType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = strlen(crt)+1; 	/* Allocated and used size of text, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->data, crt);		/* Copy the text in */
	}
	/* White Point Tag: */
	{
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = ABS_X;			/* Set some silly numbers */
		wo->data[0].Y = ABS_Y;
		wo->data[0].Z = ABS_Z;
	}
	/* Black Point Tag: */
	{
		icmXYZArray *wo;
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaBlackPointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = 0.02;			/* Doesn't take part in Absolute anymore */
		wo->data[0].Y = 0.04;
		wo->data[0].Z = 0.03;
	}
	/* Gray Tone Reproduction Curve Tags: */
	{
		icmCurve *wog;
		int i;
		if ((wog = (icmCurve *)wr_icco->add_tag(
		           wr_icco, icSigGrayTRCTag, icSigCurveType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wog->flag = icmCurveSpec; 		/* Specified version */
		wog->size = 256;				/* Number of entries (min must be 2!) */
		wog->allocate((icmBase *)wog);	/* Allocate space */
		for (i = 0; i < wog->size; i++) {
			double vv;
			vv = i/(wog->size-1.0);
			wog->data[i] = Gray_GrayY(vv);
		}
	}

	/* Write the file out */
	if ((rv = wr_icco->write(wr_icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,wr_icco->err);
	
	wr_icco->del(wr_icco);
	wr_fp->del(wr_fp);

	/* - - - - - - - - - - - - - - */
	/* Deal with reading and verifying the monochrome XYZ profile */

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(file_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",file_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icmDefaultIntent,
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = co[0]/(MON_POINTS-1.0);

			/* Do reference conversion of device -> XYZ transform */
			Gray_XYZ(check,in[0]);
	
			/* Do lookup of device -> XYZ transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);
	
			/* Check the result */
			mxd = maxdiff(out, check);
			if (mxd > 0.00005)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome XYZ Fwd %f > 0.00005",mxd);
#else
				warning ("Excessive error in Monochrome XYZ Fwd %f > 0.00005",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome XYZ fwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse lookup function */
	{
		double min[3], range[3];
		double merr = 0.0;
		icmLuBase *luo;

		/* Establish the range */
		Gray_XYZ(min,0.0);
		Gray_XYZ(range,1.0);
		range[0] -= min[0];
		range[1] -= min[1];
		range[2] -= min[2];
	
		/* Get a bwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = range[0] * co[0]/(MON_POINTS-1.0) + min[0];
			in[1] = range[1] * co[0]/(MON_POINTS-1.0) + min[1];
			in[2] = range[2] * co[0]/(MON_POINTS-1.0) + min[2];

			/* Do reference conversion of XYZ -> device transform */
			check[0] = XYZ_Gray(in);

			/* Do reverse lookup of XYZ -> device transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);

			/* Check the result */
			mxd = fabs(check[0] - out[0]);
			if (mxd > 0.00018)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome XYZ Bwd %f > 0.00018",mxd);
#else
				warning ("Excessive error in Monochrome XYZ Bwd %f > 0.00018",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome XYZ bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the lookup function, absolute colorimetric */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = co[0]/(MON_POINTS-1.0);

			/* Do reference conversion of device -> XYZ transform */
			aGray_XYZ(check,in[0]);
	
			/* Do lookup of device -> XYZ transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);
	
			/* Check the result */
			mxd = maxdiff(out, check);
			if (mxd > 0.00005)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome XYZ Abs Fwd %f > 0.00005",mxd);
#else
				warning ("Excessive error in Monochrome XYZ Abs Fwd %f > 0.00005",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome XYZ fwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse lookup function, absolute colorimetric */
	{
		double min[3], range[3];
		double merr = 0.0;
		icmLuBase *luo;

		/* Establish the range */
		/* Establish the range */
		aGray_XYZ(min,0.0);
		aGray_XYZ(range,1.0);
		range[0] -= min[0];
		range[1] -= min[1];
		range[2] -= min[2];
	
		/* Get a bwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = range[0] * co[0]/(MON_POINTS-1.0) + min[0];
			in[1] = range[1] * co[0]/(MON_POINTS-1.0) + min[1];
			in[2] = range[2] * co[0]/(MON_POINTS-1.0) + min[2];

			/* Do reference conversion of XYZ -> device transform */
			check[0] = aXYZ_Gray(in);

			/* Do reverse lookup of device -> XYZ transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);

			/* Check the result */
			mxd = fabs(check[0] - out[0]);
			if (mxd > 0.0002)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome XYZ Abs Bwd %f > 0.0002",mxd);
#else
				warning ("Excessive error in Monochrome XYZ Abs Bwd %f > 0.0002",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome XYZ bwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	/* ---------------------------------------- */
	/* Create a monochrome Lab profile to test      */
	/* ---------------------------------------- */

	/* Open up the file for writing */
	if ((wr_fp = new_icmFileStd_name(file_name,"w")) == NULL)
		error ("Write: Can't open file '%s'",file_name);

	if ((wr_icco = new_icc()) == NULL)
		error ("Write: Creation of ICC object failed");

	/* Add all the tags required */

	/* The header: */
	{
		icmHeader *wh = wr_icco->header;

		/* Values that must be set before writing */
		wh->deviceClass     = icSigDisplayClass;	/* Could use Output or Input too */
    	wh->colorSpace      = icSigGrayData;		/* It's a gray space */
    	wh->pcs             = icSigLabData;			/* Use Lab for this monochrome profile */
    	wh->renderingIntent = icRelativeColorimetric;

		/* Values that should be set before writing */
		wh->manufacturer = str2tag("tst2");
    	wh->model        = str2tag("test");
	}
	/* Profile Description Tag: */
	{
		icmTextDescription *wo;
		char *dst = "This is a test monochrome Lab style Display Profile";
		if ((wo = (icmTextDescription *)wr_icco->add_tag(
		           wr_icco, icSigProfileDescriptionTag,	icSigTextDescriptionType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = strlen(dst)+1; 	/* Allocated and used size of desc, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->desc, dst);		/* Copy the string in */
	}
	/* Copyright Tag: */
	{
		icmText *wo;
		char *crt = "Copyright 1998 Graeme Gill";
		if ((wo = (icmText *)wr_icco->add_tag(
		           wr_icco, icSigCopyrightTag,	icSigTextType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = strlen(crt)+1; 	/* Allocated and used size of text, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->data, crt);		/* Copy the text in */
	}
	/* White Point Tag: */
	{
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = ABS_X;	/* Set some silly numbers */
		wo->data[0].Y = ABS_Y;
		wo->data[0].Z = ABS_Z;
	}
	/* Black Point Tag: */
	{
		icmXYZArray *wo;
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaBlackPointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = 0.02;			/* Doesn't take part in Absolute anymore */
		wo->data[0].Y = 0.04;
		wo->data[0].Z = 0.03;
	}
	/* Gray Tone Reproduction Curve Tags: */
	{
		icmCurve *wog;
		int i;
		if ((wog = (icmCurve *)wr_icco->add_tag(
		           wr_icco, icSigGrayTRCTag, icSigCurveType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wog->flag = icmCurveSpec; 		/* Specified version */
		wog->size = 256;				/* Number of entries (min must be 2!) */
		wog->allocate((icmBase *)wog);	/* Allocate space */
		for (i = 0; i < wog->size; i++) {
			double vv;
			vv = i/(wog->size-1.0);
			wog->data[i] = Gray_GrayL(vv);
		}
	}

	/* Write the file out */
	if ((rv = wr_icco->write(wr_icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,wr_icco->err);
	
	wr_icco->del(wr_icco);
	wr_fp->del(wr_fp);

	/* - - - - - - - - - - - - - - */
	/* Deal with reading and verifying the monochrome Lab profile */

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(file_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",file_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = co[0]/(MON_POINTS-1.0);

			/* Do reference conversion of device -> Lab transform */
			Gray_Lab(check,in[0]);
	
			/* Do lookup of device -> Lab transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);
	
			/* Check the result */
			mxd = maxdiff(out, check);
			if (mxd > 0.0025)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome Lab Fwd %f > 0.0025",mxd);
#else
				warning ("Excessive error in Monochrome Lab Fwd %f > 0.0025",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome Lab fwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse lookup function */
	{
		double min, range;
		double merr = 0.0;
		icmLuBase *luo;

		/* Establish the range */
		Gray_Lab(out,0.0);
		min = out[0];
		Gray_Lab(out,1.0);
		range = out[0] - min;
	
		/* Get a bwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = range * co[0]/(MON_POINTS-1.0) + min;

			/* Do reference conversion of Lab -> device transform */
			check[0] = Lab_Gray(in);

			/* Do reverse lookup of Lab -> device transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);

			/* Check the result */
			mxd = fabs(check[0] - out[0]);
			if (mxd > 0.0002)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome Lab Bwd %f > 0.0002",mxd);
#else
				warning ("Excessive error in Monochrome Lab Bwd %f > 0.0002",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome Lab bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

#ifdef NEVER
	/* Check the fwd/bwd accuracy */
	{
		double merr = 0.0;
		icmLuBase *luof, *luob;

		/* Get a fwd conversion object */
		if ((luof = rd_icco->get_luobj(rd_icco, icmFwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		/* Get a bwd conversion object */
		if ((luob = rd_icco->get_luobj(rd_icco, icmBwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		/* Check it out */
		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = co[0]/(MON_POINTS-1.0);

			/* Do lookup of device -> Lab transform */
			if ((rv = luof->lookup(luof, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);
	
			/* Do reverse lookup of device -> Lab transform */
			if ((rv = luob->lookup(luob, check, out)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);

			mxd = fabs(in[0] - check[0]);
			if (mxd > 1e-6) {
				printf("co %d, in %f, out %f, check %f, err %f\n",
				        co[0],in[0],out[0],check[0], fabs(in[0] - check[0]));
			}

			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome Lab fwd/bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup objects */
		luof->del(luof);
		luob->del(luob);
	}

	/* Benchmark the routines */
	{
		int ii;
		icmLuBase *luo;
		double no_pixels = 0.0;
		clock_t stime,ttime;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		stime = clock();
		no_pixels = 1000.0 * 2048.0;

		for (ii = 0; ii < 1000; ii++) {
			for (co[0] = 0; co[0] < 2048; co[0]++) {
				double mxd;
				in[0] = co[0]/(2048-1.0);
	
				/* Do lookup of device -> Lab transform */
				if ((rv = luo->lookup(luo, out, in)) > 1)
					error ("%d, %s",rd_icco->errc,rd_icco->err);
			}
		}
		ttime = clock() - stime;
		printf("Done - %f seconds, rate = %f Mpix/sec\n",
	       (double)ttime/CLOCKS_PER_SEC,no_pixels * CLOCKS_PER_SEC/ttime);

		/* Done with lookup object */
		luo->del(luo);
	}

	{
		int ii;
		icmLuBase *luo;
		double no_pixels = 0.0;
		clock_t stime,ttime;

		/* Get a bwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		stime = clock();
		no_pixels = 1000.0 * 2048.0;

		for (ii = 0; ii < 1000; ii++) {
			for (co[0] = 0; co[0] < 2048; co[0]++) {
				double mxd;
				in[0] = 100.0 * co[0]/(2048-1.0);
	
				/* Do lookup of device -> Lab transform */
				if ((rv = luo->lookup(luo, out, in)) > 1)
					error ("%d, %s",rd_icco->errc,rd_icco->err);
			}
		}
		ttime = clock() - stime;
		printf("Done - %f seconds, rate = %f Mpix/sec\n",
	       (double)ttime/CLOCKS_PER_SEC,no_pixels * CLOCKS_PER_SEC/ttime);

		/* Done with lookup object */
		luo->del(luo);
	}
#endif	/* NEVER */

	/* Check the lookup function, absolute colorimetric */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = co[0]/(MON_POINTS-1.0);

			/* Do reference conversion of device -> Lab transform */
			aGray_Lab(check,in[0]);
	
			/* Do lookup of device -> Lab transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);
	
			/* Check the result */
			mxd = maxdiff(out, check);
			if (mxd > 0.003)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome Lab Fwd Abs %f > 0.003",mxd);
#else
				warning ("Excessive error in Monochrome Lab Fwd Abs %f > 0.003",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome Lab fwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse lookup function, absolute colorimetric*/
	{
		double min, range;
		double merr = 0.0;
		icmLuBase *luo;

		/* Establish the range */
		aGray_Lab(out,0.0);
		min = out[0];
		aGray_Lab(out,1.0);
		range = out[0] - min;
	
		/* Get a bwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < MON_POINTS; co[0]++) {
			double mxd;
			in[0] = range * co[0]/(MON_POINTS-1.0) + min;
			in[1] = in[2] = 0.0;

			/* Do reference conversion of Lab -> device transform */
			check[0] = aLab_Gray(in);

			/* Do reverse lookup of Lab -> device transform */
			if ((rv = luo->lookup(luo, out, in)) > 1)
				error ("%d, %s",rd_icco->errc,rd_icco->err);

			/* Check the result */
			mxd = fabs(check[0] - out[0]);
			if (mxd > 0.005)
#ifdef STOPONERROR
				error ("Excessive error in Monochrome Lab Bwd Abs %f > 0.005",mxd);
#else
				warning ("Excessive error in Monochrome Lab Bwd Abs %f > 0.005",mxd);
#endif /* STOPONERROR */
			if (mxd > merr)
				merr = mxd;
		}
		printf("Monochrome Lab bwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	/* ---------------------------------------- */
	/* Create a matrix based profile to test    */
	/* ---------------------------------------- */

	/* Open up the file for writing */
	if ((wr_fp = new_icmFileStd_name(file_name,"w")) == NULL)
		error ("Write: Can't open file '%s'",file_name);

	if ((wr_icco = new_icc()) == NULL)
		error ("Write: Creation of ICC object failed");

	/* Add all the tags required */

	/* The header: */
	{
		icmHeader *wh = wr_icco->header;

		/* Values that must be set before writing */
		wh->deviceClass     = icSigDisplayClass;	/* Could use Output or Input too */
    	wh->colorSpace      = icSigRgbData;			/* It's and RGBish space */
    	wh->pcs             = icSigXYZData;			/* Must be XYZ for matrix based profile */
    	wh->renderingIntent = icRelativeColorimetric;

		/* Values that should be set before writing */
		wh->manufacturer = str2tag("tst2");
    	wh->model        = str2tag("test");
	}
	/* Profile Description Tag: */
	{
		icmTextDescription *wo;
		char *dst = "This is a test matrix style Display Profile";
		if ((wo = (icmTextDescription *)wr_icco->add_tag(
		           wr_icco, icSigProfileDescriptionTag,	icSigTextDescriptionType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = strlen(dst)+1; 	/* Allocated and used size of desc, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->desc, dst);		/* Copy the string in */
	}
	/* Copyright Tag: */
	{
		icmText *wo;
		char *crt = "Copyright 1998 Graeme Gill";
		if ((wo = (icmText *)wr_icco->add_tag(
		           wr_icco, icSigCopyrightTag,	icSigTextType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = strlen(crt)+1; 	/* Allocated and used size of text, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->data, crt);		/* Copy the text in */
	}
	/* White Point Tag: */
	{
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = ABS_X;	/* Set some silly numbers */
		wo->data[0].Y = ABS_Y;
		wo->data[0].Z = ABS_Z;
	}
	/* Black Point Tag: */
	{
		icmXYZArray *wo;
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaBlackPointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = 0.02;			/* Doesn't take part in Absolute anymore */
		wo->data[0].Y = 0.04;
		wo->data[0].Z = 0.03;
	}
	/* Red, Green and Blue Colorant Tags: */
	{
		icmXYZArray *wor, *wog, *wob;
		if ((wor = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigRedColorantTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);
		if ((wog = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigGreenColorantTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);
		if ((wob = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigBlueColorantTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wor->size = wog->size = wob->size = 1;
		wor->allocate((icmBase *)wor);	/* Allocate space */
		wog->allocate((icmBase *)wog);
		wob->allocate((icmBase *)wob);
		wor->data[0].X = matrix[0][0]; wor->data[0].Y = matrix[1][0]; wor->data[0].Z = matrix[2][0];
		wog->data[0].X = matrix[0][1]; wog->data[0].Y = matrix[1][1]; wog->data[0].Z = matrix[2][1];
		wob->data[0].X = matrix[0][2]; wob->data[0].Y = matrix[1][2]; wob->data[0].Z = matrix[2][2];
	}
	/* Red, Green and Blue Tone Reproduction Curve Tags: */
	{
		icmCurve *wor, *wog, *wob;
		int i;
		if ((wor = (icmCurve *)wr_icco->add_tag(
		           wr_icco, icSigRedTRCTag, icSigCurveType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);
		if ((wog = (icmCurve *)wr_icco->add_tag(
		           wr_icco, icSigGreenTRCTag, icSigCurveType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);
		if ((wob = (icmCurve *)wr_icco->add_tag(
		           wr_icco, icSigBlueTRCTag, icSigCurveType)) == NULL) 
			error("add_tag failed: %d, %s",rv,wr_icco->err);

		wor->flag = wog->flag = wob->flag = icmCurveSpec; 	/* Specified version */
		wor->size = wog->size = wob->size = 256;			/* Number of entries (min must be 2!) */
		wor->allocate((icmBase *)wor);	/* Allocate space */
		wog->allocate((icmBase *)wog);
		wob->allocate((icmBase *)wob);
		for (i = 0; i < wor->size; i++) {
			double vv[3];
			vv[0] = vv[1] = vv[2] = i/(wor->size-1.0);
			RGB_RGBp(NULL, vv, vv);		/* Transfer function we want */
			wor->data[i] = vv[0];	/* Curve values 0.0 - 1.0 */
			wog->data[i] = vv[1];
			wob->data[i] = vv[2];
		}
	}

	/* Write the file out */
	if ((rv = wr_icco->write(wr_icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,wr_icco->err);
	
	wr_icco->del(wr_icco);
	wr_fp->del(wr_fp);

	/* - - - - - - - - - - - - - - */
	/* Deal with reading and verifying the Matrix based profile */

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(file_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",file_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the forward lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> XYZ transform */
					RGB_XYZp(NULL, check,in);
	
					/* Do lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 0.00005)
#ifdef STOPONERROR
						error ("Excessive error in Matrix Fwd %f > 0.00005",mxd);
#else
						warning ("Excessive error in Matrix Fwd %f > 0.00005",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Matrix fwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> XYZ */
					RGB_XYZp(NULL, check,in);
	
					/* Do reverse lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.0002)
#ifdef STOPONERROR
						error ("Excessive error in Matrix Bwd %f > 0.0002",mxd);
#else
						warning ("Excessive error in Matrix Bwd %f > 0.0002",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Matrix bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the forward absolute lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> abs XYZ transform */
					aRGB_XYZp(NULL, check,in);
	
					/* Do lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 0.00005)
#ifdef STOPONERROR
						error ("Excessive error in Abs Matrix Fwd %f > 0.00005",mxd);
#else
						warning ("Excessive error in Abs Matrix Fwd %f > 0.00005",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Matrix fwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse absolute lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> abs XYZ */
					aRGB_XYZp(NULL, check,in);
	
					/* Do reverse lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.001)
#ifdef STOPONERROR
						error ("Excessive error in Abs Matrix Bwd %f > 0.001",mxd);
#else
						warning ("Excessive error in Abs Matrix Bwd %f > 0.001",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Matrix bwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	/* ---------------------------------------- */
	/* Create a Lut16 based XYZ profile to test    */
	/* ---------------------------------------- */

	/* Open up the file for writing */
	if ((wr_fp = new_icmFileStd_name(file_name,"w")) == NULL)
		error ("Write: Can't open file '%s'",file_name);

	if ((wr_icco = new_icc()) == NULL)
		error ("Write: Creation of ICC object failed");

	/* Add all the tags required */

	/* The header: */
	{
		icmHeader *wh = wr_icco->header;

		/* Values that must be set before writing */
		wh->deviceClass     = icSigOutputClass;
    	wh->colorSpace      = icSigRgbData;			/* It's and RGBish space */
    	wh->pcs             = icSigXYZData;
    	wh->renderingIntent = icRelativeColorimetric;	/* For want of something */

		/* Values that should be set before writing */
		wh->manufacturer = str2tag("tst2");
    	wh->model        = str2tag("test");
	}
	/* Profile Description Tag: */
	{
		icmTextDescription *wo;
		char *dst = "This is a test Lut XYZ style Output Profile";
		if ((wo = (icmTextDescription *)wr_icco->add_tag(
		           wr_icco, icSigProfileDescriptionTag,	icSigTextDescriptionType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = strlen(dst)+1; 	/* Allocated and used size of desc, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->desc, dst);		/* Copy the string in */
	}
	/* Copyright Tag: */
	{
		icmText *wo;
		char *crt = "Copyright 1998 Graeme Gill";
		if ((wo = (icmText *)wr_icco->add_tag(
		           wr_icco, icSigCopyrightTag,	icSigTextType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = strlen(crt)+1; 	/* Allocated and used size of text, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->data, crt);		/* Copy the text in */
	}
	/* White Point Tag: */
	{
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = ABS_X;	/* Set some silly numbers */
		wo->data[0].Y = ABS_Y;
		wo->data[0].Z = ABS_Z;
	}
	/* Black Point Tag: */
	{
		icmXYZArray *wo;
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaBlackPointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = 0.02;			/* Doesn't take part in Absolute anymore */
		wo->data[0].Y = 0.04;
		wo->data[0].Z = 0.03;
	}

	/* 16 bit dev -> pcs lut: */
	{
		icmLut *wo;
		double xyzmin[3] = {0.0, 0.0, 0.0};
		double xyzmax[3] = {1.0, 1.0, 1.0};			/* Overide default XYZ max of 1.999969482422 */

		/* Intent 1 = relative colorimetric */
		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigAToB1Tag,	icSigLut16Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 3;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 4096;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* So we can't use it for this lut. */

		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigRgbData, 				/* Input color space */
				icSigXYZData, 				/* Output color space */
				RGB_RGBp,					/* Input transfer function, RGB->RGB' (NULL = default) */
				NULL, NULL,					/* Use default Maximum range of RGB' values */
				RGBp_XYZp,					/* RGB' -> XYZ' transfer function */
				xyzmin, xyzmax,				/* Make XYZ' range 0.0 - 1.0 for better precision */
				XYZp_XYZ) != 0)				/* Output transfer function, XYZ'->XYZ (NULL = deflt) */
			error("Setting 16 bit RGB->XYZ Lut failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 bit dev -> pcs lut - link intent 0 to intent 1 */
	{
		icmLut *wo;
		/* Intent 0 = perceptual */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigAToB0Tag,	icSigAToB1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 dev -> pcs bit lut - link intent 2 to intent 1 */
	{
		icmLut *wo;
		/* Intent 2 = saturation */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigAToB2Tag,	icSigAToB1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 bit pcs -> dev lut: */
	{
		icmLut *wo;
		double xyzmin[3] = {0.0, 0.0, 0.0};			/* XYZ' range */
		double xyzmax[3] = {1.0, 1.0, 1.0};			/* Overide default XYZ max of 1.999969482422 */
		double rgbmin[3] = {0.0, 0.0, 0.0};			/* RGB' range */
		double rgbmax[3] = {1.0, 1.0, 1.0};

		/* Intent 1 = relative colorimetric */
		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigBToA1Tag,	icSigLut16Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 3;
    	wo->clutPoints = 33;
    	wo->inputEnt = 1024;		/* (power curves are hard to represent in tables at small values) */
    	wo->outputEnt = 4096;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* (so it could be used here) */
		/* Matrix not tested:
			for (i = 0; i < 3; i++)
				for (j = 0; j < 3; j++)
					wo->e[i][j] = ??;
		*/

#ifdef REVLUTSCALE1
		{
		/* In any any real profile, you will probably be providing a clut    */
		/* function that carefully maps out of gamut PCS values to in-gamut  */
		/* device values, so the scaling done here won't be appropriate.     */ 
		/*                                                                   */
		/* For this regresion test, we are interested in maximizing accuracy */
		/* over the known gamut of the device. It is an advantage therefore  */
		/* to scale the internal lut values to this end.                     */

		/* We'll do a really simple sampling search of the device gamut to   */
		/* establish the XYZ' bounding box.                                  */

		int co[3];
		double in[3], out[3];

		xyzmin[0] = xyzmin[1] = xyzmin[2] = 1.0;
		xyzmax[0] = xyzmax[1] = xyzmax[2] = 0.0;
		for (co[0] = 0; co[0] < 11; co[0]++) {
			in[0] = co[0]/(11-1.0);
			for (co[1] = 0; co[1] < 11; co[1]++) {
				in[1] = co[1]/(11-1.0);
				for (co[2] = 0; co[2] < 11; co[2]++) {
					in[2] = co[2]/(11-1.0);
	
					/* Do RGB -> XYZ' transform */
					RGB_XYZp(NULL, out,in);
					if (out[0] < xyzmin[0])
						xyzmin[0] = out[0];
					if (out[0] > xyzmax[0])
						xyzmax[0] = out[0];
					if (out[1] < xyzmin[1])
						xyzmin[1] = out[1];
					if (out[1] > xyzmax[1])
						xyzmax[1] = out[1];
					if (out[2] < xyzmin[2])
						xyzmin[2] = out[2];
					if (out[2] > xyzmax[2])
						xyzmax[2] = out[2];
				}
			}
		}
		}
#endif

#ifdef REVLUTSCALE2
		{
		/* In any any real profile, you will probably be providing a clut    */
		/* function that carefully maps out of gamut PCS values to in-gamut  */
		/* device values, so the scaling done here won't be appropriate.     */ 
		/*                                                                   */
		/* For this regresion test, we are interested in maximizing accuracy */
		/* over the known gamut of the device.                               */
		/* By setting the min/max to a larger range than will actually be    */
		/* used, we can make sure that the extreme table values of the       */
		/* clut are not actually used, and therefore we won't see the        */
		/* rounding effects of these extreme values being clipped            */
		/* by the numerical limits of the ICC representation.                */
		/* Instead the extreme values will be clipped by the the             */
		/* higher resolution output table.                                   */
		/*                                                                   */
		/* This all assumes that the multi-d reverse transform we are trying */
		/* to represent in the profile extrapolates beyond the legal device  */
		/* value range.                                                      */
		/*                                                                   */
		/* The scaling was chosen by experiment to make sure that the full   */
		/* gamut is surrounded by one row of extrapolated, unclipped clut    */
		/* table entries.                                                    */
	
		int i;
		for (i = 0; i < 3; i++) {
			rgbmin[i] = -0.1667;	/* Magic numbers */
			rgbmax[i] =  1.1667;
		}
		}
#endif
		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigXYZData, 				/* Input color space */
				icSigRgbData, 				/* Output color space */
				XYZ_XYZp, 					/* Input transfer function, XYZ->XYZ' (NULL = default) */
				xyzmin, xyzmax,				/* Make XYZ' range 0.0 - 1.0 for better precision */
				XYZp_RGBp,					/* XYZ' -> RGB' transfer function */
				rgbmin, rgbmax,				/* Make RGB' range 0.0 - 1.333 for less clip rounding */
				RGBp_RGB) != 0)				/* Output transfer function, RGB'->RGB (NULL = deflt) */
			error("Setting 16 bit XYZ->RGB Lut failed: %d, %s",wr_icco->errc,wr_icco->err);

	}
	/* 16 bit pcs -> dev lut - link intent 0 to intent 1 */
	{
		icmLut *wo;
		/* Intent 0 = perceptual */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigBToA0Tag,	icSigBToA1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 pcs -> dev bit lut - link intent 2 to intent 1 */
	{
		icmLut *wo;
		/* Intent 2 = saturation */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigBToA2Tag,	icSigBToA1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}

	/* 16 bit pcs -> gamut lut: */
	{
		icmLut *wo;
		double xyzmin[3] = {0.0, 0.0, 0.0};			/* XYZ' range */
		double xyzmax[3] = {1.0, 1.0, 1.0};			/* Overide default XYZ max of 1.999969482422 */

		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigGamutTag,	icSigLut16Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 1;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 256;
		wo->allocate((icmBase *)wo);/* Allocate space */


		/* The matrix is only applicable to XYZ input space, */
		/* (so it could be used here) */
		/* Matrix not tested:
			for (i = 0; i < 3; i++)
				for (j = 0; j < 3; j++)
					wo->e[i][j] = ??;
		*/

		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigXYZData, 				/* Input color space */
				icSigGrayData, 				/* Output color space */
				XYZ_XYZp,					/* Input transfer function, XYZ->XYZ' (NULL = default) */
				xyzmin, xyzmax,				/* Make XYZ' range 0.0 - 1.0 for better precision */
				XYZp_BDIST,					/* XYZ' -> Boundary Distance transfer function */
				NULL, NULL,					/* Default range from clut to output table */
				BDIST_GAMMUT				/* Boundary Distance -> Out of gamut distance */
		) != 0)
			error("Setting 16 bit XYZ->Gammut Lut failed: %d, %s",wr_icco->errc,wr_icco->err);
	}

	/* Write the file out */
	if ((rv = wr_icco->write(wr_icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,wr_icco->err);
	
	wr_icco->del(wr_icco);
	wr_fp->del(wr_fp);

	/* - - - - - - - - - - - - - - */
	/* Deal with reading and verifying the Lut XYZ style profile */

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(file_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",file_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icRelativeColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> XYZ transform */
					RGB_XYZ(NULL, check, in);
	
					/* Do lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 0.00005)
#ifdef STOPONERROR
						error ("Excessive error in XYZ Lut Fwd %f > 0.00005",mxd);
#else
						warning ("Excessive error in XYZ Lut Fwd %f > 0.00005",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut XYZ fwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;
		int co[3];
		double in[3], out[3], check[3];

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icRelativeColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of XYZ -> device transform */
					RGB_XYZ(NULL, check, in);

					/* Do reverse lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);

					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.002) {
#ifdef STOPONERROR
						error ("Excessive error in XYZ Lut Bwd %f > 0.002",mxd);
#else
						warning ("Excessive error in XYZ Lut Bwd %f > 0.002",mxd);
#endif /* STOPONERROR */
					}
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut XYZ bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Absolute Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> XYZ transform */
					aRGB_XYZ(NULL, check,in);
	
					/* Do lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 0.00005)
#ifdef STOPONERROR
						error ("Excessive error in XYZ Abs Lut Fwd %f > 0.00005",mxd);
#else
						warning ("Excessive error in XYZ Abs Lut Fwd %f > 0.00005",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut XYZ fwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Absolute reverse Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of XYZ -> device transform */
					aRGB_XYZ(NULL, check,in);
	
					/* Do reverse lookup of device -> XYZ transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.002)
#ifdef STOPONERROR
						error ("Excessive error in XYZ Abs Lut Bwd %f > 0.002",mxd);
#else
						warning ("Excessive error in XYZ Abs Lut Bwd %f > 0.002",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut XYZ bwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the XYZ gamut function */
	{
		int ino,ono,iok,ook;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmGamut, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		ino = ono = iok = ook = 0;
		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					int outgamut;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do gamut lookup of XYZ transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Do reference conversion of XYZ -> RGB */
					XYZ_RGB(NULL, check,in);
	
					/* Check the result */
					outgamut = 1;	/* assume on edge */
					if (check[0] < -0.01 || check[0] > 1.01
					 || check[1] < -0.01 || check[1] > 1.01
					 || check[2] < -0.01 || check[2] > 1.01)
						outgamut = 2;	/* Definitely out of gamut */
					if (check[0] > 0.01 && check[0] < 0.99
					 && check[1] > 0.01 && check[1] < 0.99
					 && check[2] > 0.01 && check[2] < 0.99)
						outgamut = 0;	/* Definitely in gamut */

					/* Keep record of agree/disagree */
					if (outgamut <= 1) {
						ino++;
						if (out[0] <= 0.01)
							iok++;
					} else {
						ono++;
						if (out[0] > 0.01)
							ook++;
					}
				}
			}
		}
		printf("Lut XYZ gamut check inside  correct = %f%%\n",100.0 * iok/ino);
		printf("Lut XYZ gamut check outside correct = %f%%\n",100.0 * ook/ono);
		printf("Lut XYZ gamut check total   correct = %f%%\n",100.0 * (iok+ook)/(ino+ono));
		if (((double)iok/ino) < 0.99 || ((double)ook/ono) < 0.98)
#ifdef STOPONERROR
			error ("Gamut XYZ lookup has excessive error");
#else
			warning ("Gamut XYZ lookup has excessive error");
#endif /* STOPONERROR */

		/* Done with lookup object */
		luo->del(luo);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	/* ---------------------------------------- */
	/* Create a Lut16 based Lab profile to test    */
	/* ---------------------------------------- */

	/* Open up the file for writing */
	if ((wr_fp = new_icmFileStd_name(file_name,"w")) == NULL)
		error ("Write: Can't open file '%s'",file_name);

	if ((wr_icco = new_icc()) == NULL)
		error ("Write: Creation of ICC object failed");

	/* Add all the tags required */

	/* The header: */
	{
		icmHeader *wh = wr_icco->header;

		/* Values that must be set before writing */
		wh->deviceClass     = icSigOutputClass;
    	wh->colorSpace      = icSigRgbData;			/* It's and RGBish space */
    	wh->pcs             = icSigLabData;
    	wh->renderingIntent = icRelativeColorimetric;	/* For want of something */

		/* Values that should be set before writing */
		wh->manufacturer = str2tag("tst2");
    	wh->model        = str2tag("test");
	}
	/* Profile Description Tag: */
	{
		icmTextDescription *wo;
		char *dst = "This is a test Lut style Lab Output Profile";
		if ((wo = (icmTextDescription *)wr_icco->add_tag(
		           wr_icco, icSigProfileDescriptionTag,	icSigTextDescriptionType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = strlen(dst)+1; 	/* Allocated and used size of desc, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->desc, dst);		/* Copy the string in */
	}
	/* Copyright Tag: */
	{
		icmText *wo;
		char *crt = "Copyright 1998 Graeme Gill";
		if ((wo = (icmText *)wr_icco->add_tag(
		           wr_icco, icSigCopyrightTag,	icSigTextType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = strlen(crt)+1; 	/* Allocated and used size of text, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->data, crt);		/* Copy the text in */
	}
	/* White Point Tag: */
	{
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = ABS_X;	/* Set some silly numbers */
		wo->data[0].Y = ABS_Y;
		wo->data[0].Z = ABS_Z;
	}
	/* Black Point Tag: */
	{
		icmXYZArray *wo;
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaBlackPointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = 0.02;			/* Doesn't take part in Absolute anymore */
		wo->data[0].Y = 0.04;
		wo->data[0].Z = 0.03;
	}
	/* 16 bit dev -> pcs lut: */
	{
		icmLut *wo;

		/* Intent 1 = relative colorimetric */
		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigAToB1Tag,	icSigLut16Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 3;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 256;		/* I'm not going to use the output Lut */
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* so it is not used here. */

		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigRgbData, 				/* Input color space */
				icSigLabData, 				/* Output color space */
				RGB_RGBp,					/* Input transfer function, RGB->RGB' (NULL = default) */
				NULL, NULL,					/* Use default Maximum range of RGB' values */
				RGBp_Labp,					/* RGB' -> Lab' transfer function */
				NULL, NULL,					/* Use default Maximum range of Lab' values */
				Labp_Lab					/* Linear output transform Lab'->Lab */
		) != 0)
			error("Setting 16 bit RGB->Lab Lut failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 bit dev -> pcs lut - link intent 0 to intent 1 */
	{
		icmLut *wo;
		/* Intent 0 = perceptual */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigAToB0Tag,	icSigAToB1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 dev -> pcs bit lut - link intent 2 to intent 1 */
	{
		icmLut *wo;
		/* Intent 2 = saturation */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigAToB2Tag,	icSigAToB1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 bit pcs -> dev lut: */
	{
		icmLut *wo;
		double rgbmin[3] = {0.0, 0.0, 0.0};								/* RGB' range */
		double rgbmax[3] = {1.0, 1.0, 1.0};

		/* Intent 1 = relative colorimetric */
		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigBToA1Tag,	icSigLut16Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 3;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;		/* Not using this for Lab test */
    	wo->outputEnt = 4096;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* so it is not used here. */


		/* REVLUTSCALE1 could be used here, but in this case it hardly */
		/* makes any difference.                                       */

#ifdef REVLUTSCALE2
		{
		/* In any any real profile, you will probably be providing a clut    */
		/* function that carefully maps out of gamut PCS values to in-gamut  */
		/* device values, so the scaling done here won't be appropriate.     */ 
		/*                                                                   */
		/* For this regresion test, we are interested in maximizing accuracy */
		/* over the known gamut of the device.                               */
		/* By setting the min/max to a larger range than will actually be    */
		/* used, we can make sure that the extreme table values of the       */
		/* clut are not actually used, and therefore we won't see the        */
		/* rounding effects of these extreme values being clipped to         */
		/* by the numerical limits of the ICC representation.                */
		/* Instead the extreme values will be clipped by the the higher      */
		/* higher resolution output table.                                   */
		/*                                                                   */
		/* This all assumes that the multi-d reverse transform we are trying */
		/* to represent in the profile extrapolates beyond the legal device  */
		/* value range.                                                      */
		/*                                                                   */
		/* The scaling was chosen by experiment to make sure that the full   */
		/* gamut is surrounded by one row of extrapolated, unclipped clut    */
		/* table entries.                                                    */
	
		int i;
		for (i = 0; i < 3; i++) {
			rgbmin[i] = -0.1667;	/* Magic numbers */
			rgbmax[i] =  1.1667;
		}
		}
#endif
		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigLabData, 				/* Input color space */
				icSigRgbData, 				/* Output color space */
				Lab_Labp,					/* Linear input transform Lab->Lab' */
				NULL, NULL,					/* Use default Lab' range */
				Labp_RGBp,					/* Lab' -> RGB' transfer function */
				rgbmin, rgbmax,				/* Make RGB' range 0.0 - 1.333 for less clip rounding */
				RGBp_RGB) != 0)				/* Output transfer function, RGB'->RGB (NULL = deflt) */
			error("Setting 16 bit Lab->RGB Lut failed: %d, %s",wr_icco->errc,wr_icco->err);

	}
	/* 16 bit pcs -> dev lut - link intent 0 to intent 1 */
	{
		icmLut *wo;
		/* Intent 0 = perceptual */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigBToA0Tag,	icSigBToA1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 16 pcs -> dev bit lut - link intent 2 to intent 1 */
	{
		icmLut *wo;
		/* Intent 2 = saturation */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigBToA2Tag,	icSigBToA1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}

	/* 16 bit pcs -> gamut lut: */
	{
		icmLut *wo;

		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigGamutTag,	icSigLut16Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 1;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 256;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* so it can't be used here. */

		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigLabData, 				/* Input color space */
				icSigGrayData, 				/* Output color space */
				Lab_Labp,					/* Linear input transform Lab->Lab' */
				NULL, NULL	,				/* Default Lab' range */
				Labp_BDIST,					/* Lab' -> Boundary Distance transfer function */
				NULL, NULL,					/* Default range from clut to output table */
				BDIST_GAMMUT				/* Boundary Distance -> Out of gamut distance */
		) != 0)
			error("Setting 16 bit Lab->Gammut Lut failed: %d, %s",wr_icco->errc,wr_icco->err);
	}

	/* Write the file out */
	if ((rv = wr_icco->write(wr_icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,wr_icco->err);
	
	wr_icco->del(wr_icco);
	wr_fp->del(wr_fp);

	/* - - - - - - - - - - - - - - */
	/* Deal with reading and verifying the Lut Lab 16bit style profile */

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(file_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",file_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icRelativeColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab transform */
					RGB_Lab(NULL, check,in);
	
					/* Do lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 1.0)
#ifdef STOPONERROR
						error ("Excessive error in Lab16 Lut Fwd %f",mxd);
#else
						warning ("Excessive error in Lab16 Lut Fwd %f",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab16 fwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icRelativeColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab */
					RGB_Lab(NULL, check,in);
	
					/* Do reverse lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.01)
#ifdef STOPONERROR
						error ("Excessive error in Lab16 Lut Bwd %f > 0.01",mxd);
#else
						warning ("Excessive error in Lab16 Lut Bwd %f > 0.01",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab16 bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Absolute Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab transform */
					aRGB_Lab(NULL, check,in);
	
					/* Do lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 1.0)
#ifdef STOPONERROR
						error ("Excessive error in Abs Lab16 Lut Fwd %f > 1.0",mxd);
#else
						warning ("Excessive error in Abs Lab16 Lut Fwd %f > 1.0",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab16 fwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Absolute reverse Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab transform */
					aRGB_Lab(NULL, check,in);
	
					/* Do reverse lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.01)
#ifdef STOPONERROR
						error ("Excessive error in Abs Lab16 Lut Bwd %f > 0.01",mxd);
#else
						warning ("Excessive error in Abs Lab16 Lut Bwd %f > 0.01",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab16 bwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Lab gamut function */
	{
		int ino,ono,iok,ook;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmGamut, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		ino = ono = iok = ook = 0;
		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = (co[0]/(TRES-1.0)) * 100.0;					/* L */
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = ((co[1]/(TRES-1.0)) - 0.5) * 256.0;		/* a */
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					int outgamut;
					in[2] = ((co[2]/(TRES-1.0)) - 0.5) * 256.0;	/* b */
	
					/* Do gamut lookup of Lab transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Do reference conversion of Lab -> RGB */
					Lab_RGB(NULL, check,in);
	
					/* Check the result */
					outgamut = 1;	/* assume on edge */
					if (check[0] < -0.01 || check[0] > 1.01
					 || check[1] < -0.01 || check[1] > 1.01
					 || check[2] < -0.01 || check[2] > 1.01)
						outgamut = 2;	/* Definitely out of gamut */
					if (check[0] > 0.01 && check[0] < 0.99
					 && check[1] > 0.01 && check[1] < 0.99
					 && check[2] > 0.01 && check[2] < 0.99)
						outgamut = 0;	/* Definitely in gamut */

					/* Keep record of agree/disagree */
					if (outgamut <= 1) {
						ino++;
						if (out[0] <= 0.01)
							iok++;
					} else {
						ono++;
						if (out[0] > 0.01)
							ook++;
					}
				}
			}
		}
		printf("Lut Lab16 gamut check inside  correct = %f%%\n",100.0 * iok/ino);
		printf("Lut Lab16 gamut check outside correct = %f%%\n",100.0 * ook/ono);
		printf("Lut Lab16 gamut check total   correct = %f%%\n",100.0 * (iok+ook)/(ino+ono));
		if (((double)iok/ino) < 0.99 || ((double)ook/ono) < 0.98)
#ifdef STOPONERROR
			error ("Gamut Lab16 lookup has excessive error");
#else
			warning ("Gamut Lab16 lookup has excessive error");
#endif /* STOPONERROR */

		/* Done with lookup object */
		luo->del(luo);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	/* ---------------------------------------- */
	/* Create a Lut8 based Lab profile to test    */
	/* ---------------------------------------- */

	/* Open up the file for writing */
	if ((wr_fp = new_icmFileStd_name(file_name,"w")) == NULL)
		error ("Write: Can't open file '%s'",file_name);

	if ((wr_icco = new_icc()) == NULL)
		error ("Write: Creation of ICC object failed");

	/* Add all the tags required */

	/* The header: */
	{
		icmHeader *wh = wr_icco->header;

		/* Values that must be set before writing */
		wh->deviceClass     = icSigOutputClass;
    	wh->colorSpace      = icSigRgbData;			/* It's and RGBish space */
    	wh->pcs             = icSigLabData;
    	wh->renderingIntent = icRelativeColorimetric;	/* For want of something */

		/* Values that should be set before writing */
		wh->manufacturer = str2tag("tst2");
    	wh->model        = str2tag("test");
	}
	/* Profile Description Tag: */
	{
		icmTextDescription *wo;
		char *dst = "This is a test Lut style Lab Output Profile";
		if ((wo = (icmTextDescription *)wr_icco->add_tag(
		           wr_icco, icSigProfileDescriptionTag,	icSigTextDescriptionType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = strlen(dst)+1; 	/* Allocated and used size of desc, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->desc, dst);		/* Copy the string in */
	}
	/* Copyright Tag: */
	{
		icmText *wo;
		char *crt = "Copyright 1998 Graeme Gill";
		if ((wo = (icmText *)wr_icco->add_tag(
		           wr_icco, icSigCopyrightTag,	icSigTextType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = strlen(crt)+1; 	/* Allocated and used size of text, inc null */
		wo->allocate((icmBase *)wo);/* Allocate space */
		strcpy(wo->data, crt);		/* Copy the text in */
	}
	/* White Point Tag: */
	{
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = ABS_X;	/* Set some silly numbers */
		wo->data[0].Y = ABS_Y;
		wo->data[0].Z = ABS_Z;
	}
	/* Black Point Tag: */
	{
		icmXYZArray *wo;
		if ((wo = (icmXYZArray *)wr_icco->add_tag(
		           wr_icco, icSigMediaBlackPointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		wo->data[0].X = 0.02;			/* Doesn't take part in Absolute anymore */
		wo->data[0].Y = 0.04;
		wo->data[0].Z = 0.03;
	}
	/* 8 bit dev -> pcs lut: */
	{
		icmLut *wo;

		/* Intent 1 = relative colorimetric */
		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigAToB1Tag,	icSigLut8Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 3;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 256;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* so it is not used here. */

		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigRgbData, 				/* Input color space */
				icSigLabData, 				/* Output color space */
				RGB_RGBp,					/* Input transfer function, RGB->RGB' (NULL = default) */
				NULL, NULL,					/* Use default Maximum range of RGB' values */
				RGBp_Labp,					/* RGB' -> Lab' transfer function */
				NULL, NULL,					/* Use default Maximum range of Lab' values */
				Labp_Lab					/* Linear output transform Lab'->Lab */
		) != 0)
			error("Setting 8 bit RGB->Lab Lut failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 8 bit dev -> pcs lut - link intent 0 to intent 1 */
	{
		icmLut *wo;
		/* Intent 0 = perceptual */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigAToB0Tag,	icSigAToB1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 8 dev -> pcs bit lut - link intent 2 to intent 1 */
	{
		icmLut *wo;
		/* Intent 2 = saturation */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigAToB2Tag,	icSigAToB1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 8 bit pcs -> dev lut: */
	{
		icmLut *wo;
		double rgbmin[3] = {0.0, 0.0, 0.0};								/* RGB' range */
		double rgbmax[3] = {1.0, 1.0, 1.0};

		/* Intent 1 = relative colorimetric */
		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigBToA1Tag,	icSigLut8Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 3;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 256;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* so it is not used here. */


		/* REVLUTSCALE1 could be used here, but in this case it hardly */
		/* makes any difference.                                       */

#ifdef REVLUTSCALE2
		{
		/* In any any real profile, you will probably be providing a clut    */
		/* function that carefully maps out of gamut PCS values to in-gamut  */
		/* device values, so the scaling done here won't be appropriate.     */ 
		/*                                                                   */
		/* For this regresion test, we are interested in maximizing accuracy */
		/* over the known gamut of the device.                               */
		/* By setting the min/max to a larger range than will actually be    */
		/* used, we can make sure that the extreme table values of the       */
		/* clut are not actually used, and therefore we won't see the        */
		/* rounding effects of these extreme values being clipped to         */
		/* by the numerical limits of the ICC representation.                */
		/* Instead the extreme values will be clipped by the the higher      */
		/* higher resolution output table.                                   */
		/*                                                                   */
		/* This all assumes that the multi-d reverse transform we are trying */
		/* to represent in the profile extrapolates beyond the legal device  */
		/* value range.                                                      */
		/*                                                                   */
		/* The scaling was chosen by experiment to make sure that the full   */
		/* gamut is surrounded by one row of extrapolated, unclipped clut    */
		/* table entries.                                                    */
	
		int i;
		for (i = 0; i < 3; i++) {
			rgbmin[i] = -0.1667;	/* Magic numbers */
			rgbmax[i] =  1.1667;
		}
		}
#endif
		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigLabData, 				/* Input color space */
				icSigRgbData, 				/* Output color space */
				Lab_Labp,					/* Linear input transform Lab->Lab' */
				NULL, NULL,					/* Use default Lab' range */
				Labp_RGBp,					/* Lab' -> RGB' transfer function */
				rgbmin, rgbmax,				/* Make RGB' range 0.0 - 1.333 for less clip rounding */
				RGBp_RGB) != 0)				/* Output transfer function, RGB'->RGB (NULL = deflt) */
			error("Setting 8 bit Lab->RGB Lut failed: %d, %s",wr_icco->errc,wr_icco->err);

	}
	/* 8 bit pcs -> dev lut - link intent 0 to intent 1 */
	{
		icmLut *wo;
		/* Intent 0 = perceptual */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigBToA0Tag,	icSigBToA1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}
	/* 8 pcs -> dev bit lut - link intent 2 to intent 1 */
	{
		icmLut *wo;
		/* Intent 2 = saturation */
		if ((wo = (icmLut *)wr_icco->link_tag(
		           wr_icco, icSigBToA2Tag,	icSigBToA1Tag)) == NULL) 
			error("link_tag failed: %d, %s",wr_icco->errc,wr_icco->err);
	}

	/* 8 bit pcs -> gamut lut: */
	{
		icmLut *wo;

		if ((wo = (icmLut *)wr_icco->add_tag(
		           wr_icco, icSigGamutTag,	icSigLut8Type)) == NULL) 
			error("add_tag failed: %d, %s",wr_icco->errc,wr_icco->err);

		wo->inputChan = 3;
		wo->outputChan = 1;
    	wo->clutPoints = 33;
    	wo->inputEnt = 256;
    	wo->outputEnt = 256;
		wo->allocate((icmBase *)wo);/* Allocate space */

		/* The matrix is only applicable to XYZ input space, */
		/* so it can't be used here. */

		/* Use helper function to do the hard work. */
		if (wo->set_tables(wo, NULL,
				icSigLabData, 				/* Input color space */
				icSigGrayData, 				/* Output color space */
				Lab_Labp,					/* Linear input transform Lab->Lab' */
				NULL, NULL	,				/* Default Lab' range */
				Labp_BDIST,					/* Lab' -> Boundary Distance transfer function */
				NULL, NULL,					/* Default range from clut to output table */
				BDIST_GAMMUT				/* Boundary Distance -> Out of gamut distance */
		) != 0)
			error("Setting 16 bit Lab->Gammut Lut failed: %d, %s",wr_icco->errc,wr_icco->err);
	}

	/* Write the file out */
	if ((rv = wr_icco->write(wr_icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,wr_icco->err);
	
	wr_icco->del(wr_icco);
	wr_fp->del(wr_fp);

	/* - - - - - - - - - - - - - - */
	/* Deal with reading and verifying the Lut Lab 8bit style profile */

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(file_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",file_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icRelativeColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab transform */
					RGB_Lab(NULL, check,in);
	
					/* Do lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 2.1)
#ifdef STOPONERROR
						error ("Excessive error in Lab8 Lut Fwd %f > 2.1",mxd);
#else
						warning ("Excessive error in Lab8 Lut Fwd %f > 2.1",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab8 fwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the reverse Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icRelativeColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab */
					RGB_Lab(NULL, check,in);
	
					/* Do reverse lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.025)
#ifdef STOPONERROR
						error ("Excessive error in Lab8 Lut Bwd %f > 0.025",mxd);
#else
						warning ("Excessive error in Lab8 Lut Bwd %f > 0.025",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab8 bwd default intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Absolute Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab transform */
					aRGB_Lab(NULL, check,in);
	
					/* Do lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(out, check);
					if (mxd > 2.3)
#ifdef STOPONERROR
						error ("Excessive error in Abs Lab8 Lut Fwd %f > 2.3",mxd);
#else
						warning ("Excessive error in Abs Lab8 Lut Fwd %f > 2.3",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab8 fwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Absolute reverse Lut lookup function */
	{
		double merr = 0.0;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = co[0]/(TRES-1.0);
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = co[1]/(TRES-1.0);
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					double mxd;
					in[2] = co[2]/(TRES-1.0);
	
					/* Do reference conversion of device -> Lab transform */
					aRGB_Lab(NULL, check,in);
	
					/* Do reverse lookup of device -> Lab transform */
					if ((rv = luo->lookup(luo, out, check)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Check the result */
					mxd = maxdiff(in, out);
					if (mxd > 0.025)
#ifdef STOPONERROR
						error ("Excessive error in Abs Lab8 Lut Bwd %f > 0.025",mxd);
#else
						warning ("Excessive error in Abs Lab8 Lut Bwd %f > 0.025",mxd);
#endif /* STOPONERROR */
					if (mxd > merr)
						merr = mxd;
				}
			}
		}
		printf("Lut Lab8 bwd absolute intent check complete, peak error = %f\n",merr);

		/* Done with lookup object */
		luo->del(luo);
	}

	/* Check the Lab gamut function */
	{
		int ino,ono,iok,ook;
		icmLuBase *luo;

		/* Get a fwd conversion object */
		if ((luo = rd_icco->get_luobj(rd_icco, icmGamut, icmDefaultIntent, 
		                              icmSigDefaultData, icmLuOrdNorm)) == NULL)
			error ("%d, %s",rd_icco->errc, rd_icco->err);

		ino = ono = iok = ook = 0;
		for (co[0] = 0; co[0] < TRES; co[0]++) {
			in[0] = (co[0]/(TRES-1.0)) * 100.0;					/* L */
			for (co[1] = 0; co[1] < TRES; co[1]++) {
				in[1] = ((co[1]/(TRES-1.0)) - 0.5) * 256.0;		/* a */
				for (co[2] = 0; co[2] < TRES; co[2]++) {
					int outgamut;
					in[2] = ((co[2]/(TRES-1.0)) - 0.5) * 256.0;	/* b */
	
					/* Do gamut lookup of Lab transform */
					if ((rv = luo->lookup(luo, out, in)) > 1)
						error ("%d, %s",rd_icco->errc,rd_icco->err);
	
					/* Do reference conversion of Lab -> RGB */
					Lab_RGB(NULL, check,in);
	
					/* Check the result */
					outgamut = 1;	/* assume on edge */
					if (check[0] < -0.01 || check[0] > 1.01
					 || check[1] < -0.01 || check[1] > 1.01
					 || check[2] < -0.01 || check[2] > 1.01)
						outgamut = 2;	/* Definitely out of gamut */
					if (check[0] > 0.01 && check[0] < 0.99
					 && check[1] > 0.01 && check[1] < 0.99
					 && check[2] > 0.01 && check[2] < 0.99)
						outgamut = 0;	/* Definitely in gamut */

					/* Keep record of agree/disagree */
					if (outgamut <= 1) {
						ino++;
						if (out[0] <= 0.01)
							iok++;
					} else {
						ono++;
						if (out[0] > 0.01)
							ook++;
					}
				}
			}
		}
		printf("Lut Lab8 gamut check inside  correct = %f%%\n",100.0 * iok/ino);
		printf("Lut Lab8 gamut check outside correct = %f%%\n",100.0 * ook/ono);
		printf("Lut Lab8 gamut check total   correct = %f%%\n",100.0 * (iok+ook)/(ino+ono));
		if (((double)iok/ino) < 0.99 || ((double)ook/ono) < 0.98)
#ifdef STOPONERROR
			error ("Gamut Lab8 lookup has excessive error");
#else
			warning ("Gamut Lab8 lookup has excessive error");
#endif /* STOPONERROR */

		/* Done with lookup object */
		luo->del(luo);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	/* ---------------------------------------- */

	printf("Lookup test completed OK\n");
	return 0;
}

/* ------------------------------------------------ */
/* Basic printf type error() and warning() routines */

void
error(char *fmt, ...)
{
	va_list args;

	fprintf(stderr,"lutest: Error - ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit (-1);
}

void
warning(char *fmt, ...)
{
	va_list args;

	fprintf(stderr,"lutest: Warning - ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}
