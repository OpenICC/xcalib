/*
 * xcalib - download vcgt gamma tables to your X11 video card
 *
 * (c) 2004-2005 Stefan Doehla <stefan AT doehla DOT de>
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

/*
 * xcalib is a tiny tool to load the content of vcgt-Tags in ICC
 * profiles to the video card's gamma ramp. It does work with most
 * video card drivers except the generic VESA driver.
 *
 * There are three ways to parse an ICC profile:
 * - use Graeme Gill's icclib (bundled)
 * - use a patched version of Marti Maria's LCMS (patches included)
 * - use internal parsing routines for vcgt-parsing only
 *
 * Using icclib is known to work best, patched LCMS has the
 * advantage of gamma ramp interpolation and the internal routine
 * is perfect for low overhead versions of xcalib.
 */

/* vim: set ai ts=2 sw=2 expandtab: */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

/* for X11 VidMode stuff */
#ifndef WIN32GDI
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#ifdef FGLRX
# include <fglrx_gamma.h>
#endif
#else
#include <windows.h>
#include <wingdi.h>
#endif

/* for icc profile parsing */
#ifdef ICCLIB
# include <icc.h>
#elif PATCHED_LCMS
# include <lcms.h>
#endif

#include <math.h>

/* system gamma is 2.2222 on most UNIX systems
 * MacOS uses 1.8, MS-Windows 2.2
 * XFree gamma 1.0 is gamma 2.222 at the output*/
#ifndef WIN32GDI
#define SYSTEM_GAMMA  2.222222
#else
#define SYSTEM_GAMMA  2.2
#endif

/* the 4-byte marker for the vcgt-Tag */
#define VCGT_TAG     0x76636774L
#define MLUT_TAG     0x6d4c5554L

#ifndef XCALIB_VERSION
# define XCALIB_VERSION "version unknown (>0.5)"
#endif

#ifdef WIN32GDI
#define u_int16_t  WORD
#endif

/* prototypes */
void error (char *fmt, ...), warning (char *fmt, ...), message(char *fmt, ...);
#ifdef PATCHED_LCMS
BOOL ReadVCGT(cmsHPROFILE hProfile, LPGAMMATABLE * pRTable, LPGAMMATABLE * pGTable, LPGAMMATABLE * pBTable, unsigned int nEntries);
static icInt32Number  SearchTag(LPLCMSICCPROFILE Profile, icTagSignature sig);
#endif

#if 1
# define BE_INT(a)    ((a)[3]+((a)[2]*256)+((a)[1]*65536) +((a)[0]*16777216))
# define BE_SHORT(a)  ((a)[1]+((a)[0]*256))
#else
# warning "big endian is NOT TESTED"
# define BE_INT(a)    (a)
# define BE_SHORT(a)  (a)
#endif

/* internal state struct */
struct xcalib_state_t {
  unsigned int verbose;
#ifdef PATCHED_LCMS
  LPGAMMATABLE rGammaTable;
  LPGAMMATABLE gGammaTable;
  LPGAMMATABLE bGammaTable;
#endif
} xcalib_state;


void
usage (void)
{
  fprintf (stdout, "Copyright (C) 2004-2005 Stefan Doehla <stefan AT doehla DOT de>\n");
  fprintf (stdout, "THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY!\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "usage:  xcalib [-options] ICCPROFILE\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "where the available options are:\n");
#ifndef WIN32GDI
  fprintf (stdout, "    -display <host:dpy>     or -d\n");
  fprintf (stdout, "    -screen <screen-#>      or -s\n");
#endif
  fprintf (stdout, "    -clear                  or -c\n");
  fprintf (stdout, "    -noaction               or -n\n");
  fprintf (stdout, "    -verbose                or -v\n");
  fprintf (stdout, "    -printramps             or -p\n");
  fprintf (stdout, "    -loss                   or -l\n");
  fprintf (stdout, "    -help                   or -h\n");
  fprintf (stdout, "    -version\n");
  fprintf (stdout, "\n");
  fprintf (stdout,
	   "last parameter must be an ICC profile containing a vcgt-tag\n");
  fprintf (stdout, "\n");
#ifndef WIN32GDI 
  fprintf (stdout, "Example: ./xcalib -d :0 -s 0 -v gamma_1_0.icc\n");
#else
  fprintf (stdout, "Example: ./xcalib -v gamma_1_0.icc\n");
#endif
  fprintf (stdout, "\n");
  exit (0);
}

