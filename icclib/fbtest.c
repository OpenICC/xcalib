
/* 
 * International Color Consortium Format Library (icclib)
 * Compare fwd and bwd for a profile, notice ink limit of bwd profile.
 *
 * Author:  Graeme W. Gill
 * Date:    99/11/29
 * Version: 2.03
 *
 * Copyright 1998 - 2002 Graeme W. Gill
 * Please refer to Licence.txt file for details.
 */

/* TTBD:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include "icc.h"

#define TRES 11
#define HTRES 27
#define UHTRES 61

void error(char *fmt, ...), warning(char *fmt, ...);

/* - - - - - - - - - - - - - */

/* Return maximum difference */
double maxdiff(double in1[3], double in2[3]) {
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
double absdiff(double in1[3], double in2[3]) {
	double tt, rv = 0.0;
	tt = in1[0] - in2[0];
	rv += tt * tt;
	tt = in1[1] - in2[1];
	rv += tt * tt;
	tt = in1[2] - in2[2];
	rv += tt * tt;
	return sqrt(rv);
}

/* ---------------------------------------- */

void usage(void) {
	fprintf(stderr,"Check fwd to bwd abs transfer of an ICC file, V2.03\n");
	fprintf(stderr,"Author: Graeme W. Gill\n");
	fprintf(stderr,"usage: fbtest [-v] [-h] infile\n");
	fprintf(stderr," -v        verbose\n");
	fprintf(stderr," -h        high res test (%d)\n",HTRES);
	fprintf(stderr," -u        Ultra high res test (%d)\n",UHTRES);
	exit(1);
}

int
main(
	int argc,
	char *argv[]
) {
	int fa,nfa;				/* argument we're looking at */
	int verb = 0;
	int incclip = 0;
	char in_name[100];
	icmFile *rd_fp;
	icc *wr_icco, *rd_icco;		/* Keep object separate */
	int rv = 0;
	icColorSpaceSignature ins, outs;	/* Type of input and output spaces */
	double tres = TRES;
	double maxink = 0.0;
	double inklimit = 4.0;

	/* Check variables */
	int co[4];
	double in[4], out[4], check[4], temp[4];

	
	if (argc < 2)
		usage();

	/* Process the arguments */
	for(fa = 1;fa < argc;fa++) {
		nfa = fa;					/* skip to nfa if next argument is used */
		if (argv[fa][0] == '-')	{	/* Look for any flags */
			char *na = NULL;		/* next argument after flag, null if none */

			if (argv[fa][2] != '\000')
				na = &argv[fa][2];		/* next is directly after flag */
			else {
				if ((fa+1) < argc) {
					if (argv[fa+1][0] != '-') {
						nfa = fa + 1;
						na = argv[nfa];		/* next is seperate non-flag argument */
					}
				}
			}

			/* Verbosity */
			if (argv[fa][1] == 'v' || argv[fa][1] == 'V') {
				verb = 1;
			}
			/* Resolution */
			else if (argv[fa][1] == 'h' || argv[fa][1] == 'H') {
				tres = HTRES;
			}
			/* Resolution */
			else if (argv[fa][1] == 'u' || argv[fa][1] == 'U') {
				tres = UHTRES;
			}
			else if (argv[fa][1] == 'l' || argv[fa][1] == 'L') {
				int limit;
				fa = nfa;
				if (na == NULL) usage();
				limit = atoi(na);
				if (limit < 1)
					limit = 1;
				inklimit = limit/100.0;
			}

			else if (argv[fa][1] == '?')
				usage();
			else 
				usage();
		}
		else
			break;
	}

	if (fa >= argc || argv[fa][0] == '-') usage();
	strcpy(in_name,argv[fa]);

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(in_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",in_name);

	if ((rd_icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = rd_icco->read(rd_icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,rd_icco->err);

	/* Check the forward lookup against the bwd function */
	{
		double merr = 0.0;
		double aerr = 0.0;
		double nsamps = 0.0;
		icmLuBase *luo1, *luo2;

		/* Get a Device to PCS conversion object */
		if ((luo1 = rd_icco->get_luobj(rd_icco, icmFwd, icAbsoluteColorimetric, icSigLabData, icmLuOrdNorm)) == NULL) {
			if ((luo1 = rd_icco->get_luobj(rd_icco, icmFwd, icmDefaultIntent, icSigLabData, icmLuOrdNorm)) == NULL)
				error ("%d, %s",rd_icco->errc, rd_icco->err);
		}
		/* Get details of conversion */
		luo1->spaces(luo1, &ins, NULL, &outs, NULL, NULL, NULL, NULL, NULL);

		/* Get a PCS to Device conversion object */
		if ((luo2 = rd_icco->get_luobj(rd_icco, icmBwd, icAbsoluteColorimetric, icSigLabData, icmLuOrdNorm)) == NULL) {
			if ((luo2 = rd_icco->get_luobj(rd_icco, icmBwd, icmDefaultIntent, icSigLabData, icmLuOrdNorm)) == NULL)
				error ("%d, %s",rd_icco->errc, rd_icco->err);
		}

		if (ins != icSigCmykData) {
			error("Expecting CMYK device");
		}
		

		for (co[0] = 0; co[0] < tres; co[0]++) {
			for (co[1] = 0; co[1] < tres; co[1]++) {
				for (co[2] = 0; co[2] < tres; co[2]++) {
					for (co[3] = 0; co[3] < tres; co[3]++) {
						int rv1, rv2;
						double sum;
						double mxd;

						temp[0] = co[0]/(tres-1.0);
						temp[1] = co[1]/(tres-1.0);
						temp[2] = co[2]/(tres-1.0);
						temp[3] = co[3]/(tres-1.0);

						if ((temp[0] + temp[1] + temp[2] + temp[3]) > inklimit)
							continue;

						/* Generate the in-gamut PCS test point */
						if ((rv1 = luo1->lookup(luo1, in, temp)) > 1)
							error ("%d, %s",rd_icco->errc,rd_icco->err);
		
						/* Now do the check */
						/* PCS -> Device */
						if ((rv2 = luo2->lookup(luo2, out, in)) > 1)
							error ("%d, %s",rd_icco->errc,rd_icco->err);
		
						sum = out[0] + out[1] + out[2] + out[3];
						if (sum > maxink)
							maxink = sum;

						/* Device to PCS */
						if ((rv2 = luo1->lookup(luo1, check, out)) > 1)
							error ("%d, %s",rd_icco->errc,rd_icco->err);
		
						if (verb) {
							printf("%f %f %f -> %f %f %f %f -> %f %f %f [%f]\n",
							in[0],in[1],in[2],out[0],out[1],out[2],out[3],check[0],check[1],check[2],
							maxdiff(check, in));
						}

						/* Check the result */
						mxd = maxdiff(check, in);
						if (mxd > merr)
							merr = mxd;

						nsamps++;
						aerr += absdiff(check, in);
					}
				}
			}
		}
		printf("bwd to fwd check complete, peak err = %f, avg err = %f\n",merr,aerr/nsamps);
		printf("Maximum sum of device values = %5.1f%%\n",maxink * 100.0);

		/* Done with lookup object */
		luo1->del(luo1);
		luo2->del(luo2);
	}

	rd_icco->del(rd_icco);
	rd_fp->del(rd_fp);

	return 0;
}

/* ------------------------------------------------ */
/* Basic printf type error() and warning() routines */

void
error(char *fmt, ...)
{
	va_list args;

	fprintf(stderr,"icctest: Error - ");
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

	fprintf(stderr,"icctest: Warning - ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}
