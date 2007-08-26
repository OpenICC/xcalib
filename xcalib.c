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
# include <X11/Xos.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/extensions/xf86vmode.h>
# ifdef FGLRX
#  include <fglrx_gamma.h>
# endif
#else
# include <windows.h>
# include <wingdi.h>
#endif

#include <math.h>

/* the 4-byte marker for the vcgt-Tag */
#define VCGT_TAG     0x76636774L
#define MLUT_TAG     0x6d4c5554L

#ifndef XCALIB_VERSION
# define XCALIB_VERSION "version unknown (>0.5)"
#endif

/* a limit to check the table sizes (of corrupted profiles) */
#ifndef MAX_TABLE_SIZE
# define MAX_TABLE_SIZE   2e10
#endif

#ifdef WIN32GDI
# define u_int16_t  WORD
#endif

/* prototypes */
void error (char *fmt, ...), warning (char *fmt, ...), message(char *fmt, ...);

#if 1
# define BE_INT(a)    ((a)[3]+((a)[2]<<8)+((a)[1]<<16) +((a)[0]<<24))
# define BE_SHORT(a)  ((a)[1]+((a)[0]<<8))
#else
# warning "big endian is NOT TESTED"
# define BE_INT(a)    (a)
# define BE_SHORT(a)  (a)
#endif

/* internal state struct */
struct xcalib_state_t {
  unsigned int verbose;
  float redGamma;
  float redMin;
  float redMax;
  float greenGamma;
  float greenMin;
  float greenMax;
  float blueGamma;
  float blueMin;
  float blueMax;
  float gamma_cor;
} xcalib_state = {0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0};


void
usage (void)
{
  fprintf (stdout, "xcalib %s\n", XCALIB_VERSION);
  fprintf (stdout, "Copyright (C) 2004-2007 Stefan Doehla <stefan AT doehla DOT de>\n");
  fprintf (stdout, "THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY!\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "usage:  xcalib [-options] ICCPROFILE\n");
  fprintf (stdout, "     or xcalib [-options] -alter\n");
  fprintf (stdout, "\n");
  fprintf (stdout, "where the available options are:\n");
#ifndef WIN32GDI
  fprintf (stdout, "    -display <host:dpy>     or -d\n");
  fprintf (stdout, "    -screen <screen-#>      or -s\n");
#else
  fprintf (stdout, "    -screen <monitor-#>     or -s\n");
#endif
#ifdef FGLRX
  fprintf (stdout, "    -controller <card-#>    or -x\n");
#endif
  fprintf (stdout, "    -clear                  or -c\n");
  fprintf (stdout, "    -noaction <LUT-size>    or -n\n");
  fprintf (stdout, "    -verbose                or -v\n");
  fprintf (stdout, "    -printramps             or -p\n");
  fprintf (stdout, "    -loss                   or -l\n");
  fprintf (stdout, "    -invert                 or -i\n");
  fprintf (stdout, "    -gammacor <gamma>       or -gc\n");
  fprintf (stdout, "    -brightness <percent>   or -b\n");
  fprintf (stdout, "    -contrast <percent>     or -co\n");
  fprintf (stdout, "    -red <gamma> <brightness-percent> <contrast-percent>\n");
  fprintf (stdout, "    -green <gamma> <brightness-percent> <contrast-percent>\n");
  fprintf (stdout, "    -blue <gamma> <brightness-percent> <contrast-percent>\n");
#ifndef FGLRX
  fprintf (stdout, "    -alter                  or -a\n");
#endif
  fprintf (stdout, "    -help                   or -h\n");
  fprintf (stdout, "    -version\n");
  fprintf (stdout, "\n");
  fprintf (stdout,
	   "last parameter must be an ICC profile containing a vcgt-tag\n");
  fprintf (stdout, "\n");
#ifndef WIN32GDI 
  fprintf (stdout, "Example: ./xcalib -d :0 -s 0 -v bluish.icc\n");
#else
  fprintf (stdout, "Example: ./xcalib -v bluish.icc\n");
#endif
#ifndef FGLRX
  fprintf (stdout, "Example: ./xcalib -red 1.1 10.0 100.0\n");
#endif
  fprintf (stdout, "\n");
  exit (0);
}

#ifdef WIN32GDI
/* Win32 monitor enumeration - code by gl.tter ( http://gl.tter.org ) */
static unsigned int monitorSearchIndex = 0;
static HDC monitorDC = 0;

/*
 * FUNCTION MonitorEnumProc
 *
 * this is a Win32 callback function which is given as an argument
 * to EnumDisplayMonitors.
 *
 * returns
 * TRUE: if the current enumerated display is the wrong one
 * FALSE: if the right monitor was found and the DC was associated
 */
BOOL CALLBACK MonitorEnumProc (HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data)
{
  MONITORINFOEX monitorInfo;
  
  if(monitorSearchIndex++ != (unsigned int)data)
    return TRUE; /* continue enumeration */
  
  monitorInfo.cbSize = sizeof(monitorInfo);
  if(GetMonitorInfo(monitor, (LPMONITORINFO)&monitorInfo) )
    monitorDC = CreateDC(NULL, monitorInfo.szDevice, NULL, NULL);
  
  return FALSE;  /* stop enumeration */
}

/*
 * FUNCTION FindMonitor
 *
 * find a specific monitor given by index. Index -1 is the
 * primary display.
 *
 * returns the DC of the selected monitor
 */
HDC FindMonitor(int index)
{
  if(index == -1)
    return GetDC(NULL); /* return primary display context */

  monitorSearchIndex = 0;
  monitorDC = 0;
  EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, index);
  return monitorDC;
}