#if !defined(ICCLIB) && !defined(PATCHED_LCMS)
/*
 * FUNCTION read_vcgt_from_profile
 *
 * this is a parser for the vcgt tag of ICC profiles which tries to
 * resemble most of the functionality of Graeme Gill's icclib.
 *
 * It is not completely finished yet, so you might be better off using
 * the LCMS or icclib version of xcalib. It is not Big-Endian-safe!
 */
int
read_vcgt_from_profile(const char * filename, u_int16_t ** rRamp, u_int16_t ** gRamp,
		       u_int16_t ** bRamp, unsigned int * nEntries)
{
  FILE * fp;
  unsigned int bytesRead;
  unsigned int numTags=0;
  unsigned int tagName=0;
  unsigned int tagOffset=0;
  unsigned int tagSize=0;
  unsigned char cTmp[4];
  unsigned int uTmp;
  unsigned int gammaType;

  u_int16_t * redRamp, * greenRamp, * blueRamp;
  unsigned int ratio=0;
  /* formula */
  float rGamma, rMin, rMax;
  float gGamma, gMin, gMax;
  float bGamma, bMin, bMax;
  int i=0;
  /* table */
  unsigned int numChannels=0;
  unsigned int numEntries=0;
  unsigned int entrySize=0;
  int j=0;

  if(filename) {
    fp = fopen(filename, "rb");
    if(!fp)
      return -1;
  } else
    return -1;
  /* skip header */
  fseek(fp, 0+128, SEEK_SET);
  /* check num of tags in current profile */
  bytesRead = fread(cTmp, 1, 4, fp);
  numTags = BE_INT(cTmp);
  for(i=0; i<numTags; i++) {
    bytesRead = fread(cTmp, 1, 4, fp);
    tagName = BE_INT(cTmp);
    bytesRead = fread(cTmp, 1, 4, fp);
    tagOffset = BE_INT(cTmp); 
    bytesRead = fread(cTmp, 1, 4, fp);
    tagSize = BE_INT(cTmp);
    if(!bytesRead)
      goto cleanup_fileparser;
    if(tagName == MLUT_TAG)
    {
      fseek(fp, 0+tagOffset, SEEK_SET);
      message("mLUT found (Profile Mechanic)\n");
      redRamp = (unsigned short *) malloc ((256) * sizeof (unsigned short));
      greenRamp = (unsigned short *) malloc ((256) * sizeof (unsigned short));
      blueRamp = (unsigned short *) malloc ((256) * sizeof (unsigned short));
      {		  
        for(j=0; j<256; j++) {
          bytesRead = fread(cTmp, 1, 2, fp);
          redRamp[j]= BE_SHORT(cTmp);
        }
        for(j=0; j<256; j++) {
          bytesRead = fread(cTmp, 1, 2, fp);
          greenRamp[j]= BE_SHORT(cTmp);
        }
        for(j=0; j<256; j++) {
          bytesRead = fread(cTmp, 1, 2, fp);
          blueRamp[j]= BE_SHORT(cTmp);
        }
      }
      /* interpolate if vcgt size doesn't match video card's gamma table */
      if(*nEntries == 256) {
        *rRamp = redRamp;
        *gRamp = greenRamp;
        *bRamp = blueRamp;
      }
      else if(*nEntries < 256) {
        ratio = (unsigned int)(256 / (*nEntries));
        *rRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *gRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *bRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        for(j=0; j<*nEntries; j++) {
          rRamp[0][j] = redRamp[ratio*j];
          gRamp[0][j] = greenRamp[ratio*j];
          bRamp[0][j] = blueRamp[ratio*j];
        }
      }
      /* interpolation of zero order - TODO: at least bilinear */
      else if(*nEntries > 256) {
        ratio = (unsigned int)((*nEntries) / 256);
        *rRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *gRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *bRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        for(j=0; j<*nEntries; j++) {
          rRamp[0][j] = redRamp[j/ratio];
          gRamp[0][j] = greenRamp[j/ratio];
          bRamp[0][j] = blueRamp[j/ratio];
        }
      }
      goto cleanup_fileparser;
    }
    if(tagName == VCGT_TAG)
    {
      fseek(fp, 0+tagOffset, SEEK_SET);
      message("vcgt found\n");
      bytesRead = fread(cTmp, 1, 4, fp);
      tagName = BE_INT(cTmp);
      if(tagName != VCGT_TAG)
        error("invalid content of table vcgt, starting with %x",
              tagName);
        //! goto cleanup_fileparser;
      bytesRead = fread(cTmp, 1, 4, fp);
      bytesRead = fread(cTmp, 1, 4, fp);
      gammaType = BE_INT(cTmp);
      /* VideoCardGammaFormula */
      if(gammaType==1) {
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        rGamma = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        rMin = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        rMax = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        gGamma = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        gMin = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        gMax = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        bGamma = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        bMin = (float)uTmp/65536.0;
        bytesRead = fread(cTmp, 1, 4, fp);
        uTmp = BE_INT(cTmp);
        bMax = (float)uTmp/65536.0;

        if(rGamma > 5.0 || gGamma > 5.0 || bGamma > 5.0)
          error("Gamma values out of range (> 5.0): \nR: %f \tG: %f \t B: %f",
                rGamma, gGamma, bGamma);
        if(rMin >= 1.0 || gMin >= 1.0 || bMin >= 1.0)
          error("Gamma lower limit out of range (>= 1.0): \nRMin: %f \tGMin: %f \t BMin: %f",
                rMin, gMin, bMin);
        if(rMax > 1.0 || gMax > 1.0 || bMax > 1.0)
          error("Gamma upper limit out of range (> 1.0): \nRMax: %f \tGMax: %f \t BMax: %f",
                rMax, gMax, bMax);
       
        message("Red:   Gamma %f \tMin %f \tMax %f\n", rGamma, rMin, rMax);
        message("Green: Gamma %f \tMin %f \tMax %f\n", gGamma, gMin, gMax);
        message("Blue:  Gamma %f \tMin %f \tMax %f\n", bGamma, bMin, bMax);

        *rRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *gRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *bRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        for(j=0; j<*nEntries; j++) {
          rRamp[0][j] = 65536 *
            ((double) pow ((double) j * (rMax - rMin) / (double) (*nEntries),
                           rGamma * (double) SYSTEM_GAMMA
                          ) + rMin);
          gRamp[0][j] = 65536 *
            ((double) pow ((double) j * (gMax - gMin) / (double) (*nEntries),
                           gGamma * (double) SYSTEM_GAMMA
                          ) + gMin);
          bRamp[0][j] = 65536 *
            ((double) pow ((double) j * (bMax - bMin) / (double) (*nEntries),
                           bGamma * (double) SYSTEM_GAMMA
                          ) + bMin);
        }
      }
      /* VideoCardGammaTable */
      else if(gammaType==0) {
        bytesRead = fread(cTmp, 1, 2, fp);
        numChannels = BE_SHORT(cTmp);
        bytesRead = fread(cTmp, 1, 2, fp);
        numEntries = BE_SHORT(cTmp);
        bytesRead = fread(cTmp, 1, 2, fp);
        entrySize = BE_SHORT(cTmp);

        /* conecealment for AdobeGamma-Profiles */
        if(tagSize == 1584) {
          entrySize = 2;
          numEntries = 256;
          numChannels = 3;
        }

        message ("channels:        \t%d\n", numChannels);
        message ("entry size:      \t%dbits\n",entrySize  * 8);
        message ("entries/channel: \t%d\n", numEntries);
        message ("tag size:        \t%d\n", tagSize);
                                                
        if(numChannels!=3)          /* assume we have always RGB */
          goto cleanup_fileparser;

        redRamp = (unsigned short *) malloc ((numEntries) * sizeof (unsigned short));
        greenRamp = (unsigned short *) malloc ((numEntries) * sizeof (unsigned short));
        blueRamp = (unsigned short *) malloc ((numEntries) * sizeof (unsigned short));
        {		  
          for(j=0; j<numEntries; j++) {
            switch(entrySize) {
              case 1:
                bytesRead = fread(cTmp, 1, 1, fp);
                redRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                redRamp[j]= BE_SHORT(cTmp);
                break;
            }
          }
          for(j=0; j<numEntries; j++) {
            switch(entrySize) {
              case 1:
                bytesRead = fread(cTmp, 1, 1, fp);
                greenRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                greenRamp[j]= BE_SHORT(cTmp);
                break;
            }
          }
          for(j=0; j<numEntries; j++) {
            switch(entrySize) {
              case 1:
                bytesRead = fread(cTmp, 1, 1, fp);
                blueRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                blueRamp[j]= BE_SHORT(cTmp);
                break;
            }
          }
        }
        *rRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *gRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        *bRamp = (unsigned short *) malloc ((*nEntries) * sizeof (unsigned short));
        /* interpolate if vcgt size doesn't match video card's gamma table */
        if(*nEntries <= numEntries) {
          ratio = (unsigned int)(numEntries / (*nEntries));
          for(j=0; j<*nEntries; j++) {
            rRamp[0][j] = redRamp[ratio*j];
            gRamp[0][j] = greenRamp[ratio*j];
            bRamp[0][j] = blueRamp[ratio*j];
          }
        }
        /* interpolation of zero order - TODO: at least bilinear */
        else if(*nEntries > numEntries) {
          ratio = (unsigned int)((*nEntries) / numEntries);
          for(j=0; j<*nEntries; j++) {
            rRamp[0][j] = redRamp[j/ratio];
            gRamp[0][j] = greenRamp[j/ratio];
            bRamp[0][j] = blueRamp[j/ratio];
          }
        }
        free(redRamp);
        free(greenRamp);
        free(blueRamp);
      }
      goto cleanup_fileparser;
    }
  }
  /* fallback: fill gamma with default values if no valid vcgt found */
  *rRamp = (unsigned short *) malloc (256 * sizeof (unsigned short));
  *gRamp = (unsigned short *) malloc (256 * sizeof (unsigned short));
  *bRamp = (unsigned short *) malloc (256 * sizeof (unsigned short));
  for(j=0; j < 256; j++) {
    rRamp[0][j] = j << 8;
    gRamp[0][j] = j << 8;
    bRamp[0][j] = j << 8; 
  }
  *nEntries = 256;
  fclose(fp);
  return 1;
  
cleanup_fileparser:
  fclose(fp);
  return 0;
}
#endif

