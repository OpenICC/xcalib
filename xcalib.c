/*
 * xcalib
 *
 * (c) 2004 Stefan Doehla <stefan@doehla.de>
 *
 * This program is GPL-ed postcardware! please see README
 *
 * It is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

/* for icc profile parsing */
#include <icc.h>

/* for X11 VidMode stuff */
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>

#include <math.h>

/* system gamma is 2.2222 on most UNIX systems
 * MacOS uses 1.8, MS-Windows 2.2
 * XFree gamma 1.0 is gamma 2.222 at the output*/
#define SYSTEM_GAMMA  2.222222

void error (char *fmt, ...), warning (char *fmt, ...), message(char *fmt, ...);

struct xcalib_state_t {
  unsigned int verbose;
} xcalib_state;


void
usage (void)
{
  fprintf (stdout, "usage:  xcalib [-options] ICCPROFILE\n");
  fprintf (stdout, "(c) 2004 Stefan Doehla <stefan@doehla.de>\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "where the available options are:\n");
  fprintf (stdout, "    -display host:dpy       or -d\n");
  fprintf (stdout, "    -screen                 or -s\n");
  fprintf (stdout, "    -clear                  or -c\n");
  fprintf (stdout, "    -verbose                or -v\n");
  fprintf (stdout, "    -help                   or -h\n");
  fprintf (stdout, "    -version\n");
  fprintf (stdout, "\n");
  fprintf (stdout,
	   "last parameter must be an ICC profile containing vcgt-tag\n");
  fprintf (stdout, "\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  int fa, nfa;			/* argument we're looking at */
  char in_name[256];
  char tag_name[40] = { '\000' };
  int verb = 2;
  int search = 0;
  int ecount = 1;		/* Embedded count */
  int offset = 0;		/* Offset to read profile from */
  int found;
  icmFile *fp, *op;
  icc *icco;			/* ICC object */
  int rv = 0;
  u_int16_t *r_ramp, *g_ramp, *b_ramp;
  int i;
  int clear = 0;
  XF86VidModeGamma gamma;

  /* X11 */
  Display *dpy = NULL;
  int screen = -1;
  unsigned int ramp_size;
  unsigned int ramp_scaling;
  char *displayname;

  xcalib_state.verbose = 0;

  /* begin program part */
  if (argc < 2)
    usage ();

  for (i = 1; i < argc; ++i)
    {
      /* help */
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-help"))
	{
	  usage ();
	  exit (0);
	}
      /* verbose mode */
      if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "-verbose"))
	{
	  xcalib_state.verbose = 1;
	  continue;
	}
      /* version */
      if (!strcmp (argv[i], "-version"))
      {
        fprintf(stdout, "xcalib 0.3\n");
        exit (0);
      }
      /* X11 display */
      if (!strcmp (argv[i], "-d") || !strcmp (argv[i], "-display"))
	{
	  if (++i >= argc)
	    usage ();
	  displayname = argv[i];
	  continue;
	}
      /* X11 screen */
      if (!strcmp (argv[i], "-s") || !strcmp (argv[i], "-screen"))
	{
	  if (++i >= argc)
	    usage ();
	  screen = atoi (argv[i]);
	  continue;
	}
      /* clear gamma lut */
      if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "-clear"))
	{
	  clear = 1;
	  continue;
	}
      if (i != argc - 1 && !clear)
	usage ();
      if (!clear)
	strcpy (in_name, argv[i]);
    }

  if (!clear)
    {
      /* Open up the file for reading */
      if ((fp = new_icmFileStd_name (in_name, "r")) == NULL)
	error ("Can't open file '%s'", in_name);

      if ((icco = new_icc ()) == NULL)
	error ("Creation of ICC object failed");

      /* open output stream */
      if ((op = new_icmFileStd_fp (stdout)) == NULL)
	error ("Can't open stdout stream");
    }
  
  /* X11 initializing */
  if ((dpy = XOpenDisplay (displayname)) == NULL)
    {
      error ("Can't open display %s", XDisplayName (displayname));
    }
  else if (screen == -1)
    screen = DefaultScreen (dpy);

  /* clean gamma table if option set */
  gamma.red = 1.0;
  gamma.green = 1.0;
  gamma.blue = 1.0;
  if (clear)
    {
      if (!XF86VidModeSetGamma (dpy, screen, &gamma))
	{
	  XCloseDisplay (dpy);
	  error ("Unable to reset display gamma");
	}
      goto cleanupX;
    }
  
  /* get number of entries for gamma ramps */
  if (!XF86VidModeGetGammaRampSize (dpy, screen, &ramp_size))
    {
      XCloseDisplay (dpy);
      error ("Unable to query gamma ramp size");
    }

  do
    {
      found = 0;

      /* Dumb search for magic number */
      if (search)
	{
	  int fc = 0;
	  char c;

	  if (fp->seek (fp, offset) != 0)
	    break;

	  while (found == 0)
	    {
	      if (fp->read (fp, &c, 1, 1) != 1)
		{
		  break;
		}
	      offset++;

	      switch (fc)
		{
		case 0:
		  if (c == 'a')
		    fc++;
		  else
		    fc = 0;
		  break;
		case 1:
		  if (c == 'c')
		    fc++;
		  else
		    fc = 0;
		  break;
		case 2:
		  if (c == 's')
		    fc++;
		  else
		    fc = 0;
		  break;
		case 3:
		  if (c == 'p')
		    {
		      found = 1;
		      offset -= 40;
		    }
		  else
		    fc = 0;
		  break;
		}
	    }
	}

      if (search == 0 || found != 0)
	{
	  ecount++;

	  if ((rv = icco->read (icco, fp, offset)) != 0)
	    error ("%d, %s", rv, icco->err);

	  {
	    /* we only search for vcgt tag */
	    icTagSignature sig = icSigVideoCardGammaTag;

	    /* Try and locate that particular tag */
	    if ((rv = icco->find_tag (icco, sig)) != 0)
	      {
		if (rv == 1)
		  warning ("found vcgt-tag but inconsistent values",
			   tag_name);
		else
		  error ("can't find tag '%s' in file", tag2str (sig));
	      }
	    else
	      {
		icmVideoCardGamma *ob;

		if ((ob =
		     (icmVideoCardGamma *) icco->read_tag (icco,
							   sig)) == NULL)
		  {
		    warning
		      ("Failed to read video card gamma table: %d, %s",
		       tag_name, icco->errc, icco->err);
		  }
		else
		  {
		    if (ob->ttype != icSigVideoCardGammaType)
		      warning
			("Video card gamma table is in inconsistent state");
		    switch ((int) ob->tagType)
		      {
			/* video card gamme table: can be loaded directly to X-server if appropriately scaled */
		      case icmVideoCardGammaTableType:
			    message ("channels:        %d\n",
				     ob->u.table.channels);
			    message ("entry size:      %dbits\n",
				     ob->u.table.entrySize * 8);
			    message ("entries/channel: %d\n",
				     ob->u.table.entryCount);

			ramp_scaling = ob->u.table.entryCount / ramp_size;

                        /* we must interpolate in case of bigger gamma ramps of X-server than
                         * the vcg-table does contain. This is something that needs to be done
                         * in a later version TODO */
                        if(ob->u.table.entryCount < ramp_size)
                          {
                            error("vcgt-tag does not contain enough data for this Video-LUT");
                          }

			r_ramp =
			  (unsigned short *) malloc ((ramp_size + 1) *
						     sizeof (unsigned short));
			g_ramp =
			  (unsigned short *) malloc ((ramp_size + 1) *
						     sizeof (unsigned short));
			b_ramp =
			  (unsigned short *) malloc ((ramp_size + 1) *
						     sizeof (unsigned short));

                        /* TODO: allow interpolation for non-integer divisors in
                         * between Video-LUT and X gamma ramp*/
			switch (ob->u.table.entrySize)
			  {
			  case (1):	/* 8 bit */
			    for (i = 0; i < ramp_size; i++)
			      r_ramp[i] =
				(unsigned short) ((char *) ob->u.
						  table.data)[i *
							      ramp_scaling];
			    for (; i < 2 * ramp_size; i++)
			      g_ramp[i - ramp_size] =
				(unsigned short) ((char *) ob->u.
						  table.data)[i *
							      ramp_scaling];
			    for (; i < 3 * ramp_size; i++)
			      b_ramp[i - 2 * ramp_size] =
				(unsigned short) ((char *) ob->u.
						  table.data)[i *
							      ramp_scaling];
			    break;
			  case (2):	/* 16 bit */
			    for (i = 0; i < ramp_size; i++)
			      r_ramp[i] =
				(unsigned short) ((short *) ob->u.
						  table.data)[i *
							      ramp_scaling];
			    for (; i < (2 * ramp_size); i++)
			      g_ramp[i - ramp_size] =
				(unsigned short) ((short *) ob->u.
						  table.data)[i *
							      ramp_scaling];
			    for (; i < (3 * ramp_size); i++)
			      b_ramp[i - 2 * ramp_size] =
				(unsigned short) ((short *) ob->u.
						  table.data)[i *
							      ramp_scaling];
			    break;
			  }

			break;
			/* gamma formula type: currently no minimum/maximum value TODO*/
		      case icmVideoCardGammaFormulaType:
			r_ramp =
			  (unsigned short *) malloc ((ramp_size + 1) *
						     sizeof (unsigned short));
			g_ramp =
			  (unsigned short *) malloc ((ramp_size + 1) *
						     sizeof (unsigned short));
			b_ramp =
			  (unsigned short *) malloc ((ramp_size + 1) *
						     sizeof (unsigned short));

			ramp_scaling = 65563 / ramp_size;

			/* maths for gamma calculation: use system gamma as reference */
			for (i = 0; i < ramp_size; i++)
			  {
			    r_ramp[i] =
			      65563 *
			      (double) pow ((double) i /
					    (double) ramp_size,
					    ob->u.formula.redGamma *
					    (double) SYSTEM_GAMMA);
			    g_ramp[i] =
			      65563 *
			      (double) pow ((double) i /
					    (double) ramp_size,
					    ob->u.formula.greenGamma *
					    (double) SYSTEM_GAMMA);
			    b_ramp[i] =
			      65563 *
			      (double) pow ((double) i /
					    (double) ramp_size,
					    ob->u.formula.blueGamma *
					    (double) SYSTEM_GAMMA);
			  }
			break;	/* gamma formula */
		      }
		    /* debug stuff: print ramps */
			for (i = 0; i < ramp_size - 1; i++)
			  {
			    if (r_ramp[i + 1] < r_ramp[i])
			      warning ("nonsense content in red gamma table");
			    if (g_ramp[i + 1] < g_ramp[i])
			      warning
				("nonsense content in green gamma table");
			    if (b_ramp[i + 1] < b_ramp[i])
			      warning
				("nonsense content in blue gamma table");
			    message ("%d\t%d\t%d\n", r_ramp[i],
				     g_ramp[i], b_ramp[i]);
			  }
			message ("%d\t%d\t%d\n",
				 r_ramp[ramp_size - 1],
				 g_ramp[ramp_size - 1],
				 b_ramp[ramp_size - 1]);
		  }

	      }
	  }
	  offset += 128;
	}
    }
  while (found != 0);

  /* write gamma ramp to X-server */
  if (!XF86VidModeSetGammaRamp
      (dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp))
    {
      warning ("Unable to calibrate display");
    }

  icco->del (icco);
  op->del (op);
  fp->del (fp);

  message ("X-LUT size: %d\n", ramp_size);

cleanupX:
  XCloseDisplay (dpy);

  return 0;
}


/* Basic printf type error() and warning() routines */

void
error (char *fmt, ...)
{
  va_list args;

  fprintf (stderr, "Error - ");
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
  fprintf (stderr, "\n");
  exit (-1);
}

void
warning (char *fmt, ...)
{
  va_list args;

  fprintf (stdout, "Warning - ");
  va_start (args, fmt);
  vfprintf (stdout, fmt, args);
  va_end (args);
  fprintf (stdout, "\n");
}

void
message (char *fmt, ...)
{
  va_list args;

  if(xcalib_state.verbose) {
  va_start (args, fmt);
  vfprintf (stdout, fmt, args);
  va_end (args);
  }
}