#endif

/*
 * FUNCTION read_vcgt_internal
 *
 * this is a parser for the vcgt tag of ICC profiles which tries to
 * resemble most of the functionality of Graeme Gill's icclib.
 *
 * returns
 * -1: file could not be read
 * 0: file okay but doesn't contain vcgt or MLUT tag
 * 1: success
 */
int
read_vcgt_internal(const char * filename, u_int16_t * rRamp, u_int16_t * gRamp,
		       u_int16_t * bRamp, unsigned int nEntries)
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

  signed int retVal=0;

  u_int16_t * redRamp = NULL, * greenRamp = NULL, * blueRamp = NULL;
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
      return -1; /* file can not be opened */
  } else
    return -1; /* filename char pointer not valid */
  /* skip header */
  if(fseek(fp, 0+128, SEEK_SET))
    return  -1;
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
      break;
    if(tagName == MLUT_TAG)
    {
      if(fseek(fp, 0+tagOffset, SEEK_SET))
        break;
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
      /* simply copy values to the external table (and leave some values out if table size < 256) */
      ratio = (unsigned int)(256 / (nEntries));
      for(j=0; j<nEntries; j++) {
        rRamp[j] = redRamp[ratio*j];
        gRamp[j] = greenRamp[ratio*j];
        bRamp[j] = blueRamp[ratio*j];
      }
      free(redRamp);
      free(greenRamp);
      free(blueRamp);
      retVal = 1;
      break;
    }
    if(tagName == VCGT_TAG)
    {
      fseek(fp, 0+tagOffset, SEEK_SET);
      message("vcgt found\n");
      bytesRead = fread(cTmp, 1, 4, fp);
      tagName = BE_INT(cTmp);
      if(tagName != VCGT_TAG)
      {
        warning("invalid content of table vcgt, starting with %x",
              tagName);
        break;
      }
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
        {
          warning("Gamma values out of range (> 5.0): \nR: %f \tG: %f \t B: %f",
                rGamma, gGamma, bGamma);
          break;
        }
        if(rMin >= 1.0 || gMin >= 1.0 || bMin >= 1.0)
        {
          warning("Gamma lower limit out of range (>= 1.0): \nRMin: %f \tGMin: %f \t BMin: %f",
                rMin, gMin, bMin);
          break;
        }
        if(rMax > 1.0 || gMax > 1.0 || bMax > 1.0)
        {
          warning("Gamma upper limit out of range (> 1.0): \nRMax: %f \tGMax: %f \t BMax: %f",
                rMax, gMax, bMax);
          break;
        }
        message("Red:   Gamma %f \tMin %f \tMax %f\n", rGamma, rMin, rMax);
        message("Green: Gamma %f \tMin %f \tMax %f\n", gGamma, gMin, gMax);
        message("Blue:  Gamma %f \tMin %f \tMax %f\n", bGamma, bMin, bMax);

        for(j=0; j<nEntries; j++) {
          rRamp[j] = 65536.0 *
            ((double) pow ((double) j / (double) (nEntries),
                           rGamma * (double) xcalib_state.gamma_cor 
                          ) * (rMax - rMin) + rMin);
          gRamp[j] = 65536.0 *
            ((double) pow ((double) j / (double) (nEntries),
                           gGamma * (double) xcalib_state.gamma_cor
                          ) * (gMax - gMin) + gMin);
          bRamp[j] = 65536.0 *
            ((double) pow ((double) j / (double) (nEntries),
                           bGamma * (double) xcalib_state.gamma_cor
                          ) * (bMax - bMin) + bMin);
        }
        retVal = 1;
      }
      /* VideoCardGammaTable */
      else if(gammaType==0) {
        bytesRead = fread(cTmp, 1, 2, fp);
        numChannels = BE_SHORT(cTmp);
        bytesRead = fread(cTmp, 1, 2, fp);
        numEntries = BE_SHORT(cTmp);
        bytesRead = fread(cTmp, 1, 2, fp);
        entrySize = BE_SHORT(cTmp);

        /* work-around for AdobeGamma-Profiles */
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
          break;

        /* allocate tables for the file plus one entry for extrapolation */
        redRamp = (unsigned short *) malloc ((numEntries+1) * sizeof (unsigned short));
        greenRamp = (unsigned short *) malloc ((numEntries+1) * sizeof (unsigned short));
        blueRamp = (unsigned short *) malloc ((numEntries+1) * sizeof (unsigned short));
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
              case 1:                bytesRead = fread(cTmp, 1, 1, fp);
                blueRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                blueRamp[j]= BE_SHORT(cTmp);
                break;
            }
          }
        }
        
        if(numEntries >= nEntries) {
          /* simply subsample if the LUT is smaller than the number of entries in the file */
          ratio = (unsigned int)(numEntries / (nEntries));
          for(j=0; j<nEntries; j++) {
            rRamp[j] = redRamp[ratio*j];
            gRamp[j] = greenRamp[ratio*j];
            bRamp[j] = blueRamp[ratio*j];
          }
        }
        else {
          ratio = (unsigned int)(nEntries / numEntries);
          /* add extrapolated upper limit to the arrays - handle overflow */
          redRamp[numEntries] = (redRamp[numEntries-1] + (redRamp[numEntries-1] - redRamp[numEntries-2])) & 0xffff;
          if(redRamp[numEntries] < 0x4000)
            redRamp[numEntries] = 0xffff;
          
          greenRamp[numEntries] = (greenRamp[numEntries-1] + (greenRamp[numEntries-1] - greenRamp[numEntries-2])) & 0xffff;
          if(greenRamp[numEntries] < 0x4000)
            greenRamp[numEntries] = 0xffff;
          
          blueRamp[numEntries] = (blueRamp[numEntries-1] + (blueRamp[numEntries-1] - blueRamp[numEntries-2])) & 0xffff;
          if(blueRamp[numEntries] < 0x4000)
            blueRamp[numEntries] = 0xffff;
         
          for(j=0; j<numEntries; j++) {
            for(i=0; i<ratio; i++)
            {
              rRamp[j*ratio+i] = (redRamp[j]*(ratio-i) + redRamp[j+1]*(i)) / (ratio);
              gRamp[j*ratio+i] = (greenRamp[j]*(ratio-i) + greenRamp[j+1]*(i)) / (ratio);
              bRamp[j*ratio+i] = (blueRamp[j]*(ratio-i) + blueRamp[j+1]*(i)) / (ratio);
            }
          }
        }
        free(redRamp);
        free(greenRamp);
        free(blueRamp);
        retVal = 1;
      }
      break;
    } /* for all tags */
  }
  fclose(fp);
  return retVal;
}

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
  int rv = 0;
  u_int16_t *r_ramp = NULL, *g_ramp = NULL, *b_ramp = NULL;
  int i;
  int clear = 0;
  int alter = 0;
  int donothing = 0;
  int printramps = 0;
  int calcloss = 0;
  int invert = 0;
  int correction = 0;
  u_int16_t tmpRampVal = 0;
  unsigned int r_res, g_res, b_res;
  int screen = -1;

  unsigned int ramp_size = 256;
  unsigned int ramp_scaling;