int
main (int argc, char *argv[])
{
  int fa, nfa;			/* argument we're looking at */
  char in_name[256] = { '\000' };
  char tag_name[40] = { '\000' };
  int verb = 2;
  int search = 0;
  int ecount = 1;		/* Embedded count */
  int offset = 0;		/* Offset to read profile from */
  int found;
#ifdef ICCLIB
  icmFile *fp, *op;
  icc *icco;			/* ICC object */
#elif PATCHED_LCMS
  cmsHPROFILE hDisplayProfile;
#endif
  struct xcalib_state_t * x = NULL;
  int rv = 0;
  u_int16_t *r_ramp = NULL, *g_ramp = NULL, *b_ramp = NULL;
  int i;
  int clear = 0;
  int donothing = 0;
  int printramps = 0;
  int calcloss = 0;
  u_int16_t tmpRampVal = 0;
  unsigned int r_res, g_res, b_res;

  unsigned int ramp_size = 256;
  unsigned int ramp_scaling;

#ifndef WIN32GDI
  /* X11 */
  XF86VidModeGamma gamma;
  Display *dpy = NULL;
  int screen = -1;
  char *displayname = NULL;
#ifdef FGLRX
  FGLRX_X11Gamma_C16native fglrx_gammaramps;
#endif
#else
  char win_default_profile[MAX_PATH+1];
  DWORD win_profile_len;
  typedef struct _GAMMARAMP {
    WORD  Red[256];
    WORD  Green[256];
    WORD  Blue[256];
  } GAMMARAMP; 
  GAMMARAMP winGammaRamp;
  HDC hDc = NULL;
#endif

  xcalib_state.verbose = 0;
  x = &xcalib_state;

  /* begin program part */
#ifdef WIN32GDI
  for(i=0; i< ramp_size; i++) {
    winGammaRamp.Red[i] = i << 8;
    winGammaRamp.Blue[i] = i << 8;
    winGammaRamp.Green[i] = i << 8;
  }
#endif

  /* command line parsing */
  
#ifndef WIN32GDI
  if (argc < 2)
    usage ();
#endif

  for (i = 1; i < argc; ++i) {
    /* help */
    if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-help")) {
      usage ();
      exit (0);
    }
    /* verbose mode */
    if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "-verbose")) {
      xcalib_state.verbose = 1;
      continue;
    }
    /* version */
    if (!strcmp (argv[i], "-version")) {
        fprintf(stdout, "xcalib " XCALIB_VERSION "\n");
        exit (0);
    }
