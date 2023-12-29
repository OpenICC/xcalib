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

#ifdef INCLUDE_OYJL_C
# include "extras/oyjl_args.c"
#else
# include <oyjl.h>
# include <oyjl_macros.h>
# include <oyjl_version.h>
#endif
extern char **environ;
#ifdef OYJL_HAVE_LOCALE_H
# include <locale.h>
#endif
#define MY_DOMAIN "xcalib"
oyjlTranslation_s * trc = NULL;
#ifdef INCLUDE_OYJL_C
# ifdef _
#  undef _
# endif
# define _(text) text
# define oyjlUi_ToText oyjlUi_ToTextArgsBase
# define oyjlUi_Release oyjlUi_ReleaseArgs
#else
# ifdef _
#  undef _
# endif
# define _(text) oyjlTranslate( trc, text )
#endif

/* for X11 VidMode stuff */
#ifndef _WIN32
# include <X11/Xos.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/extensions/xf86vmode.h>
# include <X11/extensions/Xrandr.h>
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

#ifdef _WIN32
# define u_int16_t  WORD
#endif

/* prototypes */
void error (char *fmt, ...), warning (char *fmt, ...), message(char *fmt, ...);
int myMessage                        ( int/*oyjlMSG_e*/    error_code,
                                       const void        * context,
                                       const char        * format,
                                       ... );
#define error(...) myMessage( oyjlMSG_ERROR, 0, __VA_ARGS__ )
#define warning(format, ...) myMessage( oyjlMSG_CLIENT_CANCELED, 0, OYJL_DBG_FORMAT format, OYJL_DBG_ARGS,  __VA_ARGS__ )
#define message(format, ...) myMessage( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT format, OYJL_DBG_ARGS,  __VA_ARGS__ )
//#define message(...) myMessage( oyjlMSG_INFO, 0, __VA_ARGS__ )
#define usage() { fprintf( stderr, OYJL_DBG_FORMAT , OYJL_DBG_ARGS ); myUsage( ui ); } 

#if 1
# define BE_INT(a)    ((a)[3]+((a)[2]<<8)+((a)[1]<<16) +((a)[0]<<24))
# define BE_SHORT(a)  ((a)[1]+((a)[0]<<8))
# define ROUND(a)     ((a)+0.5)
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

#ifdef _WIN32
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