#ifndef WIN32GDI
  /* X11 */
  XF86VidModeGamma gamma;
  Display *dpy = NULL;
  char *displayname = NULL;
#ifdef FGLRX
  int controller = -1;
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
#endif
    /* X11 screen / Win32 monitor index */
    if (!strcmp (argv[i], "-s") || !strcmp (argv[i], "-screen")) {
      if (++i >= argc)
        usage ();
      screen = atoi (argv[i]);
      continue;
    }
#ifdef FGLRX
    /* ATI controller index (for FGLRX only) */
    if (!strcmp (argv[i], "-x") || !strcmp (argv[i], "-controller")) {
      if (++i >= argc)
        usage ();
      controller = atoi (argv[i]);
      continue;
    }
#endif
    /* print ramps to stdout */
    if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "-printramps")) {
      printramps = 1;
      continue;
    }
    /* print error introduced by applying ramps to stdout */
    if (!strcmp (argv[i], "-l") || !strcmp (argv[i], "-loss")) {
      calcloss = 1;
      continue;
    }
    /* invert the LUT */
    if (!strcmp (argv[i], "-i") || !strcmp (argv[i], "-invert")) {
      invert = 1;
      continue;
    }
    /* clear gamma lut */
    if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "-clear")) {
      clear = 1;
      continue;
    }