#ifndef WIN32GDI
    /* X11 display */
    if (!strcmp (argv[i], "-d") || !strcmp (argv[i], "-display")) {
      if (++i >= argc)
        usage ();
        displayname = argv[i];
        continue;
    }
    /* X11 screen */
    if (!strcmp (argv[i], "-s") || !strcmp (argv[i], "-screen")) {
      if (++i >= argc)
        usage ();
      screen = atoi (argv[i]);
      continue;
    }
#endif
    /* print ramps to stdout */
    if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "-printramps")) {
      printramps = 1;
      continue;
    }
    /* print ramps to stdout */
    if (!strcmp (argv[i], "-l") || !strcmp (argv[i], "-loss")) {
      calcloss = 1;
      continue;
    }
    /* clear gamma lut */
    if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "-clear")) {
      clear = 1;
      continue;
    }
    /* do not alter video-LUTs : work's best in conjunction with -v! */
    if (!strcmp (argv[i], "-n") || !strcmp (argv[i], "-noaction")) {
      donothing = 1;
      continue;
    }
    if (i != argc - 1 && !clear && i) {
      usage ();
    }
    if(!clear)
      strcpy (in_name, argv[i]);
  }

#ifdef WIN32GDI
  if (!clear && (in_name[0] == '\0')) {
    hDc = GetDC(NULL);
    win_profile_len = MAX_PATH;
    win_default_profile[0] = '\0';
    SetICMMode(hDc, ICM_ON);
    if(GetICMProfile(hDc, (LPDWORD) &win_profile_len, win_default_profile))
      strcpy (in_name, win_default_profile);
    else
      usage();
  }
