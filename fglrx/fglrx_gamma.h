/*****************************************************************************/
/*                                                                           */
/* NAME:      fglrx_gamma.h                                                  */
/*                                                                           */
/*            FGLRXGAMMA extension interface library                         */
/*                                                                           */
/* Copyright (c) 2002,2003  ATI Research, Starnberg, Germany                 */
/*                                                                           */
/*****************************************************************************/

#ifndef __FGLRX_GAMMA_H__
#define __FGLRX_GAMMA_H__

//////////////////////////////////////////////////////////////////////////////
// includes

#include <X11/extensions/xf86vmode.h>


//////////////////////////////////////////////////////////////////////////////
// macro defines

// boolean constants
#define FALSE   (1==0)
#define TRUE    (1==1)

// ordinal constants
#define FGLRX_GAMMA_RAMP_SIZE     256


//////////////////////////////////////////////////////////////////////////////
// type defines

// basic types
typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef unsigned int  UINT;
typedef unsigned int HANDLE;
#ifndef ULONG_PTR
typedef ULONG ULONG_PTR;
#endif // ULONG_PTR

// structured types
typedef XF86VidModeGamma FGLRX_X11Gamma_float;

typedef struct {
    UINT    red;                 /* red color value for gamma correction table */
    UINT    green;               /* green color value for gamma correction table */
    UINT    blue;                /* blue color value for gamma correction table */
} FGLRX_X11Gamma_UINT, FGLRX_X11Gamma_uint_1024;

typedef struct {
    CARD16  red;                 /* red color value for gamma correction table */
    CARD16  green;               /* green color value for gamma correction table */
    CARD16  blue;                /* blue color value for gamma correction table */
} FGLRX_X11Gamma_C16, FGLRX_X11Gamma_C16_1024;

typedef struct {
    CARD16  RGamma[FGLRX_GAMMA_RAMP_SIZE];    /* red color value for gamma correction table */
    CARD16  GGamma[FGLRX_GAMMA_RAMP_SIZE];    /* green color value for gamma correction table */
    CARD16  BGamma[FGLRX_GAMMA_RAMP_SIZE];    /* blue color value for gamma correction table */
} FGLRX_X11Gamma_C16native, FGLRX_X11Gamma_C16native_1024;


//////////////////////////////////////////////////////////////////////////////
// exported functions from libfglrx_gamma.h

extern Bool FGLRX_X11SetGammaRamp_float
    (Display *dpy, int screen, int controller, int size, FGLRX_X11Gamma_float *Gamma);
extern Bool FGLRX_X11SetGammaRamp_uint_1024
    (Display *dpy, int screen, int controller, int size, FGLRX_X11Gamma_uint_1024 *Gamma);
extern Bool FGLRX_X11SetGammaRamp_C16_1024
    (Display *dpy, int screen, int controller, int size, FGLRX_X11Gamma_C16_1024 *Gamma);
extern Bool FGLRX_X11SetGammaRamp_C16native_1024
    (Display *dpy, int screen, int controller, int size, FGLRX_X11Gamma_C16native_1024 *Gamma);

extern Bool FGLRX_X11GetGammaRampSize
    (Display *dpy, int screen, int *size);

#endif /* __FGLRX_GAMMA_H__ */

//////////////////////////////////////////////////////////////////////////////
// EOF