#ifndef FGLRX
    /* alter existing lut */
    if (!strcmp (argv[i], "-a") || !strcmp (argv[i], "-alter")) {
      alter = 1;
      continue;
    }
#endif
    /* do not alter video-LUTs : work's best in conjunction with -v! */
    if (!strcmp (argv[i], "-n") || !strcmp (argv[i], "-noaction")) {
      donothing = 1;
      if (++i >= argc)
        usage();
      ramp_size = atoi(argv[i]);
      continue;
    }
    /* global gamma correction value (use 2.2 for WinXP Color Control-like behaviour) */
    if (!strcmp (argv[i], "-gc") || !strcmp (argv[i], "-gammacor")) {
      if (++i >= argc)
        usage();
      xcalib_state.gamma_cor = atof (argv[i]);
      correction = 1;
      continue;
    }
    /* take additional brightness into account */
    if (!strcmp (argv[i], "-b") || !strcmp (argv[i], "-brightness")) {
      double brightness = 0.0;
      if (++i >= argc)
        usage();
      brightness = atof(argv[i]);
      if(brightness < 0.0 || brightness > 99.0)
      {
        warning("brightness is out of range 0.0-99.0");
        continue;
      }
      xcalib_state.redMin = xcalib_state.greenMin = xcalib_state.blueMin = brightness / 100.0;
      xcalib_state.redMax = xcalib_state.greenMax = xcalib_state.blueMax =
        (1.0 - xcalib_state.blueMin) * xcalib_state.blueMax + xcalib_state.blueMin;
      
      correction = 1;
      continue;
    }
    /* take additional contrast into account */
    if (!strcmp (argv[i], "-co") || !strcmp (argv[i], "-contrast")) {
      double contrast = 100.0;
      if (++i >= argc)
        usage();
      contrast = atof(argv[i]);
      if(contrast < 1.0 || contrast > 100.0)
      {
        warning("contrast is out of range 1.0-100.0");
        continue;
      }
      xcalib_state.redMax = xcalib_state.greenMax = xcalib_state.blueMax = contrast / 100.0;
      xcalib_state.redMax = xcalib_state.greenMax = xcalib_state.blueMax =
        (1.0 - xcalib_state.blueMin) * xcalib_state.blueMax + xcalib_state.blueMin;
 
      correction = 1;
      continue;
    }
    /* additional red calibration */ 
    if (!strcmp (argv[i], "-red")) {
      double gamma = 1.0, brightness = 0.0, contrast = 100.0;
      if (++i >= argc)
        usage();
      gamma = atof(argv[i]);
      if(gamma < 0.1 || gamma > 5.0)
      {
        warning("gamma is out of range 0.1-5.0");
        continue;
      }
      if (++i >= argc)
        usage();
      brightness = atof(argv[i]);
      if(brightness < 0.0 || brightness > 99.0)
      {
        warning("brightness is out of range 0.0-99.0");
        continue;
      }
      if (++i >= argc)
        usage();
      contrast = atof(argv[i]);
      if(contrast < 1.0 || contrast > 100.0)
      {
        warning("contrast is out of range 1.0-100.0");
        continue;
      }
 
      xcalib_state.redMin = brightness / 100.0;
      xcalib_state.redMax =
        (1.0 - xcalib_state.redMin) * (contrast / 100.0) + xcalib_state.redMin;
      xcalib_state.redGamma = gamma;
 
      correction = 1;
      continue;
    }
    /* additional green calibration */
    if (!strcmp (argv[i], "-green")) {
      double gamma = 1.0, brightness = 0.0, contrast = 100.0;
      if (++i >= argc)
        usage();
      gamma = atof(argv[i]);
      if(gamma < 0.1 || gamma > 5.0)
      {
        warning("gamma is out of range 0.1-5.0");
        continue;
      }
      if (++i >= argc)
        usage();
      brightness = atof(argv[i]);
      if(brightness < 0.0 || brightness > 99.0)
      {
        warning("brightness is out of range 0.0-99.0");
        continue;
      }
      if (++i >= argc)
        usage();
      contrast = atof(argv[i]);
      if(contrast < 1.0 || contrast > 100.0)
      {
        warning("contrast is out of range 1.0-100.0");
        continue;
      }
 
      xcalib_state.greenMin = brightness / 100.0;
      xcalib_state.greenMax =
        (1.0 - xcalib_state.greenMin) * (contrast / 100.0) + xcalib_state.greenMin;
      xcalib_state.greenGamma = gamma;
 
      correction = 1;
      continue;
    }
    /* additional blue calibration */
    if (!strcmp (argv[i], "-blue")) {
      double gamma = 1.0, brightness = 0.0, contrast = 100.0;
      if (++i >= argc)
        usage();
      gamma = atof(argv[i]);
      if(gamma < 0.1 || gamma > 5.0)
      {
        warning("gamma is out of range 0.1-5.0");
        continue;
      }
      if (++i >= argc)
        usage();
      brightness = atof(argv[i]);
      if(brightness < 0.0 || brightness > 99.0)
      {
        warning("brightness is out of range 0.0-99.0");
        continue;
      }
      if (++i >= argc)
        usage();
      contrast = atof(argv[i]);
      if(contrast < 1.0 || contrast > 100.0)
      {
        warning("contrast is out of range 1.0-100.0");
        continue;
      }
 
      xcalib_state.blueMin = brightness / 100.0;
      xcalib_state.blueMax =
        (1.0 - xcalib_state.blueMin) * (contrast / 100.0) + xcalib_state.blueMin;
      xcalib_state.blueGamma = gamma;
 
      correction = 1;
      continue;
    }
 
    if (i != argc - 1 && !clear && i) {
      usage ();
    }
    if(!clear || !alter)
    {
      if(strlen(argv[i]) < 255)
        strcpy (in_name, argv[i]);
      else
        usage ();
    }
  }

