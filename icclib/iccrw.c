
/* 
 * International Color Consortium Format Library (icclib)
 * Profile read then re-write skeleton utility.
 *
 * Author:  Graeme W. Gill
 * Date:    99/11/29
 * Version: 2.03
 *
 * Copyright 1997, 1998, 1999, 2000, 2001 Graeme W. Gill
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

#undef TEST_VIDGAMTAG		/* Add ColorSync 2.5 VideoCardGamma tag with linear table */
#undef TEST_SRGB_FIX		/* Some test code */
#undef WP_PATCH			/* Overwrite the white point */

void error(char *fmt, ...), warning(char *fmt, ...);

void usage(void) {
	fprintf(stderr,"Read and then re-write an ICC profile V2.03\n");
	fprintf(stderr,"Author: Graeme W. Gill\n");
	fprintf(stderr,"usage: iccrw readprofile writeprofile\n");
	exit(1);
}

int
main(int argc, char *argv[]) {
	int fa,nfa;				/* argument we're looking at */
	char in_name[100];
	char out_name[100];
	icmFile *rd_fp, *wr_fp;
	icc *icco;
	int i;
	int verb = 0;
	int rv = 0;

	if (argc < 3)
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

			/* No options */
			usage();

		} else
			break;
	}

	if (fa >= argc || argv[fa][0] == '-') usage();
	strcpy(in_name,argv[fa++]);

	if (fa >= argc || argv[fa][0] == '-') usage();
	strcpy(out_name,argv[fa]);

	/* Open up the profile for reading */
	if ((rd_fp = new_icmFileStd_name(in_name,"r")) == NULL)
		error ("Can't open file '%s'",in_name);

	if ((icco = new_icc()) == NULL)
		error ("Creation of ICC object failed");

	/* Read header etc. */
	if ((rv = icco->read(icco,rd_fp,0)) != 0)
		error ("%d, %s",rv,icco->err);

	/* Read every tag */
	if (icco->read_all_tags(icco) != 0) {
		error("Unable to read all tags: %d, %s",icco->errc,icco->err);
	}

	rd_fp->del(rd_fp);

	/* ======================================= */
	/* Change profile in here.                 */

#ifdef TEST_SRGB_FIX		/* Some test code */
	/* Try deleting the black point tag */
	{
		if (icco->delete_tag(icco, icSigMediaBlackPointTag) != 0) {
			error("Unable to delete blackpoint tag: %d, %s",icco->errc,icco->err);
		}
	}

	/* Fix up sRGB profile curve */
	{
		icmCurve *ro;
		int i;

		/* Try and read the tag from the file */
		ro = (icmCurve *)icco->read_tag(icco, icSigRedTRCTag);
		if (ro == NULL) 
			error("Unable to read rTRC");

		/* Need to check that the cast is appropriate. */
		if (ro->ttype != icSigCurveType)
			error("rTRC is not CurveType");

		for (i = 0; i < ro->size; i++) {
			double vi, vo;
			vi = i/(double)(ro->size-1);

			if (vi < 0.03928) {
				vo = vi/12.92;
			} else {
				vo = pow((0.055+vi)/1.055,2.4);
			}
			ro->data[i] = vo;
		}
	}

	/* Show we modified this ICC file */
	icco->header->cmmId = str2tag("argl");		/* CMM for profile - Argyll CMM */
#endif

#ifdef TEST_VIDGAMTAG
	/* delete video card gamma tag (c/o Neil Okamoto) */
	{
		if (icco->find_tag(icco, icSigVideoCardGammaTag) == 0)
			if (icco->delete_tag(icco, icSigVideoCardGammaTag) != 0)
				error("Unable to delete videocardgamma tag: %d, %s",icco->errc,icco->err);
	}
	/* Add a video card gamma table */
	{
		int c,i;
		icmVideoCardGamma *wo;
		wo = (icmVideoCardGamma *)icco->add_tag(icco, icSigVideoCardGammaTag, icSigVideoCardGammaType);
		if (wo == NULL)
			error ("Unable to add VideoCardGamma tag");

		wo->tagType = icmVideoCardGammaTableType;
		wo->u.table.channels = 3;             /* rgb */
		wo->u.table.entryCount = 256;         /* full lut */
		wo->u.table.entrySize = 1;            /* byte */
		wo->allocate((icmBase*)wo);
		for (c=0; c<3; c++)
			for (i=0; i<256; i++)
				((unsigned char*)wo->u.table.data)[256*c+i] = 255-i;
	}

	/* Show we modified this ICC file */
	icco->header->cmmId = str2tag("argl");		/* CMM for profile - Argyll CMM */
#endif

#ifdef WP_PATCH			/* Overwrite the white point */
	/* delete white point tag */
	{
		if (icco->find_tag(icco, icSigMediaWhitePointTag) == 0)
			if (icco->delete_tag(icco, icSigMediaWhitePointTag) != 0)
				error("Unable to delete white point tag tag: %d, %s",icco->errc,icco->err);
	}
	/* Add a new white point tag */
	{
		double lab[3];
		icmXYZArray *wo;
		/* Note that tag types icSigXYZType and icSigXYZArrayType are identical */
		if ((wo = (icmXYZArray *)icco->add_tag(
		           icco, icSigMediaWhitePointTag, icSigXYZArrayType)) == NULL) 
			error("add_tag failed: %d, %s",icco->errc, icco->err);

		wo->size = 1;
		wo->allocate((icmBase *)wo);	/* Allocate space */
		lab[0] = 79.8296;
		lab[1] = -0.004042 + 0.842312;
		lab[2] = 3.019928 + 0.810044;
		icmLab2XYZ(&icmD50, lab, lab);
		icmAry2XYZ(wo->data[0], lab);
	}

	/* Show we modified this ICC file */
	icco->header->cmmId = str2tag("argl");		/* CMM for profile - Argyll CMM */
#endif

	/* ======================================= */
	
	/* Open up the other profile for writing */
	if ((wr_fp = new_icmFileStd_name(out_name,"w")) == NULL)
		error ("Can't open file '%s'",out_name);

	if ((rv = icco->write(icco,wr_fp,0)) != 0)
		error ("Write file: %d, %s",rv,icco->err);

	icco->del(icco);
	wr_fp->del(wr_fp);

	return 0;
}

/* ------------------------------------------------ */
/* Basic printf type error() and warning() routines */

void
error(char *fmt, ...)
{
	va_list args;

	fprintf(stderr,"iccrw: Error - ");
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

	fprintf(stderr,"iccrw: Warning - ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}