float        LinInterpolateRampU16   ( unsigned short    * ramp,
                                       int                 ramp_size,
                                       float               pos )
{
  unsigned short val1, val2;
  float start, dist, result;
  
  if(!ramp)
    return 0.0;
  
  if(pos < 0)
    return ramp[0];
    
  if(pos > ramp_size-1)
    return ramp[ramp_size-1];
  
  dist = modff( pos, &start );
  val1 = ramp[(int)start];
  val2 = ramp[(int)start+1];
    
  result = val2 - val1;
  result *= dist;
  result += val1;
    
  return result;
}


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
      message("mLUT found (Profile Mechanic) %s", filename);
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
      for(j=0; j<nEntries; j++)
      {
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
      message("vcgt found %s", filename);
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
      if(gammaType==1)
      {
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
        message("Red:   Gamma %f \tMin %f \tMax %f", rGamma, rMin, rMax);
        message("Green: Gamma %f \tMin %f \tMax %f", gGamma, gMin, gMax);
        message("Blue:  Gamma %f \tMin %f \tMax %f", bGamma, bMin, bMax);

        for(j=0; j<nEntries; j++)
        {
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
      else if(gammaType==0)
      {
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

        message ("channels:        \t%d", numChannels);
        message ("entry size:      \t%dbits",entrySize  * 8);
        message ("entries/channel: \t%d", numEntries);
        message ("tag size:        \t%d", tagSize);
                                                
        if(numChannels!=3)          /* assume we have always RGB */
          break;

        /* allocate tables for the file plus one entry for extrapolation */
        redRamp = (unsigned short *) malloc ((numEntries+1) * sizeof (unsigned short));
        greenRamp = (unsigned short *) malloc ((numEntries+1) * sizeof (unsigned short));
        blueRamp = (unsigned short *) malloc ((numEntries+1) * sizeof (unsigned short));
        {
          rMax = gMax = bMax = -1;
          rMin = gMin = bMin = 65536;
          for(j=0; j<numEntries; j++)
          {
            switch(entrySize)
            {
              case 1:
                bytesRead = fread(cTmp, 1, 1, fp);
                redRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                redRamp[j]= BE_SHORT(cTmp);
                break;
            }
            if(rMax < redRamp[j])
              rMax = redRamp[j];
            if(rMin > redRamp[j])
              rMin = redRamp[j];
          }
          for(j=0; j<numEntries; j++)
          {
            switch(entrySize)
            {
              case 1:
                bytesRead = fread(cTmp, 1, 1, fp);
                greenRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                greenRamp[j]= BE_SHORT(cTmp);
                break;
            }
            if(gMax < greenRamp[j])
              gMax = greenRamp[j];
            if(gMin > greenRamp[j])
              gMin = greenRamp[j];
          }
          for(j=0; j<numEntries; j++)
          {
            switch(entrySize)
            {
              case 1:                bytesRead = fread(cTmp, 1, 1, fp);
                blueRamp[j]= cTmp[0] << 8;
                break;
              case 2:
                bytesRead = fread(cTmp, 1, 2, fp);
                blueRamp[j]= BE_SHORT(cTmp);
                break;
            }
            if(bMax < blueRamp[j])
              bMax = blueRamp[j];
            if(bMin > blueRamp[j])
              bMin = blueRamp[j];
          }
        }
        if( abs(rMax-rMin) < 65535/20 &&
            abs(gMax-gMin) < 65535/20 &&
            abs(bMax-bMin) < 65535/20
          )
        {
          warning ("Contrast below 5%% in ICC profile '%s'", filename);
          warning ("min/max for red: %g / %g  green: %g / %g  blue: %g / %g", rMin, rMax, gMin, gMax, bMin, bMax );
          retVal = -1;
          break;
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
              rRamp[j*ratio+i] = (int)LinInterpolateRampU16( redRamp, numEntries, (j*ratio+i)*(double)(numEntries-1)/(double)(nEntries-1));
              gRamp[j*ratio+i] = (int)LinInterpolateRampU16( greenRamp, numEntries, (j*ratio+i)*(double)(numEntries-1)/(double)(nEntries-1));
              bRamp[j*ratio+i] = (int)LinInterpolateRampU16( blueRamp, numEntries, (j*ratio+i)*(double)(numEntries-1)/(double)(nEntries-1));
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

int          myMessage               ( int/*oyjlMSG_e*/    error_code,
                                       const void        * context_object OYJL_UNUSED,
                                       const char        * format,
                                       ... )
{
  int error = 0;
  const char * status_text = NULL;   
#if !defined (OYJL_ARGS_BASE)          
  char * text = NULL;                  
                                       
  OYJL_CREATE_VA_STRING(format, text, malloc, return 1)    
#endif /* OYJL_ARGS_BASE */

  if(!xcalib_state.verbose && error_code == oyjlMSG_INFO)
    return error;

  if(error_code == oyjlMSG_INFO) status_text = oyjlTermColor(oyjlGREEN,"Info: ");
  if(error_code == oyjlMSG_CLIENT_CANCELED) status_text = oyjlTermColor(oyjlBLUE,"Client Canceled: ");
  if(error_code == oyjlMSG_INSUFFICIENT_DATA) status_text = oyjlTermColor(oyjlRED,_("Insufficient Data:"));
  if(error_code == oyjlMSG_ERROR) status_text = oyjlTermColor(oyjlRED,_("Usage Error:"));
  if(error_code == oyjlMSG_PROGRAM_ERROR) status_text = oyjlTermColor(oyjlRED,_("Program Error:"));
  if(error_code == oyjlMSG_SECURITY_ALERT) status_text = oyjlTermColor(oyjlRED,_("Security Alert:"));

  if(status_text)
    fprintf( stderr, "%s ", status_text );
#if !defined (OYJL_ARGS_BASE)
  if(text)
    fprintf( stderr, "%s\n", text ); 
  free( text ); text = 0;
#else /* OYJL_ARGS_BASE */
  if(format)
    fprintf( stderr, "%s\n", format ); 
  oyjlArgsBaseLoadCore(); /* how to pass through va_args */
#endif /* OYJL_ARGS_BASE */            
  fflush( stderr );                    

  
  return error;
}

void myUsage( oyjlUi_s * ui )
{
  char * t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_HELP, 0 );
  if(t) { puts( t ); free(t); t = NULL; }
}


static oyjlOptionChoice_s * listInput ( oyjlOption_s * o OYJL_UNUSED, int * y OYJL_UNUSED, oyjlOptions_s * opts OYJL_UNUSED )
{   
  oyjlOptionChoice_s * c = NULL;

  int size = 0, i,n = 0;
  char * result = oyjlReadCommandF( &size, "r", malloc, "oyranos-profile oyjl-list" );
  char ** list = oyjlStringSplit2( result, "\n", NULL, &n, NULL,0 );

  if(list)
  {
    c = calloc(n+1, sizeof(oyjlOptionChoice_s));
    if(c)
    {
      for(i = 0; i < n; ++i)
      {
        c[i].nick = strdup( list[i] );
        c[i].name = strdup("");
        c[i].description = strdup("");
        c[i].help = strdup("");
      }
    }
    free(list);
  }

  return c;
}
static oyjlOptionChoice_s * listDisplay ( oyjlOption_s * o OYJL_UNUSED, int * y OYJL_UNUSED, oyjlOptions_s * opts OYJL_UNUSED )
{   
  oyjlOptionChoice_s * c = NULL;

  int size = 0, i,n = 0;
  char * result = oyjlReadCommandF( &size, "r", malloc, "echo $DISPLAY" );
  char ** list = oyjlStringSplit2( result, "\n", NULL, &n, NULL,0 );

  if(list)
  {
    c = calloc(n+1, sizeof(oyjlOptionChoice_s));
    if(c)
    {
      for(i = 0; i < n; ++i)
      {
        c[i].nick = strdup( list[i] );
        c[i].name = strdup("");
        c[i].description = strdup("");
        c[i].help = strdup("");
      }
    }
    free(list);
  }

  return c;
}
static oyjlOptionChoice_s * listScreen ( oyjlOption_s * o OYJL_UNUSED, int * y OYJL_UNUSED, oyjlOptions_s * opts OYJL_UNUSED )
{   
  oyjlOptionChoice_s * c = NULL;

  int size = 0, i,n = 0;
  char * result = oyjlReadCommandF( &size, "r", malloc, "xrandr | grep Screen" );
  char ** list = oyjlStringSplit2( result, "\n", NULL, &n, NULL,0 );

  if(list)
  {
    c = calloc(n+1, sizeof(oyjlOptionChoice_s));
    if(c)
    {
      for(i = 0; i < n; ++i)
      {
        if(!list[i][0]) break;
        c[i].nick = strdup( oyjlTermColorF(oyjlNO_MARK, "%d", i ) );
        c[i].name = strdup("");
        c[i].description = strdup("");
        c[i].help = strdup("");
      }
    }
    free(list);
  }

  return c;
}
static oyjlOptionChoice_s * listOutput ( oyjlOption_s * o OYJL_UNUSED, int * y OYJL_UNUSED, oyjlOptions_s * opts OYJL_UNUSED )
{   
  oyjlOptionChoice_s * c = NULL;

  int size = 0, i,n = 0;
  char * result = oyjlReadCommandF( &size, "r", malloc, "xrandr --listactivemonitors | grep x" );
  char ** list = oyjlStringSplit2( result, "\n", NULL, &n, NULL,0 );

  if(list)
  {
    c = calloc(n+1, sizeof(oyjlOptionChoice_s));
    if(c)
    {
      for(i = 0; i < n; ++i)
      {
        if(!list[i][0]) break;
        c[i].nick = strdup( oyjlTermColorF(oyjlNO_MARK, "%d", i ) );
        c[i].name = strdup("");
        c[i].description = strdup("");
        c[i].help = strdup("");
      }
    }
    free(list);
  }

  return c;
}

/* This function is called the
 * * first time for GUI generation and then
 * * for executing the tool.
 */
int myMain( int argc, const char ** argv )
{
  const char * in_name = NULL;
  int i;


  int error = 0;
  int state = 0;
  int clear = 0;
  const char * display = 0;
  const char * screen = 0;
  const char * output = 0;
#ifdef FGLRX
  int controller = -1;
#endif
  int verbose = 0;
  int invert = 0;
  const char * icc_file_name = 0;
  int alter = 0;
  double gamma_ = 0.0;
  double brightness = -1.0;
  double contrast = 0;
  double red_gamma = 0;
  double red_brightness = -1.0;
  double red_contrast = 0;
  double green_gamma = 0;
  double green_brightness = -1.0;
  double green_contrast = 0;
  double blue_gamma = 0;
  double blue_brightness = -1.0;
  double blue_contrast = 0;
  int noaction = 0;
  const char * printramps = 0;
  int loss = 0;
  const char * help = 0;
  int version = 0;
  const char * render = 0;
  const char * export_var = 0;

  /* handle options */
  /* Select a nick from *version*, *manufacturer*, *copyright*, *license*,
   * *url*, *support*, *download*, *sources*, *oyjl_module_author* and
   * *documentation*. Choose what you see fit. Add new ones as needed. */
  oyjlUiHeaderSection_s sections[] = {
    /* type, nick,            label, name,                     description */
    {"oihs", "version",       NULL,  _(XCALIB_VERSION),              NULL},
    {"oihs", "manufacturer",  NULL,  _("Stefan Dohla <stefan AT doehla DOT de>"),    _("http://www.etg.e‐technik.uni‐erlangen.de/web/doe/xcalib/")},
    {"oihs", "documentation", NULL,  NULL,                     _("The tool loads ’vcgt’‐tag of ICC profiles to the server using the XRandR/XVidMode/GDI Extension in order to load calibrate curves to your graphics card.")},
    {"oihs", "date",          NULL,  NULL,                     _("December 14, 2023")},
    {"",0,0,0,0}};

  /* declare the option choices  *   nick,          name,               description,                  help */
  oyjlOptionChoice_s p_choices[] = {{"TEXT",        "TEXT",             NULL,                         NULL},
                                    {"SVG",         "SVG",              NULL,                         NULL},
                                    {NULL,NULL,NULL,NULL}};
  oyjlOptionChoice_s E_choices[] = {{_("DISPLAY"),  _("Under X11 systems this variable will hold the display name as used for the -d and -s option."),NULL,NULL},
                                    {NULL,NULL,NULL,NULL}};

  oyjlOptionChoice_s A_choices[] = {{_("Assign the VCGT curves of a ICC profile to a screen"),_("xcalib ‐d :0 ‐s 0 ‐v profile_with_vcgt_tag.icc"),NULL,NULL},
                                    {_("Reset a screens hardware LUT in order to do a calibration"),_("xcalib ‐d :0 ‐s 0 ‐c"),NULL,NULL},
                                    {NULL,NULL,NULL,NULL}};

  oyjlOptionChoice_s L_choices[] = {{_("oyjl-args(1)"),NULL,            NULL,                         NULL},
                                    {NULL,NULL,NULL,NULL}};

  /* declare options - the core information; use previously declared choices */
  oyjlOption_s oarray[] = {
  /* type,   flags,                      o,  option,          key,      name,          description,                  help, value_name,         
        value_type,              values,             variable_type, variable_name, properties */
    {"oiwi", 0,                          "c","clear",         NULL,     _("Clear"),    _("Clear Gamma LUT"),         _("Reset the Video Card Gamma Table (VCGT) to linear values."),NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&clear},   NULL},
    {"oiwi", OYJL_OPTION_FLAG_EDITABLE,  "d","display",       NULL,     _("Display"),  _("host:dpy"),                NULL, _("STRING"),
        oyjlOPTIONTYPE_FUNCTION,   {.getChoices = listDisplay}, oyjlSTRING, {.s=&display},NULL},
    {"oiwi", OYJL_OPTION_FLAG_EDITABLE,  "s","screen",        NULL,     _("Screen"),   _("Screen Number"),           NULL, _("NUMBER"),
        oyjlOPTIONTYPE_FUNCTION,   {.getChoices = listScreen}, oyjlSTRING, {.s=&screen},NULL},
#ifdef FGLRX
    {"oiwi", OYJL_OPTION_FLAG_EDITABLE,  "x","controller",    NULL,     _("Controller"),   _("ATI Controller Index"),_("For FGLRX only"), _("NUMBER"),
        oyjlOPTIONTYPE_CHOICE,   {0},                oyjlINT,       {.i=&controller},  NULL},
#endif
    {"oiwi", OYJL_OPTION_FLAG_EDITABLE|OYJL_OPTION_FLAG_IMMEDIATE,  "o","output",        NULL,     _("Output"),   _("Output Number"),           _("It appears in the order as listed in xrandr tool."),_("NUMBER"),
        oyjlOPTIONTYPE_FUNCTION,   {.getChoices = listOutput}, oyjlSTRING, {.s=&output},NULL},
    {"oiwi", 0,                          "i","invert",        NULL,     _("Invert"),   _("Invert the LUT"),          NULL, NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&invert},  NULL},
    {"oiwi", OYJL_OPTION_FLAG_EDITABLE,  "@",NULL,            NULL,     _("ICC Profle"),_("File Name of a ICC Profile"),NULL,_("ICC_FILE_NAME"),
        oyjlOPTIONTYPE_FUNCTION,   {.getChoices = listInput}, oyjlSTRING, {.s=&icc_file_name},NULL},
    {"oiwi", 0,                          "a","alter",         NULL,     _("Alter"),    _("Alter Table"),             _("Works according to parameters without ICC Profile."),NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&alter},   NULL},
    {"oiwi", 0,                          "n","noaction",      NULL,     _("No Action"), _("Do not alter video-LUTs."),_("Work's best in conjunction with -v!"), NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&noaction},           NULL},
    {"oiwi", OYJL_OPTION_FLAG_ACCEPT_NO_ARG|OYJL_OPTION_FLAG_IMMEDIATE,"p","printramps", NULL,     _("Print Ramps"), _("Print Values on stdout."),NULL, _("FORMAT"),
        oyjlOPTIONTYPE_CHOICE,   {.choices.list = (oyjlOptionChoice_s*)oyjlStringAppendN( NULL, (const char*)p_choices, sizeof(p_choices), 0 )},                oyjlSTRING,       {.s=&printramps},        NULL},
    {"oiwi", 0,                          "l","loss",          NULL,     _("Loss"),     _("Print error introduced by applying ramps to stdout."),NULL, NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&loss},        NULL},
    {"oiwi", 0,                          "g","gamma",         NULL,     _("Gamma"),    _("Specify Gamma"),           _("Global gamma correction value (use 2.2 for WinXP Color Control-like behaviour)"), _("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 1, .start = 0.1, .end = 5, .tick = 0.1}},oyjlDOUBLE,{.d=&gamma_},NULL},
    {"oiwi", 0,                          "b","brightness",    NULL,     _("Brightness"),_("Specify Lightness Percentage"),NULL,_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 0, .start = 0.0, .end = 99, .tick = 1}},oyjlDOUBLE,{.d=&brightness},NULL},
    {"oiwi", 0,                          "k","contrast",      NULL,     _("Contrast"), _("Specify Contrast Percentage"),_("Set maximum value relative to brightness."),_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 100, .start = 1.0, .end = 100, .tick = 1}},oyjlDOUBLE,{.d=&contrast},NULL},
    {"oiwi", 0,                          "R","red-gamma",     NULL,     _("Red Gamma"),_("Specify Red Gamma "),      NULL, _("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 1, .start = 0.1, .end = 5, .tick = 0.1}},oyjlDOUBLE,{.d=&red_gamma},NULL},
    {"oiwi", OYJL_OPTION_FLAG_IMMEDIATE, "S","red-brightness",NULL,     _("Red Brightness"),_("Specify Red Brightness Percentage"),NULL,_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 0.0, .start = 0.0, .end = 99, .tick = 1}},oyjlDOUBLE,{.d=&red_brightness},NULL},
    {"oiwi", OYJL_OPTION_FLAG_IMMEDIATE, "T","red-contrast",  NULL,     _("Red Contrast"),_("Specify Red Contrast Percentage"),_("Set maximum value relative to brightness."),_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 100, .start = 1.0, .end = 100, .tick = 1}},oyjlDOUBLE,{.d=&red_contrast},NULL},
    {"oiwi", 0,                          "G","green-gamma",   NULL,     _("Green Gamma"),_("Specify Green Gamma "),  NULL, _("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 1.0, .start = 0.1, .end = 5, .tick = 0.1}},oyjlDOUBLE,{.d=&green_gamma},NULL},
    {"oiwi", OYJL_OPTION_FLAG_IMMEDIATE, "H","green-brightness",NULL,   _("Green Brightness"),_("Specify Green Brightness Percentage"),NULL,_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 0, .start = 0.0, .end = 99, .tick = 1}},oyjlDOUBLE,{.d=&green_brightness},NULL},
    {"oiwi", OYJL_OPTION_FLAG_IMMEDIATE, "I","green-contrast",NULL,     _("Green Contrast"),_("Specify Green Contrast Percentage"),_("Set maximum value relative to brightness."),_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 100, .start = 1.0, .end = 100, .tick = 1}},oyjlDOUBLE,{.d=&green_contrast},NULL},
    {"oiwi", 0,                          "B","blue-gamma",    NULL,     _("Blue Gamma"),_("Specify Blue Gamma "),    NULL, _("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 1.0, .start = 0.1, .end = 5, .tick = 0.1}},oyjlDOUBLE,{.d=&blue_gamma},NULL},
    {"oiwi", OYJL_OPTION_FLAG_IMMEDIATE, "C","blue-brightness",NULL,    _("Blue Brightness"),_("Specify Blue Brightness Percentage"),NULL,_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 0, .start = 0.0, .end = 99, .tick = 1}},oyjlDOUBLE,{.d=&blue_brightness},NULL},
    {"oiwi", OYJL_OPTION_FLAG_IMMEDIATE, "D","blue-contrast", NULL,     _("Blue Contrast"),_("Specify Blue Contrast Percentage"),_("Set maximum value relative to brightness."),_("NUMBER"),
        oyjlOPTIONTYPE_DOUBLE,   {.dbl = {.d = 100, .start = 1.0, .end = 100, .tick = 1}},oyjlDOUBLE,{.d=&blue_contrast},NULL},
    {"oiwi", OYJL_OPTION_FLAG_ACCEPT_NO_ARG, "h","help",      NULL,     NULL,          NULL,                         NULL, NULL,
        oyjlOPTIONTYPE_CHOICE,   {0},                oyjlSTRING,    {.s=&help}, NULL},
    {"oiwi", 0,                        NULL, "synopsis",      NULL,     NULL,          NULL,                         NULL, NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlNONE,      {0}, NULL },
    {"oiwi", 0,                          "v","verbose",       NULL,     _("Verbose"),  _("Verbose"),                 NULL, NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&verbose}, NULL},
    {"oiwi", 0,                          "V","version",       NULL,     _("Version"),  _("Version"),                 NULL, NULL,
        oyjlOPTIONTYPE_NONE,     {0},                oyjlINT,       {.i=&version}, NULL},
    {"oiwi", OYJL_OPTION_FLAG_EDITABLE,  NULL,"render",       NULL,     _("Render"),   NULL,                         NULL, _("STRING"),
        oyjlOPTIONTYPE_CHOICE,   {0},                oyjlSTRING,    {.s=&render},  NULL},
    {"oiwi", 0,                          "E","man-environment_variables",NULL,    NULL,NULL,                   NULL, NULL,
        oyjlOPTIONTYPE_CHOICE,   {.choices = {(oyjlOptionChoice_s*) oyjlStringAppendN( NULL, (const char*)E_choices, sizeof(E_choices), malloc ), 0}},oyjlNONE,{0},NULL},
    {"oiwi", 0,                          "A","man-examples",  NULL,     NULL, NULL,                      NULL, NULL,
        oyjlOPTIONTYPE_CHOICE,   {.choices = {(oyjlOptionChoice_s*) oyjlStringAppendN( NULL, (const char*)A_choices, sizeof(A_choices), malloc ), 0}},oyjlNONE,{0},NULL},
    {"oiwi", 0,                          "L","man-see_also",  NULL,     NULL, NULL,                      NULL, NULL,
        oyjlOPTIONTYPE_CHOICE,   {.choices = {(oyjlOptionChoice_s*) oyjlStringAppendN( NULL, (const char*)L_choices, sizeof(L_choices), malloc ), 0}},oyjlNONE,{0},NULL},
    /* default option template -X|--export */
    {"oiwi", 0, "X", "export", NULL, NULL, NULL, NULL, NULL, oyjlOPTIONTYPE_CHOICE, {.choices = {NULL, 0}}, oyjlSTRING, {.s=&export_var}, NULL },
    {"",0,0,NULL,NULL,NULL,NULL,NULL, NULL, oyjlOPTIONTYPE_END, {0},oyjlNONE,{0},0}
  };

  /* declare option groups, for better syntax checking and UI groups */
  oyjlOptionGroup_s groups[] = {
  /* type,   flags, name,               description,                  help,               mandatory,     optional,      detail,        properties */
    {"oiwg", 0,     NULL,               _("Set basic parameters"),    NULL,               NULL,          NULL,          "d,s,o,a,n,p,l", NULL},
    {"oiwg", 0,     NULL,               _("Assign"),                  NULL,               "@",           NULL,          "@",           NULL},
    {"oiwg", 0,     NULL,               _("Clear"),                   NULL,               "c,d,s",       "o,v",         "c",           NULL},
    {"oiwg", 0,     NULL,               _("Invert"),                  NULL,               "i,d,s,@|a",   "o,v,n,p,l",   "i",           NULL},
    {"oiwg", 0,     NULL,               _("Overall Appearance"),      NULL,               "g,b,k,d,s,@|a","o,v,n,p,l",  "g,b,k",       NULL},
    {"oiwg", 0,     NULL,               _("Per Channel Appearance"),  NULL,               "R,G,B,d,s,@|a","S,T,H,I,C,D,o,v,n,p,l","R,S,T,G,H,I,B,C,D",NULL},
    {"oiwg", 0,     NULL,               _("Show"),                    NULL,               "p,d,s",       "o,v",         "p",           NULL},
    {"oiwg", 0,     _("Misc"),          _("General options"),         NULL,               "h,V,render",  "v",           "h,render,V,v",NULL},
    {"",0,0,0,0,0,0,0,0}
  };

  oyjlUi_s * ui = oyjlUi_Create( argc, argv, /* argc+argv are required for parsing the command line options */
                                       "xcalib", _("Monitor Calibration Loader"), _("Tiny monitor calibration loader for Xorg and Windows."),
#ifdef __ANDROID__
                                       ":/images/logo.svg", // use qrc
#else
                                       "xcalib",
#endif
                                       sections, oarray, groups, &state );
  if( state & oyjlUI_STATE_EXPORT && !ui )
    goto clean_main;
  if(state & oyjlUI_STATE_HELP)
  {
    fprintf( stderr, "%s\n\tman xcalib\n\n", _("For more information read the man page:") );
    goto clean_main;
  }

  if(ui && verbose)
  {
#ifndef INCLUDE_OYJL_C
    char * json = oyjlOptions_ResultsToJson( ui->opts, OYJL_JSON );
    if(json)
      fputs( json, stderr );
    fputs( "\n", stderr );
#endif

    int count = 0;
    char ** results = oyjlOptions_ResultsToList( ui->opts, NULL, &count );
    for(i = 0; i < count; ++i) fprintf( stderr, "%s\n", results[i] );
    oyjlStringListRelease( &results, count, free );
    fputs( "\n", stderr );
  }

  if(ui && (export_var && strcmp(export_var,"json+command") == 0))
  {
    char * json = oyjlUi_ToText( ui, oyjlARGS_EXPORT_JSON, 0 ),
         * json_commands = NULL;
    oyjlStringAdd( &json_commands, malloc, free, "{\n  \"command_set\": \"%s\",", argv[0] );
    oyjlStringAdd( &json_commands, malloc, free, "%s", &json[1] ); /* skip opening '{' */
    puts( json_commands );
    goto clean_main;
  }

  /* Render boilerplate */
  if(ui && render)
  {
#if !defined(NO_OYJL_ARGS_RENDER)
    int debug = verbose;
    oyjlTermColorInit( OYJL_RESET_COLORTERM | OYJL_FORCE_COLORTERM ); /* show rich text format on non GNU color extension environment */
    oyjlArgsRender( argc, argv, NULL, NULL,NULL, debug, ui, myMain );
#else
    fprintf( stderr, "No render support compiled in. For a GUI you might by able to use -X json+command and load into oyjl-args-render viewer.\n" );
#endif
  } else if(ui)
  {
    /* ... working code goes here ... */
  u_int16_t *r_ramp = NULL, *g_ramp = NULL, *b_ramp = NULL;
  int i;
  int donothing = noaction;
  int calcloss = loss;
  int correction = 0;
  u_int16_t tmpRampVal = 0;
  unsigned int r_res, g_res, b_res;
  in_name = icc_file_name;

#ifdef FGLRX
  unsigned
#endif
           int ramp_size = 256;

#ifndef _WIN32
  /* X11 */
  XF86VidModeGamma gamma;
  Display *dpy = NULL;
  char *displayname = display;
  int xoutput = output?atoi(output):0;
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

  xcalib_state.verbose = verbose;

  /* begin program part */
#ifdef _WIN32
  for(i=0; i< ramp_size; i++) {
    winGammaRamp.Red[i] = i << 8;
    winGammaRamp.Blue[i] = i << 8;
    winGammaRamp.Green[i] = i << 8;
  }
#endif

  /* command line parsing */
  
#ifndef _WIN32
  if (argc < 2)
    usage ();
#endif

    if(gamma_ != 0.0)
    {
      xcalib_state.gamma_cor = gamma_;
      if(xcalib_state.verbose)
        message ("gamma: %f", xcalib_state.gamma_cor);
      correction = 1;
    }
    /* take additional brightness into account */
    if (brightness != -1.0) {
      xcalib_state.redMin = xcalib_state.greenMin = xcalib_state.blueMin = brightness / 100.0;
      xcalib_state.redMax = xcalib_state.greenMax = xcalib_state.blueMax =
        (1.0 - xcalib_state.blueMin) * xcalib_state.blueMax + xcalib_state.blueMin;
      
      correction = 1;
    }
    /* take additional contrast into account */
    if (contrast != 0.0) {
      xcalib_state.redMax = xcalib_state.greenMax = xcalib_state.blueMax = contrast / 100.0;
      xcalib_state.redMax = xcalib_state.greenMax = xcalib_state.blueMax =
        (1.0 - xcalib_state.blueMin) * xcalib_state.blueMax + xcalib_state.blueMin;
 
      correction = 1;
    }
    /* additional red calibration */ 
    if (red_gamma != 0.0) {
      double gamma = red_gamma,
             brightness = red_brightness != -1.0 ? red_brightness : 0.0,
             contrast = red_contrast != 0.0 ? red_contrast : 100.0;
 
      xcalib_state.redMin = brightness / 100.0;
      xcalib_state.redMax =
        (1.0 - xcalib_state.redMin) * (contrast / 100.0) + xcalib_state.redMin;
      xcalib_state.redGamma = gamma;
 
      correction = 1;
    }
    /* additional green calibration */
    if (green_gamma != 0.0) {
      double gamma = green_gamma,
             brightness = green_brightness != -1.0 ? green_brightness : 0.0,
             contrast = green_contrast != 0.0 ? green_contrast : 100.0;
 
      xcalib_state.greenMin = brightness / 100.0;
      xcalib_state.greenMax =
        (1.0 - xcalib_state.greenMin) * (contrast / 100.0) + xcalib_state.greenMin;
      xcalib_state.greenGamma = gamma;
 
      if(xcalib_state.verbose)
        message ("green gamma: %f %f %f", gamma, brightness, contrast);
      correction = 1;
    }
    /* additional blue calibration */
    if (blue_gamma != 0.0) {
      double gamma = blue_gamma,
             brightness = blue_brightness != -1.0 ? blue_brightness : 0.0,
             contrast = blue_contrast != 0.0 ? blue_contrast : 100.0;
 
      xcalib_state.blueMin = brightness / 100.0;
      xcalib_state.blueMax =
        (1.0 - xcalib_state.blueMin) * (contrast / 100.0) + xcalib_state.blueMin;
      xcalib_state.blueGamma = gamma;
 
      correction = 1;
    }
 
#ifdef _WIN32
  if ((!clear || !alter) && (!in_name || in_name[0] == '\0')) {
    hDc = FindMonitor(atoi(screen));
    win_profile_len = MAX_PATH;
    win_default_profile[0] = '\0';
    SetICMMode(hDc, ICM_ON);
    if(GetICMProfileA(hDc, (LPDWORD) &win_profile_len, (LPSTR)win_default_profile))
    {
      if(strlen(win_default_profile) < 255)
        in_name = win_default_profile;
      else
        usage();
    }
    else
      usage();
  }
#endif

#ifndef _WIN32
  /* X11 initializing */
  int scr = 0;
  if ((dpy = XOpenDisplay (displayname)) == NULL) {
    if(!donothing)
      error ("Can't open display %s", XDisplayName (displayname));
    else
      warning("Can't open display %s", XDisplayName (displayname));
  }
  else if (!screen)
    scr = DefaultScreen (dpy);

  int xrr_version = -1;
  int crtc = 0;
  int major_versionp = 0;
  int minor_versionp = 0;
  int n = 0;
  Window root = RootWindow(dpy, scr);

  XRRQueryVersion( dpy, &major_versionp, &minor_versionp );
  xrr_version = major_versionp*100 + minor_versionp;

  if(xrr_version >= 102)
  {                           
    XRRScreenResources * res = XRRGetScreenResources( dpy, root );
    int ncrtc = 0;

    n = res->noutput;
    for( i = 0; i < n; ++i )
    {
      RROutput output = res->outputs[i];
      XRROutputInfo * output_info = XRRGetOutputInfo( dpy, res,
                                                        output);
      if(output_info->crtc)
        if(ncrtc++ == xoutput)
        {
          crtc = output_info->crtc;
          ramp_size = XRRGetCrtcGammaSize( dpy, crtc );
          message ("XRandR output:      \t%s", output_info->name);
        }

      XRRFreeOutputInfo( output_info ); output_info = 0;
    }
    //XRRFreeScreenResources(res); res = 0;
  }

  /* clean gamma table if option set */
  gamma.red = 1.0;
  gamma.green = 1.0;
  gamma.blue = 1.0;
  if (clear) {
#ifndef FGLRX
    if(xrr_version >= 102)
    {
      XRRCrtcGamma * gamma = XRRAllocGamma (ramp_size);
      if(!gamma)
        warning ("Unable to clear screen gamma. %s", output);
      else
      {
        for(i=0; i < ramp_size; ++i)
          gamma->red[i] = gamma->green[i] = gamma->blue[i] = i * 65535 / ramp_size;
        XRRSetCrtcGamma (dpy, crtc, gamma);
        XRRFreeGamma (gamma);
      }
    } else
    if (!XF86VidModeSetGamma (dpy, scr, &gamma))
    {
#else
    for(i = 0; i < 256; i++) {
      fglrx_gammaramps.RGamma[i] = i << 2;
      fglrx_gammaramps.GGamma[i] = i << 2;
      fglrx_gammaramps.BGamma[i] = i << 2;
    }
    if (!FGLRX_X11SetGammaRamp_C16native_1024(dpy, scr, controller, 256, &fglrx_gammaramps)) {
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
    if (xrr_version < 102 && !XF86VidModeGetGammaRampSize (dpy, scr, &ramp_size)) {
#else
    if (!FGLRX_X11GetGammaRampSize(dpy, scr, &ramp_size)) {
#endif
      XCloseDisplay (dpy);
      if(!donothing)
        error ("Unable to query gamma ramp size");
      else {
        warning ("Unable to query gamma ramp size - assuming 256 | %d", ramp_size);
        ramp_size = 256;
      }
    }
  }
#else /* _WIN32 */
  if(!donothing) {
    if(!hDc)
      hDc = FindMonitor(scr);
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

  int has_name = in_name && in_name[0] != '\000';
  int print_only = printramps && !has_name;
  if(!alter && !print_only)
  {
    if( (i = read_vcgt_internal(in_name, r_ramp, g_ramp, b_ramp, ramp_size)) <= 0) {
      if(i<0)
        warning ("Unable to read file \"%s\"", in_name?in_name:"----");
      if(i == 0)
        warning ("No calibration data in ICC profile '%s' found", in_name);
      free(r_ramp);
      free(g_ramp);
      free(b_ramp);
      return 0;
    }
  } else {
#ifndef _WIN32
    if (xrr_version >= 102)
    {
      XRRCrtcGamma * gamma = 0;
      if((gamma = XRRGetCrtcGamma(dpy, crtc)) == 0 )
        warning ("XRRGetCrtcGamma() is unable to get display calibration", output );

      for (i = 0; i < ramp_size; i++) {
        r_ramp[i] = gamma->red[i];
        g_ramp[i] = gamma->green[i];
        b_ramp[i] = gamma->blue[i];
      }
    }
    else if (!XF86VidModeGetGammaRamp (dpy, scr, ramp_size, r_ramp, g_ramp, b_ramp))
      warning ("XF86VidModeGetGammaRamp() is unable to get display calibration", output);
#else
    if (!GetDeviceGammaRamp(hDc, &winGammaRamp))
      warning ("GetDeviceGammaRamp() is unable to get display calibration", output);

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

    redMin = (double)r_ramp[0] / 65535.0;
    redMax = (double)r_ramp[ramp_size - 1] / 65535.0;
    redBrightness = redMin * 100.0;
    redContrast = (redMax - redMin) / (1.0 - redMin) * 100.0; 
    message("Red Brightness: %f   Contrast: %f  Max: %f  Min: %f", redBrightness, redContrast, redMax, redMin);
  }

  {
    float greenBrightness = 0.0;
    float greenContrast = 100.0;
    float greenMin = 0.0;
    float greenMax = 1.0;

    greenMin = (double)g_ramp[0] / 65535.0;
    greenMax = (double)g_ramp[ramp_size - 1] / 65535.0;
    greenBrightness = greenMin * 100.0;
    greenContrast = (greenMax - greenMin) / (1.0 - greenMin) * 100.0; 
    message("Green Brightness: %f   Contrast: %f  Max: %f  Min: %f", greenBrightness, greenContrast, greenMax, greenMin);
  }

  {
    float blueBrightness = 0.0;
    float blueContrast = 100.0;
    float blueMin = 0.0;
    float blueMax = 1.0;

    blueMin = (double)b_ramp[0] / 65535.0;
    blueMax = (double)b_ramp[ramp_size - 1] / 65535.0;
    blueBrightness = blueMin * 100.0;
    blueContrast = (blueMax - blueMin) / (1.0 - blueMin) * 100.0; 
    message("Blue Brightness: %f   Contrast: %f  Max: %f  Min: %f", blueBrightness, blueContrast, blueMax, blueMin);
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
    message("Altering Red LUTs with   Gamma %f   Min %f   Max %f",
       xcalib_state.redGamma, xcalib_state.redMin, xcalib_state.redMax);
    message("Altering Green LUTs with   Gamma %f   Min %f   Max %f",
       xcalib_state.greenGamma, xcalib_state.greenMin, xcalib_state.greenMax);
    message("Altering Blue LUTs with   Gamma %f   Min %f   Max %f",
       xcalib_state.blueGamma, xcalib_state.blueMin, xcalib_state.blueMax);
  }

  if(!invert) {
    /* ramps should be increasing - otherwise content is nonsense! */
    for (i = 0; i < ramp_size - 1; i++) {
      if (r_ramp[i + 1] < r_ramp[i])
        warning ("red gamma table not increasing [%d]%d %d", i, r_ramp[i], r_ramp[i + 1]);
      if (g_ramp[i + 1] < g_ramp[i])
        warning ("green gamma table not increasing [%d]%d %d", i, r_ramp[i], r_ramp[i + 1]);
      if (b_ramp[i + 1] < b_ramp[i])
        warning ("blue gamma table not increasing [%d]%d %d", i, r_ramp[i], r_ramp[i + 1]);
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
    char * tr = NULL, * tg = NULL, * tb = NULL;
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
    myMessage( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT "%s %d  %s %d  %s %d  colors lost", OYJL_DBG_ARGS, oyjlTermColorPtr(oyjlRED, &tr, "R:"), ramp_size - r_res, oyjlTermColorPtr(oyjlGREEN, &tg, "G:"), ramp_size - g_res, oyjlTermColorPtr(oyjlBLUE, &tb, "B:"), ramp_size - b_res );
    free(tr); free(tg); free(tb);
  }
#ifdef _WIN32
  for (i = 0; i < ramp_size; i++) {
    winGammaRamp.Red[i] = r_ramp[i];
    winGammaRamp.Green[i] = g_ramp[i];
    winGammaRamp.Blue[i] = b_ramp[i];
  }

#endif
 
  if(printramps)
  {
    if(strcasecmp(printramps, "svg") == 0)
    {
#ifdef OYJL_HAVE_LOCALE_H
      char * save_locale = oyjlStringCopy( setlocale(LC_NUMERIC, 0 ), malloc );
      setlocale(LC_NUMERIC, "C");
#endif
      char * svg = oyjlStringCopy("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",0);
      /* header */
      oyjlStringAdd( &svg, 0,0, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"1024\" height=\"1024\" viewBox=\"0 0 1024 1024\">\n");
      /* background rectangle */
      oyjlStringAdd( &svg, 0,0, "<rect x=\"-102.4\" y=\"-102.4\" width=\"1228.8\" height=\"1228.8\" fill=\"rgb(67.08324%, 67.07561%, 67.084765%)\" fill-opacity=\"0.5\"/>\n");
      /* add frame */
      oyjlStringAdd( &svg, 0,0, "<path fill=\"none\" stroke-width=\"5.6\" stroke=\"rgb(0%, 0%, 0%)\" d=\"M 25.398438 25.398438 L 998.199219 25.398438 L 998.199219 998.199219 L 25.398438 998.199219 Z M 25.398438 25.398438 \"/>\n");
      /* red curve */
      oyjlStringAdd( &svg, 0,0, "<path fill=\"none\" stroke-width=\"4\" stroke-linecap=\"butt\" stroke-linejoin=\"miter\" stroke=\"rgb(100%, 0%, 0%)\" d=\"");
      for(i=0; i<ramp_size; i++)
        oyjlStringAdd( &svg, 0,0, "%s %g %g ", i==0?"M":"L", 51.5 + (973.5-51.5) / ramp_size * i, 973.5 - r_ramp[i]/65535.0 * (973.5-51.5) );
      oyjlStringAdd( &svg, 0,0, " \"/>\n" );
      /* green curve */
      oyjlStringAdd( &svg, 0,0, "<path fill=\"none\" stroke-width=\"4\" stroke-linecap=\"butt\" stroke-linejoin=\"miter\" stroke=\"rgb(0%, 100%, 0%)\" d=\"");
      for(i=0; i<ramp_size; i++)
        oyjlStringAdd( &svg, 0,0, "%s %g %g ", i==0?"M":"L", 51.5 + (973.5-51.5) / ramp_size * i, 973.5 - g_ramp[i]/65535.0 * (973.5-51.5) );
      oyjlStringAdd( &svg, 0,0, " \"/>\n" );
      /* blue curve */
      oyjlStringAdd( &svg, 0,0, "<path fill=\"none\" stroke-width=\"4\" stroke-linecap=\"butt\" stroke-linejoin=\"miter\" stroke=\"rgb(0%, 0%, 100%)\" d=\"");
      for(i=0; i<ramp_size; i++)
        oyjlStringAdd( &svg, 0,0, "%s %g %g ", i==0?"M":"L", 51.5 + (973.5-51.5) / ramp_size * i, 973.5 - b_ramp[i]/65535.0 * (973.5-51.5) );
      oyjlStringAdd( &svg, 0,0, " \"/>\n" );
      /* close */
      oyjlStringAdd( &svg, 0,0, "</svg>\n" );
#ifdef OYJL_HAVE_LOCALE_H
      setlocale(LC_NUMERIC, save_locale);
      if(save_locale) free( save_locale );
#endif
      fputs( svg, stdout );
      free(svg);
    }
    else
    {
      for(i=0; i<ramp_size; i++)
        fprintf(stdout,"%d %d %d\n", r_ramp[i], g_ramp[i], b_ramp[i]);
    }
  }

  if(!donothing) {
    /* write gamma ramp to X-server */
#ifndef _WIN32
# ifdef FGLRX
    for(i = 0; i < ramp_size; i++) {
      fglrx_gammaramps.RGamma[i] = r_ramp[i] >> 6;
      fglrx_gammaramps.GGamma[i] = g_ramp[i] >> 6;
      fglrx_gammaramps.BGamma[i] = b_ramp[i] >> 6;
    }
    if (!FGLRX_X11SetGammaRamp_C16native_1024(dpy, scr, controller, ramp_size, &fglrx_gammaramps))
# else
    if(xrr_version >= 102)
    {
      XRRCrtcGamma * gamma = XRRAllocGamma (ramp_size);
      if(!gamma)
        warning ("Unable to calibrate display", output);
      else
      {
        for(i=0; i < ramp_size; ++i)
        {
          gamma->red[i] = r_ramp[i];
          gamma->green[i] = g_ramp[i];
          gamma->blue[i] = b_ramp[i];
        }
        XRRSetCrtcGamma (dpy, crtc, gamma);
        XRRFreeGamma (gamma);
      }
    } else
    if (!XF86VidModeSetGammaRamp (dpy, scr, ramp_size, r_ramp, g_ramp, b_ramp))
# endif
#else
    if (!SetDeviceGammaRamp(hDc, &winGammaRamp))
#endif
      warning ("Unable to calibrate display", output);
  }

  message ("X-LUT size:      \t%d", ramp_size);

  free(r_ramp);
  free(g_ramp);
  free(b_ramp);

cleanupX:
#ifndef _WIN32
  if(dpy)
    if(!donothing)
      XCloseDisplay (dpy);
#endif

  }
  else error = 1;

  clean_main:
  {
    int i = 0;
    while(oarray[i].type[0])
    {
      if(oarray[i].value_type == oyjlOPTIONTYPE_CHOICE && oarray[i].values.choices.list)
        free(oarray[i].values.choices.list);
      ++i;
    }
  }
  oyjlUi_Release( &ui );

  return error;
}

extern int * oyjl_debug;
char ** environment = NULL;
int main( int argc_, char**argv_, char ** envv )
{
  int argc = argc_;
  char ** argv = argv_;
  oyjlTranslation_s * trc_ = NULL;
  const char * loc = NULL;
  const char * lang;

#ifdef __ANDROID__
  argv = calloc( argc + 2, sizeof(char*) );
  memcpy( argv, argv_, (argc + 2) * sizeof(char*) );
  argv[argc++] = "--render=gui"; /* start Renderer (e.g. QML) */
  environment = environ;
#else
  environment = envv;
#endif

  /* language needs to be initialised before setup of data structures */
  int use_gettext = 0;
#ifdef OYJL_HAVE_LIBINTL_H
  use_gettext = 1;
#endif
#ifdef OYJL_HAVE_LOCALE_H
  loc = oyjlSetLocale(LC_ALL,"");
#endif
  lang = getenv("LANG");
  if(!loc)
  {
    loc = lang;
    fprintf( stderr, "%s", oyjlTermColor(oyjlRED,"Usage Error:") );
    fprintf( stderr, " Environment variable possibly not correct. Translations might fail - LANG=%s\n", oyjlTermColor(oyjlBOLD,lang) );
  }
  if(lang)
    loc = lang;

  if(loc)
  {
    const char * my_domain = MY_DOMAIN;
# include "xcalib.i18n.h"
    int size = sizeof(xcalib_i18n_oiJS);
    oyjl_val static_catalog = (oyjl_val) oyjlStringAppendN( NULL, (const char*) xcalib_i18n_oiJS, size, malloc );
    if(my_domain && strcmp(my_domain,"oyjl") == 0)
      my_domain = NULL;
    trc = trc_ = oyjlTranslation_New( loc, my_domain, &static_catalog, 0,0,0,0 );
  }
  oyjlInitLanguageDebug( "xcalib", NULL, NULL, use_gettext, NULL, NULL, &trc_, NULL );
  if(MY_DOMAIN && strcmp(MY_DOMAIN,"oyjl") == 0)
    trc = oyjlTranslation_Get( MY_DOMAIN );

  myMain(argc, (const char **)argv);

  oyjlTranslation_Release( &trc_ );
  oyjlLibRelease();

#ifdef __ANDROID__
  free( argv );
#endif

  return 0;
}


