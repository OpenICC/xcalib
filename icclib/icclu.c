
/* 
 * International Color Consortium Format Library (icclib)
 * Profile color lookup utility.
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

/*

	This file is intended to exercise the ability
	of the icc library to translate colors through a profile.
	It also serves as a concise example of how to do this.

 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include "icc.h"

void error(char *fmt, ...), warning(char *fmt, ...);

void usage(void) {
	fprintf(stderr,"Translate colors through an ICC profile, V2.03\n");
	fprintf(stderr,"Author: Graeme W. Gill\n");
	fprintf(stderr,"usage: icclu [-v level] [-f func] [-i intent] [-o order] profile\n");
	fprintf(stderr," -v            Verbose\n");
	fprintf(stderr," -f function   f = forward, b = backwards, g = gamut, p = preview\n");
	fprintf(stderr," -i intent     p = perceptual, r = relative colorometric,\n");
	fprintf(stderr,"               s = saturation, a = absolute\n");
	fprintf(stderr," -p oride      x = XYZ_PCS, l = Lab_PCS,\n");
	fprintf(stderr," -o order      n = normal (priority: lut > matrix > monochrome)\n");
	fprintf(stderr,"               r = reverse (priority: monochrome > matrix > lut)\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"    The colors to be translated should be fed into stdin,\n");
	fprintf(stderr,"    one input color per line, white space separated.\n");
	fprintf(stderr,"    A line starting with a # will be ignored.\n");
	fprintf(stderr,"    A line not starting with a number will terminate the program.\n");
	exit(1);
}

int
main(int argc, char *argv[]) {
	int fa,nfa;				/* argument we're looking at */
	char prof_name[100];
	icmFile *fp;
	icc *icco;
	int verb = 0;
	int rv = 0;
	char buf[200];
	double in[MAX_CHAN], out[MAX_CHAN];

	icmLuBase *luo;
	icColorSpaceSignature ins, outs;	/* Type of input and output spaces */
	int inn, outn;						/* Number of components */
	icmLuAlgType alg;					/* Type of lookup algorithm */

	/* Lookup parameters */
	icmLookupFunc     func   = icmFwd;				/* Default */
	icRenderingIntent intent = icmDefaultIntent;	/* Default */
	icColorSpaceSignature pcsor = icmSigDefaultData;	/* Default */
	icmLookupOrder    order  = icmLuOrdNorm;		/* Default */
	
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

			if (argv[fa][1] == '?')
				usage();

			/* Verbosity */
			else if (argv[fa][1] == 'v' || argv[fa][1] == 'V') {
				verb = 1;
			}
			/* function */
			else if (argv[fa][1] == 'f' || argv[fa][1] == 'F') {
				fa = nfa;
				if (na == NULL) usage();
    			switch (na[0]) {
					case 'f':
					case 'F':
						func = icmFwd;
						break;
					case 'b':
					case 'B':
						func = icmBwd;
						break;
					case 'g':
					case 'G':
						func = icmGamut;
						break;
					case 'p':
					case 'P':
						func = icmPreview;
						break;
					default:
						usage();
				}
			}

			/* Intent */
			else if (argv[fa][1] == 'i' || argv[fa][1] == 'I') {
				fa = nfa;
				if (na == NULL) usage();
    			switch (na[0]) {
					case 'p':
					case 'P':
						intent = icPerceptual;
						break;
					case 'r':
					case 'R':
						intent = icRelativeColorimetric;
						break;
					case 's':
					case 'S':
						intent = icSaturation;
						break;
					case 'a':
					case 'A':
						intent = icAbsoluteColorimetric;
						break;
					default:
						usage();
				}
			}

			/* Search order */
			else if (argv[fa][1] == 'o' || argv[fa][1] == 'O') {
				fa = nfa;
				if (na == NULL) usage();
    			switch (na[0]) {
					case 'n':
					case 'N':
						order = icmLuOrdNorm;
						break;
					case 'r':
					case 'R':
						order = icmLuOrdRev;
						break;
					default:
						usage();
				}
			}

			/* PCS overide */
			else if (argv[fa][1] == 'p' || argv[fa][1] == 'P') {
				fa = nfa;
				if (na == NULL) usage();
    			switch (na[0]) {
					case 'x':
					case 'X':
						pcsor = icSigXYZData;
						break;
					case 'l':
					case 'L':
						pcsor = icSigLabData;
						break;
					default:
						usage();
				}
			}

			else 
				usage();
		} else
			break;
	}

	if (fa >= argc || argv[fa][0] == '-') usage();
	strcpy(prof_name,argv[fa]);

	/* Open up the profile for reading */
	if ((fp = new_icmFileStd_name(prof_name,"r")) == NULL)
		error ("Can't open file '%s'",prof_name);

	if ((icco = new_icc()) == NULL)
		error ("Creation of ICC object failed");

	if ((rv = icco->read(icco,fp,0)) != 0)
		error ("%d, %s",rv,icco->err);

	if (verb) {
		icmFile *op;
		if ((op = new_icmFileStd_fp(stdout)) == NULL)
			error ("Can't open stdout");
		icco->header->dump(icco->header, op, 1);
		op->del(op);
	}

	/* Get a conversion object */
	if ((luo = icco->get_luobj(icco, func, intent, pcsor, order)) == NULL)
		error ("%d, %s",icco->errc, icco->err);

	/* Get details of conversion (Arguments may be NULL if info not needed) */
	luo->spaces(luo, &ins, &inn, &outs, &outn, &alg, NULL, NULL, NULL);

	/* Process colors to translate */
	for (;;) {
		int i,j;
		char *bp, *nbp;

		/* Read in the next line */
		if (fgets(buf, 200, stdin) == NULL)
			break;
		if (buf[0] == '#') {
			fprintf(stdout,"%s\n",buf);
			continue;
		}
		/* For each input number */
		for (bp = buf-1, nbp = buf, i = 0; i < MAX_CHAN; i++) {
			bp = nbp;
			in[i] = strtod(bp, &nbp);
			if (nbp == bp)
				break;			/* Failed */
		}
		if (i == 0)
			break;

		/* Do conversion */
		if ((rv = luo->lookup(luo, out, in)) > 1)
			error ("%d, %s",icco->errc,icco->err);

		/* Output the results */
		for (j = 0; j < inn; j++) {
			if (j > 0)
				fprintf(stdout," %f",in[j]);
			else
				fprintf(stdout,"%f",in[j]);
		}
		printf(" [%s] -> %s -> ", icm2str(icmColorSpaceSignature, ins),
		                          icm2str(icmLuAlg, alg));

		for (j = 0; j < outn; j++) {
			if (j > 0)
				fprintf(stdout," %f",out[j]);
			else
				fprintf(stdout,"%f",out[j]);
		}
		printf(" [%s]", icm2str(icmColorSpaceSignature, outs));

		if (rv == 0)
			fprintf(stdout,"\n");
		else
			fprintf(stdout," (clip)\n");

	}

	/* Done with lookup object */
	luo->del(luo);

	icco->del(icco);

	fp->del(fp);

	return 0;
}


/* Basic printf type error() and warning() routines */

void
error(char *fmt, ...)
{
	va_list args;

	fprintf(stderr,"icclu: Error - ");
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

	fprintf(stderr,"icclu: Warning - ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}