#ifdef WIN32GDI
  if ((!clear || !alter) && (in_name[0] == '\0')) {
    hDc = FindMonitor(screen);
    win_profile_len = MAX_PATH;
    win_default_profile[0] = '\0';
    SetICMMode(hDc, ICM_ON);
    if(GetICMProfileA(hDc, (LPDWORD) &win_profile_len, (LPSTR)win_default_profile))
    {
      if(strlen(win_default_profile) < 255)
        strcpy (in_name, win_default_profile);
      else
        usage();
    }
    else
      usage();
  }
#endif

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
    if (!FGLRX_X11SetGammaRamp_C16native_1024(dpy, screen, controller, 256, &fglrx_gammaramps)) {
#endif
      XCloseDisplay (dpy);
      error ("Unable to reset display gamma");
    }
    goto cleanupX;
  }
  
  /* get number of entries for gamma ramps */
  if(!donothing)
  {
#ifndef FGLRX
    if (!XF86VidModeGetGammaRampSize (dpy, screen, &ramp_size)) {
#else
    if (!FGLRX_X11GetGammaRampSize(dpy, screen, &ramp_size)) {
#endif
      XCloseDisplay (dpy);
      if(!donothing)
        error ("Unable to query gamma ramp size");
      else {
        warning ("Unable to query gamma ramp size - assuming 256");
        ramp_size = 256;
      }
    }
  }
#else /* WIN32GDI */
  if(!donothing) {
    if(!hDc)
      hDc = FindMonitor(screen);
    if (clear) {
      if (!SetDeviceGammaRamp(hDc, &winGammaRamp))
        error ("Unable to reset display gamma");
      goto cleanupX;
    }
  }
#endif

  /* check for ramp size being a power of 2 and inside the supported range */
  switch(ramp_size)
  {
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
    case 1024:
    case 2048:
    case 4096:
    case 8192:
    case 16384:
    case 32768:
    case 65536:
      break;
    default:
      error("unsupported ramp size %u", ramp_size);
  }
  
  r_ramp = (unsigned short *) malloc (ramp_size * sizeof (unsigned short));
  g_ramp = (unsigned short *) malloc (ramp_size * sizeof (unsigned short));
  b_ramp = (unsigned short *) malloc (ramp_size * sizeof (unsigned short));

  if(!alter)
  {
    if( (i = read_vcgt_internal(in_name, r_ramp, g_ramp, b_ramp, ramp_size)) <= 0) {
      if(i<0)
        warning ("Unable to read file '%s'", in_name);
      if(i=0)
        warning ("No calibration data in ICC profile '%s' found", in_name);
      free(r_ramp);
      free(g_ramp);
      free(b_ramp);
      exit(0);
    }
  } else {
#ifndef WIN32GDI
    if (!XF86VidModeGetGammaRamp (dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp))
      warning ("Unable to get display calibration");
#else
    if (!GetDeviceGammaRamp(hDc, &winGammaRamp))
      warning ("Unable to get display calibration");

    for (i = 0; i < ramp_size; i++) {
      r_ramp[i] = winGammaRamp.Red[i];
      g_ramp[i] = winGammaRamp.Green[i];
      b_ramp[i] = winGammaRamp.Blue[i];
    }
#endif
  }

  {
    float redBrightness = 0.0;
    float redContrast = 100.0;
    float redMin = 0.0;
    float redMax = 1.0;
    float redGamma = 1.0;

    redMin = (double)r_ramp[0] / 65535.0;
    redMax = (double)r_ramp[ramp_size - 1] / 65535.0;
    redBrightness = redMin * 100.0;
    redContrast = (redMax - redMin) / (1.0 - redMin) * 100.0; 
    message("Red Brightness: %f   Contrast: %f  Max: %f  Min: %f\n", redBrightness, redContrast, redMax, redMin);
  }

  {
    float greenBrightness = 0.0;
    float greenContrast = 100.0;
    float greenMin = 0.0;
    float greenMax = 1.0;
    float greenGamma = 1.0;

    greenMin = (double)g_ramp[0] / 65535.0;
    greenMax = (double)g_ramp[ramp_size - 1] / 65535.0;
    greenBrightness = greenMin * 100.0;
    greenContrast = (greenMax - greenMin) / (1.0 - greenMin) * 100.0; 
    message("Green Brightness: %f   Contrast: %f  Max: %f  Min: %f\n", greenBrightness, greenContrast, greenMax, greenMin);
  }

  {
    float blueBrightness = 0.0;
    float blueContrast = 100.0;
    float blueMin = 0.0;
    float blueMax = 1.0;
    float blueGamma = 1.0;

    blueMin = (double)b_ramp[0] / 65535.0;
    blueMax = (double)b_ramp[ramp_size - 1] / 65535.0;
    blueBrightness = blueMin * 100.0;
    blueContrast = (blueMax - blueMin) / (1.0 - blueMin) * 100.0; 
    message("Blue Brightness: %f   Contrast: %f  Max: %f  Min: %f\n", blueBrightness, blueContrast, blueMax, blueMin);
  }

  if(correction != 0)
  {
    for(i=0; i<ramp_size; i++)
    {
      r_ramp[i] =  65536.0 * (((double) pow (((double) r_ramp[i]/65536.0),
                                xcalib_state.redGamma * (double) xcalib_state.gamma_cor
                  ) * (xcalib_state.redMax - xcalib_state.redMin)) + xcalib_state.redMin);
      g_ramp[i] =  65536.0 * (((double) pow (((double) g_ramp[i]/65536.0),
                                xcalib_state.greenGamma * (double) xcalib_state.gamma_cor
                  ) * (xcalib_state.greenMax - xcalib_state.greenMin)) + xcalib_state.greenMin);
      b_ramp[i] =  65536.0 * (((double) pow (((double) b_ramp[i]/65536.0),
                                xcalib_state.blueGamma * (double) xcalib_state.gamma_cor
                  ) * (xcalib_state.blueMax - xcalib_state.blueMin)) + xcalib_state.blueMin); 
    }
    message("Altering Red LUTs with   Gamma %f   Min %f   Max %f\n",
       xcalib_state.redGamma, xcalib_state.redMin, xcalib_state.redMax);
    message("Altering Green LUTs with   Gamma %f   Min %f   Max %f\n",
       xcalib_state.greenGamma, xcalib_state.greenMin, xcalib_state.greenMax);
    message("Altering Blue LUTs with   Gamma %f   Min %f   Max %f\n",
       xcalib_state.blueGamma, xcalib_state.blueMin, xcalib_state.blueMax);
  }

  if(!invert) {
    /* ramps should be monotonic - otherwise content is nonsense! */
    for (i = 0; i < ramp_size - 1; i++) {
      if (r_ramp[i + 1] < r_ramp[i])
        warning ("red gamma table not monotonic");
      if (g_ramp[i + 1] < g_ramp[i])
        warning ("green gamma table not monotonic");
      if (b_ramp[i + 1] < b_ramp[i])
        warning ("blue gamma table not monotonic");
    }
  } else {
    for (i = 0; i < ramp_size; i++) {
      if(i >= ramp_size / 2)
        break;
      tmpRampVal = r_ramp[i];
      r_ramp[i] = r_ramp[ramp_size - i - 1];
      r_ramp[ramp_size - i - 1] = tmpRampVal;
      tmpRampVal = g_ramp[i];
      g_ramp[i] = g_ramp[ramp_size - i - 1];
      g_ramp[ramp_size - i - 1] = tmpRampVal;
      tmpRampVal = b_ramp[i];
      b_ramp[i] = b_ramp[ramp_size - i - 1];
      b_ramp[ramp_size - i - 1] = tmpRampVal;
    }
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
    if (!FGLRX_X11SetGammaRamp_C16native_1024(dpy, screen, controller, ramp_size, &fglrx_gammaramps))
# else
    if (!XF86VidModeSetGammaRamp (dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp))
# endif
#else
    if (!SetDeviceGammaRamp(hDc, &winGammaRamp))
#endif
      warning ("Unable to calibrate display");
  }

  message ("X-LUT size:      \t%d\n", ramp_size);

  free(r_ramp);
  free(g_ramp);
  free(b_ramp);

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