#endif

  if (!clear) {
#ifdef ICCLIB
    /* Open up the file for reading */
    if ((fp = new_icmFileStd_name (in_name, "r")) == NULL)
      error ("Can't open file '%s'", in_name);
    if ((icco = new_icc ()) == NULL)
      error ("Creation of ICC object failed");
#elif PATCHED_LCMS
    hDisplayProfile = cmsOpenProfileFromFile(in_name, "r");
    if(hDisplayProfile == NULL)
      error ("Can't open ICC profile");
#endif 
  }
  
#ifndef WIN32GDI
  /* X11 initializing */
  if ((dpy = XOpenDisplay (displayname)) == NULL) {
    if(!donothing)
      error ("Can't open display %s", XDisplayName (displayname));
    else
      warning("Can't open display %s", XDisplayName (displayname));
  }
  else if (screen == -1)
    screen = DefaultScreen (dpy);

  /* clean gamma table if option set */
  gamma.red = 1.0;
  gamma.green = 1.0;
  gamma.blue = 1.0;
  if (clear) {
#ifndef FGLRX
    if (!XF86VidModeSetGamma (dpy, screen, &gamma)) {
#else
    for(i = 0; i < 256; i++) {
      fglrx_gammaramps.RGamma[i] = i << 2;
      fglrx_gammaramps.GGamma[i] = i << 2;
      fglrx_gammaramps.BGamma[i] = i << 2;
    }
    if (!FGLRX_X11SetGammaRamp_C16native_1024(dpy, screen, -1, 256, &fglrx_gammaramps)) {
#endif
      XCloseDisplay (dpy);
      error ("Unable to reset display gamma");
    }
    goto cleanupX;
  }
  
  /* get number of entries for gamma ramps */
#ifndef FGLRX
  if (!XF86VidModeGetGammaRampSize (dpy, screen, &ramp_size)) {
#else
  if (!FGLRX_X11GetGammaRampSize(dpy, screen, &ramp_size)) {
#endif
    XCloseDisplay (dpy);
    if(!donothing)
      error ("Unable to query gamma ramp size");
    else {
      warning ("Unable to query gamma ramp size");
      ramp_size = 256;
    }
  }
#else
  if(!hDc)
    hDc = GetDC(NULL);
  if (clear) {
    if (!SetDeviceGammaRamp(hDc, &winGammaRamp))
      error ("Unable to reset display gamma");
    goto cleanupX;
  }
#endif

#if !defined(ICCLIB) && !defined(PATCHED_LCMS)
  if(read_vcgt_from_profile(in_name, &r_ramp, &g_ramp, &b_ramp, &ramp_size) < 0) {
    error ("Unable to read file '%s'", in_name);
  }
#endif 

#ifdef ICCLIB
  do {
    found = 0;

    /* Dumb search for magic number */
    if (search) {
      int fc = 0;
      char c;

      if (fp->seek (fp, offset) != 0)
        break;

      while (found == 0) {
        if (fp->read (fp, &c, 1, 1) != 1) 
          break;
        offset++;

        switch (fc) {
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
            if (c == 'p') {
              found = 1;
              offset -= 40;
            }
            else
              fc = 0;
            break;
        }
      }
    }
    if (search == 0 || found != 0) {
      ecount++;
      if ((rv = icco->read (icco, fp, offset)) != 0)
        error ("%d, %s", rv, icco->err);

      {
        /* we only search for vcgt tag */
        icTagSignature sig = icSigVideoCardGammaTag;

        /* Try and locate that particular tag */
        if ((rv = icco->find_tag (icco, sig)) != 0) {
          if (rv == 1)
            warning ("found vcgt-tag but inconsistent values", tag_name);
          else
            error ("can't find tag '%s' in file", tag2str (sig));
        }
        else {
          icmVideoCardGamma *ob;
          if ((ob = (icmVideoCardGamma *) icco->read_tag (icco,sig)) == NULL)
            warning("Failed to read video card gamma table: %d, %s", tag_name, icco->errc, icco->err);
          else {
            if (ob->ttype != icSigVideoCardGammaType)
              warning("Video card gamma table is in inconsistent state");
            switch ((int) ob->tagType) {
              /* video card gamme table: can be loaded directly to X-server if appropriately scaled */
              case icmVideoCardGammaTableType:
                message ("channels:        \t%d\n", ob->u.table.channels);
                message ("entry size:      \t%dbits\n", ob->u.table.entrySize * 8);
                message ("entries/channel: \t%d\n", ob->u.table.entryCount);

                ramp_scaling = ob->u.table.entryCount / ramp_size;

                /* we must interpolate in case of bigger gamma ramps of X-server than
                 * the vcg-table does contain. This is something that needs to be done
                 * in a later version TODO */
                if(ob->u.table.entryCount < ramp_size)
                  error("vcgt-tag does not contain enough data for this Video-LUT");

                r_ramp = (unsigned short *) malloc ((ramp_size + 1) * sizeof (unsigned short));
                g_ramp = (unsigned short *) malloc ((ramp_size + 1) * sizeof (unsigned short));
                b_ramp = (unsigned short *) malloc ((ramp_size + 1) * sizeof (unsigned short));

                /* TODO: allow interpolation for non-integer divisors in
                 * between Video-LUT and X gamma ramp*/
                switch (ob->u.table.entrySize) {
                  case (1):     /* 8 bit */
                    for (i = 0; i < ramp_size; i++)
                      r_ramp[i] = 0xff * (unsigned short) 
                        ((char *) ob->u.table.data)[i*ramp_scaling];
                    for (; i < 2 * ramp_size; i++)
                      g_ramp[i - ramp_size] = 0xff * (unsigned short) 
                        ((char *) ob->u.table.data)[i*ramp_scaling];
                    for (; i < 3 * ramp_size; i++)
                      b_ramp[i - 2 * ramp_size] = 0xff * (unsigned short) 
                        ((char *) ob->u.table.data)[i*ramp_scaling];
                    break;
                  case (2):     /* 16 bit */
                    for (i = 0; i < ramp_size; i++)
                      r_ramp[i] = (unsigned short) 
                        ((short *) ob->u.table.data)[i*ramp_scaling];
                    for (; i < (2 * ramp_size); i++)
                      g_ramp[i - ramp_size] = (unsigned short) 
                        ((short *) ob->u.table.data)[i*ramp_scaling];
                    for (; i < (3 * ramp_size); i++)
                      b_ramp[i - 2 * ramp_size] = (unsigned short) 
                        ((short *) ob->u.table.data)[i*ramp_scaling];
                    break;
                  }
                  break;
              /* gamma formula type: currently no minimum/maximum value TODO*/
              case icmVideoCardGammaFormulaType:
                r_ramp = (unsigned short *) malloc ((ramp_size + 1) * sizeof (unsigned short));
                g_ramp = (unsigned short *) malloc ((ramp_size + 1) * sizeof (unsigned short));
                b_ramp = (unsigned short *) malloc ((ramp_size + 1) * sizeof (unsigned short));

                ramp_scaling = 65536 / ramp_size;

                /* maths for gamma calculation: use system gamma as reference */
                for (i = 0; i < ramp_size; i++) {
                  r_ramp[i] = 65536 * (double) 
                    pow ((double) i / (double) ramp_size, 
                        ob->u.formula.redGamma * (double) SYSTEM_GAMMA);
                  g_ramp[i] = 65536 * (double) 
                    pow ((double) i / (double) ramp_size, 
                        ob->u.formula.greenGamma * (double) SYSTEM_GAMMA);
                  b_ramp[i] = 65536 * (double) 
                    pow ((double) i / (double) ramp_size,
                        ob->u.formula.blueGamma * (double) SYSTEM_GAMMA);
                }
                break;  /* gamma formula */
            }
          }
        }
      }
      offset += 128;
    }
  } while (found != 0);

#elif PATCHED_LCMS
  x->rGammaTable = x->gGammaTable =x->bGammaTable = NULL;
  x->rGammaTable = cmsAllocGamma(ramp_size);
  x->gGammaTable = cmsAllocGamma(ramp_size);
  x->bGammaTable = cmsAllocGamma(ramp_size);

  if(!cmsTakeVideoCardGammaTable(hDisplayProfile, &(x->rGammaTable), &(x->gGammaTable), &(x->bGammaTable) ) )
    error("xcalib exited due to error in reading vcgt");
   
  /* XVidMode gamma ramps have same size than LCMS GammaTable-content */
  r_ramp = x->rGammaTable->GammaTable;
  g_ramp = x->gGammaTable->GammaTable;
  b_ramp = x->bGammaTable->GammaTable;

#endif

  /* ramps should be monotonic - otherwise content is nonsense! */
  for (i = 0; i < ramp_size - 1; i++) {
    if (r_ramp[i + 1] < r_ramp[i])
      warning ("nonsense content in red gamma table");
    if (g_ramp[i + 1] < g_ramp[i])
      warning ("nonsense content in green gamma table");
    if (b_ramp[i + 1] < b_ramp[i])
      warning ("nonsense content in blue gamma table");
  }
  if(calcloss) {
    fprintf(stdout, "Resolution loss for %d entries:\n", ramp_size);
    r_res = 0;
    g_res = 0;
    b_res = 0;
    tmpRampVal = 0xffff;
    for(i = 0; i < ramp_size; i++) {
      if ((r_ramp[i] & 0xff00) != (tmpRampVal & 0xff00)) {
        r_res++;
      }
      tmpRampVal = r_ramp[i];
    }
    tmpRampVal = 0xffff;
    for(i = 0; i < ramp_size; i++) {
      if ((g_ramp[i] & 0xff00) != (tmpRampVal & 0xff00)) {
        g_res++;
      }
      tmpRampVal = g_ramp[i];
    }
    tmpRampVal = 0xffff;
    for(i = 0; i < ramp_size; i++) {
      if ((b_ramp[i] & 0xff00) != (tmpRampVal & 0xff00)) {
        b_res++;
      }
      tmpRampVal = b_ramp[i];
    }
    fprintf(stdout, "R: %d\tG: %d\t B: %d\t colors lost\n", ramp_size - r_res, ramp_size - g_res, ramp_size - b_res );
  }
#ifdef WIN32GDI
  for (i = 0; i < ramp_size; i++) {
    winGammaRamp.Red[i] = r_ramp[i];
    winGammaRamp.Green[i] = g_ramp[i];
    winGammaRamp.Blue[i] = b_ramp[i];
  }

#endif
 
  if(printramps)
    for(i=0; i<ramp_size; i++)
      fprintf(stdout,"%x %x %x\n", r_ramp[i], g_ramp[i], b_ramp[i]);

  if(!donothing) {
    /* write gamma ramp to X-server */
#ifndef WIN32GDI
# ifdef FGLRX
    for(i = 0; i < ramp_size; i++) {
      fglrx_gammaramps.RGamma[i] = r_ramp[i] >> 6;
      fglrx_gammaramps.GGamma[i] = g_ramp[i] >> 6;
      fglrx_gammaramps.BGamma[i] = b_ramp[i] >> 6;
    }
    if (!FGLRX_X11SetGammaRamp_C16native_1024(dpy, screen, -1, ramp_size, &fglrx_gammaramps))
# else
    if (!XF86VidModeSetGammaRamp (dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp))
# endif
#else
    if (!SetDeviceGammaRamp(hDc, &winGammaRamp))
#endif
      warning ("Unable to calibrate display");
  }

#ifdef ICCLIB
  icco->del (icco);
  fp->del (fp);
#elif PATCHED_LCMS
  if(x->rGammaTable!=NULL)
    cmsFreeGamma(x->rGammaTable);
  if(x->gGammaTable!=NULL)
    cmsFreeGamma(x->gGammaTable);
  if(x->bGammaTable!=NULL)
    cmsFreeGamma(x->bGammaTable);
#else
  if(r_ramp)
    free(r_ramp);
  if(g_ramp)
    free(g_ramp);
  if(b_ramp)
    free(b_ramp);
#endif

  message ("X-LUT size:      \t%d\n", ramp_size);

cleanupX:
#ifndef WIN32GDI
  if(dpy)
    if(!donothing)
      XCloseDisplay (dpy);
#endif

  return 0;
}

/* Basic printf type error() and warning() routines */

/* errors are printed to stderr */
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

/* warnings are printed to stdout */
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

/* messages are printed only if the verbose flag is set */
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

