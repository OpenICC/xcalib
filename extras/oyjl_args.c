/** @file oyjl_args.c
 *
 *  oyjl - UI helpers
 *
 *  @par Copyright:
 *            2018-2023 (C) Kai-Uwe Behrmann
 *
 *  @brief    Oyjl argument handling
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *
 * Copyright (c) 2018-2022  Kai-Uwe Behrmann  <ku.b@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * This file can be included alone without any other file from the project.
 * Then it provides init, i18n, i/o and cli parsing capabilities.
 * With the OYJL_ARGS_BASE macro set, it omits init and i18n for smaller
 * code. That might be helpful, in case init and i18n are provided by other
 * means. The functionality fits then the oyjl_args_base.h header file and
 * the libOyjlArgsBase shared library.
 */

#ifndef OYJL_ARGS_C
#define OYJL_ARGS_C

#include <stddef.h>
#include <stdlib.h> /* malloc() */
#include <stdio.h>
#include <string.h> /* fprintf() */
#include <ctype.h> /* toupper() */
#include <wchar.h>  /* wcslen() */
#include <stdarg.h>

/* C++ includes and definitions */
#ifdef __cplusplus
#include <cmath>
#define USE_NEW
#else
#include <math.h>
#endif


#if defined(OYJL_INTERNAL) && !defined(OYJL_ARGS_BASE)
int oyjl_internal = 1;
#include "oyjl.h"
#include "oyjl_macros.h"
#include "oyjl_version.h"
#include "oyjl_git_version.h"
#ifdef OYJL_HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef OYJL_HAVE_LANGINFO_H
#include <langinfo.h> /* nl_langinfo() */
#endif
extern int * oyjl_debug;
#include "oyjl_i18n_internal.h"
#include "oyjl_tree_internal.h"

#else /* OYJL_INTERNAL */

#ifndef DOXYGEN /* doxygen shall skip this undocumented fragments */
#include <stdint.h> /* intptr_t */
#ifdef OYJL_HAVE_LOCALE_H
#include <locale.h> /* setlocale LC_NUMERIC */
#endif
#ifdef OYJL_HAVE_LANGINFO_H
#include <langinfo.h> /* nl_langinfo() */
#endif
#ifndef OYJL_VERSION
#define OYJL_VERSION 100
#define OYJL_VERSION_NAME "1.0.0"
#define OYJL_GIT_VERSION  ""
#define OYJL_VERSION_A 1	/**< version variable */
#define OYJL_DOMAIN  "oyjl"
#endif
#ifndef OYJL_LOCALEDIR
#define OYJL_LOCALEDIR  ""
#endif
#define      oyjlIsString_m( text ) (text&&text[0])
int my_debug = 0;
int * oyjl_debug = &my_debug;
#if   (__GNUC__*100 + __GNUC_MINOR__) >= 406
#define OYJL_UNUSED                    __attribute__ ((unused))
#elif defined(__clang__)
#define OYJL_UNUSED                    __attribute__ ((unused))
#elif defined(_MSC_VER)
#define OYJL_UNUSED                    __declspec(unused)
#else
#define OYJL_UNUSED
#endif
# ifdef _
#  undef _
# endif
# ifdef OYJL_HAVE_LIBINTL_H
#  include <libintl.h> /* bindtextdomain() */
#  define _(text) dgettext( OYJL_DOMAIN, text )
# else
#  define _(x) x
# endif
# define OYJL_OBSERVE                   0x200000
# define OYJL_NO_OPTIMISE               0x800000

#if defined(_WIN32)                    
# define oyjlPOPEN_m    _popen
# define oyjlPCLOSE_m   _pclose
#else
# define oyjlPOPEN_m    popen
# define oyjlPCLOSE_m   pclose
#endif

# ifndef OYJL_DBG_FORMAT
#  if defined(__GNUC__)
#   define  OYJL_DBG_FORMAT "%s:%d %s() "
#   define  OYJL_DBG_ARGS   strrchr(__FILE__,'/') ? strrchr(__FILE__,'/')+1 : __FILE__,__LINE__,__func__
#  else
#   define  OYJL_DBG_FORMAT "%s:%d "
#   define  OYJL_DBG_ARGS   strrchr(__FILE__,'/') ? strrchr(__FILE__,'/')+1 : __FILE__,__LINE__
#  endif
#  define    OYJL_KEEP_LOCALE            0x01
# endif
# ifndef OYJL_CREATE_VA_STRING
# define OYJL_CREATE_VA_STRING(format_, text_, alloc_, error_action) \
{ \
  va_list list; \
  size_t sz = 0; \
  int len = 0; \
  void*(* allocate_)(size_t size) = alloc_; \
  if(!allocate_) allocate_ = malloc; \
\
  text_ = NULL; \
  \
  va_start( list, format_); \
  len = vsnprintf( text_, sz, format_, list); \
  va_end  ( list ); \
\
  { \
    text_ = allocate_( sizeof(char) * len + 2 ); \
    if(!text_) \
    { \
      oyjlMessage_p( oyjlMSG_ERROR, 0, OYJL_DBG_FORMAT "could not allocate memory", OYJL_DBG_ARGS ); \
      error_action; \
    } \
    va_start( list, format_); \
    len = vsnprintf( text_, len+1, format_, list); \
    va_end  ( list ); \
  } \
}
#endif

# define oyjlAllocHelper_m(ptr_, type, size_, alloc_func, action) { \
  if ((size_) <= 0) {                                       \
      oyjlMessage_p( oyjlMSG_INSUFFICIENT_DATA, 0, "Nothing to allocate"); \
  } else {                                                  \
      void*(*a)(size_t size) = alloc_func;                  \
      if(!a) a = malloc;                                    \
      ptr_ = (type*) a(sizeof (type) * (size_t)(size_));    \
      memset( ptr_, 0, sizeof (type) * (size_t)(size_) );   \
  }                                                         \
  if (ptr_ == NULL) {                                       \
      oyjlMessage_p( oyjlMSG_ERROR, 0, "Out of memory"); \
    action;                                                 \
  }                                                         \
}
# define OYJL_ENUM_CASE_TO_STRING(case_) case case_: return #case_

typedef enum {
  oyjlOBJECT_NONE,
  oyjlOBJECT_OPTION = 1769433455,
  oyjlOBJECT_OPTION_GROUP = 1735879023,
  oyjlOBJECT_OPTIONS = 1937205615,
  oyjlOBJECT_UI_HEADER_SECTION = 1936222575,
  oyjlOBJECT_UI = 1769302383,
  oyjlOBJECT_TR = 1920231791,
  oyjlOBJECT_JSON = 1397385583
} oyjlOBJECT_e;

typedef enum oyjlOPTIONTYPE_e {
    oyjlOPTIONTYPE_START,
    oyjlOPTIONTYPE_CHOICE,
    oyjlOPTIONTYPE_FUNCTION,
    oyjlOPTIONTYPE_DOUBLE,
    oyjlOPTIONTYPE_NONE,
    oyjlOPTIONTYPE_END
} oyjlOPTIONTYPE_e;

typedef enum oyjlVARIABLETYPE_e {
    oyjlNONE,
    oyjlSTRING,
    oyjlDOUBLE,
    oyjlINT
} oyjlVARIABLE_e;

typedef struct oyjlOptionChoice_s {
  char * nick;
  char * name;
  char * description;
  char * help;
} oyjlOptionChoice_s;

typedef struct oyjlOptions_s oyjlOptions_s;
typedef struct oyjlOption_s oyjlOption_s;

typedef union oyjlVariable_u {
  const char ** s;
  double * d;
  int * i;
} oyjlVariable_u;

typedef union oyjlOption_u {
  oyjlOptionChoice_s * (*getChoices)( oyjlOption_s * opt, int * selected, oyjlOptions_s * context );
  struct {
    oyjlOptionChoice_s * list;
    int selected;
  } choices;
  struct {
    double d;
    double start;
    double end;
    double tick;
  } dbl;
} oyjlOption_u;

#define OYJL_OPTION_FLAG_EDITABLE      0x001
#define OYJL_OPTION_FLAG_ACCEPT_NO_ARG 0x002
#define OYJL_OPTION_FLAG_NO_DASH       0x004
#define OYJL_OPTION_FLAG_REPETITION    0x008
#define OYJL_OPTION_FLAG_MAINTENANCE   0x100
#define OYJL_OPTION_FLAG_IMMEDIATE     0x200
struct oyjlOption_s {
  char type[8];
  unsigned int flags;
  const char * o;
  const char * option;
  const char * key;
  const char * name;
  const char * description;
  const char * help;
  const char * value_name;
  oyjlOPTIONTYPE_e value_type;
  oyjlOption_u values;
  oyjlVARIABLE_e variable_type;
  oyjlVariable_u variable;
  const char * properties;
};

#define OYJL_GROUP_FLAG_SUBCOMMAND     0x080
#define OYJL_GROUP_FLAG_EXPLICITE      0x100
#define OYJL_GROUP_FLAG_GENERAL_OPTS   0x200
typedef struct oyjlOptionGroup_s {
  char type [8];
  unsigned int flags;
  const char * name;
  const char * description;
  const char * help;
  const char * mandatory;
  const char * optional;
  const char * detail;
  const char * properties;
} oyjlOptionGroup_s;

struct oyjlOptions_s {
  char type [8];
  oyjlOption_s * array;
  oyjlOptionGroup_s * groups;
  void * user_data;
  int argc;
  const char ** argv;
  void * private_data;
};

#define OYJL_QUIET                     0x100000
typedef enum {
  oyjlOPTION_NONE,
  oyjlOPTION_USER_CHANGED,
  oyjlOPTION_MISSING_VALUE,
  oyjlOPTION_UNEXPECTED_VALUE,
  oyjlOPTION_NOT_SUPPORTED,
  oyjlOPTION_DOUBLE_OCCURENCE,
  oyjlOPTIONS_MISSING,
  oyjlOPTION_NO_GROUP_FOUND,
  oyjlOPTION_SUBCOMMAND,
  oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND
} oyjlOPTIONSTATE_e;

typedef struct oyjlUiHeaderSection_s {
  char type [8];
  const char * nick;
  const char * label;
  const char * name;
  const char * description;
} oyjlUiHeaderSection_s;

typedef struct oyjlUi_s {
  char type [8];
  const char * app_type;
  const char * nick;
  const char * name;
  const char * description;
  const char * logo;
  oyjlUiHeaderSection_s * sections;
  oyjlOptions_s * opts;                /* info for UI logic */
} oyjlUi_s;

oyjlOption_s * oyjlOptions_GetOptionL( oyjlOptions_s     * opts,
                                       const char        * ostring,
                                       int                 flags );
enum {
  oyjlUI_STATE_NONE,
  oyjlUI_STATE_HELP,
  oyjlUI_STATE_VERBOSE = 2,
  oyjlUI_STATE_EXPORT = 4,
  oyjlUI_STATE_OPTION = 24,
  oyjlUI_STATE_NO_CHECKS = 0x1000,
  oyjlUI_STATE_NO_RELEASE = 0x2000
};
#if defined (OYJL_ARGS_BASE)
//#define oyjlBT oyjlBTArgsBase
//#define oyjlMessageFunc oyjlMessageFuncArgsBase
#define oyjlOption_PrintArg_ oyjlOption_PrintArgArgsBase_
#define oyjlOption_PrintArg_Double_ oyjlOption_PrintArg_DoubleArgsBase_
#define oyjlReadCmdToMem_ oyjlReadCmdToMemArgsBase_
#define oyjlStringSplit2 oyjlStringSplit2ArgsBase
#define oyjlUi_ReleaseArgs oyjlUi_ReleaseArgsBase
#define oyjlUi_ToText oyjlUi_ToTextArgsBase
void       oyjlArgsBaseLoadCore      ( );
#else  /* OYJL_ARGS_BASE */
# if defined( OYJL_INTERNAL)
#  define    oyjlArgsBaseLoadCore()
# else
#  define oyjlUi_ToText oyjlUi_ToTextArgsBase
void       oyjlArgsBaseLoadCore      ( );
# endif
#endif /* OYJL_ARGS_BASE */
oyjlOPTIONSTATE_e oyjlOptions_GetResult (
                                       oyjlOptions_s     * opts,
                                       const char        * opt,
                                       const char       ** result_string,
                                       double            * result_dbl,
                                       int               * result_int );
char **  oyjlOptions_ResultsToList   ( oyjlOptions_s     * opts,
                                       const char        * oc,
                                       int               * count );
oyjlUiHeaderSection_s * oyjlUi_GetHeaderSection (
                                       oyjlUi_s          * ui,
                                       const char        * nick );
void               oyjlUi_ReleaseArgs( oyjlUi_s         ** ui );
char *             oyjlUi_ToJson     ( oyjlUi_s          * ui,
                                       int                 flags );
char *             oyjlUi_ToMan      ( oyjlUi_s          * ui,
                                       int                 flags );
char *             oyjlUi_ToMarkdown ( oyjlUi_s          * ui,
                                       int                 flags );
char *             oyjlUi_ExportToJson(oyjlUi_s          * ui,
                                       int                 flags );
typedef enum {
  oyjlMSG_INFO = 400,
  oyjlMSG_CLIENT_CANCELED,
  oyjlMSG_INSUFFICIENT_DATA,
  oyjlMSG_ERROR,
  oyjlMSG_PROGRAM_ERROR,
  oyjlMSG_SECURITY_ALERT
} oyjlMSG_e;
typedef int (* oyjlMessage_f)        ( int/*oyjlMSG_e*/    error_code,
                                       const void        * context,
                                       const char        * format,
                                       ... );
typedef enum {
  oyjlNO_MARK = 1,
  oyjlRED,
  oyjlGREEN,
  oyjlBLUE,
  oyjlBOLD,
  oyjlITALIC,
  oyjlUNDERLINE
} oyjlTEXTMARK_e;
#if !defined (OYJL_ARGS_BASE)
const char *   oyjlTermColor        ( oyjlTEXTMARK_e rgb, const char * text);
#else /* OYJL_ARGS_BASE */
const char *   oyjlTermColor_       ( oyjlTEXTMARK_e rgb OYJL_UNUSED, const char * text)
{ oyjlArgsBaseLoadCore();
  return text;
}
const char * (*oyjlTermColorArgsBase)(oyjlTEXTMARK_e rgb, const char * text) = oyjlTermColor_;
#define oyjlTermColor oyjlTermColorArgsBase
#endif /* OYJL_ARGS_BASE */
#if !defined (OYJL_INTERNAL) || defined (OYJL_ARGS_BASE)
extern int          (*oyjlArgsRenderBase)   ( int                 argc,
                                       const char       ** argv,
                                       const char        * json,
                                       const char        * commands,
                                       const char        * output,
                                       int                 debug,
                                       oyjlUi_s          * ui,
                                       int               (*callback)(int argc, const char ** argv) );
int            oyjlArgsRender_       ( int                 argc,
                                       const char       ** argv,
                                       const char        * json,
                                       const char        * commands,
                                       const char        * output,
                                       int                 debug,
                                       oyjlUi_s          * ui,
                                       int               (*callback)(int argc, const char ** argv) )
{
  int result = -1;
  oyjlArgsBaseLoadCore();
  if(oyjlArgsRender_ != oyjlArgsRenderBase)
    result = oyjlArgsRenderBase(argc, argv, json, commands, output, debug, ui, callback);
  return result;
}
int          (*oyjlArgsRenderBase)   ( int                 argc,
                                       const char       ** argv,
                                       const char        * json,
                                       const char        * commands,
                                       const char        * output,
                                       int                 debug,
                                       oyjlUi_s          * ui,
                                       int               (*callback)(int argc, const char ** argv) ) = oyjlArgsRender_;
#define oyjlArgsRender oyjlArgsRenderBase
#endif /* !OYJL_INTERNAL || OYJL_ARGS_BASE */

#if !defined (OYJL_ARGS_BASE)
# if defined (OYJL_INTERNAL)
char *   oyjlBT                      ( int                 stack_limit OYJL_UNUSED );
# else
char *   oyjlBT                      ( int                 stack_limit OYJL_UNUSED ) { return strdup(""); }
# endif
#else /* OYJL_ARGS_BASE */
char *   oyjlBT_                     ( int                 stack_limit OYJL_UNUSED )
{ oyjlArgsBaseLoadCore();
  return strdup("");
}
char * (*oyjlBTArgsBase)             ( int                 stack_limit ) = oyjlBT_;
#define oyjlBT oyjlBTArgsBase
#endif /* OYJL_ARGS_BASE */
extern oyjlMessage_f oyjlMessage_p;
int          oyjlMessageFunc         ( int/*oyjlMSG_e*/    error_code,
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
oyjlMessage_f oyjlMessage_p = oyjlMessageFunc;
char *     oyjlStringCopy            ( const char        * string,
                                       void*            (* alloc)(size_t))
{
  char * text_copy = NULL;

  if(string)
  {
    oyjlAllocHelper_m( text_copy, char, strlen(string) + 1,
                        alloc, return NULL );
    strcpy( text_copy, string );
  }
    
  return text_copy;
}
#if !defined (OYJL_ARGS_BASE)
int        oyjlStringAdd             ( char             ** string,
                                       void*            (* alloc)(size_t size),
                                       void             (* deAlloc)(void * data ),
                                       const char        * format,
                                                           ... )
{
  char * text_copy = NULL;
  char * text = 0;

  void*(* allocate)(size_t size) = alloc?alloc:malloc;
  void (* deAllocate)(void * data ) = deAlloc?deAlloc:free;

  OYJL_CREATE_VA_STRING(format, text, alloc, return 1)

  if(string && *string)
  {
    int l = strlen(*string),
        l2 = strlen(text);
    text_copy = allocate( l2 + l + 1 );
    strcpy( text_copy, *string );
    strcpy( &text_copy[l], text );

    deAllocate(*string);
    *string = text_copy;

    deAllocate(text);

  } else if(string)
    *string = text;
  else if(text)
    deAllocate( text );

  return 0;
}
#endif /* OYJL_ARGS_BASE */
char*      oyjlStringAppendN         ( const char        * text,
                                       const char        * append,
                                       int                 append_len,
                                       void*            (* alloc)(size_t size) )
{
  char * text_copy = NULL;
  int text_len = 0;

  if(text)
    text_len = strlen(text);

  if(text_len || append_len)
  {
    oyjlAllocHelper_m( text_copy, char,
                        text_len + append_len + 1,
                        alloc, return NULL );

    if(text_len)
      memcpy( text_copy, text, text_len );

    if(append_len)
      memcpy( &text_copy[text_len], append, append_len );

    text_copy[text_len+append_len] = '\000';
  }

  return text_copy;
}
void       oyjlStringAddN            ( char             ** text,
                                       const char        * append,
                                       int                 append_len,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  char * text_copy = NULL;

  if(!text) return;
  if(!deAlloc) deAlloc = free;

  text_copy = oyjlStringAppendN(*text, append, append_len, alloc);

  if(*text)
    deAlloc(*text);

  *text = text_copy;

  return;
}
void       oyjlStringPush            ( char             ** text,
                                       const char        * append,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  oyjlStringAddN(text, append, strlen(append), alloc, deAlloc );
}
int        oyjlStringReplace         ( char             ** text,
                                       const char        * search,
                                       const char        * replacement,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  char * t = 0;
  const char * start, * end;
  int n = 0;

  void*(* allocate)(size_t size) = alloc?alloc:malloc;
  void (* deAllocate)(void * data ) = deAlloc?deAlloc:free;

  if(!text)
    return 0;

  start = end = *text;

  if(start && search && replacement)
  {
    int s_len = strlen(search);
    while((end = strstr(start,search)) != 0)
    {
      oyjlStringAddN( &t, start, end-start, allocate, deAllocate );
      oyjlStringPush( &t, replacement, allocate, deAllocate );
      ++n;
      if(strlen(end) >= (size_t)s_len)
        start = end + s_len;
      else
      {
        if(strstr(start,search) != 0)
          oyjlStringPush( &t, replacement, allocate, deAllocate );
        start = end = end + s_len;
        break;
      }
    }
    if(n && start && end == NULL)
      oyjlStringPush( &t, start, allocate, deAllocate );
  }

  if(t)
  {
    deAllocate(*text);
    *text = t;
  }

  return n;
}
int oyjlStringStartsWith             ( const char        * text,
                                       const char        * pattern )
{
  int text_len = text ? strlen( text ) : 0,
      pattern_len = pattern ? strlen( pattern ) : 0;

  if(text_len && text_len >= pattern_len && memcmp(text, pattern, pattern_len) == 0)
    return 1;
  else
    return 0;
}


#if !defined (OYJL_ARGS_BASE)
typedef struct oyjl_string_s * oyjl_str;
const char*oyjlStr                   ( oyjl_str            string );
void       oyjlStr_Release           ( oyjl_str          * string_ptr );
struct oyjl_string_s
{
    char * s;                          /**< @brief UTF-8 text */
    size_t len;                        /**< @brief string length. */
    size_t alloc_len;                  /**< @brief last string allocation. */
    void*(*alloc)(size_t);             /**< @brief custom allocator; optional, default is malloc */
    void (*deAlloc)(void*);            /**< @brief custom deallocator; optional, default is free */
    int    alloc_count;
};
oyjl_str   oyjlStr_New               ( size_t              length,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  struct oyjl_string_s * string = NULL;
  if(!alloc) alloc = malloc;
  if(!deAlloc) deAlloc = free;

  oyjlAllocHelper_m( string, struct oyjl_string_s, 1, alloc, return NULL );
  string->len = 0;
  if(length == 0)
    length = 8;
  oyjlAllocHelper_m( string->s, char, length, alloc, return NULL );
  string->s[0] = '\000';
  string->alloc_len = length;
  string->alloc = alloc;
  string->deAlloc = deAlloc;
  string->alloc_count = 1;

  return (oyjl_str) string;
}
int        oyjlStr_AppendN           ( oyjl_str            string,
                                       const char        * append,
                                       int                 append_len )
{
  struct oyjl_string_s * str = string;
  int error = 0;
  if(append && append_len)
  {
    if((append_len + str->len) >= str->alloc_len)
    {
      int len = (append_len + str->len) * 2;
      char * edit = str->s;
      oyjlAllocHelper_m( str->s, char, len, str->alloc, return 1 );
      str->alloc_len = len;
      ++str->alloc_count;
      memcpy(str->s, edit, str->len);
      str->deAlloc(edit);
    }
    memcpy( &str->s[str->len], append, append_len );
    str->len += append_len;
    str->s[str->len] = '\000';
  }
  return error;
}
int        oyjlStr_Push              ( oyjl_str            string,
                                       const char        * text )
{
  return oyjlStr_AppendN( string, text, strlen(text) );
}
int        oyjlStr_Add               ( oyjl_str            string,
                                       const char        * format,
                                                           ... )
{
  struct oyjl_string_s * str = string;
  char * text = 0;

  void*(* allocate)(size_t size) = str->alloc;
  void (* deAllocate)(void * data ) = str->deAlloc;

  OYJL_CREATE_VA_STRING(format, text, allocate, return 1)

  if(text)
  {
    oyjlStr_Push( string, text );
    deAllocate( text );
  }

  return 0;
}
int        oyjlStr_Replace           ( oyjl_str            text,
                                       const char        * search,
                                       const char        * replacement,
                                       void             (* modifyReplacement)
                                                             (const char * text,
                                                              const char * start,
                                                              const char * end,
                                                              const char * search,
                                                              const char ** replace,
                                                              int * replace_len,
                                                              void * user_data),
                                       void              * user_data )
{
  struct oyjl_string_s * str = text;
  oyjl_str t = NULL;
  const char * start, * end;
  int n = 0;

  if(!text)
    return 0;

  start = end = oyjlStr(text);

  if(start && search && replacement)
  {
    int s_len = strlen(search);
    while((end = strstr(start,search)) != 0)
    {
      if(!t) t = oyjlStr_New(10,0,0);
      oyjlStr_AppendN( t, start, end-start );
      if(modifyReplacement) modifyReplacement( oyjlStr(text), start, end, search, &replacement, &s_len, user_data );
      oyjlStr_Push( t, replacement );
      ++n;
      if(strlen(end) >= (size_t)s_len)
        start = end + s_len;
      else
      {
        if(strstr(start,search) != 0)
          oyjlStr_Push( t, replacement );
        start = end = end + s_len;
        break;
      }
    }
    if(n && start && end == NULL)
      oyjlStr_Push( t, start );
  }

  if(t)
  {
    void (* deAlloc)(void*) = str->deAlloc;
    if(str->s && str->alloc_len) deAlloc(str->s);
    str->len = str->alloc_len = 0;

    deAlloc = t->deAlloc;
    if(str->alloc == t->alloc)
    {
      str->s = t->s;
      str->len = t->len;
      str->alloc_len = t->alloc_len;
      str->alloc_count = t->alloc_count;
      deAlloc(t); t = NULL;
    } else
    {
      int length = 8;
      oyjlAllocHelper_m( str->s, char, length, str->alloc, return 0 );
      str->s[0] = '\000';
      str->alloc_len = length;
      oyjlStr_Push( str, oyjlStr(t) );
      oyjlStr_Release( &t );
    }
  }

  return n;
}
char *     oyjlStr_Pull              ( oyjl_str            str )
{
  struct oyjl_string_s * string;
  char * t = NULL;
  int length = 8;

  if(!str) return t;

  string = str;
  t = string->s;
  string->s = NULL;

  string->len = 0;
  oyjlAllocHelper_m( string->s, char, length, string->alloc, return NULL );
  string->s[0] = '\000';
  string->alloc_len = length;
  string->alloc_count = 1;
  
  return t;
}
void       oyjlStr_Clear             ( oyjl_str            string )
{
  struct oyjl_string_s * str = string;
  void (* deAlloc)(void*) = str->deAlloc;
  char * s = oyjlStr_Pull( string );
  if(s) deAlloc(s);
}
void       oyjlStr_Release           ( oyjl_str          * string_ptr )
{
  struct oyjl_string_s * str;
  if(!string_ptr) return;
  str = *string_ptr;
  void (* deAlloc)(void*) = str->deAlloc;
  if(str->s) deAlloc(str->s);
  deAlloc(str);
  *string_ptr = NULL;
}

const char*oyjlStr                   ( oyjl_str            string )
{
  return (const char*)string->s;
}
#endif /* OYJL_ARGS_BASE */

int      oyjlStringToLong            ( const char        * text,
                                       long              * value,
                                       const char       ** end )
{
  char * end_ = 0;
  int error = -1;
  *value = strtol( text, &end_, 0 );
  if(end_ && end_ != text && isdigit(text[0]) && !isdigit(end_[0]) )
  {
    if(end_[0] && end_ != text)
    {
      error = -1;
      if(end)
        *end = end_;
    }
    else
      error = 0;
  }
  else
    error = 1;
  return error;
}
int          oyjlStringToDouble      ( const char        * text,
                                       double            * value,
                                       const char       ** end,
                                       int                 flags OYJL_UNUSED )
{
  char * end_ = NULL, * t = NULL;
  int len, pos = 0;
  int error = -1;
#ifdef OYJL_HAVE_LOCALE_H
  char * save_locale = oyjlStringCopy( setlocale(LC_NUMERIC, 0 ), malloc );
  if(!(flags & OYJL_KEEP_LOCALE))
    setlocale(LC_NUMERIC, "C");
#endif

  if(text && text[0])
    len = strlen(text);
  else
  {
    *value = NAN;
    error = 1;
    goto clean_oyjlStringToDouble;
  }

  /* avoid irritating valgrind output of "Invalid read of size 8"
   * might be a glibc error or a false positive in valgrind */
  oyjlAllocHelper_m( t, char, len + 2*sizeof(double) + 1, malloc, error = 1; goto clean_oyjlStringToDouble);
  memset( t, 0, len + 2*sizeof(double) + 1 );

  /* remove leading empty space */
  while(text[pos] && isspace(text[pos])) pos++;
  memcpy( t, &text[pos], len );

  *value = strtod( t, &end_ );

  if(end_ && end_ != text && isdigit(text[0]) && !isdigit(end_[0]) )
  {
    if(end_[0] && end_ != text)
    {
      error = -1;
      if(end)
      {
        end_ = strstr( text, end_ );
        *end = end_;
      }
    }
    else
      error = 0;
  }
  else if(end_ && end_ == t)
  {
    *value = NAN;
    error = 2;
  }

clean_oyjlStringToDouble:

#ifdef OYJL_HAVE_LOCALE_H
  if(!(flags & OYJL_KEEP_LOCALE))
    setlocale(LC_NUMERIC, save_locale);
  if(save_locale) free( save_locale );
#endif

  if(t) free( t );

  return error;
}
void       oyjlStringListPush        ( char            *** list,
                                       int               * n,
                                       const char        * string,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  char ** nlist = 0;
  int n_alt;

  if(!list || !n) return;

  if(!deAlloc) deAlloc = free;
  n_alt = *n;

  oyjlAllocHelper_m(nlist, char*, n_alt + 2, alloc, return);

  memmove( nlist, *list, sizeof(char*) * n_alt);
  nlist[n_alt] = oyjlStringCopy( string, alloc );
  nlist[n_alt+1] = NULL;

  *n = n_alt + 1;

  if(*list)
    deAlloc(*list);

  *list = nlist;
}
#if !defined(OYJL_ARGS_BASE)
const char *   oyjlStringGetNext     ( const char        * text )
{
  /* remove leading white space */
  if(text && text[0] && isspace(text[0]))
  {
    while( text && text[0] && isspace(text[0]) ) text++;
    return text;
  }

  /* find the end of the word */
  while( text && text[0] && !isspace(text[0]) ) text++;

  /* find the next word */
  while( text && text[0] && isspace(text[0]) ) text++;

  return text && text[0] ? text : NULL;
}
static int     oyjlStringNextSpace_  ( const char        * text )
{
  int len = 0;
  while( text && text[len] && !isspace(text[len]) ) len++;
  return len;
}
static char ** oyjlStringSplitSpace_ ( const char        * text,
                                       int               * count,
                                       void*            (* alloc)(size_t))
{
  char ** list = NULL;
  int n = 0, i;

  /* split the string by empty space */
  if(text && text[0])
  {
    const char * tmp = text;

    if(!alloc) alloc = malloc;

    if(tmp && tmp[0] && !isspace(tmp[0])) ++n;
    while( tmp && tmp[0] && (tmp = oyjlStringGetNext( tmp )) != NULL ) ++n;

    if((list = alloc( (n+1) * sizeof(char*) )) == 0) return NULL;
    memset( list, 0, (n+1) * sizeof(char*) );

    {
      const char * start = text;
      if(start && isspace(start[0]))
        start = oyjlStringGetNext( start );

      for(i = 0; i < n; ++i)
      {
        int len = oyjlStringNextSpace_( start );

        if((list[i] = alloc( len+1 )) == 0) return NULL;

        memcpy( list[i], start, len );
        list[i][len] = 0;
        start = oyjlStringGetNext( start );
      }
    }
  }

  if(count)
    *count = n;

  return list;
}
#endif /* OYJL_ARGS_BASE */

const char * oyjlStringDelimiter ( const char * text, const char * delimiter, int * length )
{
  int i,j, dn = delimiter ? strlen(delimiter) : 0, len = text?strlen(text):0;
  for(j = 0; j < len; ++j)
    for(i = 0; i < dn; ++i)
      if(text[j] && text[j] == delimiter[i])
      {
        if(length)
          *length = j;
        return &text[j];
      }
  return NULL;
}
char **        oyjlStringSplit2      ( const char        * text,
                                       const char        * delimiter,
                                       const char        *(splitFunc)( const char * text, const char * delimiter, int * length ),
                                       int               * count,
                                       int              ** index,
                                       void*            (* alloc)(size_t))
{
  char ** list = 0;
  int n = 0, i;

  if(!splitFunc)
    splitFunc = oyjlStringDelimiter;

  /* split the path search string by a delimiter */
  if(text && text[0])
  {
    const char * tmp = text;

    if(!alloc) alloc = malloc;

    if(!delimiter || !delimiter[0])
#if !defined(OYJL_ARGS_BASE)
      return oyjlStringSplitSpace_( text, count, alloc );
#else /* OYJL_ARGS_BASE */
    {
      oyjlMessage_p( oyjlMSG_PROGRAM_ERROR, 0, OYJL_DBG_FORMAT "missing argument: delimiter\n", OYJL_DBG_ARGS );
      return NULL;
    }
#endif /* OYJL_ARGS_BASE */

    tmp = splitFunc(tmp, delimiter, NULL);
    if(tmp == text) ++n;
    tmp = text;
    do { ++n;
    } while( (tmp = splitFunc(tmp + 1, delimiter, NULL)) );

    tmp = 0;

    if((list = alloc( (n+1) * sizeof(char*) )) == NULL) return NULL;
    memset( list, 0, (n+1) * sizeof(char*) );
    if(index && (*index = alloc( n * sizeof(int) )) == NULL) { free(list); return NULL; }
    if(index) memset( *index, 0, n * sizeof(int) );

    {
      const char * start = text;
      for(i = 0; i < n; ++i)
      {
        intptr_t len = 0;
        int length = 0;
        const char * end = splitFunc(start, delimiter, &length);
        if(index && length) (*index)[i] = length + start - text;

        if(end > start)
          len = end - start;
        else if (end == start)
          len = 0;
        else
          len = strlen(start);

        if((list[i] = alloc( len+1 )) == 0) return NULL;

        memcpy( list[i], start, len );
        list[i][len] = 0;
        start += len + 1;
      }
    }
  }

  if(count)
    *count = n;

  return list;
}
/*
* Index into the table below with the first byte of a UTF-8 sequence to
* get the number of trailing bytes that are supposed to follow it.
* Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
* left as-is for anyone who may want to do such conversion, which was
* allowed in earlier algorithms.
*/
static const char oyjl_trailingBytesForUTF8_[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};
int        oyjlStringSplitUTF8       ( const char        * text,
                                       char            *** mbchars,
                                       void*            (* alloc)(size_t) )
{
  int len = strlen(text), wlen = 0;
  int pos = 0;
  if(mbchars)
    oyjlAllocHelper_m( *mbchars, char*, len + 1, alloc, return -2);
  while(pos < len && text[pos])
  {
    const char * ctext = &text[pos];
    int c = (unsigned char)ctext[0];
    int trailing_bytes = oyjl_trailingBytesForUTF8_[c];
    if(trailing_bytes > 3)
      break;
    if(mbchars)
    {
      oyjlAllocHelper_m( (*mbchars)[wlen], char, 4 + 1, alloc, return -2);
      memcpy((*mbchars)[wlen], ctext, trailing_bytes + 1);
    }
    pos += trailing_bytes;
    /*fprintf( stderr, "WString: current char: %d  next letter: %s trailing bytes: %d\n", c, &text[pos+1], trailing_bytes );*/
    ++pos;
    ++wlen;
  }
  return wlen;
}

char **    oyjlStringListCatList     ( const char       ** list,
                                       int                 n_alt,
                                       const char       ** append,
                                       int                 n_app,
                                       int               * count,
                                       void*            (* alloc)(size_t) )
{
  char ** nlist = 0;

  {
    int i = 0;
    int n = 0;

    if(n_alt || n_app)
      oyjlAllocHelper_m(nlist, char*, n_alt + n_app +1, alloc, return NULL);

    for(i = 0; i < n_alt; ++i)
    {
      if(list[i])
        nlist[n] = oyjlStringCopy( list[i], alloc );
      n++;
    }

    for(i = 0; i < n_app; ++i)
    {
      nlist[n] = oyjlStringCopy( append[i], alloc );
      n++;
    }

    if(count)
      *count = n;
  }

  return nlist;
}
void       oyjlStringListRelease  ( char            *** l,
                                       int                 size,
                                       void             (* deAlloc)(void*) )
{
  if(!deAlloc) deAlloc = free;

  if( l )
  {
    char ** list = *l;

    if(list)
    {
      size_t i;
      for(i = 0; (int)i < size; ++i)
        if((list)[i])
          deAlloc( (list)[i] );
      deAlloc( list );
      *l = NULL;
    }
  }
}
void     oyjlStringListAddList       ( char            *** list,
                                       int               * n,
                                       const char       ** append,
                                       int                 n_app,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  int alt_n = 0;
  char ** tmp;

  if(!list) return;

  if(n) alt_n = *n;
  tmp = oyjlStringListCatList((const char**)*list, alt_n, append, n_app,
                                     n, alloc);

  oyjlStringListRelease(list, alt_n, deAlloc);

  *list = tmp;
}
#ifndef OYJL_CTEND
/* true color codes */
#define OYJL_RED_TC "\033[38;2;240;0;0m"
#define OYJL_GREEN_TC "\033[38;2;0;250;100m"
#define OYJL_BLUE_TC "\033[38;2;0;150;255m"
/* basic color codes */
#define OYJL_BOLD "\033[1m"
#define OYJL_ITALIC "\033[3m"
#define OYJL_UNDERLINE "\033[4m"
#define OYJL_RED_B "\033[0;31m"
#define OYJL_GREEN_B "\033[0;32m"
#define OYJL_BLUE_B "\033[0;34m"
/* switch back */
#define OYJL_CTEND "\033[0m"
#define OYJL_X11_CLUT_256_BASE "\033[38;5;"
#endif
#if !defined (OYJL_ARGS_BASE)
const char * oyjlTermColorToPlainArgs( const char        * text );
const char * oyjlTermColorToPlain    ( const char        * text,
                                       int                 flags OYJL_UNUSED )
{
  return oyjlTermColorToPlainArgs( text );
}
#endif /* OYJL_ARGS_BASE */

int            oyjlVersion           ( int                 type OYJL_UNUSED )
{
  return OYJL_VERSION;
}

const char *   oyjlVersionName       ( int                 type )
{
  if(type == 1) return OYJL_GIT_VERSION;
  return OYJL_VERSION_NAME;
}

#define WARNc_S(...) oyjlMessage_p( oyjlMSG_ERROR, 0, __VA_ARGS__ )
char *     oyjlReadFileStreamToMem   ( FILE              * fp,
                                       int               * size )
{
  size_t mem_size = 256;
  char* mem;
  int c;

  if(!fp) return NULL;

  mem = (char*) malloc(mem_size+1);
  if(!mem) return NULL;

  if(size)
  {
    *size = 0;
    do
    {
      c = getc(fp);
      if(*size >= (int)mem_size)
      {
        mem_size *= 2;
        mem = (char*) realloc( mem, mem_size+1 );
        if(!mem) { *size = 0; return NULL; }
      }
      mem[(*size)++] = c;
    } while(!feof(fp));

    --*size;
    mem[*size] = '\000';
  }

  return mem;
}
#if !defined(OYJL_ARGS_BASE)
#include <sys/stat.h> /* stat() */
#include <errno.h>
int oyjlIsFileFull_ (const char* fullFileName, const char * read_mode)
{
  struct stat status;
  int r = 0;
  const char* name = fullFileName;

  memset(&status,0,sizeof(struct stat));
  if(name && name[0])
    r = stat(name, &status);

  if(r != 0 && *oyjl_debug > 1)
  switch (errno)
  {
    case EACCES:       WARNc_S("Permission denied: %s", name); break;
    case EIO:          WARNc_S("EIO : %s", name); break;
    case ENAMETOOLONG: WARNc_S("ENAMETOOLONG : %s", name); break;
    case ENOENT:       WARNc_S("A component of the name/file_name does not exist, or the file_name is an empty string: \"%s\"", name); break;
    case ENOTDIR:      WARNc_S("ENOTDIR : %s", name); break;
    case ELOOP:        WARNc_S("Too many symbolic links encountered while traversing the name: %s", name); break;
    case EOVERFLOW:    WARNc_S("EOVERFLOW : %s", name); break;
    default:           WARNc_S("%s : %s", strerror(errno), name); break;
  }

  r = !r &&
       (   ((status.st_mode & S_IFMT) & S_IFREG)
        || ((status.st_mode & S_IFMT) & S_IFLNK)
                                                );

  if (r)
  {
    FILE* fp = fopen (name, read_mode);
    if (!fp) {
      oyjlMessage_p( oyjlMSG_INFO, 0, "not existent: %s", name );
      r = 0;
    } else {
      fclose (fp);
    }
  }

  return r;
}
char *       oyjlReAllocFromStdMalloc_(char              * mem,
                                       int               * size,
                                       void*            (* alloc)(size_t) )
{
  if(mem)
  {
    if(alloc != malloc)
    {
      char* temp = mem;

      mem = alloc( *size + 1 );
      if(mem)
      {
        memcpy( mem, temp, *size );
        mem[*size] = '\000';
      }
      else
        *size = 0;

      free( temp );
    } else
      mem[*size] = '\000';
  }

  return mem;
}
char * oyjlFindApplication_(const char * app_name)
{
  const char * path = getenv("PATH");
  char * full_app_name = NULL;
  if(path && app_name)
  {
    int paths_n = 0, i;
    char ** paths = oyjlStringSplit2( path, ":", NULL, &paths_n, NULL, malloc );
    for(i = 0; i < paths_n; ++i)
    {
      char * full_name = NULL;
      int found;
      oyjlStringAdd( &full_name, 0,0, "%s/%s", paths[i], app_name );
      found = oyjlIsFileFull_( full_name, "rb" );
      if(found)
      {
        i = paths_n;
        full_app_name = strdup( full_name );
      }
      free( full_name );
      if(found) break;
    }
    oyjlStringListRelease( &paths, paths_n, free );
  }
  return full_app_name;
}
#endif /* OYJL_ARGS_BASE */
char * oyjlReadCmdToMem_             ( const char        * command,
                                       int               * size,
                                       const char        * mode,
                                       void*            (* alloc)(size_t) )
{
  char * text = 0;
  FILE * fp = 0;

  if(!alloc) alloc = malloc;

  if(command && command[0] && size )
  {
    {
#if !defined (OYJL_ARGS_BASE)
      if(*oyjl_debug && (strstr(command, "addr2line") == NULL || *oyjl_debug > 1))
        oyjlMessage_p( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT "%s", OYJL_DBG_ARGS, command );
#endif /* OYJL_ARGS_BASE */
      fp = oyjlPOPEN_m( command, mode );
    }
    if(fp)
    {
      int mem_size = 0;
      char* mem = NULL;

      text = oyjlReadFileStreamToMem(fp, size);

      if(!feof(fp))
      {
        if(text) { free( text ); text = NULL; }
        *size = 0;
        mem_size = 1024;
        mem = (char*) malloc(mem_size+1);
        oyjlPCLOSE_m(fp);
        fp = oyjlPOPEN_m( command, mode );
      }
      if(fp)
      while(!feof(fp))
      {
        if(*size >= mem_size)
        {
          mem_size *= 10;
          mem = realloc( mem, mem_size+1 );
          if(!mem) { *size = 0; break; }
        }
        if(mem)
          *size += fread( &mem[*size], sizeof(char), mem_size-*size, fp );
      }
      if(fp && mem)
      {
#if !defined (OYJL_ARGS_BASE)
        mem = oyjlReAllocFromStdMalloc_( mem, size, alloc );
#endif /* OYJL_ARGS_BASE */
        text = mem;
      }
      if(fp)
        oyjlPCLOSE_m(fp);
      fp = 0;

#if !defined (OYJL_ARGS_BASE)
      if(*size == 0)
      {
        char * t = strdup(command);
        char * end = strstr( t?t:"", " " ), * app;
        if(end)
          end[0] = '\000';

        if((app = oyjlFindApplication_( t )) == NULL)
          oyjlMessage_p( oyjlMSG_ERROR,0, OYJL_DBG_FORMAT "%s: \"%s\"",
                         OYJL_DBG_ARGS, _("Program not found"), command?command:"");

        if(t) free(t);
        if(app) free(app);
      }
#endif /* OYJL_ARGS_BASE */
    }
  }

  return text;
}

#if !defined(OYJL_ARGS_BASE)
/** @brief Read a stream from shell command.
 */
char *     oyjlReadCommandF          ( int               * size,
                                       const char        * mode,
                                       void*            (* alloc)(size_t),
                                       const char        * format,
                                                           ... )
{
  char * result = NULL;
  char * text = 0;

  if(!alloc) alloc = malloc;

  OYJL_CREATE_VA_STRING(format, text, malloc, return NULL)

  result = oyjlReadCmdToMem_( text, size, mode, alloc );

  free(text);

  return result;
}

typedef struct oyjl_val_s * oyjl_val;
typedef struct oyjlTranslation_s oyjlTranslation_s; /* from oyjl.h */
typedef char*(*oyjlTranslate_f)      ( oyjlTranslation_s * context,
                                       const char        * string );
void           oyjlTranslation_Release(oyjlTranslation_s** context_ );
struct oyjlTranslation_s
{
  char type [8];                       /**< @brief must be 'oitr' */
  const char * loc;                    /**< @brief original provided locale */
  char * lang;                         /**< @brief optimised loc for translator */
  const char * domain;                 /**< @brief identiefier for catalog */
  oyjl_val catalog;                    /**< @brief the translation tables */
  int start;                           /**< @brief lang start in catalog paths */
  int end;                             /**< @brief lang end in catalog paths */
  oyjlTranslate_f translator;          /**< @brief the function */
  void * user_data;                    /**< @brief additional data for translator */
  void (*deAlloc)(void*);              /**< @brief custom deallocator; optional */
  int flags;                           /**< @brief flags for translator; optional */
};

char *         oyjlLanguage          ( const char        * loc )
{
  char * t = NULL;

  if(loc[0] == 'C')
    t = strdup("");
  else
    if(strchr(loc,'_') != NULL)
  {
    t = strdup(loc);
    char * tmp = strchr(t,'_');
    tmp[0] = '\000';
  } else
    t = strdup(loc);

  if(*oyjl_debug) fprintf(stderr, OYJL_DBG_FORMAT "loc=\"%s\" -> \"%s\"\n", OYJL_DBG_ARGS, loc, t );
  return t;
}
oyjlTranslation_s* oyjlTranslation_New(const char        * loc,
                                       const char        * domain,
                                       oyjl_val          * catalog,
                                       oyjlTranslate_f     translator,
                                       void              * user_data,
                                       void             (* deAlloc)(void*),
                                       int                 flags )
{
  oyjlTranslation_s * context = NULL;
  char * lang = NULL;

  oyjlAllocHelper_m( context, struct oyjlTranslation_s, 1, malloc, return NULL );
  memcpy( context->type, "oitr", 4 );
  context->loc = loc;
  context->domain = domain;
  if(*oyjl_debug > 1)
    fprintf(stderr, OYJL_DBG_FORMAT "loc: %s domain: %s\n", OYJL_DBG_ARGS, loc, domain );
  if(catalog)
  {
    context->catalog = *catalog;
    *catalog = NULL;
  }
  context->translator = translator;
  context->user_data = user_data;
  context->deAlloc = deAlloc;
  context->flags = flags;
  /*lang  = oyjlLangForCatalog_( loc, context->catalog,
                                    &context->start, &context->end,
                                    context->flags );*/
  context->lang = lang;

  return context;
}
int oyjlTranslation_Check_           ( oyjlTranslation_s * context )
{
  int success = 1;
  if( context && *(oyjlOBJECT_e*)context != oyjlOBJECT_TR)
  {
    char * a = (char*)context;
    char type[5] = {a[0],a[1],a[2],a[3],0};
    char * t = oyjlBT(0);
    fprintf(stderr, "%sUnexpected object: \"%s\"(expected: \"oyjlTranslation_s\")\n", t, type );
    free(t);
    success = 0;
    return success;
  }
  return success;
}
const char * oyjlTranslation_GetDomain(oyjlTranslation_s * context )
{
  return context && oyjlTranslation_Check_(context) ? context->domain : NULL;
}
const char * oyjlTranslation_GetLang ( oyjlTranslation_s * context )
{
  return !context || !oyjlTranslation_Check_(context) ? "" : context->lang ? context->lang : context->loc;
}
void         oyjlTranslation_SetFlags( oyjlTranslation_s * context,
                                       int                 flags )
{
  if(context && oyjlTranslation_Check_(context))
    context->flags = flags;
}
oyjlTranslation_s ** oyjl_translation_context_ = NULL;
int         oyjl_translation_context_reserve_ = 0;
int          oyjlTranslation_Set     ( const char        * domain,
                                       oyjlTranslation_s** context )
{
  int i = 0, pos = -1;
  oyjlTranslation_s * oyjl_tr_context = NULL;
  int state = -1;

  if(context && *context && !oyjlTranslation_Check_(*context))
    return -2;

  if(!domain)
  {
    oyjlMessage_p( oyjlMSG_INSUFFICIENT_DATA, 0, OYJL_DBG_FORMAT "domain arg missed", OYJL_DBG_ARGS );
    return state;
  }
  state = 0;

  while(oyjl_translation_context_ && oyjl_translation_context_[i])
  {
    oyjlTranslation_s * context = oyjl_translation_context_[i];
    if(context->domain && domain && strcmp(context->domain, domain) == 0)
    {
      pos = i;
      break;
    }
    ++i;
  }
  if(pos >= 0)
  {
    oyjl_tr_context = oyjl_translation_context_[pos];
    state |= 1;
    if(*oyjl_debug)
    {
      int erase = context && oyjl_tr_context && oyjl_tr_context != *context,
          keep = context && oyjl_tr_context == *context,
          replace = !erase && context && *context;
      char * t = oyjlBT(0);
      oyjlMessage_p( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT "%s[%d] domain: \"%s\" show", OYJL_DBG_ARGS, t, pos, domain, erase?"erase":keep?"keep":replace?"replace":"show on return" );
      free(t);
    }
  }

  if(context && oyjl_tr_context == *context)
    return 1|4;

  if(context && oyjl_tr_context && oyjl_tr_context != *context)
  {
    oyjlTranslation_Release(&oyjl_translation_context_[pos]);
    state |= 2;
  }

  if(context && *context)
  {
    if(!oyjl_translation_context_)
    {
      oyjlAllocHelper_m( oyjl_translation_context_, oyjlTranslation_s*, 10, malloc, return -2 );
      oyjl_translation_context_reserve_ = 10;
    }
    if(i == oyjl_translation_context_reserve_)
    {
      oyjl_translation_context_ = realloc( oyjl_translation_context_, sizeof(oyjlTranslation_s*) * oyjl_translation_context_reserve_ * 2 );
      if(!oyjl_translation_context_)
      {
        oyjlMessage_p( oyjlMSG_ERROR, 0, OYJL_DBG_FORMAT "domain: \"%s\" alloc failed: %d", OYJL_DBG_ARGS, domain, i );
        return -2;
      } else
        oyjl_translation_context_reserve_ *= 2;
    }
    oyjl_translation_context_[i] = *context;
    *context = NULL;
  }

  return state;
}
oyjlTranslation_s* oyjlTranslation_Get(const char        * domain )
{
  oyjlTranslation_s * context = NULL;

  if(oyjl_translation_context_ && domain)
  {
    int i = 0;
    while(oyjl_translation_context_[i])
    {
      context = oyjl_translation_context_[i];
      if(context->domain && domain && strcmp(context->domain, domain) == 0)
        break;
      else
        context = NULL;
      ++i;
    }
  }

  return context;
}
static char * oyjl_nls_path_ = NULL;
extern char * oyjl_term_color_;
extern char * oyjl_term_color_html_;
extern char * oyjl_term_color_plain_;
void oyjlLibRelease() {
  if(oyjl_nls_path_) { putenv("NLSPATH=C"); free(oyjl_nls_path_); oyjl_nls_path_ = NULL; }
  if(oyjl_translation_context_)
  {
    int i = 0;
    while(oyjl_translation_context_[i])
    {
      oyjlTranslation_Release( &oyjl_translation_context_[i] );
      ++i;
    }
    free(oyjl_translation_context_);
    oyjl_translation_context_ = NULL;
    oyjl_translation_context_reserve_ = 0;
  }
  if(oyjl_term_color_) { free(oyjl_term_color_); oyjl_term_color_ = NULL; }
  if(oyjl_term_color_html_) { free(oyjl_term_color_html_); oyjl_term_color_html_ = NULL; }
  if(oyjl_term_color_plain_) { free(oyjl_term_color_plain_); oyjl_term_color_plain_ = NULL; }
}
void oyjlTreeFree (oyjl_val v)
{
    if (v == NULL) return;

    /*if((long)v->type != oyjlOBJECT_JSON)
      oyjlValueClear (v);*/

    free(v);
}
void           oyjlTranslation_Release(oyjlTranslation_s** context_ )
{
  oyjlTranslation_s * context;
  if(!(context_ && *context_ && oyjlTranslation_Check_(*context_)))
    return;

  context = *context_;
  context->loc = NULL;
  if(context->catalog)
    oyjlTreeFree( context->catalog );
  context->catalog = NULL;
  context->translator = NULL;
  if(context->deAlloc)
    context->deAlloc( context->user_data );
  context->user_data = NULL;
  context->deAlloc = NULL;
  context->flags = 0;
  context->start = 0;
  context->end = 0;
  if(context->lang)
    free(context->lang);
  context->lang = NULL;
  free(context);
  context = NULL;

  *context_ = context;
}

#define OyjlToString2_M(t) OyjlToString_M(t)
#define OyjlToString_M(t) #t
void   oyjlGettextSetup_             ( int                 use_gettext OYJL_UNUSED,
                                       const char        * loc_domain OYJL_UNUSED,
                                       const char        * env_var_locdir OYJL_UNUSED,
                                       const char        * default_locdir OYJL_UNUSED )
{
#ifdef OYJL_HAVE_LIBINTL_H
  {
    if( use_gettext )
    {
      char * var = NULL, * tmp = NULL;
      const char * environment_locale_dir = NULL,
                 * locpath = NULL,
                 * path = NULL,
                 * domain_path = default_locdir;

      if(getenv(env_var_locdir) && strlen(getenv(env_var_locdir)))
      {
        tmp = strdup(getenv(env_var_locdir));
        environment_locale_dir = domain_path = tmp;
        if(*oyjl_debug)
          oyjlMessage_p( oyjlMSG_INFO, 0,"found environment variable: %s=%s", env_var_locdir, domain_path );
      } else
        if(environment_locale_dir == NULL && getenv("LOCPATH") && strlen(getenv("LOCPATH")))
      {
        domain_path = NULL;
        locpath = getenv("LOCPATH");
        if(*oyjl_debug)
          oyjlMessage_p( oyjlMSG_INFO, 0,"found environment variable: LOCPATH=%s", locpath );
      } else
        if(*oyjl_debug)
        oyjlMessage_p( oyjlMSG_INFO, 0,"no %s or LOCPATH environment variable found; using default path: %s", env_var_locdir, domain_path );

      if(domain_path || locpath)
      {
        oyjlStringPush( &var, "NLSPATH=", 0,0 );
        oyjlStringPush( &var, domain_path ? domain_path : locpath, 0,0 );
      }
      if(var) /* do not release */
      {
        putenv(var); /* Solaris */
        if(oyjl_nls_path_)
          free(oyjl_nls_path_);
        oyjl_nls_path_ = var;
      }

      /* LOCPATH appears to be ignored by bindtextdomain("domain", NULL),
       * so it is set here to bindtextdomain(). */
      path = domain_path ? domain_path : locpath;
# ifdef OYJL_HAVE_LIBINTL_H
      const char * d = textdomain( loc_domain );
      const char * dpath = bindtextdomain( loc_domain, path );
      if(*oyjl_debug)
        oyjlMessage_p( oyjlMSG_INFO, 0,"bindtextdomain( \"%s\", \"%s\"/%s ) = ", loc_domain, path, dpath, d );
#endif
      if(*oyjl_debug)
      {
        char * fn = NULL;
        int stat = -1;
        const char * gettext_call = OyjlToString2_M(_());
        const char * domain = textdomain(NULL);

        if(path)
          oyjlStringAdd( &fn, 0,0, "%s/de/LC_MESSAGES/%s.mo", path ? path : "", loc_domain);
        if(fn)
          stat = oyjlIsFileFull_( fn, "r" );
        oyjlMessage_p( oyjlMSG_INFO, 0,"bindtextdomain(\"%s\"/%s) to %s\"%s\" %s for %s  test:%s", loc_domain, domain, locpath?"effectively ":"", path ? path : "", (stat > 0)?"Looks good":"Might fail", gettext_call, _("Example") );
        if(fn) free(fn);
      }
      if(tmp)
        free(tmp);
    }
  }
#endif /* OYJL_HAVE_LIBINTL_H */
}
void   oyjlInitI18n_                 ( const char        * loc OYJL_UNUSED )
{
#ifndef OYJL_SKIP_TRANSLATE
  oyjl_val oyjl_catalog = NULL;
  oyjlTranslation_s * trc = NULL;
  int use_gettext = 0;
#ifdef OYJL_HAVE_LIBINTL_H
  use_gettext = 1;
#else
# if 0
# include "liboyjl.i18n.h"
  int size = sizeof(liboyjl_i18n_oiJS);
  oyjl_catalog = (oyjl_val) oyjlStringAppendN( NULL, (const char*) liboyjl_i18n_oiJS, size, malloc );
  if(*oyjl_debug)
    oyjlMessage_p( oyjlMSG_INFO, 0,OYJL_DBG_FORMAT "loc: \"%s\" domain: \"%s\" catalog-size: %d", OYJL_DBG_ARGS, loc, OYJL_DOMAIN, size );
# endif
#endif
  oyjlGettextSetup_( use_gettext, OYJL_DOMAIN, "OYJL_LOCALEDIR", OYJL_LOCALEDIR );
  trc = oyjlTranslation_New( loc, OYJL_DOMAIN, &oyjl_catalog, 0,0,0, *oyjl_debug > 1?OYJL_OBSERVE:0 );
  oyjlTranslation_SetFlags( trc, 0 );
  oyjlTranslation_Set( OYJL_DOMAIN, &trc );
#endif
}

int oyjlInitLanguageDebug            ( const char        * project_name,
                                       const char        * env_var_debug,
                                       int               * debug_variable,
                                       int                 use_gettext OYJL_UNUSED,
                                       const char        * env_var_locdir OYJL_UNUSED,
                                       const char        * default_locdir OYJL_UNUSED,
                                       oyjlTranslation_s** context OYJL_UNUSED,
                                       oyjlMessage_f       msg )
{
  int error = -1;
  oyjlTranslation_s * trc = context?*context:NULL;
  const char * loc = oyjlTranslation_GetLang( trc );
  const char * loc_domain = oyjlTranslation_GetDomain( trc );

  if(!msg) msg = oyjlMessage_p;

  if(debug_variable)
    oyjl_debug = debug_variable;
  oyjlMessage_p = msg;

  if(debug_variable && getenv(env_var_debug))
  {
    *debug_variable = atoi(getenv(env_var_debug));
    if(*debug_variable)
    {
      int v = oyjlVersion(0);
      if(*debug_variable)
        msg( oyjlMSG_INFO, 0, "%s (Oyjl compile v: %s runtime v: %d)", project_name, OYJL_VERSION_NAME, v );
    }
  }

  if(*oyjl_debug)
    oyjlMessage_p( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT "loc: %s loc_domain: %s", OYJL_DBG_ARGS, loc, loc_domain );

  oyjlInitI18n_( loc );

  if(loc_domain)
  {
    oyjlGettextSetup_( use_gettext, loc_domain, env_var_locdir?env_var_locdir:"OYJL_LOCALEDIR", default_locdir?default_locdir:OYJL_LOCALEDIR );
    int state = oyjlTranslation_Set( loc_domain, context ); /* just pass domain in */
    if(*oyjl_debug)
      msg( oyjlMSG_INFO, 0, "use_gettext: %d loc_domain: %s env_var_locdir: %s default_locdir: %s oyjlTranslation_Set: %d", use_gettext, loc_domain, env_var_locdir, default_locdir, state );
  }

  return error;
}
void       oyjlTranslation_SetLocale ( oyjlTranslation_s * context,
                                       const char        * loc )
{
  if(context && oyjlTranslation_Check_(context) && loc && loc[0])
  {
    context->loc = loc;
    context->start = 0;
    context->end = 0;
    if(context->lang)
      free(context->lang);
    /*context->lang  = oyjlLangForCatalog_( loc, context->catalog,
                                          &context->start, &context->end,
                                          context->flags );*/
  }
}
const char *   oyjlLang              ( const char        * loc )
{
  const char * lang = NULL;

  if(oyjl_translation_context_)
  {
    int i = 0;
    while(oyjl_translation_context_[i])
    {
      oyjlTranslation_s * context = oyjl_translation_context_[i];
      const char * domain = oyjlTranslation_GetDomain(context);
      if(*oyjl_debug >= 1)
      {
        char * t = oyjlBT(0);
        oyjlMessage_p( oyjlMSG_INFO, 0, "%s", t );
        free(t);
        oyjlMessage_p( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT "loc: %s context[%d]->loc: %s lang: %s domain: %s", OYJL_DBG_ARGS, loc, i, context->loc, lang, domain );
      }
      oyjlTranslation_SetLocale( context, loc );
      lang = context->lang?context->lang:context->loc;
      if(*oyjl_debug >= 1)
        oyjlMessage_p( oyjlMSG_INFO, 0, OYJL_DBG_FORMAT "loc: %s context[%d]->loc: %s lang: %s", OYJL_DBG_ARGS, loc, i, context->loc, lang );
      ++i;
    }
  }

  return lang;
}
#endif /* OYJL_ARGS_BASE */

#define oyjlMEMORY_ALLOCATION_SECTIONS 0x01
#define oyjlMEMORY_ALLOCATION_ARRAY    0x02
#define oyjlMEMORY_ALLOCATION_GROUPS   0x04
#define oyjlMEMORY_ALLOCATION_OPTIONS  0x08

#if !defined(OYJL_TREE_INTERNAL_H)
typedef struct {
  char       ** options;
  char       ** values;
  int           count;
  int           group;
  void        * attr;
  int           memory_allocation;
} oyjlOptsPrivate_s;
#endif
#endif /* DOXYGEN */
#endif /* OYJL_INTERNAL */

#ifdef OYJL_HAVE_LANGINFO_H
#include <langinfo.h> /* nl_langinfo() */
#endif

/** \addtogroup oyjl_args
 *  @{ */

/** @brief    Release dynamic structure
 *  @memberof oyjlOptionChoice_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
void oyjlOptionChoice_Release     ( oyjlOptionChoice_s**choices )
{
  int n = 0,i;
  oyjlOptionChoice_s * ca;
  if(!choices || !*choices) return;
  ca = *choices;
  while(ca[n].nick && ca[n].nick[0] != '\000') ++n;
  for(i = 0; i < n; ++i)
  {
    oyjlOptionChoice_s * c = &ca[i];
    if(c->nick) free(c->nick);
    if(c->name) free(c->name);
    if(c->description) free(c->description);
    if(c->help) free(c->help);
  }
  *choices = NULL;
  free(*choices);
}

#if !defined(OYJL_ARGS_BASE)
void oyjlOptsPrivate_Release         ( oyjlOptsPrivate_s** results_ )
{
  if(results_)
  {
    oyjlOptsPrivate_s * results = *results_;
    if(results)
    {
      oyjlStringListRelease( &results->options, results->count, free );
      oyjlStringListRelease( &results->values,  results->count, free );
      results->count = 0;
      free(results);
    }
  }
}
/** @} *//* oyjl_args */

/** \addtogroup oyjl_core
 *  @{ *//* oyjl_core */
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
static int oyjlTermColorCheck_       ( int                 flags )
{
  struct stat sout, serr;
  int color_term = 0;

  if( fstat( fileno(stdout), &sout ) == -1 )
    return 0;

  if(flags & OYJL_OBSERVE)
  switch( sout.st_mode & S_IFMT )
  {
    case S_IFBLK:  fprintf(stderr, "block device\n");            break;
    case S_IFCHR:  fprintf(stderr, "character device\n");        break;
    case S_IFDIR:  fprintf(stderr, "directory\n");               break;
    case S_IFIFO:  fprintf(stderr, "FIFO/pipe\n");               break;
    case S_IFLNK:  fprintf(stderr, "symlink\n");                 break;
    case S_IFREG:  fprintf(stderr, "regular file\n");            break;
    case S_IFSOCK: fprintf(stderr, "socket\n");                  break;
    default:       fprintf(stderr, "unknown?\n");                break;
  }

  if( fstat( fileno(stderr), &serr ) == -1 )
    return color_term;

  if(flags & OYJL_OBSERVE)
  switch( serr.st_mode & S_IFMT )
  {
    case S_IFBLK:  fprintf(stderr, "block device\n");            break;
    case S_IFCHR:  fprintf(stderr, "character device\n");        break;
    case S_IFDIR:  fprintf(stderr, "directory\n");               break;
    case S_IFIFO:  fprintf(stderr, "FIFO/pipe\n");               break;
    case S_IFLNK:  fprintf(stderr, "symlink\n");                 break;
    case S_IFREG:  fprintf(stderr, "regular file\n");            break;
    case S_IFSOCK: fprintf(stderr, "socket\n");                  break;
    default:       fprintf(stderr, "unknown?\n");                break;
  }

  if( ( S_ISCHR( sout.st_mode ) &&
        S_ISCHR( serr.st_mode ) ) ||
      S_ISFIFO( sout.st_mode ) )
    color_term = 1;
  if(flags & OYJL_OBSERVE)
    fprintf(stderr, "color_term: %d\n", color_term );

  return color_term;
}
#ifndef OYJL_FORCE_COLORTERM
#define OYJL_FORCE_COLORTERM           0x01
#define OYJL_FORCE_NO_COLORTERM        0x02
#define OYJL_RESET_COLORTERM           0x04
#endif
/** @brief setup formating for terminals
 *  @param[in]     flags               support
 *                                     - ::OYJL_FORCE_COLORTERM
 *                                     - ::OYJL_FORCE_NO_COLORTERM
 *                                     - ::OYJL_RESET_COLORTERM
 *  @return                            state
 *                                     - 1 for simple color term
 *                                     - 2 for truecolor term
 *  @version Oyjl: 1.0.0
 *  @date    2022/05/08
 *  @since   2020/03/09 (Oyjl: 1.0.0)
 */
int          oyjlTermColorInit       ( int                 flags )
{
  int color_env = 0;
  static int oyjl_colorterm_init_ = 0;
  static const char * oyjl_colorterm_ = NULL;
  static int oyjl_truecolor_ = 0,
             oyjl_color_ = 0;
  if(!oyjl_colorterm_init_ || flags & OYJL_RESET_COLORTERM)
  {
    oyjl_colorterm_init_ = 1;
    oyjl_colorterm_ = getenv("COLORTERM");
    if(flags & OYJL_OBSERVE)
      fprintf(stderr, "%s COLORTERM\n", getenv("COLORTERM")?"has":"no" );
    oyjl_color_ = oyjl_colorterm_ != NULL ? 1 : 0;
    if(!oyjl_colorterm_) oyjl_colorterm_ = getenv("TERM");
    oyjl_truecolor_ = oyjl_colorterm_ && strcmp(oyjl_colorterm_,"truecolor") == 0;
    if(!oyjlTermColorCheck_(flags))
      oyjl_truecolor_ = oyjl_color_ = 0;
    if( getenv("FORCE_COLORTERM") || flags & OYJL_FORCE_COLORTERM )
      oyjl_truecolor_ = oyjl_color_ = 1;
    if(flags & OYJL_OBSERVE)
      fprintf(stderr, "%s FORCE_COLORTERM  %s flags & OYJL_FORCE_COLORTERM\n", getenv("FORCE_COLORTERM")?"has":"no", flags & OYJL_FORCE_COLORTERM ? "use":"no" );
    if( getenv("FORCE_NO_COLORTERM") || flags & OYJL_FORCE_NO_COLORTERM )
      oyjl_truecolor_ = oyjl_color_ = 0;
    if(flags & OYJL_OBSERVE)
      fprintf(stderr, "%s FORCE_NO_COLORTERM  %s flags & OYJL_FORCE_NO_COLORTERM\n", getenv("FORCE_NO_COLORTERM")?"has":"no", flags & OYJL_FORCE_NO_COLORTERM ? "use":"no" );
    if(*oyjl_debug || flags & OYJL_OBSERVE)
      fprintf(stderr, "color: %d truecolor: %d oyjl_colorterm_: %s\n", oyjl_color_, oyjl_truecolor_, oyjl_colorterm_ );
  }
  color_env = (oyjl_color_ ? 0x01 : 0x00) | (oyjl_truecolor_ ? 0x02 : 0x00);
  return color_env;
}

#ifndef OYJL_CTEND
/* true color codes */
#define OYJL_RED_TC "\033[38;2;240;0;0m"
#define OYJL_GREEN_TC "\033[38;2;0;250;100m"
#define OYJL_BLUE_TC "\033[38;2;0;150;255m"
/* basic color codes */
#define OYJL_BOLD "\033[1m"
#define OYJL_ITALIC "\033[3m"
#define OYJL_UNDERLINE "\033[4m"
#define OYJL_RED_B "\033[0;31m"
#define OYJL_GREEN_B "\033[0;32m"
#define OYJL_BLUE_B "\033[0;34m"
/* switch back */
#define OYJL_CTEND "\033[0m"
/* 256 CLUT */
#define OYJL_X11_CLUT_256_BASE "\033[38;5;"
#endif
const char * oyjlTermColorPtr( oyjlTEXTMARK_e rgb, char ** color_text, const char * text)
{
  int color_env = oyjlTermColorInit( *oyjl_debug > 1?OYJL_OBSERVE:0 ),
      color = color_env & 0x01,
      truecolor = color_env & 0x02;

  if(*color_text) free(*color_text);
  *color_text = NULL;

  switch(rgb)
  {
    case oyjlNO_MARK:   oyjlStringAdd( color_text, 0,0, "%s", text ); break;
    case oyjlRED:       oyjlStringAdd( color_text, 0,0, "%s%s%s", truecolor          ? OYJL_RED_TC   : color ? OYJL_RED_B   : "", text, truecolor || color ? OYJL_CTEND : "" ); break;
    case oyjlGREEN:     oyjlStringAdd( color_text, 0,0, "%s%s%s", truecolor          ? OYJL_GREEN_TC : color ? OYJL_GREEN_B : "", text, truecolor || color ? OYJL_CTEND : "" ); break;
    case oyjlBLUE:      oyjlStringAdd( color_text, 0,0, "%s%s%s", truecolor          ? OYJL_BLUE_TC  : color ? OYJL_BLUE_B  : "", text, truecolor || color ? OYJL_CTEND : "" ); break;
    case oyjlBOLD:      oyjlStringAdd( color_text, 0,0, "%s%s%s", truecolor || color ? OYJL_BOLD                            : "", text, truecolor || color ? OYJL_CTEND : "" ); break;
    case oyjlITALIC:    oyjlStringAdd( color_text, 0,0, "%s%s%s", truecolor || color ? OYJL_ITALIC                          : "", text, truecolor || color ? OYJL_CTEND : "" ); break;
    case oyjlUNDERLINE: oyjlStringAdd( color_text, 0,0, "%s%s%s", truecolor || color ? OYJL_UNDERLINE                       : "", text, truecolor || color ? OYJL_CTEND : "" ); break;
  }
  return *color_text;
}
char * oyjl_term_color_ = NULL;
/** @brief text formating for terminals
 *
 *  Input text can be up to 200 chars wide. The returned text is cached.
 *  So copy or flush the text before reusing this function.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2022/09/15
 *  @since   2019/06/16 (Oyjl: 1.0.0)
 */
const char * oyjlTermColor( oyjlTEXTMARK_e rgb, const char * text)
{
  if(!text)
    return "---";

  oyjlTermColorPtr( rgb, &oyjl_term_color_, text );
  return oyjl_term_color_;
}
char * oyjl_term_color_f_ = NULL;
/** @brief variable text formating for terminals
 *
 *  @see oyjlTermColor()
 *
 *  @version Oyjl: 1.0.0
 *  @date    2022/09/15
 *  @since   2022/06/06 (Oyjl: 1.0.0)
 */
const char * oyjlTermColorF( oyjlTEXTMARK_e rgb, const char * format, ...)
{
  char * tmp = NULL;
  const char * text = format;

  if(strchr(format, '%'))
  { OYJL_CREATE_VA_STRING(format, tmp, malloc, return NULL)
    text = tmp;
  }

  oyjlTermColorPtr( rgb, &oyjl_term_color_f_, text );

  if(tmp) free(tmp);
  return oyjl_term_color_f_;
}
/** @brief variable text formating for terminals
 *
 *  @see oyjlTermColorF()
 *
 *  @version Oyjl: 1.0.0
 *  @date    2023/11/25
 *  @since   2023/11/26 (Oyjl: 1.0.0)
 */
const char * oyjlTermColorFPtr( oyjlTEXTMARK_e rgb, char ** color_text, const char * format, ...)
{
  char * tmp = NULL;
  const char * text = format;

  if(strchr(format, '%'))
  { OYJL_CREATE_VA_STRING(format, tmp, malloc, return NULL)
    text = tmp;
  }

  oyjlTermColorPtr( rgb, color_text, text );

  if(tmp) free(tmp);
  return *color_text;
}



char * oyjl_term_color_html_ = NULL;
char * oyjl_term_color_plain_ = NULL;
/** @brief convert a subset of HTML to terminal colors
 *
 *  The supported codes are "<strong>", "<em>", "<u>" and "&nbsp;".
 *
 *  @version Oyjl: 1.0.0
 *  @date    2022/05/08
 *  @since   2021/11/20 (Oyjl: 1.0.0)
 */
const char * oyjlTermColorFromHtml   ( const char        * text,
                                       int                 flags )
{
  int color_env = oyjlTermColorInit( flags ),
      color = color_env & 0x01,
      truecolor = color_env & 0x02;
  const char * t,
             * bold = color || truecolor ? OYJL_BOLD : "",
             * italic = color || truecolor ? OYJL_ITALIC : "",
             * underline = color || truecolor ? OYJL_UNDERLINE : "",
             * end = color || truecolor ? OYJL_CTEND : "";
  oyjl_str tmp = oyjlStr_New(10,0,0);
  oyjlStr_Push( tmp, text );
  oyjlStr_Replace( tmp, "<strong>", bold, 0,NULL );
  oyjlStr_Replace( tmp, "</strong>", end, 0,NULL );
  oyjlStr_Replace( tmp, "<em>", italic, 0,NULL );
  oyjlStr_Replace( tmp, "</em>", end, 0,NULL );
  oyjlStr_Replace( tmp, "<u>", underline, 0,NULL );
  oyjlStr_Replace( tmp, "</u>", end, 0,NULL );
  oyjlStr_Replace( tmp, "</font>", end, 0,NULL );
  oyjlStr_Replace( tmp, "&nbsp;", " ", 0,NULL );
  oyjlStr_Replace( tmp, "<br />", "", 0,NULL );
  t = oyjlStr(tmp);
  if(oyjl_term_color_html_) free(oyjl_term_color_html_);
  oyjl_term_color_html_ = strdup( t );
  oyjlStr_Release( &tmp );
  return oyjl_term_color_html_;
}

const char * oyjlX11_CLUT_256[256] = {
    /* 8 colors */
    "000000",
    "800000",
    "008000",
    "808000",
    "000080",
    "800080",
    "008080",
    "c0c0c0",

    /* "bright" version of 8 colors. */
    "808080",
    "ff0000",
    "00ff00",
    "ffff00",
    "0000ff",
    "ff00ff",
    "00ffff",
    "ffffff",

    /**/
    "000000",
    "00005f",
    "000087",
    "0000af",
    "0000d7",
    "0000ff",
    "005f00",
    "005f5f",
    "005f87",
    "005faf",
    "005fd7",
    "005fff",
    "008700",
    "00875f",
    "008787",
    "0087af",
    "0087d7",
    "0087ff",
    "00af00",
    "00af5f",
    "00af87",
    "00afaf",
    "00afd7",
    "00afff",
    "00d700",
    "00d75f",
    "00d787",
    "00d7af",
    "00d7d7",
    "00d7ff",
    "00ff00",
    "00ff5f",
    "00ff87",
    "00ffaf",
    "00ffd7",
    "00ffff",
    "5f0000",
    "5f005f",
    "5f0087",
    "5f00af",
    "5f00d7",
    "5f00ff",
    "5f5f00",
    "5f5f5f",
    "5f5f87",
    "5f5faf",
    "5f5fd7",
    "5f5fff",
    "5f8700",
    "5f875f",
    "5f8787",
    "5f87af",
    "5f87d7",
    "5f87ff",
    "5faf00",
    "5faf5f",
    "5faf87",
    "5fafaf",
    "5fafd7",
    "5fafff",
    "5fd700",
    "5fd75f",
    "5fd787",
    "5fd7af",
    "5fd7d7",
    "5fd7ff",
    "5fff00",
    "5fff5f",
    "5fff87",
    "5fffaf",
    "5fffd7",
    "5fffff",
    "870000",
    "87005f",
    "870087",
    "8700af",
    "8700d7",
    "8700ff",
    "875f00",
    "875f5f",
    "875f87",
    "875faf",
    "875fd7",
    "875fff",
    "878700",
    "87875f",
    "878787",
    "8787af",
    "8787d7",
    "8787ff",
    "87af00",
    "87af5f",
    "87af87",
    "87afaf",
    "87afd7",
    "87afff",
    "87d700",
    "87d75f",
    "87d787",
    "87d7af",
    "87d7d7",
    "87d7ff",
    "87ff00",
    "87ff5f",
    "87ff87",
    "87ffaf",
    "87ffd7",
    "87ffff",
    "af0000",
    "af005f",
    "af0087",
    "af00af",
    "af00d7",
    "af00ff",
    "af5f00",
    "af5f5f",
    "af5f87",
    "af5faf",
    "af5fd7",
    "af5fff",
    "af8700",
    "af875f",
    "af8787",
    "af87af",
    "af87d7",
    "af87ff",
    "afaf00",
    "afaf5f",
    "afaf87",
    "afafaf",
    "afafd7",
    "afafff",
    "afd700",
    "afd75f",
    "afd787",
    "afd7af",
    "afd7d7",
    "afd7ff",
    "afff00",
    "afff5f",
    "afff87",
    "afffaf",
    "afffd7",
    "afffff",
    "d70000",
    "d7005f",
    "d70087",
    "d700af",
    "d700d7",
    "d700ff",
    "d75f00",
    "d75f5f",
    "d75f87",
    "d75faf",
    "d75fd7",
    "d75fff",
    "d78700",
    "d7875f",
    "d78787",
    "d787af",
    "d787d7",
    "d787ff",
    "d7af00",
    "d7af5f",
    "d7af87",
    "d7afaf",
    "d7afd7",
    "d7afff",
    "d7d700",
    "d7d75f",
    "d7d787",
    "d7d7af",
    "d7d7d7",
    "d7d7ff",
    "d7ff00",
    "d7ff5f",
    "d7ff87",
    "d7ffaf",
    "d7ffd7",
    "d7ffff",
    "ff0000",
    "ff005f",
    "ff0087",
    "ff00af",
    "ff00d7",
    "ff00ff",
    "ff5f00",
    "ff5f5f",
    "ff5f87",
    "ff5faf",
    "ff5fd7",
    "ff5fff",
    "ff8700",
    "ff875f",
    "ff8787",
    "ff87af",
    "ff87d7",
    "ff87ff",
    "ffaf00",
    "ffaf5f",
    "ffaf87",
    "ffafaf",
    "ffafd7",
    "ffafff",
    "ffd700",
    "ffd75f",
    "ffd787",
    "ffd7af",
    "ffd7d7",
    "ffd7ff",
    "ffff00",
    "ffff5f",
    "ffff87",
    "ffffaf",
    "ffffd7",
    "ffffff",
    /* gray scale */
    "080808",
    "121212",
    "1c1c1c",
    "262626",
    "303030",
    "3a3a3a",
    "444444",
    "4e4e4e",
    "585858",
    "626262",
    "6c6c6c",
    "767676",
    "808080",
    "8a8a8a",
    "949494",
    "9e9e9e",
    "a8a8a8",
    "b2b2b2",
    "bcbcbc",
    "c6c6c6",
    "d0d0d0",
    "dadada",
    "e4e4e4",
    "eeeeee"
};

int          oyjlTermColor256GetIndex( const char        * term_color )
{
  long index = 0;
  /*int offset = strlen(OYJL_X11_CLUT_256_BASE);*/
  char * number = strdup(strstr(term_color,"38;5;") + 5);
  char * t = strchr( number, 'm' );
  if(t) t[0] = '\000';
  if(oyjlStringToLong( number, &index, 0 ) != 0)
    index = -1;
  free(number);
  return (int)index;
}

static void oyjlConvertXterm256ToHex_(const char * text OYJL_UNUSED, const char * start OYJL_UNUSED, const char * end, const char * search, const char ** replace, int * r_len, void * data)
{
  int index = oyjlTermColor256GetIndex(end);
  char ** txt = data, * t = *txt;
  int bold = strstr(search, "\033[1;") ? 1 : 0;
  int italic = strstr(search, "\033[3;") ? 1 : 0;
  int bold_italic = bold || italic ? 2 : 0;
  if(t) { free(t); *txt = t = NULL; }
  if(0 <= index && index <= 255)
  {
    if(bold_italic)
      oyjlStringAdd(&t, 0,0, "%s", bold ? "<strong>" : "<em>" );
    oyjlStringAdd(&t, 0,0, "<font color=\"#%s\">", oyjlX11_CLUT_256[index]);
    *replace = *txt = t;
    *r_len = bold_italic + 7 + (index >= 100 ? 3 : index >= 10 ? 2 : 1) + 1;
  }
  else
  {
    const char * m = strchr(end, 'm');
    const char * index = end+bold_italic+7;
    int add = 0;
    if(m - (index) < 4)
      add = m-index + 1;
    *replace = "";
    *r_len = bold_italic + 7 + add;
  }
}


#ifndef OYJL_WRAP
#define OYJL_WRAP                      0x001
#endif
/** @brief convert internal used terminal colors to HTML
 *
 *  Support OYJL_WRAP in flags
 *
 *  @version Oyjl: 1.0.0
 *  @date    2022/09/14
 *  @since   2022/06/10 (Oyjl: 1.0.0)
 */
const char * oyjlTermColorToHtml     ( const char        * text,
                                       int                 flags )
{
  const char * t;
  char * txt = NULL;
  oyjl_str tmp = oyjlStr_New(10,0,0);
  oyjlStr_Push( tmp, text );
  oyjlStr_Replace( tmp, "<", "&lt;", 0,NULL );
  oyjlStr_Replace( tmp, ">", "&gt;", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_RED_TC, "<font color=red>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_GREEN_TC, "<font color=green>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_BLUE_TC, "<font color=blue>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_BOLD, "<strong>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_ITALIC, "<em>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_UNDERLINE, "<u>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_RED_B, "<font color=red>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_GREEN_B, "<font color=green>", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_BLUE_B, "<font color=blue>", 0,NULL );
  /* ansi colors */
  oyjlStr_Replace( tmp, "\033[00;31m", "<font color=red>", 0,NULL );
  oyjlStr_Replace( tmp, "\033[00;32m", "<font color=green>", 0,NULL );
  oyjlStr_Replace( tmp, "\033[00;33m", "<font color=orange>", 0,NULL );
  oyjlStr_Replace( tmp, "\033[00;34m", "<font color=blue>", 0,NULL );
  oyjlStr_Replace( tmp, "\033[00;35m", "<font color=magenta>", 0,NULL );
  oyjlStr_Replace( tmp, "\033[00;39m", "", 0,NULL ); /* used as default foreground color */
  oyjlStr_Replace( tmp, OYJL_X11_CLUT_256_BASE, "", oyjlConvertXterm256ToHex_,&txt );
  oyjlStr_Replace( tmp, "\033[1;38;5;", "", oyjlConvertXterm256ToHex_,&txt ); /* bold xterm256 color */
  oyjlStr_Replace( tmp, "\033[3;38;5;", "", oyjlConvertXterm256ToHex_,&txt ); /* italic xterm256 color */
  if(txt) { free(txt); txt = NULL; }
  oyjlStr_Replace( tmp, OYJL_CTEND, "</u></strong></em></font>", 0,NULL );
#define OYJL_CTENDM "\033[m"
  oyjlStr_Replace( tmp, OYJL_CTENDM, "</u></strong></em></font>", 0,NULL );
  oyjlStr_Replace( tmp, "  ", "&nbsp;&nbsp;", 0,NULL );
  oyjlStr_Replace( tmp, "\t", "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", 0,NULL );
  oyjlStr_Replace( tmp, "\n", "<br />\n", 0,NULL );
  t = oyjlStr(tmp);
  if(oyjl_term_color_html_) { free(oyjl_term_color_html_); oyjl_term_color_html_ = NULL; }
  if(flags & OYJL_WRAP)
    oyjlStringAdd( &oyjl_term_color_html_, 0,0, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
<html><body>\n%s</body></html>", t );
  else
    oyjl_term_color_html_ = strdup( t );
  oyjlStr_Release( &tmp );
  return oyjl_term_color_html_;
}


const char * oyjlTermColorToPlainArgs ( const char       * text )
{
  const char * t;
  oyjl_str tmp = oyjlStr_New(10,0,0);
  oyjlStr_Push( tmp, text );
  oyjlStr_Replace( tmp, OYJL_RED_TC, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_GREEN_TC, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_BLUE_TC, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_BOLD, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_ITALIC, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_UNDERLINE, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_RED_B, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_GREEN_B, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_BLUE_B, "", 0,NULL );
  oyjlStr_Replace( tmp, OYJL_CTEND, "", 0,NULL );
  t = oyjlStr(tmp);
  if(oyjl_term_color_plain_) free(oyjl_term_color_plain_);
  oyjl_term_color_plain_ = strdup( t );
  oyjlStr_Release( &tmp );
  return oyjl_term_color_plain_;
}
#endif /* OYJL_ARGS_BASE */
/** @} *//* oyjl_core */

/** \addtogroup oyjl_args
 *  @{ */

/** @brief    Return number of array elements
 *  @memberof oyjlOptionChoice_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2019/06/17
 *  @since   2019/06/17 (Oyjl: 1.0.0)
 */
int  oyjlOptionChoice_Count          ( oyjlOptionChoice_s* list )
{
  int n = 0;
  if(!list) return 0;
  while(list[n].nick)
    ++n;
  return n;
}


/** @brief    Return number of "oiwi" array elements
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
int oyjlOptions_Count             ( oyjlOptions_s  * opts )
{
  int n = 0;
  if(!(opts && opts->array)) return 0;
  while( *(oyjlOBJECT_e*)&opts->array[n] /*"oiwi"*/ == oyjlOBJECT_OPTION) ++n;
  return n;
}

/** @brief    Return number of "oiwi" groups elements
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
int oyjlOptions_CountGroups       ( oyjlOptions_s  * opts )
{
  int n = 0;
  if(!(opts && opts->groups)) return 0;
  while( *(oyjlOBJECT_e*)&opts->groups[n] /*"oiwg"*/ == oyjlOBJECT_OPTION_GROUP) ++n;
  return n;
}

char *         oyjlOptionGetKey_     ( const char        * ostring )
{
  char * str = NULL, * t;

  if(!(ostring && ostring[0]))
    return str;

  if(ostring[0] == '-') ++ostring; /* Interprete "-o". */
  if(ostring[0] == '-') ++ostring; /* Interprete "--option" */

  str = oyjlStringCopy(ostring, malloc);
  t = strchr(str, '='); /* Interprete "--option=arg" with arg */

  if(t)
    t[0] = '\000';
  t = strchr(str, '.'); /* Interprete "--option.attr" with attribute */
  if(t)
    t[0] = '\000';

  return str;
}
int oyjlOptions_GroupHasOptionL_     ( oyjlOptions_s     * opts,
                                       int                 group_pos,
                                       const char        * option )
{
  int found = 0;
  char ** list;
  int i, n;
  oyjlOptionGroup_s * g = &opts->groups[group_pos];
  char * copt = oyjlOptionGetKey_( option );
  if(g->mandatory && g->mandatory[0])
  {
    n = 0;
    list = oyjlStringSplit2( g->mandatory, "|,", 0, &n, NULL, malloc );
    for( i = 0; i  < n; ++i )
    {
      const char * opt = list[i];
      if(strcmp(opt, copt) == 0)
      {
        found = 1;
        if(*oyjl_debug)
          fprintf( stderr, "%s found inside %s\n", option, g->mandatory );
        break;
      }
    }
    oyjlStringListRelease( &list, n, free );
  }

  if(g->optional && g->optional[0] && found == 0)
  {
    n = 0;
    list = oyjlStringSplit2( g->optional, "|,", 0, &n, NULL, malloc );
    for( i = 0; i  < n; ++i )
    {
      const char * opt = list[i];
      if(strcmp(opt, copt) == 0)
      {
        found = 2;
        break;
      }
    }
    oyjlStringListRelease( &list, n, free );
  }

  free(copt);

  return found;
}

/** @internal
 *  @return                            - 1 for number
 *                                     - 2 for symbolic */
int oyjlManArgIsNum_( const char * arg )
{
  int is_num_arg = 0, i = 0;
  double dbl = 0.0;
  char * t = oyjlStringCopy(arg, 0);
  if(!arg) return is_num_arg;
  if(strlen(arg) >= 3 && tolower(arg[0]) == 'n' && tolower(arg[1]) == 'u' && tolower(arg[2]) == 'm')
    is_num_arg = 2;
  while(t[i] && t[i] != '\000' && t[i] != '|') ++i;
  t[i] = '\000';
  if(t[0] && oyjlStringToDouble( t, &dbl, 0,0 ) == 0)
    is_num_arg = 1;
  free(t);
  return is_num_arg;
}

int oyjlManArgIsEditable_( const char * arg )
{
  int is_edit_arg = 0;
  if(!arg) return is_edit_arg;
  if(oyjlManArgIsNum_(arg) == 2 && strchr(arg,'|') == NULL)
    ++is_edit_arg;
  /* explicite more expressions */
  if( strstr(arg, "...") != NULL )
    ++is_edit_arg;
  else
  { /* two upper letters after an other */
    int i = 0;
    while(arg[i])
    {
      char c = arg[i], c2 = arg[i+1];
      if(isupper(c) && isupper(c2))
      {
        ++is_edit_arg;
        break;
      }
      ++i;
    }
  }
  return is_edit_arg;
}


#define OYJL_IS_NOT_O( x ) (!o || !o->o || strcmp(o->o,x) != 0)
#define OYJL_IS_NOT_OPT( x ) (!o || !o->option || strcmp(o->option,x) != 0)
#define OYJL_IS_O( x ) (o && o->o && strcmp(o->o,x) == 0)
#if !defined(OYJL_TREE_INTERNAL_H)
enum {
  oyjlOPTIONSTYLE_ONELETTER = 0x01,                     /* the single dash '-' style option is prefered: -o */
  oyjlOPTIONSTYLE_STRING = 0x02,                        /* add double dash '-' style option as well: --long-option */
  oyjlOPTIONSTYLE_OPTIONAL_START = 0x04,                /* print the option as optional: EBNF [--option] */
  oyjlOPTIONSTYLE_OPTIONAL_END = 0x08,                  /* print the option as optional: EBNF [--option] */
  oyjlOPTIONSTYLE_OPTIONAL_INSIDE_GROUP = 0x10,         /* add second level grouping: [[--option1|--option2]] */
  oyjlOPTIONSTYLE_MAN = 0x20,                           /* add Unix MAN page groff syntax decoration */
  oyjlOPTIONSTYLE_MARKDOWN = 0x40,                      /* add markdown syntax decoration */
  oyjlOPTIONSTYLE_GROUP_SUBCOMMAND = 0x080,             /* supresses dash(es): option */
  oyjlOPTIONSTYLE_GROUP_EXPLICITE = 0x100,              /* -- just for alignment */
  oyjlOPTIONSTYLE_GROUP_GENERAL_OPTS = 0x200,           /* -- just for alignment */
  oyjlOPTIONSTYLE_OPTION_ONLY = 0x8000                  /* print the option without any args */
};
#endif
#define oyjlOPTIONSTYLE_OPTIONAL (oyjlOPTIONSTYLE_OPTIONAL_START | oyjlOPTIONSTYLE_OPTIONAL_END)
#define oyjlOPTIONSTYLE_LINK_GROUP     0x2000           /* add link to group, e.g. from synopsis line in markdown */
#define oyjlOPTIONSTYLE_LINK_SYNOPSIS  0x4000           /* add link to synopsis, e.g. in markdown */
char *       oyjlOption_PrintArg_    ( oyjlOption_s      * o,
                                       int                 style OYJL_UNUSED )
{
  char * text = NULL;
  int sub_command = ((style & OYJL_GROUP_FLAG_SUBCOMMAND) && o->option) ? OYJL_GROUP_FLAG_SUBCOMMAND : 0;
#if !defined(OYJL_ARGS_BASE)
  if(!o) return oyjlStringCopy("", malloc);
  if(style & oyjlOPTIONSTYLE_OPTIONAL_START)
    oyjlStringPush( &text, "[", malloc, free );
  if((style & oyjlOPTIONSTYLE_ONELETTER || !o->option) && o->o && o->o[0] && OYJL_IS_NOT_O("@") && OYJL_IS_NOT_O("#") && !sub_command)
  {
    if(style & oyjlOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "\\fB\\-%s\\fR", o->o?o->o:o->option );
    else if(style & oyjlOPTIONSTYLE_MARKDOWN)
      oyjlStringAdd( &text, malloc, free, "<strong>-%s</strong>", o->o?o->o:o->option );
    else
      oyjlStringAdd( &text, malloc, free, "-%s", oyjlTermColor( oyjlBOLD, o->o?o->o:o->option ) );
  }
  if((style & oyjlOPTIONSTYLE_ONELETTER || style & oyjlOPTIONSTYLE_STRING) && OYJL_IS_O("#"))
    oyjlStringPush( &text, "|", malloc, free );
  if(style & oyjlOPTIONSTYLE_ONELETTER && style & oyjlOPTIONSTYLE_STRING && OYJL_IS_NOT_O("@") && OYJL_IS_NOT_O("#") && o->o && o->o[0] && o->option && !sub_command)
    oyjlStringAddN( &text, "|", 1, malloc, free );
  if( o->option && o->option[0] &&
      ( style & oyjlOPTIONSTYLE_STRING ||
        ( !(o->o && o->o[0]) &&
          OYJL_IS_NOT_O("@") &&
          OYJL_IS_NOT_O("#")
        ) ||
        ( sub_command && ( style & oyjlOPTIONSTYLE_ONELETTER || style & oyjlOPTIONSTYLE_STRING ) )
      )
    )
  {
    if(style & oyjlOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "\\fB%s%s\\fR", sub_command ? "" : "\\-\\-", o->option );
    else if(style & oyjlOPTIONSTYLE_MARKDOWN)
      oyjlStringAdd( &text, malloc, free, "<strong>%s%s</strong>", sub_command ? "" : "--", o->option );
    else
      oyjlStringAdd( &text, malloc, free, "%s%s", sub_command ? "" : "--", oyjlTermColor( oyjlBOLD, o->option ) );
  }

  if(o->value_name && o->value_name[0] && !(style & oyjlOPTIONSTYLE_OPTION_ONLY))
  {
    const char * value_name = o->value_name;
    int needs_edit_dots = 0;
    int is_editable_arg = oyjlManArgIsEditable_( value_name );
    if( o->flags & OYJL_OPTION_FLAG_EDITABLE &&
        !is_editable_arg &&
        strstr(value_name, "...") == NULL )
      needs_edit_dots = 1;
    int m = value_name[0] == '['; /* m=move : migth be explicitely used together with OYJL_OPTION_FLAG_ACCEPT_NO_ARG */
    if(m) /* expect optional arg(s) for this option and *move* the first opening '[' square bracket before the equal '=' sign */
      ++value_name;
    if(style & oyjlOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "%s\\fI%s%s%s%s\\fR",
          OYJL_IS_NOT_O("@") && OYJL_IS_NOT_O("#") && !(m == 0 && o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG) ? " ":"",
          (m == 0 && o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG)?"[=":"",
          o->value_name,
          needs_edit_dots?"...":"",
          (m == 0 && o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG)?"]":"" );
    else
    {
      if(style & oyjlOPTIONSTYLE_MARKDOWN)
        oyjlStringAdd( &text, malloc, free, "%s%s%s%s%s%s</em>",
            (m||o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG)?"<em>[":"",
            OYJL_IS_NOT_O("@")?"=":"",
            (m||o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG) ? "" : "<em>",
            value_name,
            needs_edit_dots?"...":"",
            (m == 0 && o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG)?"]":"" ); /* allow for easier word wrap in table */
      else if(style & oyjlOPTIONSTYLE_OPTIONAL_INSIDE_GROUP)
        oyjlStringAdd( &text, malloc, free, "%s%s%s",
            (!o->o || strcmp(o->o, "@")) != 0?"=":"",
            oyjlTermColor(oyjlITALIC,o->value_name),
            needs_edit_dots?"...":"" );
      else
      {
        char * t = NULL;
        oyjlStringAdd( &t, malloc, free, "%s%s%s%s%s",
            (m||o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG)?"[":"",
            (!o->o || strcmp(o->o, "@")) != 0?"=":"",
            value_name,
            needs_edit_dots?"...":"",
            (m == 0 && o->flags&OYJL_OPTION_FLAG_ACCEPT_NO_ARG)?"]":"" );
        oyjlStringPush( &text, oyjlTermColor(oyjlITALIC,t), malloc, free );
        free(t);
      }
    }
  }
  if(o->flags & OYJL_OPTION_FLAG_REPETITION)
    oyjlStringPush( &text, " ...", malloc, free );
  if(style & oyjlOPTIONSTYLE_OPTIONAL_END)
    oyjlStringPush( &text, "]", malloc, free );
#else
  text = malloc(2+(o->o?2:strlen(o->option)+1) + (o->value_name?strlen(o->value_name)+2:0));
  sprintf(text, "%s%s%s%s", sub_command ? "" : o->o?"-":"--", o->o?o->o:o->option, o->value_name?"=":"", o->value_name?o->value_name:"");
#endif /* OYJL_ARGS_BASE */
  return text;
}
#define oyjlHELP 0x01
#define oyjlDESCRIPTION 0x02
#define oyjlNO_BRACKETS 0x04
/** @internal
 * 
 * @param[in]      flags               modifier
 *                                     - oyjlDESCRIPTION
 *                                     - oyjlHELP
 *                                     - oyjlNO_BRACKETS
 */
static char* oyjlOption_PrintArg_Double_( oyjlOption_s   * o,
                                       int                 flags )
{
  char * text = NULL;
#if !defined(OYJL_ARGS_BASE)
  oyjlStringAdd( &text, malloc, free, "%s%s%s %s%s%s%g [≥%g ≤%g Δ%g]%s",
      (o->description && flags & oyjlDESCRIPTION) ? o->description:"",
      (o->help && flags & oyjlHELP)?": ":"", (o->help && flags & oyjlHELP)?o->help :"",
      flags&oyjlNO_BRACKETS?"":"(", o->value_name?o->value_name:"", o->value_name?":":"", o->values.dbl.d, o->values.dbl.start, o->values.dbl.end, o->values.dbl.tick, flags&oyjlNO_BRACKETS?"":")" );
#else
  text = malloc((o->value_name?strlen(o->value_name):0) + 80);
  sprintf(text, "%s%s%s%g [≥%g ≤%g Δ%g]%s",
      !(flags&oyjlNO_BRACKETS)?"(":"", o->value_name?o->value_name:"", o->value_name?":":"", o->values.dbl.d, o->values.dbl.start, o->values.dbl.end, o->values.dbl.tick, !(flags&oyjlNO_BRACKETS)?")":"" );
#endif /* OYJL_ARGS_BASE */
  return text;
}

oyjlOptionChoice_s oyjl_h_choices_[] = {
                                    {"-", "", "", ""},
                                    {"synopsis", "", "", ""},
                                    {NULL,NULL,NULL,NULL}};
oyjlOptionChoice_s oyjl_X_choices_[] = {
                                    {"man", "", "", ""},
                                    {"markdown", "", "", ""},
                                    {"json", "", "", ""},
                                    {"json+command", "", "", ""},
                                    {"export", "", "", ""},
                                    {NULL,NULL,NULL,NULL}};
oyjlOptionChoice_s oyjl_R_choices_[] = {
                                    {"gui", "", "", ""},
                                    {"cli", "", "", ""},
                                    {"web", "", "", ""},
                                    {"-","","",""},
                                    {NULL,NULL,NULL,NULL}};
static void oyjlOptions_EnrichInbuild_( oyjlOption_s * o )
{
  /* enrich inbuild variables */
  if(!o->o)
    return;
  if(strcmp(o->o, "h") == 0)
  {
    if((o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL) ||
        o->value_type == oyjlOPTIONTYPE_FUNCTION ||
        o->value_type == oyjlOPTIONTYPE_NONE
      )
    {
      if(o->value_type == oyjlOPTIONTYPE_CHOICE)
      {
        oyjl_h_choices_[0].name = _("Full Help");
        oyjl_h_choices_[0].description = _("Print help for all groups");
        oyjl_h_choices_[0].help = "";
        oyjl_h_choices_[1].name = _("Synopsis");
        oyjl_h_choices_[1].description = _("List groups");
        oyjl_h_choices_[1].help = _("Show all groups including syntax");
        o->values.choices.list = (oyjlOptionChoice_s*) oyjlStringAppendN( NULL, (const char*)oyjl_h_choices_, sizeof(oyjl_h_choices_), malloc );
        if(o->variable_type != oyjlSTRING)
        {
          fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
          fprintf(stderr, "\"help\" has wrong variable_type=%s. Need %s together with oyjlOPTIONTYPE_CHOICE\n",
              o->variable_type==oyjlNONE?"oyjlNONE":o->variable_type==oyjlDOUBLE?"oyjlDOUBLE":o->variable_type==oyjlINT?"oyjlINT":"----",
              oyjlTermColor(oyjlGREEN, "oyjlSTRING") );
        }
      }
      if(o->name == NULL)
      {
        o->name = _("help");
        if(o->description == NULL)
        {
          o->description = _("Print help text");
          if(o->help == NULL)
            o->help = _("Show usage information and hints for the tool.");
        }
        if(o->value_name == NULL)
          o->value_name = "synopsis|...";
      }
    }
  }
  if(strcmp(o->o, "X") == 0)
  {
    if(o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL)
    {
      oyjl_X_choices_[0].name = _("Man");
      oyjl_X_choices_[0].description = _("Unix Man page");
      oyjl_X_choices_[0].help = _("Get a unix man page");
      oyjl_X_choices_[1].name = _("Markdown");
      oyjl_X_choices_[1].description = _("Formated text");
      oyjl_X_choices_[1].help = _("Get formated text");
      oyjl_X_choices_[2].name = _("Json");
      oyjl_X_choices_[2].description = _("GUI");
      oyjl_X_choices_[2].help = _("Get a Oyjl Json UI declaration");
      oyjl_X_choices_[3].name = _("Json + Command");
      oyjl_X_choices_[3].description = _("GUI + Command");
      oyjl_X_choices_[3].help = _("Get Oyjl Json UI declaration incuding command");
      oyjl_X_choices_[4].name = _("Export");
      oyjl_X_choices_[4].description = _("All available data");
      oyjl_X_choices_[4].help = _("Get UI data for developers. The format can be converted by the oyjl-args tool.");
      o->values.choices.list = (oyjlOptionChoice_s*) oyjlStringAppendN( NULL, (const char*)oyjl_X_choices_, sizeof(oyjl_X_choices_), malloc );
      if(o->value_name == NULL)
      {
        o->value_name = "json|json+command|man|markdown";
        if(o->name == NULL)
        {
          o->name = _("export");
          if(o->description == NULL)
          {
            o->description = _("Export formated text");
            if(o->help == NULL)
              o->help = _("Get UI converted into text formats");
          }
        }
      }
    }
  }
  if(strcmp(o->o, "R") == 0)
  {
    if(o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL)
    {
      oyjl_R_choices_[0].name = _("Gui");
      oyjl_R_choices_[0].description = _("Show UI");
      oyjl_R_choices_[0].help = _("Display a interactive graphical User Interface.");
      oyjl_R_choices_[1].name = _("Cli");
      oyjl_R_choices_[1].description = _("Show UI");
      oyjl_R_choices_[1].help = _("Print on Command Line Interface.");
      oyjl_R_choices_[2].name = _("Web");
      oyjl_R_choices_[2].description = _("Start Web Server");
      oyjl_R_choices_[2].help = _("Start a local Web Service to connect a Webbrowser with. Use the -R=web:help sub option to see more information.");
      o->values.choices.list = (oyjlOptionChoice_s*) oyjlStringAppendN( NULL, (const char*)oyjl_R_choices_, sizeof(oyjl_R_choices_), malloc );
      if(o->value_name == NULL)
      {
        o->value_name = "gui|cli|web|";
        if(o->name == NULL)
        {
          o->name = _("render");
          if(o->description == NULL)
          {
            o->description = _("Select Renderer");
            if(o->help == NULL)
              o->help = _("Select and possibly configure Renderer. -R=\"gui\" will just launch a graphical UI. -R=\"web:port=port_number:https_key=TLS_private_key_filename:https_cert=TLS_CA_certificate_filename:css=layout_filename.css\" will launch a local Web Server, which listens on local port.");
          }
        }
      }
    }
  }
}

#if !defined (OYJL_ARGS_BASE)
static int oyjlStringDelimiterCount_( const char * text, const char * delimiter )
{
  int i,j, dn = delimiter ? strlen(delimiter) : 0, len = text?strlen(text):0, n = 0;
  if(len) ++n;
  for(j = 0; j < len; ++j)
    for(i = 0; i < dn; ++i)
      if(text[j] && text[j] == delimiter[i])
        ++n;
  return n;
}
#endif /* OYJL_ARGS_BASE */

/** @brief    Obtain the specified option from one letter member::o
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2019/07/29
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlOption_s * oyjlOptions_GetOption ( oyjlOptions_s     * opts,
                                       const char        * ol )
{
  int i;
  int nopts = oyjlOptions_Count( opts );
  oyjlOption_s * o = NULL;

  if(!ol) return o;

  for(i = 0; i < nopts; ++i)
  {
    o = &opts->array[i];
    if(o->o && strcmp(o->o, ol) == 0)
    {
      if( strcmp(ol, "h") == 0 && ((o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL) ||
                                    o->value_type == oyjlOPTIONTYPE_FUNCTION ||
                                    o->value_type == oyjlOPTIONTYPE_NONE) )
        oyjlOptions_EnrichInbuild_(o);
      if( strcmp(ol, "X") == 0 && o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL )
        oyjlOptions_EnrichInbuild_(o);
      if( strcmp(ol, "R") == 0 && o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL )
        oyjlOptions_EnrichInbuild_(o);

      return o;
    }
    else
      o = NULL;
  }
  return o;
}

int oyjlOptions_IsOn_                ( oyjlOptions_s     * opts,
                                       const char        * opt )
{
  int found = 0;
  oyjlOption_s * o;
  if(!opts || !opt) return found;
  o = oyjlOptions_GetOptionL( opts, opt, 0 );
  if(o && o->variable_type == oyjlINT && o->variable.i)
    found = *o->variable.i;
  if(o && o->variable_type == oyjlSTRING && o->variable.s)
    found = *o->variable.s && (*o->variable.s)[0];
  if(o && o->variable_type == oyjlNONE)
    oyjlOptions_GetResult( opts, o->o?o->o:o->option, 0, 0, &found );
  return found;
}
#define OYJL_CASE_COMPARE              0x01
#define OYJL_LAZY                      0x02
#define OYJL_SET                       0x04
int oyjlOptions_HasValue_            ( oyjlOptions_s     * opts,
                                       const char        * opt,
                                       const char        * value,
                                       int                 flags )
{
  int count = 0, i,
      found = 0;
  char ** results = oyjlOptions_ResultsToList( opts, opt, &count );
  for(i = 0; i < count; ++i)
  {
    const char * val = results[i];
    if(flags & OYJL_CASE_COMPARE)
    {
      if(strcasecmp(val, value) == 0)
        ++found;
    } else if(flags & OYJL_LAZY)
    {
      if(strstr(val, value) != NULL)
        ++found;
    } else
      if(strcmp(val, value) == 0)
        ++found;
    if(found)
    {
      if(flags & OYJL_SET)
      {
        oyjlOption_s * o = oyjlOptions_GetOptionL( opts, opt, OYJL_QUIET );
        if(o)
        {
          long l;
          switch(o->variable_type)
          {
            case oyjlNONE:   break;
            case oyjlSTRING: *o->variable.s = value; break;
            case oyjlDOUBLE: oyjlStringToDouble( value, o->variable.d, NULL,0 ); break;
            case oyjlINT:    oyjlStringToLong( value, &l, NULL );
                             *o->variable.i = l; break;
          }
        }
      }
      break;
    }
  }
  oyjlStringListRelease( &results, count, free );
  return found;
}

/** @brief    Obtain the specified option from option string
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2021/03/04
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlOption_s * oyjlOptions_GetOptionL( oyjlOptions_s     * opts,
                                       const char        * ostring,
                                       int                 flags )
{
  int i;
  int nopts = oyjlOptions_Count( opts );
  oyjlOption_s * o = NULL;
  char * str, * t;
  char ol[8];

  memset(ol, 0, 8);

  if(!opts || !(ostring && ostring[0]))
    return o;

  if(ostring[0] == '-') ++ostring; /** Interprete "-o". */
  if(ostring[0] == '-') ++ostring; /** Interprete "--option" */

  str = oyjlStringCopy(ostring, malloc);
  t = strchr(str, '='); /** Interprete "--option=arg" with arg */

  if(t)
    t[0] = '\000';
  t = strchr(str, '.'); /** Interprete "--option.attr" with attribute */
  if(t)
    t[0] = '\000';

  if(str[0])
  {
    int len = strlen(str);
    if(len == 1)
      strcpy( ol, str );
    else
    {
      int l = 0;
      oyjlStringDelimiter(str, "|,", &l);
      if(l > 0)
        str[l] = '\000';

      if(oyjlStringSplitUTF8(str,NULL,0) == 1)
        strcpy( ol, str );
    }
  }

  for(i = 0; i < nopts; ++i)
  {
    o = &opts->array[i];
    if( (ol[0] && o->o && strcmp(o->o, ol) == 0) ||
        (!ol[0] && o->option && strcmp(o->option, str) == 0)
      )
    {
      if( strcmp(str, "help") == 0 && ((o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL) ||
                                        o->value_type == oyjlOPTIONTYPE_FUNCTION))
        oyjlOptions_EnrichInbuild_(o);
      if( strcmp(str, "export") == 0 && o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL )
        oyjlOptions_EnrichInbuild_(o);
      if( strcmp(str, "render") == 0 && o->value_type == oyjlOPTIONTYPE_CHOICE && o->values.choices.list == NULL )
        oyjlOptions_EnrichInbuild_(o);
      free(str);

      return o;
    }
    else
      o = NULL;
  }
  if( !(flags & OYJL_QUIET) &&
      strcmp(ostring,"h") != 0 &&
      !oyjlOptions_IsOn_( opts, "h" ) )
  {
    fprintf( stderr, "%s%s: %s %d\n", *oyjl_debug?oyjlBT(0):"", _("Option not found"), oyjlTermColor(oyjlBOLD,str), flags );
  }
  free(str);

  return o;
}
static oyjlOPTIONSTATE_e oyjlOptions_Check_(
                                       oyjlOptions_s  * opts )
{
  int i,j;
  int nopts = oyjlOptions_Count( opts );
  oyjlOption_s * o = NULL, * b = NULL;

  for(i = 0; i < nopts; ++i)
  {
    o = &opts->array[i];
    for(j = i+1; j < nopts; ++j)
    {
      b = &opts->array[j];
      if(o->o && b->o && strcmp(o->o, b->o) == 0)
      {
        fprintf( stderr, "%s %s \'%s\'\n", oyjlTermColor(oyjlRED,_("Usage Error:")), _("Double occuring option"), b->o );
        return oyjlOPTION_DOUBLE_OCCURENCE;
      }

    }
    /* some error checking */
    /*if(!(('0' <= o->o && o->o <= '9') ||
         ('a' <= o->o && o->o <= 'z') ||
         ('A' <= o->o && o->o <= 'Z') ||
         o->o == '@' || o->o == '#' || o->o == '\000' || o->o == '|'))
    {
      fprintf( stderr, "%s %s \'%s\' %s\n", _("Usage Error:"), _("Option not supported"), o->o, _("need range 0-9 or a-z or A-Z") );
      return oyjlOPTION_NOT_SUPPORTED;
    }*/
    if(OYJL_IS_NOT_O("#") && o->value_name && o->value_name[0] && o->value_type == oyjlOPTIONTYPE_NONE)
    {
      fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
      fprintf( stderr, "%s \'%s\': %s\n", _("Option not supported"), oyjlTermColor(oyjlBOLD,o->o), _("need a value_type") );
      return oyjlOPTION_NOT_SUPPORTED;
    }
    if( OYJL_IS_NOT_O("#") &&
        OYJL_IS_NOT_O("h") &&
        OYJL_IS_NOT_O("X") &&
        OYJL_IS_NOT_O("R") &&
        o->value_type == oyjlOPTIONTYPE_CHOICE &&
        !((o->flags & OYJL_OPTION_FLAG_EDITABLE) || o->values.choices.list))
    {
      fputs( oyjlBT(0), stderr );
      fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
      fprintf( stderr, "%s \'%s\' %s\n", _("Option not supported"), oyjlTermColor(oyjlBOLD,o->o?o->o:o->option), _("needs OYJL_OPTION_FLAG_EDITABLE or choices") );
      if(!getenv("OYJL_NO_EXIT")) exit(1);
      return oyjlOPTION_NOT_SUPPORTED;
    }
  }
  return oyjlOPTION_NONE;
}
#if 0
/* list of static strings */
void       oyjlStringListStaticAdd   ( const char      *** list,
                                       int               * n,
                                       const char        * string,
                                       void*            (* alloc)(size_t),
                                       void             (* deAlloc)(void*) )
{
  const char ** nlist = 0;
  int n_alt;

  if(!list || !n) return;

  if(!deAlloc) deAlloc = free;
  n_alt = *n;

  oyjlAllocHelper_m(nlist, const char*, n_alt + 2, alloc, return);

  memmove( nlist, *list, sizeof(const char*) * n_alt);
  nlist[n_alt] = string;
  nlist[n_alt+1] = NULL;

  *n = n_alt + 1;

  if(*list)
    deAlloc(*list);

  *list = nlist;
}
#endif

/* support OYJL_NO_OPTIMISE */
char * oyjlOptionsResultValueCopy_   ( const char        * arg,
                                       int                 flags )
{
  char * value = NULL;
  int len;
  if(!arg)
    return value;

  if(arg[0] == '\000')
    return oyjlStringCopy( arg, 0 );

  if(arg[0] == '"' && !(flags & OYJL_NO_OPTIMISE ))
    ++arg;

  value = oyjlStringCopy( arg, 0 );
  len = strlen(value);
  if(value[len - 1] == '"' && !(flags & OYJL_NO_OPTIMISE ))
    value[len - 1] = '\000';

  return value;
}

void oyjlOptions_Print_              ( oyjlOptions_s     * opts,
                                       int                 pos )
{
  int i;
  for(i = 0; i < opts->argc; ++i)
    fprintf( stderr, "%s ", i == pos ? oyjlTermColor( oyjlBOLD, opts->argv[i] ) : opts->argv[i] );
  fprintf( stderr, "\n" );
}

/** @brief    Parse the options into a private data structure
 *  @memberof oyjlOptions_s
 *
 *  The returned status can be used to detect usage errors and hint them on
 *  the command line.
 *  In the usual case where the variable fields are set, the results
 *  will be set too.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/10/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlOPTIONSTATE_e oyjlOptions_Parse  ( oyjlOptions_s     * opts )
{
  oyjlOPTIONSTATE_e state = oyjlOPTION_NONE;
  oyjlOption_s * o;
  oyjlOptsPrivate_s * result;

  if(!opts) return state;

  result = (oyjlOptsPrivate_s*) opts->private_data;
  if(!result)
    result = (oyjlOptsPrivate_s*) calloc( 1, sizeof(oyjlOptsPrivate_s) );

  /* parse the command line arguments */
  if(!result->values)
  {
    int i, len = 0;
    for(i = 1; i < opts->argc; ++i)
      if(opts->argv[i])
        len += strlen(opts->argv[i]);
    /* The first string contains the detected single char options */
    result->options = (char**) calloc( len + 1, sizeof(char*) );
    result->values = (char**) calloc( len + 1, sizeof(char*) );
    result->group = -1;
    if((state = oyjlOptions_Check_(opts)) != oyjlOPTION_NONE)
      goto clean_parse;
    for(i = 1; i < opts->argc; ++i)
    {
      const char * str = opts->argv[i];
      const char * long_arg = NULL;
      const char * value = NULL;
      int l = 0,
          require_value, might_have_value = 0;

      if(str)
          l = strlen(str);
      else
          continue;

      if(strstr(str,"-qmljsdebugger") != NULL) /* detect a QML option */
        continue;

      /* parse -a | -a value | -a=value | -ba | -ba value | -ba=value */
           if(l > 1 && str[0] == '-' && str[1] != '-')
      {
        int j;
        char ** multi_byte_letters = NULL;
        l = oyjlStringSplitUTF8(str, &multi_byte_letters, malloc);
        for(j = 1; j < l; ++j)
        {
          const char * arg = multi_byte_letters[j];
          o = oyjlOptions_GetOption( opts, arg );
          if( !o )
          {
            oyjlOptions_Print_( opts, i );
            fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s \'%s\'\n", _("Option not supported"), oyjlTermColor(oyjlBOLD,arg) );
            state = oyjlOPTION_NOT_SUPPORTED;
            break;
          }
          require_value = o->value_type != oyjlOPTIONTYPE_NONE;
          if(o->flags & OYJL_OPTION_FLAG_ACCEPT_NO_ARG)
            might_have_value = 1;

          value = NULL;

          if( (require_value || might_have_value) &&
              j == l-1 && opts->argc > i+1 && (opts->argv[i+1][0] != '-' || strlen(opts->argv[i+1]) <= 1) )
          {
            value = opts->argv[i+1];
            ++i;
          }
          else if( str[j+1] == '=' )
          {
            ++j;
            value = &str[j+1];
            j = l;
          }

          if( require_value )
          {
            if(!(value && value[0]) && !might_have_value)
            {
                char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
                oyjlOptions_Print_( opts, i );
                fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
                fprintf( stderr, "%s \'%s\' (%s)\n", _("Option needs a argument"), oyjlTermColor(oyjlBOLD,arg), t );
                free(t);
              state = oyjlOPTION_MISSING_VALUE;
            }

            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_(value?value:"1",-o->flags);
            ++result->count;
          }
          else if( might_have_value )
          {
            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_(value?value:"1",o->flags);
            ++result->count;
          }
          else if(!require_value && !value)
          {
            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_("1",o->flags);
            ++result->count;
          }
          else
          {
            char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
            oyjlOptions_Print_( opts, i );
            fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s \'%s\' (%s)\n", _("Option has a unexpected argument"), arg?arg:"", t );
            free(t);
            state = oyjlOPTION_UNEXPECTED_VALUE;
            j = l;
          }
        }
        oyjlStringListRelease( &multi_byte_letters, l, free );
      }
      /* parse --arg | --arg value | --arg=value */
      else if(l > 2 && str[0] == '-' && str[1] == '-')
      {
        long_arg = &str[2];
        o = oyjlOptions_GetOptionL( opts, long_arg, 0 );
        if( !o ||
            (o && o->flags & OYJL_OPTION_FLAG_NO_DASH) )
        {
          oyjlOptions_Print_( opts, i );
          fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
          fprintf( stderr, "%s \'%s\'\n", _("Option not supported with double dash"), oyjlTermColor(oyjlBOLD,long_arg) );
          state = oyjlOPTION_NOT_SUPPORTED;
          goto clean_parse;
        }
        require_value = o->value_type != oyjlOPTIONTYPE_NONE;
        if(o->flags & OYJL_OPTION_FLAG_ACCEPT_NO_ARG)
          might_have_value = 1;

        value = NULL;

        if( strchr(str, '=') != NULL )
          value = strchr(str, '=') + 1;
        else if( (require_value || might_have_value) &&
            opts->argc > i+1 && opts->argv[i+1][0] != '-' )
        {
          value = opts->argv[i+1];
          ++i;
        }

        if( require_value )
        {
          if( !(value && value[0]) && !might_have_value )
          {
            char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
            oyjlOptions_Print_( opts, i );
            fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s \'%s\' (%s)\n", _("Option needs a argument"), oyjlTermColor(oyjlBOLD,long_arg), t );
            free(t);
            state = oyjlOPTION_MISSING_VALUE;
          }

          result->options[result->count] = strdup(o->o?o->o:o->option);
          result->values[result->count] = oyjlOptionsResultValueCopy_(value?value:"1",o->flags);
          ++result->count;
        } else
        {
          if(!value)
          {
            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_("1",o->flags);
            ++result->count;
          }
          else if( might_have_value && value )
          {
            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_(value,o->flags);
            ++result->count;
          }
          else
          {
            char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
            oyjlOptions_Print_( opts, i );
            fprintf( stderr, "%s %s \'%s\' (%s)\n", oyjlTermColor(oyjlRED,_("Usage Error:")), _("Option has a unexpected argument"), value?value:"", t );
            free(t);
            state = oyjlOPTION_UNEXPECTED_VALUE;
          }
        }
      }
      /* parse anonymous value, if requested */
      else
      {
        o = NULL;

        /* support sub command group selector */
        if( l > 2 )
        {
          o = oyjlOptions_GetOptionL( opts, str, OYJL_QUIET );
          if(o && o->flags & OYJL_OPTION_FLAG_ACCEPT_NO_ARG)
            might_have_value = 1;
          if( strchr(str, '=') != NULL )
            value = strchr(str, '=') + 1;
          if(o && !(o->flags & OYJL_OPTION_FLAG_NO_DASH))
          {
            oyjlOptions_Print_( opts, i );
            fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s \'%s\'\n", _("Option not supported without double dash"), oyjlTermColor(oyjlBOLD,str) );
            state = oyjlOPTION_NOT_SUPPORTED;
            goto clean_parse;
          }
          if( might_have_value )
          {
            value = NULL;

            if( strchr(str, '=') != NULL )
              value = strchr(str, '=') + 1;
            else if( opts->argc > i+1 && opts->argv[i+1][0] != '-' )
            {
              value = opts->argv[i+1];
              ++i;
            }
            if(!result->options[result->count])
              result->options[result->count] = strdup(o->o?o->o:o->option);
            if(value)
              result->values[result->count] = oyjlOptionsResultValueCopy_(value,o->flags);
            else
              result->values[result->count] = oyjlOptionsResultValueCopy_("1",o->flags);
            state = oyjlOPTION_SUBCOMMAND;
          }
          else if( o && !might_have_value && value )
          {
            char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING | OYJL_GROUP_FLAG_SUBCOMMAND );
            oyjlOptions_Print_( opts, i );
            fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s (%s)\n", _("This option expects no arguments"), t );
            fprintf( stderr, "%s\n", _("Options with arguments are not allowed in sub command style. A sub command has no leading '--'. It is a mandatory option of a option group.") );
            free(t);
            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_("0",o->flags);
            state = oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND;
          }
          else if(o && o->value_type == oyjlOPTIONTYPE_NONE)
          {
            result->options[result->count] = strdup(o->o?o->o:o->option);
            result->values[result->count] = oyjlOptionsResultValueCopy_("1",o->flags);
            state = oyjlOPTION_SUBCOMMAND;
          }
          else if(o)
          {
            if(!result->options[result->count])
              result->options[result->count] = strdup(o->o?o->o:o->option);
            if(!result->values[result->count])
              result->values[result->count] = oyjlOptionsResultValueCopy_("1",o->flags);
          }
        }

        if(!o)
        {
          o = oyjlOptions_GetOption( opts, "@" );
          if(o)
            value = str;
          if(value)
          {
            result->values[result->count] = strdup(value);
            result->options[result->count] = oyjlOptionsResultValueCopy_("@",o?o->flags:0);
          }
          else
          {
            oyjlOptions_Print_( opts, i );
            fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s: \"%s\"\n", _("Unbound options are not supported"), oyjlTermColor(oyjlBOLD,opts->argv[i]) );
            state = oyjlOPTION_NOT_SUPPORTED;
            goto clean_parse;
          }
        }
        ++result->count;
      }
    }
    opts->private_data = result;

    i = 0;
    while(result->options[i])
    {
      oyjlOption_s * o = oyjlOptions_GetOptionL( opts, result->options[i], 0 );
      if(o)
      switch(o->variable_type)
      {
        case oyjlNONE:   break;
        case oyjlSTRING: oyjlOptions_GetResult( opts, o->o?o->o:o->option, o->variable.s, 0, 0 ); break;
        case oyjlDOUBLE: oyjlOptions_GetResult( opts, o->o?o->o:o->option, 0, o->variable.d, 0 ); break;
        case oyjlINT:    oyjlOptions_GetResult( opts, o->o?o->o:o->option, 0, 0, o->variable.i ); break;
      }
      ++i;
    }

    o = oyjlOptions_GetOption( opts, "#" );
    if(opts->argc == 1)
    {
      if(!o)
      {
        oyjlOptions_Print_( opts, 0 );
        fprintf( stderr, "%s %s\n", oyjlTermColor(oyjlRED,_("Usage Error:")), _("Optionless mode not supported. (That would need '#' option declaration.)") );
        state = oyjlOPTIONS_MISSING;
        return state;
      } else if(o->variable_type == oyjlINT && o->variable.i)
        *o->variable.i = 1;
    }

    /** Put the count of found anonymous arguments into '@' options variable.i of variable_type oyjlINT. */
    o = oyjlOptions_GetOption( opts, "@" );
    if(o && o->variable_type == oyjlINT && o->variable.i)
    {
      int count = 0;
      /* detect all '@' anonymous arguments */
      char ** results = oyjlOptions_ResultsToList( opts, "@", &count );
      *o->variable.i = count;
      oyjlStringListRelease( &results, count, free );
    }
  }

  if(opts->private_data != result) free(result);
  return state;

clean_parse:
  oyjlStringListRelease( &result->options, result->count, free );
  oyjlStringListRelease( &result->values, result->count, free );
  result->count = 0;
  result->group = -1;
  if(opts->private_data != result) free(result);

  return state;
}

/** @brief    Obtain the parsed result
 *  @memberof oyjlOptions_s
 *
 *  This function is only useful, if the results shall be obtained
 *  independently from oyjlOption_s::variable after oyjlOptions_Parse().
 *
 *  If the option was not specified the state oyjlOPTION_NONE will be
 *  returned and otherwise oyjlOPTION_USER_CHANGED. With result_int and
 *  a option type of oyjlOPTIONTYPE_NONE, the number of occurences is
 *  obtained, e.g. -vvv will give result_int = 3. A option type
 *  oyjlOPTIONTYPE_DOUBLE can ask for the floating point result with a
 *  result_dbl argument.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2019/08/04
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlOPTIONSTATE_e oyjlOptions_GetResult (
                                       oyjlOptions_s     * opts,
                                       const char        * opt,
                                       const char       ** result_string,
                                       double            * result_dbl,
                                       int               * result_int )
{
  oyjlOPTIONSTATE_e state = oyjlOPTION_NONE;
  int pos = -1, i, hits = 0, verbose = 0;
  const char * t;
  oyjlOptsPrivate_s * results;
  oyjlOption_s * o, * v;

  if(!opts)
  {
    char * t = oyjlBT(0);
    oyjlMessage_p( oyjlMSG_PROGRAM_ERROR, 0, OYJL_DBG_FORMAT "missing argument: opts\n%s", OYJL_DBG_ARGS, t );
    free(t);
    return state;
  }
  o = oyjlOptions_GetOptionL( opts, opt, 0 );
  if(!o)
    return state;
  results = opts->private_data;

  v = oyjlOptions_GetOptionL( opts, "v", 0 );
  if(v && v->variable_type == oyjlINT && v->variable.i)
    verbose = *v->variable.i;

  if(!results || !results->values)
  /* parse the command line arguments */
    state = oyjlOptions_Parse( opts );
  if(state != oyjlOPTION_NONE)
    return state;

  results = opts->private_data;
  if(!results)
    return state;

  if(opt == NULL && results->count)
  {
    if(result_int)
      *result_int = results->count;
    return oyjlOPTION_USER_CHANGED;
  }

  /* flat search */
  for(i = 0; i < results->count; ++i)
  {
    if(strcmp(results->options[i], opt) == 0)
    {
      pos = i;
      ++hits;
      state = oyjlOPTION_USER_CHANGED;
      if(verbose)
        fprintf( stderr, OYJL_DBG_FORMAT "%s[%d]: \"%s\"\n", OYJL_DBG_ARGS, opt, hits, results->values[pos] );
    }
  }
  /* object search */
  if(pos == -1)
  for(i = 0; i < results->count; ++i)
  {
    if((o->o && strcmp(results->options[i], o->o) == 0) ||
       (o->option && strcmp(results->options[i], o->option) == 0))
    {
      pos = i;
      ++hits;
      state = oyjlOPTION_USER_CHANGED;
    }
  }

  if(pos == -1)
    return oyjlOPTION_NONE;

  t = results->values[pos];

  if(result_string)
    *result_string = t;

  if(result_dbl)
  {
    oyjlStringToDouble( t, result_dbl, 0,0 );
    if( o->value_type == oyjlOPTIONTYPE_DOUBLE &&
        ( o->values.dbl.start > *result_dbl ||
          o->values.dbl.end < *result_dbl) )
    {
      char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
      char * desc = oyjlOption_PrintArg_Double_( o, 0 );
      i = pos + 1;
      oyjlOptions_Print_( opts, i );
      fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
      fprintf( stderr, "%s: \"%s\" %s %s !: %g\n", _("Option has a different value range"), oyjlTermColor(oyjlBOLD,opts->argv[i]), t, desc, *result_dbl );
      free( t );
      free( desc );
    }
  }
  if(result_int)
  {
    if(o->value_type == oyjlOPTIONTYPE_NONE)
    {
      *result_int = hits;
    } else if(t)
    {
      long lo = 0;
      if(oyjlStringToLong( t, &lo, 0 ) == 0)
        *result_int = lo;
    }
  }

  return state;
}

/** @brief    Convert the parsed content to a text list
 *  @memberof oyjlOptions_s
 *
 *  This function is part of libOyjlArgsBase.
 *
 *  @param[in]     opts                the argument object
 *  @param[in]     oc                  a filter; use NULL to get all results;
 *                                     e.g. use "@" for all anonymous results
 *  @param[out]    count               the number of matched results
 *  @return                            a possibly filterd string list of results;
 *                                     Without a filter it contains the dash(es),
 *                                     the argument Id possibly followed by equal sign '='
 *                                     and the result following. Building a
 *                                     command line call is easy.
 *                                     With oc filter it contains only results
 *                                     if apply and without Id.
 *                                     The memory is owned by caller.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2021/05/28
 *  @since   2019/03/25 (Oyjl: 1.0.0)
 */
char **  oyjlOptions_ResultsToList   ( oyjlOptions_s     * opts,
                                       const char        * oc,
                                       int               * count )
{
  char * text = NULL,
       ** list = NULL;
  oyjlOptsPrivate_s * results = NULL;
  int i,list_len = 0;
  oyjlOption_s * o = oyjlOptions_GetOptionL( opts, oc, 0 );

  if(!opts) return NULL;
  results = opts->private_data;
  if(!results || !results->values)
  {
    if(oyjlOptions_Parse( opts ))
      return NULL;

    results = opts->private_data;
    if(!results)
      return NULL;
  }

  for(i = 0; i < results->count; ++i)
  {
    const char * option = results->options[i];
    const char * value = results->values[i];
    oyjlOption_s * opt = oyjlOptions_GetOptionL( opts, option, 0 );
    int no_arg = 0;
    /** Keep the -@=XXX unbound option around in order to simplify interpreting the options.
     *  Thus sub commands like "prog opt" can be easily separated from "prog unbound.txt" .
     *  For a correct command line the "-@=" must be omitted.
     */
    int no_opt = 0; /*(opt && opt->o && (strcmp(opt->o,"#") == 0 || strcmp(opt->o,"@") == 0)); */
    int dash = 0;
    if(opt)
    {
      if(opt->value_type == oyjlOPTIONTYPE_NONE)
        no_arg = 1;

      if(opt->flags & OYJL_OPTION_FLAG_NO_DASH || no_opt)
        dash = 0;
      else if(option && option[0])
      {
        if(strlen(option) == 1)
          dash = 1;
        else dash = 2;
      }
    }

    if(oc == NULL)
#if !defined(OYJL_ARGS_BASE)
      oyjlStringAdd( &text, malloc, free, "%s%s%s%s", dash?((dash == 1)?"-":"--"):"", no_opt?"":option, no_arg || no_opt?"":"=", no_arg?"":value );
#else
    {
      if(dash) oyjlStringPush( &text, dash == 1?"-":"--", 0,0 );
      if(no_opt == 0) oyjlStringPush( &text, option, 0,0 );
      if(no_arg == 0) oyjlStringPush( &text, value, 0,0 );
    }
#endif /* OYJL_ARGS_BASE */
    else if(option && option[0] &&
            ((o->o && o->o[0] && strcmp(option,o->o) == 0) ||
             (o->option && o->option[0] && strcmp(option,o->option) == 0)))
      oyjlStringPush( &text, value, malloc, free );
    if(text)
    {
      oyjlStringListPush( &list, &list_len, text, malloc, free );
      free(text); text = NULL;
    }
  }
  if(count)
    *count = list_len;

  return list;
}

#if !defined (OYJL_ARGS_BASE)
/** @brief    Convert the parsed content to simple text
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2021/05/29
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
char * oyjlOptions_ResultsToText  ( oyjlOptions_s  * opts )
{
  char * text = NULL;
  oyjlOptsPrivate_s * results = opts->private_data;
  int i;

  if(!results || !results->values)
  {
    if(oyjlOptions_Parse( opts ))
      return NULL;

    results = opts->private_data;
    if(!results)
      return NULL;
  }

  for(i = 0; i < results->count; ++i)
  {
    const char * option = results->options[i];
    const char * value = results->values[i];
    oyjlStringAdd( &text, malloc, free, "%s=%s\n", option, value );
  }

  return text;
}

/** @internal
 *  @brief   provide identifier string for a group
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/04/18
 *  @since   2020/04/18 (Oyjl: 1.0.0)
 */
const char * oyjlOptions_GetGroupId_ ( oyjlOptions_s  *    opts,
                                       oyjlOptionGroup_s * g )
{
  char ** m_list = NULL;
  const char * group_id = NULL;
  int m = 0;

  m_list = oyjlStringSplit2( g->mandatory, "|,", 0, &m, NULL, malloc );

  if(m_list && m)
  {
    const char * option = m_list[0];
    oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
    if(option[0] != '@' && !(option[0] == '#' && m) && o)
      group_id = o->option;
  }

  oyjlStringListRelease( &m_list, m, free );
  return group_id;
}
            
/** @internal
 *  @brief    Print synopsis of a option group to stderr
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/04/13
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
char * oyjlOptions_PrintHelpSynopsis_( oyjlOptions_s  *    opts,
                                       oyjlOptionGroup_s * g,
                                       int                 style )
{
  int i;
  char ** m_list = NULL, ** on_list = NULL;
  int * m_index = NULL, * on_index = NULL;
  int m = oyjlStringDelimiterCount_(g->mandatory, ",|");
  int on = oyjlStringDelimiterCount_(g->optional, ",|");
  int opt_group = 0;
  int gstyle = style | g->flags;
  char next_delimiter, at_delimiter = '\000';
  const char * prog = opts->argv[0];
  char * text = oyjlStringCopy( "", malloc );
  if(prog && strchr(prog,'/'))
    prog = strrchr(prog,'/') + 1;

  style |= g->flags;

  if( m || on )
  {
    if(style & oyjlOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "\\fB%s\\fR", prog );
    else if(style & oyjlOPTIONSTYLE_MARKDOWN)
    {
      if(style & oyjlOPTIONSTYLE_LINK_SYNOPSIS)
        oyjlStringPush( &text, " <a href=\"#synopsis\">", malloc, free );
      oyjlStringAdd( &text, malloc, free, "<strong>%s</strong>", prog );
      if(style & oyjlOPTIONSTYLE_LINK_SYNOPSIS)
        oyjlStringPush( &text, "</a>", malloc, free );
    }
    else
      oyjlStringPush( &text, oyjlTermColor(oyjlBOLD,prog), malloc, free );
  }
  else
    return text;

  m_list = oyjlStringSplit2( g->mandatory, "|,", 0, &m, &m_index, malloc );
  for(i = 0; i < m; ++i)
  {
    char * option = m_list[i];
    oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
    next_delimiter = g->mandatory[m_index[i]];
    if(!o)
    {
      fprintf(stdout, "%s %s: option not declared: \"%s\" \"%s\"\n", oyjlBT(0), g->name?oyjlTermColor(oyjlBOLD,g->name):"---", option, g->mandatory);
      if(!getenv("OYJL_NO_EXIT")) exit(1); else return text;
    }

    if(option[0] == '@')
      at_delimiter = next_delimiter;
    else
    if(at_delimiter == '|' && at_delimiter != next_delimiter)
      at_delimiter = '\000';
    else
    if(at_delimiter == '|' && at_delimiter == next_delimiter)
      continue;
    else
    if(option[0] != '@' && !(option[0] == '#' && m+on == 1))
    {
      int s = style;
      if(i) s = style & (~OYJL_GROUP_FLAG_SUBCOMMAND);
      char * t = oyjlOption_PrintArg_(o, s);
      const char * group_id = oyjlOptions_GetGroupId_( opts, g );
      if(i == 0 && o->option && style & oyjlOPTIONSTYLE_MARKDOWN && style & oyjlOPTIONSTYLE_LINK_GROUP && group_id)
        oyjlStringAdd( &text, malloc, free, " <a href=\"#%s\">%s</a>", group_id, t );
      else
        oyjlStringAdd( &text, malloc, free, " %s", t );
      free(t);
    }

    if(next_delimiter == '|' && !(option[0] == '#') && !(option[0] == '@'))
      oyjlStringPush( &text, " |", malloc, free );
  }

  on_list = oyjlStringSplit2( g->optional, "|,", 0, &on, &on_index, malloc );
  style = style & (~OYJL_GROUP_FLAG_SUBCOMMAND);
  for(i = 0; i < on; ++i)
  {
    char * option = on_list[i];
    oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
    next_delimiter = g->optional[on_index[i]];
    gstyle = style | oyjlOPTIONSTYLE_OPTIONAL;
    if(i < on - 1 && next_delimiter == '|')
    {
      if(opt_group == 0)
        gstyle = style | oyjlOPTIONSTYLE_OPTIONAL_START | oyjlOPTIONSTYLE_OPTIONAL_INSIDE_GROUP;
      else
        gstyle = style | oyjlOPTIONSTYLE_OPTIONAL_INSIDE_GROUP;
      opt_group = 1;
    }
    else if(opt_group)
    {
      gstyle = style | oyjlOPTIONSTYLE_OPTIONAL_END;
      opt_group = 0;
    }
    else if(!o)
    {
      fprintf(stdout, "%s%s: option not declared: %s\n", oyjlBT(0), g->name?g->name:"---", &g->optional[i]);
      if(!getenv("OYJL_NO_EXIT")) exit(1);
    }
    {
      char * t = oyjlOption_PrintArg_(o, gstyle);
      oyjlStringAdd( &text, malloc, free, "%s%s", gstyle & oyjlOPTIONSTYLE_OPTIONAL_START ? " ":"", t );
      free(t);
    }
    if(next_delimiter == '|')
    {
      oyjlStringPush( &text, "|", malloc, free );
    }
  }
  for(i = 0; i < m; ++i)
  {
    char * option = m_list[i];
    oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
    next_delimiter = g->mandatory[m_index[i]];
    if(next_delimiter != '|' && !o)
    {
      fprintf(stdout, "%s %s: option not declared: %s\n", oyjlBT(0), g->name?g->name:"---", option);
      if(!getenv("OYJL_NO_EXIT")) exit(1);
    }
    if(strcmp(option, "@") == 0)
    {
      oyjlStringAdd( &text, malloc, free, " %s%s",
          o->value_name?o->value_name:"...",
          o->value_name && o->flags & OYJL_OPTION_FLAG_REPETITION ? " ..." : "" );
      at_delimiter = next_delimiter;
    }
    else
    if(at_delimiter == '|')
    {
      char * t = oyjlOption_PrintArg_(o, style);
      oyjlStringAdd( &text, malloc, free, " %c %s", at_delimiter, t );
      free(t);
    }
    else
    if(at_delimiter == '|' && at_delimiter != next_delimiter)
      at_delimiter = '\000';
  }
  oyjlStringListRelease( &m_list, m, free );
  oyjlStringListRelease( &on_list, on, free );
  free( m_index ); m_index = NULL;
  free( on_index ); on_index = NULL;
  return text;
}
#endif /* OYJL_ARGS_BASE */

/* @return  a options index in the list of a groups mandatory options
 * - >= 0 - if found
 * - -1 - if not found
 */
int oyjlOption_MandatoryIndex_       ( oyjlOption_s      * opt,
                                       oyjlOptionGroup_s * g )
{
  int found = -1;
  const char * mandatory = g->mandatory,
             * m;
  int n = 0, i;
  char ** list = oyjlStringSplit2( mandatory, "|,", 0, &n, NULL, malloc );
  for(i = 0; i < n; ++i)
  {
    m = list[i];
    if( (opt->o && strcmp(m, opt->o) == 0) ||
        (opt->option && strcmp(m, opt->option) == 0) )
      found = i;
  }
  oyjlStringListRelease( &list, n, free );
  return found;
}

oyjlOptionChoice_s * oyjlOption_EnrichInbuildFunc_( oyjlOption_s * o, int * selected OYJL_UNUSED, oyjlOptions_s * opts )
{
  /* enrich inbuild variables */
  oyjlOptionChoice_s * h_choices = NULL;

  if(OYJL_IS_O("h"))
  {
    int ng = oyjlOptions_CountGroups(opts),
        pos = 0, i;

    h_choices = (oyjlOptionChoice_s*) calloc( sizeof(oyjlOptionChoice_s), ng + 3 );

    h_choices[pos].nick = "1";
    h_choices[pos].name = _("Full Help");
    h_choices[pos].description = _("Print help for all groups");
    h_choices[pos].help = "";
    ++pos;
    h_choices[pos].nick = "synopsis";
    h_choices[pos].name = _("Synopsis");
    h_choices[pos].description = _("List groups");
    h_choices[pos].help = _("Show all groups including syntax");
    ++pos;
    for(i = 0; i < ng; ++i)
    {
      oyjlOptionGroup_s * g = &opts->groups[i];
      h_choices[pos].nick = (char*)g->name;
      h_choices[pos].name = (char*)g->description;
      h_choices[pos].description = (char*)g->help;
      ++pos;
    }

    if(o->value_name == NULL)
    {
      o->value_name = "synopsis|...";
      if(o->name == NULL)
      {
        o->name = _("Help");
        if(o->description == NULL)
        {
          o->description = _("Print help text");
          if(o->help == NULL)
            o->help = _("Show usage information and hints for the tool.");
        }
      }
    }
  }

  return h_choices;
}

/** @brief    access the oyjlOption_s::properties by key
 *  @memberof oyjlOption_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2022/06/23
 *  @since   2022/06/23 (Oyjl: 1.0.0)
 */
char * oyjlOption_PropertiesGetValue ( oyjlOption_s      * o,
                                       const char        * key )
{
  int i,n = 0;
  char ** list = NULL,
       * value = NULL;
  const char * properties = o->properties;
  if(properties && strstr( properties, key ))
  {
    list = oyjlStringSplit2( properties, "\n", 0, &n, NULL, 0 );
    for(i = 0; i < n; ++i)
    {
      char * line = list[i];
      int len = strlen(line),
          keylen = strlen(key);
      if(len > keylen && memcmp(line, key, keylen) == 0 && line[keylen] == '=')
        value = oyjlStringCopy( &line[keylen+1], 0 );
    }
    oyjlStringListRelease( &list, n, free );
    n = 0;
  }
  return value;
}

char * oyjlReadCmdToMem_             ( const char        * command,
                                       int               * size,
                                       const char        * mode,
                                       void*            (* alloc)(size_t) );
static oyjlOptionChoice_s * oyjlOptionChoice_FromPropertyFileNames_ ( oyjlOption_s * o, int * y OYJL_UNUSED, oyjlOptions_s * opts OYJL_UNUSED )
{
  oyjlOptionChoice_s * c = NULL;

  int size = 0, i,n = 0;
  char * result,
      ** list = NULL,
       * value = oyjlOption_PropertiesGetValue( o, "file_names"),
       * command = oyjlStringCopy( "ls -1 ", 0 );

  oyjlStringReplace( &value, ";", " ", malloc, free );
  oyjlStringPush( &command, value, 0,0 );
  result = oyjlReadCmdToMem_( command, &size, "r", NULL );
  if(result)
  {
    list = oyjlStringSplit2( result, "\n", 0, &n, NULL, 0 );
    free(result);
  }

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
    oyjlStringListRelease( &list, n, free );
  }

  return c;
}

static oyjlOptionChoice_s ** oyjl_get_choices_list_ = NULL;
static int * oyjl_get_choices_list_selected_;
oyjlOptionChoice_s * oyjlOption_GetChoices_ (
                                       oyjlOption_s      * o,
                                       int               * selected,
                                       oyjlOptions_s     * opts )
{
  int nopts = oyjlOptions_Count( opts ), pos = -1, i;
  if(!o) return NULL;

  for(i = 0; i < nopts; ++i)
    if(o == &opts->array[i])
      pos = i;

  if(!oyjl_get_choices_list_)
  {
    int i;
    oyjl_get_choices_list_selected_ = calloc( sizeof(int), nopts + 1 );
    for(i = 0; i < nopts; ++i) oyjl_get_choices_list_selected_[i] = -1;
    oyjl_get_choices_list_ = calloc( sizeof(oyjlOptionChoice_s*), nopts + 1 );
  }

  if( !oyjl_get_choices_list_[pos] ||
      (selected && oyjl_get_choices_list_selected_[pos] == -1) )
  {
    if(!o->values.getChoices)
      o->values.getChoices = oyjlOption_EnrichInbuildFunc_;
    oyjl_get_choices_list_[pos] = o->values.getChoices(o, selected ? &oyjl_get_choices_list_selected_[pos] : selected, opts );
    if(!oyjl_get_choices_list_[pos] && o->properties && strstr(o->properties, "file_names"))
    {
      o->values.getChoices = oyjlOptionChoice_FromPropertyFileNames_;
      oyjl_get_choices_list_[pos] = o->values.getChoices(o, selected ? &oyjl_get_choices_list_selected_[pos] : selected, opts );
    }
  }

  if(selected)
    *selected = oyjl_get_choices_list_selected_[pos];
  return oyjl_get_choices_list_[pos];
}

#if !defined (OYJL_ARGS_BASE)
#define OYJL_HELP_SUBSECTION "  "
#define OYJL_HELP_COMMAND    "    "
#define OYJL_HELP_OPTION     "      "
#define OYJL_HELP_ARG        "        "
#define OYJL_HELP_HELP       "          "
#include <stdarg.h> /* va_list */
FILE * oyjl_help_zout = NULL;
/*
 * param[in]   style                   support:
 *                                     - 0 : for printing help, returns NULL
 *                                     - oyjlOPTIONSTYLE_MAN
 *                                     - oyjlOPTIONSTYLE_MARKDOWN
 */
char * oyjlOptionChoice_Print_       ( oyjlOptionChoice_s* c,
                                       oyjlOption_s      * o,
                                       int                 style )
{
  char * t = NULL, * text = NULL;
  int has_comment = oyjlIsString_m(c->name) ||
                    oyjlIsString_m(c->description) ||
                    oyjlIsString_m(c->help);
  t = oyjlOption_PrintArg_( o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_OPTION_ONLY | style);
  switch(style)
  {
    case 0:
      oyjlStringAdd( &text, 0,0, OYJL_HELP_ARG "  %s%s%s%s%s%s%s%s%s\n",
      t?t:"", t?" ":"",
      c->nick,
      has_comment?"\t\t# ":"",
      c->name && c->nick[0] ? c->name :
        oyjlIsString_m(c->description)?c->description:"",
      c->description&&c->description[0]?" : ":"",
      c->description?c->description:"",
      c->help&&c->help[0]?" - ":"",
      c->help?c->help:"" );
    break;
    case oyjlOPTIONSTYLE_MAN:
      oyjlStringAdd( &text, malloc,free, "\t%s%s%s%s%s%s%s%s%s\n.br\n",
      t?t:"", t?" ":"",
      c->nick,
      has_comment?"\t\t# ":"",
      c->name && c->nick[0] ? c->name :
        oyjlIsString_m(c->description)?c->description:"",
      c->description&&c->description[0]?" : ":"",
      c->description?c->description:"",
      c->help&&c->help[0]?" - ":"",
      c->help?c->help:"" );
    break;
    case oyjlOPTIONSTYLE_MARKDOWN:
      oyjlStringAdd( &text, malloc,free, "   <tr><td style='padding-left:0.5em'>%s%s%s</td>%s%s%s%s%s%s%s\n",
      t?t:"", t?" ":"",
      c->nick,
      has_comment?"<td># ":"",
      c->name && c->nick[0] ? c->name :
        oyjlIsString_m(c->description)?c->description:"",
      c->description&&c->description[0]?" : ":"",
      c->description?c->description:"",
      c->help&&c->help[0]?" - ":"",
      c->help?c->help:"",
      has_comment?"</td></tr>":"" );
    break;
  }
  if(t) free(t);
  return text;
}


#endif /* OYJL_ARGS_BASE */

/** @brief    Allocate a new options structure
 *  @memberof oyjlOptions_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2019/03/24
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlOptions_s * oyjlOptions_New      ( int                 argc,
                                       const char       ** argv )
{
  oyjlOptions_s * opts = (oyjlOptions_s*) calloc( sizeof(oyjlOptions_s), 1 );
  oyjlOptsPrivate_s * results;;
  memcpy( opts->type, "oiws", 4 );

  opts->argc = argc;
  opts->argv = argv;

  results = (oyjlOptsPrivate_s*) calloc( sizeof(oyjlOptsPrivate_s), 1 );
  results->memory_allocation = oyjlMEMORY_ALLOCATION_OPTIONS;
  opts->private_data = results;

  return opts;
}

/** @brief    Allocate a new ui structure
 *  @memberof oyjlUi_s
 *
 *  The oyjlUi_s contains already options in the opts member.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2019/03/24
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlUi_s* oyjlUi_New                 ( int                 argc,
                                       const char       ** argv )
{
  oyjlUi_s * ui = calloc( sizeof(oyjlUi_s), 1 );
  memcpy( ui->type, "oiui", 4 );
  ui->opts = oyjlOptions_New( argc, argv );
  return ui;
}

#if !defined (OYJL_ARGS_BASE)
/** @brief    Copy structure
 *  @memberof oyjlUi_s
 *
 *  The oyjlUi_s string members are mostly references.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/07/31
 *  @since   2020/07/31 (Oyjl: 1.0.0)
 */
oyjlUi_s *         oyjlUi_Copy       ( oyjlUi_s          * src )
{
  oyjlUi_s * ui = NULL;
  if(!src) return ui;
  int size;

  if( *(oyjlOBJECT_e*)src != oyjlOBJECT_UI)
  {
    char * a = (char*)src;
    char type[5] = {a[0],a[1],a[2],a[3],0};
    fprintf(stderr, "Unexpected object: \"%s\"(expected: \"oyjlUi_s\")\n", type );
    return ui;
  }

  size = sizeof(oyjlUi_s);
  ui = (oyjlUi_s*)oyjlStringAppendN( NULL, (const char*)src, size, malloc );
  size = sizeof(oyjlOptions_s);
  ui->opts = (oyjlOptions_s*)oyjlStringAppendN( NULL, (const char*)src->opts, size, malloc );
  if(src->opts->private_data)
  {
    oyjlOptsPrivate_s * results = src->opts->private_data;
    if(results)
    {
      oyjlOptsPrivate_s * results_dst = (oyjlOptsPrivate_s*) calloc( 1, sizeof(oyjlOptsPrivate_s) );
      results_dst->options = oyjlStringListCatList( (const char**)results->options, results->count+1, NULL,0, &results_dst->count, malloc );
      if(results->values)
        results_dst->values = oyjlStringListCatList( (const char**)results->values, results->count+1, NULL,0, &results_dst->count, malloc );
      results_dst->group = results->group;
      results_dst->memory_allocation = oyjlMEMORY_ALLOCATION_OPTIONS;
      ui->opts->private_data = results_dst;
    }
  }

  return ui;
}
#endif /* OYJL_ARGS_BASE */

/** @internal
 *  @return                            is_double_string  */
int oyjlManAddOptionToGroupList_     ( char            *** group,
                                       int               * group_n,
                                       char                o,
                                       const char        * option,
                                       int                 flags )
{
  char * double_string = NULL; /* detect and skip */
  int is_double_string = 0, i;
  char ** g = *group;
  char oo[] = {o,0,0,0};
  const char * opt[2] = {NULL,NULL};

  if(g)
  {
    for(i = 0; i < *group_n; ++i)
    {
      char * goption = g[i];
      if(o)
      {
        if(strlen(goption) == 1 && goption[0] == o)
          double_string = goption;
      }
      else if(option)
        if(strcmp(goption, option) == 0)
          double_string = goption;
      if(double_string)
      {
        is_double_string = 1;
        break;
      }
    }
  }

  if(is_double_string)
  {
    if(!(flags & OYJL_QUIET))
    {
      fprintf( stderr, OYJL_DBG_FORMAT "attempt to add pre existing option \"%s\" to group ", OYJL_DBG_ARGS, double_string );
      for(i = 0; i < *group_n; ++i)
      {
        fprintf( stderr, "%s", i?",":"" );
        if(o)
          fprintf( stderr, "%c", o );
        else
          fprintf( stderr, "%s", option?option:"" ); 
      }
      fprintf( stderr, "; ignoring\n" );
    }
    return is_double_string;
  }

  if(o)
    opt[0] = oo;
  else if(option)
    opt[0] = option;

  if(opt[0])
    oyjlStringListAddList( group, group_n, opt, 1, 0,0 );

  return 0;
}

int oyjlManAddOptionToGroup_         ( char             ** group,
                                       char                o,
                                       const char        * option,
                                       const char        * delimiter,
                                       int                 flags )
{
  int is_double_string = 0;
  char * g = *group;
  if(g)
  {
    int list_n = 0;
    char ** list = oyjlStringSplit2( *group, "|,", 0, &list_n, NULL, malloc );
    is_double_string = oyjlManAddOptionToGroupList_( &list, &list_n, o, option, flags );

    oyjlStringListRelease( &list, list_n, free );
    if(is_double_string)
    {
      if(!(flags & OYJL_QUIET))
        fprintf( stderr, OYJL_DBG_FORMAT "attempt to add pre existing option \"%s\" to group \"%s\"; ignoring\n", OYJL_DBG_ARGS, option?option:"", *group );
      return is_double_string;
    }
  }

  if(g && g[0] && (o || option))
    oyjlStringPush( group, delimiter?delimiter:",", 0,0 );
  if(o)
  {
    oyjlStringPush( group, " ", 0,0 );
    (*group)[strlen(*group)-1] = o;
  }
  else if(option)
    oyjlStringPush( group, option, 0,0 );

  return 0;
}

static oyjlOPTIONSTATE_e oyjlUi_Check_(oyjlUi_s          * ui,
                                       int                 flags OYJL_UNUSED )
{
  oyjlOPTIONSTATE_e status = oyjlOPTION_NONE;
  int i,ng;
  oyjlOptions_s * opts;
  char * mandatory_all = NULL,
       * optional_all = NULL,
       * detail_all = NULL;
 
  if(!ui) return status;
  opts = ui->opts;

  ng = oyjlOptions_CountGroups(opts);
  if(!ng)
  {
    fprintf(stderr, "no ui::opts::groups\n");
    status = oyjlOPTION_MISSING_VALUE;
  }

  if(!ui->nick || !ui->nick[0])
  {
    fprintf(stderr, "no ui::nick\n");
    status = oyjlOPTION_MISSING_VALUE;
  }

  if(!ui->name || !ui->name[0])
  {
    fprintf(stderr, "no ui::name\n");
    status = oyjlOPTION_MISSING_VALUE;
  }

  for(i = 0; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    char ** list;
    int j, n;
    if(g->mandatory && g->mandatory[0])
    {
      n = 0;
      list = oyjlStringSplit2( g->mandatory, "|,", 0, &n, NULL, malloc );
      for( j = 0; j  < n; ++j )
      {
        const char * option = list[j];
        oyjlManAddOptionToGroup_( &mandatory_all, 0, option, ",", flags );
      }
      oyjlStringListRelease( &list, n, free );
    }
    if(g->optional && g->optional[0])
    {
      n = 0;
      list = oyjlStringSplit2( g->optional, "|,", 0, &n, NULL, malloc );
      for( j = 0; j  < n; ++j )
      {
        const char * option = list[j];
        oyjlManAddOptionToGroup_( &optional_all, 0, option, ",", flags );
      }
      oyjlStringListRelease( &list, n, free );
    }
    if(g->detail && g->detail[0])
    {
      n = 0;
      list = oyjlStringSplit2( g->detail, "|,", 0, &n, NULL, malloc );
      for( j = 0; j  < n; ++j )
      {
        const char * option = list[j];
        oyjlManAddOptionToGroup_( &detail_all, 0, option, ",", flags );
      }
      oyjlStringListRelease( &list, n, free );
    }
  }
  if(mandatory_all && detail_all)
  {
    char ** mlist;
    int mn;
    char ** list;
    int j, n;
    mlist = oyjlStringSplit2( mandatory_all, "|,", 0, &mn, NULL, malloc );
    for(i = 0; i < mn; ++i)
    {
      int found = 0;
      char * moption = mlist[i];
      if(strcmp(moption,"#") == 0) continue;
      n = 0;
      list = oyjlStringSplit2( detail_all, "|,", 0, &n, NULL, malloc );
      for( j = 0; j  < n; ++j )
      {
        const char * option = list[j];
        if(strcmp(option, moption) == 0)
          ++found;
      }
      if(!found)
      {
        fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
        fprintf(stderr, "\"%s\" not found in any group->details\n", oyjlTermColor(oyjlBOLD, moption) );
        status = oyjlOPTION_MISSING_VALUE;
      }
      oyjlStringListRelease( &list, n, free );
    }
    oyjlStringListRelease( &mlist, mn, free );
  }

  for(i = 0; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    int * d_index = NULL, d = 0;
    char ** d_list = oyjlStringSplit2( g->detail, "|,", 0, &d, &d_index, malloc );
    int j;
    if(g->mandatory && g->mandatory[0])
    {
      int n = 0;
      char ** list = oyjlStringSplit2( g->mandatory, "|,", 0, &n, NULL, malloc );
      for( j = 0; j  < n; ++j )
      {
        const char * option = list[j];
        if( !g->detail )
        {
          fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
          fprintf(stderr, "\"%s\" not found in group->details\n", option );
          status = oyjlOPTION_MISSING_VALUE;
        }
      }
      oyjlStringListRelease( &list, n, free );
    }
    for(j = 0; j < d; ++j)
    {
      const char * option = d_list[j];
      oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
      if(!o)
      {
        fprintf(stdout, "%s %s: option not declared: %s\n", oyjlBT(0), g->name?g->name:"---", option);
        if(!getenv("OYJL_NO_EXIT")) exit(1);
      } else
      switch(o->value_type)
      {
        case oyjlOPTIONTYPE_CHOICE:
          {
            int n = 0;
            while(o->values.choices.list && o->values.choices.list[n].nick && o->values.choices.list[n].nick[0] != '\000')
              ++n;
            if(!o->value_name)
            {
              char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
              fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
              fprintf( stderr, "%s%s (%s)\n", oyjlBT(0), _("This option needs oyjlOption_s::value_name defined"), t );
              if(!getenv("OYJL_NO_EXIT")) exit(1);
              status = oyjlOPTION_NOT_SUPPORTED;
            }
            if( !n && !(o->flags & OYJL_OPTION_FLAG_EDITABLE) &&
                strcmp(o->o, "h") != 0 && strcmp(o->o, "X") != 0 && strcmp(o->o, "R") != 0 )
            {
              fputs( oyjlBT(0), stderr );
              fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
              fprintf( stderr, "%s \'%s\' %s\n", _("Option not supported"), oyjlTermColor(oyjlBOLD,o->o?o->o:o->option), _("needs OYJL_OPTION_FLAG_EDITABLE or choices") );
              if(!getenv("OYJL_NO_EXIT")) exit(1);
              status = oyjlOPTION_NOT_SUPPORTED;
            }
          }
          break;
        case oyjlOPTIONTYPE_FUNCTION:
          break;
        case oyjlOPTIONTYPE_DOUBLE:
          if( o->value_type == oyjlOPTIONTYPE_DOUBLE &&
              ( o->values.dbl.start > o->values.dbl.d ||
                o->values.dbl.end < o->values.dbl.d) )
          {
            char * t = oyjlOption_PrintArg_(o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING);
            char * txt = oyjlOption_PrintArg_Double_( o, oyjlNO_BRACKETS );
            fputs( oyjlBT(0), stderr );
            fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
            fprintf( stderr, "%s \'%s\' %s\n", _("Option range error"), t, oyjlTermColor(oyjlBOLD,txt) );
            free(t);
            free(txt);
            if(!getenv("OYJL_NO_EXIT")) exit(1);
            status = oyjlOPTION_NOT_SUPPORTED;
          }
          break;
        case oyjlOPTIONTYPE_NONE:
        break;
        case oyjlOPTIONTYPE_START: break;
        case oyjlOPTIONTYPE_END: break;
      }
    }
    oyjlStringListRelease( &d_list, d, free );
    free( d_index );
  }

  if(mandatory_all) free(mandatory_all);
  if(optional_all) free(optional_all);
  if(detail_all) free(detail_all);

  return status;
}

#ifndef DOXYGEN /* doxygen shall skip this undocumented fragments */
#ifndef OYJL_ARGS_H
typedef enum {
  oyjlARGS_EXPORT_HELP,
  oyjlARGS_EXPORT_JSON,
  oyjlARGS_EXPORT_MAN,
  oyjlARGS_EXPORT_MARKDOWN,
  oyjlARGS_EXPORT_EXPORT
} oyjlARGS_EXPORT_e;
#endif
#endif
char *             oyjlUi_ToText     ( oyjlUi_s          * ui,
                                       oyjlARGS_EXPORT_e   type,
                                       int                 flags );

const char * oyjlOPTIONSTATE_eToString_( oyjlOPTIONSTATE_e i )
{
  switch(i)
  {
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_NONE);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_USER_CHANGED);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_MISSING_VALUE);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_UNEXPECTED_VALUE);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_NOT_SUPPORTED);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_DOUBLE_OCCURENCE);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTIONS_MISSING);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_NO_GROUP_FOUND);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_SUBCOMMAND);
    OYJL_ENUM_CASE_TO_STRING(oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND);
  }
  return "";
}
/** @brief    Create a new UI structure from options
 *  @memberof oyjlUi_s
 *
 *  This function is part of libOyjlArgsBase.
 *
 *  This is a high level convinience function.
 *  The returned oyjlUi_s is a comlete description of the UI and can be
 *  used instantly. The options are parsed, errors are printed, help text
 *  is printed for the boolean -h/--help option. Boolean -v/--verbose
 *  is handled too. The results are set to the declared variables. 
 *  The app_type defaults to "tool", but it can be replaced if needed.
 *
 *  @code
  oyjlUi_s * ui = oyjlUi_FromOptions ( "myCl",
                                       _("My Command"),
                                       _("My Command line tool from Me"),
                                       "my_logo",
                                       info, opts, NULL )
    @endcode
 *
 *  @param[in]     nick                four byte string; e.g. "myCl"
 *  @param[in]     name                short name of the tool; i18n;
 *                 e.g. _("My Command")
 *  @param[in]     description         compact sentence starting with full name; i18n;
 *                 e.g. _("My Command line tool from Me")
 *  @param[in]     logo                icon name; This variable must contain
 *                 the file name only, without ending. The icon needs
 *                 to be installed in typical icon search path and will be
 *                 detected there. e.g. "my_logo" points to "my_logo.{png|svg}"
 *  @param[in]     info                general information for rich UI's and
 *                                     for help text
 *  @param[in,out] opts                The main option declaration, with
 *                 syntax declaration and variable passing for setting results.
 *                 The option grouping declares
 *                 dependencies of options and provides a UI layout.
 *  @param[in,out] status              inform about processing
 *                                     - ::oyjlUI_STATE_HELP : help was detected, printed and oyjlUi_s was released
 *                                     - ::oyjlUI_STATE_EXPORT : export of json, man or markdown was detected, printed and oyjlUi_s was released
 *                                     - ::oyjlUI_STATE_VERBOSE : verbose was detected
 *                                     - ::oyjlUI_STATE_OPTION+ : error occured in option parser, message printed, ::oyjlOPTIONSTATE_e is placed in >> ::oyjlUI_STATE_OPTION and oyjlUi_s was released
 *                                     - ::oyjlUI_STATE_NO_CHECKS : skip any checks during creation; Only useful if part of the passed in data is omitted or needs to be passed through.
 *  @return                            UI object for later use
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/10/13
 *  @since   2020/10/13 (Oyjl: 1.0.0)
 */
oyjlUi_s *         oyjlUi_FromOptions( const char        * nick,
                                       const char        * name,
                                       const char        * description,
                                       const char        * logo,
                                       oyjlUiHeaderSection_s * info,
                                       oyjlOptions_s     * opts,
                                       int               * status )
{
  int help = 0, verbose = 0, version = 0, i,ng,nopts, * rank_list = 0, max = -1, pass_group = 0;
  const char * export = NULL;
  const char * help_string = NULL;
  oyjlOption_s * X, * h = NULL;
  oyjlOptionGroup_s * g = NULL;
  oyjlOPTIONSTATE_e opt_state = oyjlOPTION_NONE;
  oyjlOptsPrivate_s * results;
  int flags = 0, optionless = 0;//, anonymous = 0;
  char * t;

  if(status)
    flags = *status;

  /* allocate options structure */
  oyjlUi_s * ui = calloc( sizeof(oyjlUi_s), 1 ); /* argc+argv are required for parsing the command line options */
  memcpy( ui->type, "oiui", 4 );
  ui->opts = opts;
  /* tell about the tool */
  if(!(flags & oyjlUI_STATE_NO_CHECKS))
    ui->app_type = "tool";
  ui->nick = nick;
  ui->name = name;
  ui->description = description;
  ui->logo = logo;

  /* Select from *version*, *manufacturer*, *copyright*, *license*, *url*,
   * *support*, *download*, *sources*, *oyjl_module_author* and
   * *documentation* what you see fit. Add new ones as needed. */
  ui->sections = info;

  /* get results and check syntax ... */
  results = ui->opts->private_data;
  if(!results || !results->values)
    opt_state = oyjlOptions_Parse( ui->opts );
  if(opt_state == oyjlOPTION_NOT_SUPPORTED)
    goto FromOptions_done;
  results = ui->opts->private_data;

  version = oyjlOptions_IsOn_( ui->opts, "V" );
  X = oyjlOptions_GetOption( ui->opts, "X" );
  if(X && X->variable_type == oyjlSTRING && X->variable.s)
    export = *X->variable.s;
  /* Give "json+command" and "json" priority over e.g. "man" and others */
  if(export)
  {
    if(oyjlOptions_HasValue_( ui->opts, "X", "json+command", OYJL_CASE_COMPARE | OYJL_SET ))
      export = *X->variable.s;
    else
    if(oyjlOptions_HasValue_( ui->opts, "X", "json", OYJL_CASE_COMPARE | OYJL_SET ))
      export = *X->variable.s;
  }
  help = oyjlOptions_IsOn_( ui->opts, "h" );;
  h = oyjlOptions_GetOption( ui->opts, "h" );
  verbose = oyjlOptions_IsOn_( ui->opts, "v" );
  if(verbose)
  {
    if(status && verbose)
      *status |= oyjlUI_STATE_VERBOSE;
    if(verbose)
    {
      flags |= oyjlUI_STATE_VERBOSE;
      fprintf( stderr, "verbose %d\n", verbose );
    }
  }
  /* enrich inbuild */
  oyjlOptions_GetOption( ui->opts, "R" );

  /* detect valid group match(es) and report missing mandatory one */
  ng = oyjlOptions_CountGroups(ui->opts);
  if(ng)
    oyjlAllocHelper_m( rank_list, int, ng, malloc, if(!(flags&oyjlUI_STATE_NO_RELEASE)) oyjlUi_ReleaseArgs( &ui); return NULL );

  for(i = 0; i < ng; ++i)
  {
    int mgoup_index = 0;
    oyjlOption_s * o;
    const char * option;
    char ** list = NULL;
    int n = 0, j;
    g = &ui->opts->groups[i];
    /* test mandatory options for correct numbers */
    if(g->mandatory && g->mandatory[0])
    {
      const char * mandatory = g->mandatory;
      int k, found = 0;

      list = oyjlStringSplit2( mandatory, "|,", 0, &n, NULL, malloc );

      if( strchr(mandatory,'#') != NULL &&
          results ? (results->count == 0) : 0 )
        optionless = 1;

      ++mgoup_index;

      for( j = 0; j  < n; ++j )
      {
        const char * moption = list[j];
        o = oyjlOptions_GetOptionL( ui->opts, moption, 0 );
        if(!o)
        {
          fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
          fprintf( stderr, "%s g[%d]=%s.mandatory=%s[%d](%s)\n", _("This option is not defined"), i, g->name,g->mandatory,j, moption && moption[0]?moption:"---" );
        }
        for( k = 0; k  < (results?results->count:0); ++k )
        {
          const char * roption = results->options[k];
          if(strcmp(roption, "h") == 0)
          {
            const char * v = results->values[k];
            o = NULL;
            if(strcmp(v, "1") != 0) /* skip --help no arg results */
              o = oyjlOptions_GetOptionL( ui->opts, v, 0 );
            if(o)
              roption = o->o?o->o:o->option;
            if(g->name && strcmp(v,g->name) == 0)
              ++found;
          }
          if( strcmp(moption, roption) == 0 &&
              strcmp(roption, "h") != 0 )
            ++found;
          if(i == mgoup_index && j == 0)
          {
            o = oyjlOptions_GetOptionL( ui->opts, roption, 0 );
            if(o && o->flags & OYJL_OPTION_FLAG_MAINTENANCE)
              ++pass_group; 
          }
        }
      }
      oyjlStringListRelease( &list, n, free );
      rank_list[i] = found;
      if(found && max < found)
        max = found;
    } else
    if(help)
    {
      int k, found = 0;
      for( k = 0; k  < (results?results->count:0); ++k )
      {
        const char * roption = results->options[k];
        if(strcmp(roption, "h") == 0)
        {
          const char * v = results->values[k];
          if(g->name && strcmp(v,g->name) == 0)
            ++found;
        }
      }
      rank_list[i] = found;
      if(found && max < found)
        max = found;
      if(h && h->variable_type == oyjlSTRING && h->variable.s)
        help_string = *h->variable.s;
      if( help_string &&
          strcmp(help_string, "oyjl-list") == 0 &&
          h->value_type == oyjlOPTIONTYPE_FUNCTION )
      {
        int selected = 0;
        oyjlOptionChoice_s * choices = oyjlOption_EnrichInbuildFunc_( h, &selected, opts );
        int n = oyjlOptionChoice_Count( choices );
        for(k = 0; k < n; ++k)
          fprintf( stdout, "%s\n", choices[k].nick );
        //oyjlOptionChoice_Release( &choices );
        free(rank_list);
        free(choices);
        if(!(flags&oyjlUI_STATE_NO_RELEASE))
        {
          if(verbose)
            oyjlOptions_Print_( ui->opts, 0 );
          oyjlUi_ReleaseArgs( &ui);
        }
        if(status)
          *status |= oyjlUI_STATE_EXPORT;
        return NULL;
      }
    }

    list = oyjlStringSplit2( g->optional, "|,", 0, &n, NULL, malloc );
    for( j = 0; j  < n; ++j )
    {
      option = list[j];
      o = oyjlOptions_GetOptionL( ui->opts, option, 0 );
      if(!o)
      {
        fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
        fprintf( stderr, "%s g[%d]=%s.optional=%s[%d](%s)\n", _("This option is not defined"), i,g->name,g->optional,j, option && option[0]?option:"---" );
      }
    }
    oyjlStringListRelease( &list, n, free );

    list = oyjlStringSplit2( g->detail, "|,", 0, &n, NULL, malloc );
    for( j = 0; j  < n; ++j )
    {
      option = list[j];
      o = oyjlOptions_GetOptionL( ui->opts, option, 0 );
      if(!o)
      {
        fputs( oyjlTermColor(oyjlRED,_("Program Error:")), stderr ); fputs( " ", stderr );
        fprintf( stderr, "%s g[%d]=%s.detail=%s[%d](%s)\n", _("This option is not defined"), i,g->name,g->detail,j, option && option[0]?option:"---" );
      }
    }
    oyjlStringListRelease( &list, n, free );
  }

  if(results && results->count == 0)
  {
    oyjlOption_s * o = oyjlOptions_GetOption( ui->opts, "#" );
    if(o && o->flags & OYJL_OPTION_FLAG_MAINTENANCE)
      optionless = 1;
  }

  if(max > -1)
  {
    for(i = 0; i < ng; ++i)
      if(rank_list[i] == max)
      {
        g = &ui->opts->groups[i];
        if(results)
          results->group = i;
        break;
      }
  }
  else if(!optionless && opt_state != oyjlOPTION_NOT_SUPPORTED && !help && !pass_group && !version && !export)
  {
    if(opt_state == oyjlOPTION_NONE || opt_state)
      oyjlOptions_Print_( ui->opts, 0 );
    fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
    /* "Missing mandatory option. No usage mode in synopsis lines found." */
    fprintf( stderr, "%s %s %s\n", _("Missing mandatory option. No usage mode in"), oyjlTermColor(oyjlBOLD,_("Synopsis")), _("lines found.") );

    if(opt_state == oyjlOPTION_NONE)
      opt_state = oyjlOPTION_NO_GROUP_FOUND;

    if(opt_state != oyjlOPTIONS_MISSING)
    {
      char * t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_HELP, -2 );
      if(t) { puts( t ); free(t); t = NULL; }
    }
  }
  free(rank_list);
  if(results && results->group >= 0)
  {
    i = results->group;
    g = &ui->opts->groups[i];
    if(g->flags & OYJL_GROUP_FLAG_SUBCOMMAND)
    {
      int li_n = 0, i;
      char ** li = oyjlStringSplit2( g->mandatory, "|,", 0, &li_n, NULL, 0 );
      const char * first_opt = results->options[0];
      int found_mandatory_at_first_pos = 0;
      for( i = 0; i < li_n; ++i )
        if(strcmp(first_opt, li[i]) == 0)
          ++found_mandatory_at_first_pos;
      opt_state = oyjlOPTION_SUBCOMMAND;

      if(!found_mandatory_at_first_pos)
      {
        oyjlOptions_Print_( ui->opts, 1 );
        fputs( oyjlTermColor(oyjlRED,_("Usage Error:")), stderr ); fputs( " ", stderr );
        fprintf(stderr, "%s:", _("A mandatory sub command option needs to be placed first") );
        for( i = 0; i < li_n; ++i )
        {
          const char * option = li[i];
          oyjlOption_s * o = oyjlOptions_GetOptionL( ui->opts, option, 0 );
          t = oyjlOption_PrintArg_( o, oyjlOPTIONSTYLE_STRING | OYJL_GROUP_FLAG_SUBCOMMAND );
          fprintf(stderr, " %s", oyjlTermColor(oyjlBOLD, t));
          free(t);
        }
        fprintf(stderr, "\n");
        if(status)
          *status |= oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND;
        opt_state = oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND;
      }
      oyjlStringListRelease( &li, li_n, 0 );
    }
  }

  if(opt_state != oyjlOPTION_NONE && *oyjl_debug)
  {
    fputs( oyjlOPTIONSTATE_eToString_( opt_state ), stderr );
    fputs( "\n", stderr );
  }

  if( opt_state == oyjlOPTIONS_MISSING ||
      (opt_state == oyjlOPTION_MISSING_VALUE && results && results->group >= 0) )
  {
    char * t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_HELP, results && results->group >= 0?-3:-2 );
    if(t) { puts( t ); free(t); t = NULL; }
    if(!(flags&oyjlUI_STATE_NO_RELEASE))
      oyjlUi_ReleaseArgs( &ui);
    if(status)
      *status |= oyjlUI_STATE_HELP;
    return NULL;
  }
  if( ( opt_state == oyjlOPTION_NONE ||
        opt_state == oyjlOPTION_SUBCOMMAND ) &&
      !(flags & oyjlUI_STATE_NO_CHECKS)
    )
    opt_state = oyjlUi_Check_(ui, *oyjl_debug?0:OYJL_QUIET );

  if(version)
  {
    oyjlUiHeaderSection_s * version = oyjlUi_GetHeaderSection( ui, "version" );
    oyjlUiHeaderSection_s * author = oyjlUi_GetHeaderSection( ui, "manufacturer" );
    oyjlUiHeaderSection_s * copyright = oyjlUi_GetHeaderSection( ui, "copyright" );
    oyjlUiHeaderSection_s * license = oyjlUi_GetHeaderSection( ui, "license" );
    char * prog = NULL;
    char * v = version && version->name ? oyjlStringCopy( oyjlTermColor( oyjlITALIC, version->name ), NULL ) : NULL;

    if(!verbose && ui->opts->argv[0] && strchr(ui->opts->argv[0],'/'))
      prog = oyjlStringCopy( oyjlTermColor( oyjlBOLD, strrchr(ui->opts->argv[0],'/') + 1 ), NULL );
    else
      prog = oyjlStringCopy( oyjlTermColor( oyjlBOLD, ui->opts->argv[0] ), NULL );
    fprintf( stdout, "%s v%s%s%s%s - %s\n%s\n%s%s%s\n%s%s%s\n\n", prog,
                                      v ? v : "",
                                      version && version->description && verbose ? "(" : "",
                                      version && version->description && verbose ? version->description : "",
                                      version && version->description && verbose ? ")" : "",
                                      ui->description ? ui->description : ui->name ? ui->name : "",
                                      copyright && copyright->name ? copyright->name : "",
                                      license ? _("License"):"", license?":\t":"", license && license->name ? license->name : "",
                                      author ? _("Author"):"", author?": \t":"", author && author->name ? author->name : "" );
    if(!(flags&oyjlUI_STATE_NO_RELEASE))
    {
      if(verbose)
        oyjlOptions_Print_( ui->opts, 0 );
      oyjlUi_ReleaseArgs( &ui);
    }
    free(prog);
    if(v) free(v);
    if(status)
      *status |= oyjlUI_STATE_HELP;
    return NULL;
  }

  if(export)
  {
    if(status)
      *status |= oyjlUI_STATE_EXPORT;
    if(strcmp(export, "json") == 0)
    {
      t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_JSON, flags );
      if(t) { puts( t ); free(t); }
      if(!(flags&oyjlUI_STATE_NO_RELEASE))
      {
        if(verbose)
          oyjlOptions_Print_( ui->opts, 0 );
        oyjlUi_ReleaseArgs( &ui);
      }
      return NULL;
    }
    if(strcmp(export, "json+command") == 0)
    {
      return ui;
    }
    if(strcmp(export, "man") == 0)
    {
      t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_MAN, flags );
      if(t) { puts( t ); free(t); }
      if(!(flags&oyjlUI_STATE_NO_RELEASE))
      {
        if(verbose)
          oyjlOptions_Print_( ui->opts, 0 );
        oyjlUi_ReleaseArgs( &ui);
      }
      return NULL;
    }
    if(strcmp(export, "markdown") == 0)
    {
      t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_MARKDOWN, flags );
      if(t) { puts( t ); free(t); }
      if(!(flags&oyjlUI_STATE_NO_RELEASE))
      {
        if(verbose)
          oyjlOptions_Print_( ui->opts, 0 );
        oyjlUi_ReleaseArgs( &ui);
      }
      return NULL;
    }
    if(strcmp(export, "export") == 0)
    {
      t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_EXPORT, flags );
      if(t) { puts( t ); free(t); }
      if(!(flags&oyjlUI_STATE_NO_RELEASE))
      {
        if(verbose)
          oyjlOptions_Print_( ui->opts, 0 );
        oyjlUi_ReleaseArgs( &ui);
      }
      return NULL;
    }
  }

  if( help &&
      ( opt_state == oyjlOPTION_NONE ||
        opt_state == oyjlOPTION_MISSING_VALUE ||
        opt_state == oyjlOPTION_SUBCOMMAND ||
        opt_state == oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND )
    )
  {
    oyjlOption_s * synopsis = oyjlOptions_GetOptionL( ui->opts, "synopsis", 0 );
    char * t = oyjlUi_ToText( ui, oyjlARGS_EXPORT_HELP, (results && results->group >= 0) ? -1 : (results && results->count >= 1 && strcasecmp(results->values[0],"synopsis") == 0 && synopsis) ? -2 : verbose );
    if(t) { puts( t ); free(t); t = NULL; }
    if(!(flags&oyjlUI_STATE_NO_RELEASE))
    {
      if(verbose)
        oyjlOptions_Print_( ui->opts, 0 );
      oyjlUi_ReleaseArgs( &ui);
    }
    if(status)
      *status |= oyjlUI_STATE_HELP;
    return NULL;
  }

  nopts = oyjlOptions_Count( ui->opts );
  if(opt_state != oyjlOPTION_NOT_ALLOWED_AS_SUBCOMMAND)
  for(i = 0; i < nopts; ++i)
  {
    const char * value = NULL;
    oyjlOption_s * o = &ui->opts->array[i];
    oyjlOptions_GetResult( opts, o->o?o->o:o->option, &value, 0, 0 );
    if( value &&
        strcmp(value, "oyjl-list") == 0 &&
        o->value_type == oyjlOPTIONTYPE_FUNCTION &&
        !(export && strcmp(export, "json+command") == 0))
    {
      int n = 0,l;
      oyjlOptionChoice_s * list;
      list = oyjlOption_GetChoices_(o, NULL, opts );
      if(list)
        while(list[n].nick && list[n].nick[0] != '\000')
          ++n;
      for(l = 0; l < n; ++l)
        fprintf( stdout, "%s\n", list[l].nick );
      /* not possible, as the result of oyjlOption_GetChoices_() is cached - oyjlOptionChoice_Release( &list ); */

      if(n == 0) break;
      if(status)
        *status |= oyjlUI_STATE_EXPORT;
      if(!(flags&oyjlUI_STATE_NO_RELEASE))
      {
        if(verbose)
          oyjlOptions_Print_( ui->opts, 0 );
        oyjlUi_ReleaseArgs( &ui);
      }
      return NULL;
    }
    if( value &&
        strcmp(value, "oyjl-list") == 0 &&
        o->value_type == oyjlOPTIONTYPE_CHOICE )
    {
      int n = 0,l;
      oyjlOptionChoice_s * list = o->values.choices.list;
      if(list)
        while(list[n].nick && list[n].nick[0] != '\000')
          ++n;
      for(l = 0; l < n; ++l)
        fprintf( stdout, "%s\n", list[l].nick );

      if(n == 0) break;
      if(status)
        *status |= oyjlUI_STATE_EXPORT;
      if(!(flags&oyjlUI_STATE_NO_RELEASE))
      {
        if(verbose)
          oyjlOptions_Print_( ui->opts, 0 );
        oyjlUi_ReleaseArgs( &ui);
      }
      return NULL;
    }
  }
  /* done with options handling */

  FromOptions_done:
  /* ... and report detected errors */
  if(!export && !version && opt_state != oyjlOPTION_NONE)
  {
    fputs( "\n", stderr );
    fputs( _("... try with --help|-h option for usage text. give up"), stderr );
    fputs( "\n", stderr );
    if(!(flags&oyjlUI_STATE_NO_RELEASE))
      oyjlUi_ReleaseArgs( &ui);
    if(status)
      *status = opt_state << oyjlUI_STATE_OPTION;
    return NULL;
  }

  return ui;
}

/** @brief    Create a new UI structure
 *  @memberof oyjlUi_s
 *
 *  This is a high level convinience function.
 *  The returned oyjlUi_s is a comlete description of the UI and can be
 *  used instantly. The options are parsed, errors are printed, help text
 *  is printed for the boolean -h/--help option. Boolean -v/--verbose
 *  is handled too. The results are set to the declared variables. 
 *  The app_type defaults to "tool", but it can be replaced if needed.
 *
 *  @code
  oyjlUi_s * ui = oyjlUi_Create( argc, argv,
                                       "myCl",
                                       _("My Command"),
                                       _("My Command line tool from Me"),
                                       "my_logo",
                                       info, options, groups, NULL )
    @endcode
 *
 *  @param[in]     argc                number of command line arguments
 *  @param[in]     argv                command line args from C/C++ main()
 *  @param[in]     nick                four byte string; e.g. "myCl"
 *  @param[in]     name                short name of the tool; i18n;
 *                 e.g. _("My Command")
 *  @param[in]     description         compact sentence starting with full name; i18n;
 *                 e.g. _("My Command line tool from Me")
 *  @param[in]     logo                icon name; This variable must contain
 *                 the file name only, without ending. The icon needs
 *                 to be installed in typical icon search path and will be
 *                 detected there. e.g. "my_logo" points to "my_logo.{png|svg}"
 *  @param[in]     info                general information for rich UI's and
 *                                     for help text
 *  @param[in,out] options             the main option declaration, with
 *                 syntax declaration and variable passing for setting results
 *  @param[in]     groups              the option grouping declares
 *                 dependencies of options and provides a UI layout
 *  @param[in,out] status              inform about processing
 *                                     - ::oyjlUI_STATE_HELP : help was detected, printed and oyjlUi_s was released
 *                                     - ::oyjlUI_STATE_EXPORT : export of json, man or markdown was detected, printed and oyjlUi_s was released
 *                                     - ::oyjlUI_STATE_VERBOSE : verbose was detected
 *                                     - ::oyjlUI_STATE_OPTION+ : error occured in option parser, message printed, ::oyjlOPTIONSTATE_e is placed in >> ::oyjlUI_STATE_OPTION and oyjlUi_s was released
 *                                     - ::oyjlUI_STATE_NO_CHECKS : skip any checks during creation; Only useful if part of the passed in data is omitted or needs to be passed through.
 *  @return                            UI object for later use
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/04/05
 *  @since   2018/08/20 (OpenICC: 0.1.1)
 */
oyjlUi_s *  oyjlUi_Create            ( int                 argc,
                                       const char       ** argv,
                                       const char        * nick,
                                       const char        * name,
                                       const char        * description,
                                       const char        * logo,
                                       oyjlUiHeaderSection_s * info,
                                       oyjlOption_s      * options,
                                       oyjlOptionGroup_s * groups,
                                       int               * status )
{
  /* allocate options structure */
  oyjlOptions_s * opts = oyjlOptions_New( argc, argv ); /* argc+argv are required for parsing the command line options */
  opts->array = options;
  opts->groups = groups;
  oyjlUi_s * ui = oyjlUi_FromOptions( nick, name, description, logo, info, opts, status );

  return ui;
}

/** @brief    Release "oiui"
 *  @memberof oyjlUi_s
 *
 *  Release oyjlUi_s::opts, oyjlUi_s::private_data and oyjlUi_s.
 *
 *  @version Oyjl: 1.0.0
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
void           oyjlUi_ReleaseArgs ( oyjlUi_s      ** ui OYJL_UNUSED )
{
#if !defined(OYJL_ARGS_BASE)
  int memory_allocation = 0;
  if(!ui || !*ui) return;
  if( *(oyjlOBJECT_e*)*ui != oyjlOBJECT_UI)
  {
    char * a = (char*)*ui;
    char type[5] = {a[0],a[1],a[2],a[3],0};
    fprintf(stderr, "Unexpected object: \"%s\"(expected: \"oyjlUi_s\")\n", type );
    return;
  }
  if((*ui)->opts->private_data)
  {
    oyjlOptsPrivate_s * results = (*ui)->opts->private_data;
    if(results)
    {
      memory_allocation = results->memory_allocation;
      if(results->memory_allocation)
      {
        int nopts = oyjlOptions_Count( (*ui)->opts ), i;
        if(memory_allocation & oyjlMEMORY_ALLOCATION_ARRAY)
        for(i = 0; i < nopts; ++i)
        {
          oyjlOption_s * opt = &(*ui)->opts->array[i];
          if(opt->value_type == oyjlOPTIONTYPE_CHOICE && opt->values.choices.list)
            free(opt->values.choices.list);
        }
        if((*ui)->sections && memory_allocation & oyjlMEMORY_ALLOCATION_SECTIONS)
          free((*ui)->sections);
        if((*ui)->opts->array && memory_allocation & oyjlMEMORY_ALLOCATION_ARRAY)
          free((*ui)->opts->array);
        if((*ui)->opts->groups && memory_allocation & oyjlMEMORY_ALLOCATION_GROUPS)
          free((*ui)->opts->groups);
      }
      oyjlStringListRelease( &results->options, results->count, free );
      oyjlStringListRelease( &results->values,  results->count, free );
      results->count = 0;
      free((*ui)->opts->private_data);
    }
  }
  if((*ui)->opts && memory_allocation & oyjlMEMORY_ALLOCATION_OPTIONS) free((*ui)->opts);
  free((*ui));
  *ui = NULL;
#endif /* OYJL_ARGS_BASE */
}

/** @brief    Return the number of sections of type "oihs"
 *  @memberof oyjlUiHeaderSection_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/06/01
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
int     oyjlUiHeaderSection_Count    ( oyjlUiHeaderSection_s * sections )
{
  int n = 0;
  if(!sections) return 0;
  while( *(oyjlOBJECT_e*)&sections[n] /*"oihs"*/ == oyjlOBJECT_UI_HEADER_SECTION) ++n;
  return n;
}

/** @brief    Add new header section at end
 *  @memberof oyjlUiHeaderSection_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/07/16
 *  @since   2020/06/05 (Oyjl: 1.0.0)
 */
oyjlUiHeaderSection_s * oyjlUiHeaderSection_Append (
                                       oyjlUiHeaderSection_s * sections,
                                       const char        * nick,
                                       const char        * label,
                                       const char        * name,
                                       const char        * description )
{
  int n = oyjlUiHeaderSection_Count( sections );
  oyjlUiHeaderSection_s * info = (oyjlUiHeaderSection_s*) calloc(sizeof(oyjlUiHeaderSection_s), n+2);

  if(!info) return NULL;

  if(n)
    memcpy( info, sections, sizeof(oyjlUiHeaderSection_s) * n );

  sprintf( info[n].type, "%s", "oihs" );
  info[n].nick = nick;
  info[n].label = label;
  info[n].name = name;
  info[n].description = description;

  return info;
}

/** @brief    Return the section which was specified by its nick name
 *  @memberof oyjlUi_s
 *
 *  @version Oyjl: 1.0.0
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
oyjlUiHeaderSection_s * oyjlUi_GetHeaderSection (
                                       oyjlUi_s          * ui,
                                       const char        * nick )
{
  oyjlUiHeaderSection_s * section = NULL;
  int i, count = oyjlUiHeaderSection_Count(ui->sections);
  for(i = 0; i < count; ++i)
    if( strcmp(ui->sections[i].nick, nick) == 0 )
      section = &ui->sections[i];
  return section;
}

#if !defined (OYJL_ARGS_BASE)
char *       oyjlStringToUpper_      ( const char        * t )
{
  char * text = oyjlStringCopy(t, malloc);
  int slen = strlen(t), i;
  for(i = 0; i < slen; ++i)
    text[i] = toupper(t[i]);
  return text;
}
char *       oyjlStringToLower_      ( const char        * t )
{
  char * text = oyjlStringCopy(t, malloc);
  int slen = strlen(t), i;
  for(i = 0; i < slen; ++i)
    text[i] = tolower(t[i]);
  return text;
}


#define ADD_SECTION( sec, link, format, ... ) { \
  oyjlStringAdd( &text, malloc, free, "\n<h2>%s <a href=\"#toc\" name=\"%s\">&uarr;</a></h2>\n\n" format, sec, link, __VA_ARGS__ ); \
  oyjlStringListPush( sections, sn, sec, 0,0 ); \
  oyjlStringListPush( sections, sn, link, 0,0 ); }

static char * oyjlExtraManSection_   ( oyjlOptions_s     * opts,
                                       const char        * opt_name,
                                       int                 flags,
                                       char            *** sections,
                                       int               * sn )
{
  oyjlOption_s * o = oyjlOptions_GetOptionL( opts, opt_name, 0 );
  char * text = NULL;
  if(o)
  {
    int n = 0,l;
    if(o->value_type == oyjlOPTIONTYPE_CHOICE)
    {
      oyjlOptionChoice_s * list = o->values.choices.list;
      if(!list)
      {
        fprintf( stderr, "%s %s", oyjlTermColor(oyjlRED,_("Program Error:")), _("Missing choices list") );
        char * t = oyjlOption_PrintArg_( o, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING );
        fprintf( stderr, " %s\n", oyjlTermColor(oyjlBOLD, t) );
        free(t);
        return text;
      }
      while((list[n].nick && list[n].nick[0] != '\000') || (list[n].name && list[n].name[0] != '\000')) ++n;
      if(n)
      {
        char * up = oyjlStringToUpper_( &opt_name[4] );
        oyjlStringReplace( &up, "_", " ", malloc, free );
        const char * section = up;
        if(strcasecmp(section,"EXAMPLES") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("EXAMPLES"):_("Examples");
        else if(strcasecmp(section,"EXIT-STATE") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("EXIT-STATE"):_("Exit State");
        else if(strcasecmp(section,"ENVIRONMENT VARIABLES") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("ENVIRONMENT VARIABLES"):_("Environment Variables");
        else if(strcasecmp(section,"HISTORY") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("HISTORY"):_("History");
        else if(strcasecmp(section,"FILES") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("FILES"):_("Files");
        else if(strcasecmp(section,"SEE AS WELL") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("SEE AS WELL"):_("See As Well");
        else if(strcasecmp(section,"SEE ALSO") == 0)
          section = (flags & oyjlOPTIONSTYLE_MARKDOWN || flags & oyjlOPTIONSTYLE_MAN)?_("SEE ALSO"):_("See Also");
        if(flags & oyjlOPTIONSTYLE_MARKDOWN)
        {
          char * low = oyjlStringToLower_( &opt_name[4] );
          oyjlStringReplace( &low, "_", "", malloc, free );
          ADD_SECTION( _(section), low, "", "" )
          free(low);
        }
        else if(flags & oyjlOPTIONSTYLE_MAN)
          oyjlStringAdd( &text, malloc, free, ".SH %s\n", _(section) );
        else
          oyjlStringAdd( &text, malloc, free, "\n%s:", oyjlTermColor( oyjlBOLD,_(o->name&&o->name[0]?o->name:section) ) );
        for(l = 0; l < n; ++l)
        {
          if(flags & oyjlOPTIONSTYLE_MARKDOWN)
          {
            if( strcmp(up,"SEE AS WELL") == 0 ||
                strcmp(up,"SEE ALSO") == 0 )
            {
              int li_n = 0, i;
              char ** li = oyjlStringSplit2( list[l].nick, 0, 0, &li_n, NULL, malloc );
              for(i = 0; i < li_n; ++i)
              {
                char * md = oyjlStringCopy( li[i], 0 );
                int len = strlen(md), is_man_page = 0;
                if(len > 3 && md[len-3] == '(' && md[len-1] == ')')
                  ++is_man_page;
                if(is_man_page)
                {
                  char * end = oyjlStringCopy( &md[len-3], malloc );
                  char * t;
                  md[len-3] = '\000';
                  t = oyjlStringCopy( md, 0 );
                  oyjlStringReplace( &md, "-", "", malloc, free );

                  oyjlStringAdd( &text, malloc, free, "&nbsp;&nbsp;[%s](%s.html)<a href=\"%s.md\">%s</a>", t, md, md, end );
                  free( t );
                  free( end );
                }
                else
                  oyjlStringAdd( &text, malloc, free, " %s", md );

                free( md );
              }
              oyjlStringPush( &text, "\n\n", malloc, free );
              oyjlStringListRelease( &li, li_n, free );
            }
            else
            oyjlStringAdd( &text, malloc, free, "#### %s\n", list[l].nick[0] ? list[l].nick : list[l].name && list[l].name[0] ? list[l].name : "" );
            if(list[l].nick[0] && list[l].name && list[l].name[0])
            {
              if(strlen(list[l].name) > 5 && memcmp(list[l].name,"http",4) == 0)
                oyjlStringAdd( &text, malloc, free, "&nbsp;&nbsp;<a href=\"%s\">%s</a>\n", list[l].name, list[l].name );
               else
                oyjlStringAdd( &text, malloc, free, "&nbsp;&nbsp;%s\n", list[l].name );
            }
            if(list[l].nick[0] && list[l].name && list[l].name[0] && list[l].description && list[l].description[0])
            oyjlStringPush( &text, "  <br />\n", malloc, free );
            if(list[l].description && list[l].description[0])
            oyjlStringAdd( &text, malloc, free, "&nbsp;&nbsp;%s\n", list[l].description );
            if(list[l].help && list[l].help[0])
            oyjlStringAdd( &text, malloc, free, "  <br />\n&nbsp;&nbsp;%s\n", list[l].help );
          }
          else if(flags & oyjlOPTIONSTYLE_MAN)
          {
            oyjlStringPush( &text, ".TP\n", malloc, free );
            if(list[l].nick && list[l].nick[0])
            oyjlStringAdd( &text, malloc, free, "%s\n.br\n", list[l].nick );
            if(list[l].name && list[l].name[0])
            oyjlStringAdd( &text, malloc, free, "%s\n", list[l].name );
            if(list[l].description && list[l].description[0])
            oyjlStringAdd( &text, malloc, free, ".br\n%s\n", list[l].description );
            if(list[l].help && list[l].help[0])
            oyjlStringAdd( &text, malloc, free, ".br\n%s\n", list[l].help );
          }
          else
          {
            oyjlTEXTMARK_e mark = oyjlBOLD;
            if( strcmp(up,"SEE AS WELL") == 0 ||
                strcmp(up,"SEE ALSO") == 0 )
              mark = oyjlNO_MARK;
            oyjlStringPush( &text, "\n", malloc, free );
            if(list[l].nick && list[l].nick[0])
            oyjlStringAdd( &text, malloc, free, "  %s\n", oyjlTermColor( mark,list[l].nick ) );
            if(list[l].name && list[l].name[0])
            oyjlStringAdd( &text, malloc, free, "    %s\n", !(list[l].nick && list[l].nick[0]) ? oyjlTermColor( mark,list[l].name) : list[l].name );
            if(list[l].description && list[l].description[0])
            oyjlStringAdd( &text, malloc, free, "    %s\n", list[l].description );
            if(list[l].help && list[l].help[0])
            oyjlStringAdd( &text, malloc, free, "    %s\n", list[l].help );
          }
        }
        free(up);
      }
    }
  }
  return text;
}

static char * oyjlExtraManSections_( oyjlOptions_s  * opts, int flags, char *** sections, int * sn )
{
  char * text = NULL;
  int nopts = oyjlOptions_Count( opts );
  int l;
  for(l = 0; l < nopts; ++l)
  {
    oyjlOption_s * o = &opts->array[l];
    const char * option = o->option;
    int olen = option ? strlen(option) : 0;
    if(olen > 7 && option[0] == 'm' && option[1] == 'a' && option[2] == 'n' && option[3] == '-')
    {
      char * tmp = oyjlExtraManSection_(opts, option, flags, sections, sn);
      if(tmp)
      {
        oyjlStringPush( &text, tmp, malloc, free );
        free(tmp);
      }
    }
  }
  return text;
}

/** @brief    Return a MAN page from options
 *  @memberof oyjlUi_s
 *
 *  Some manual pages (MAN pages) might contain some additional sections.
 *  They are supported as options. To generate a custom MAN page section,
 *  add a blind option to your options list and set the oyjlOption_s::o
 *  char to something non interupting like, dot '.' or similar.
 *  The oyjlOption_s::option string
 *  contains "man-section_head", with "section_head" being adapted to your
 *  needs. The "man-" identifier part will be cut off and 
 *  "section_head" will become uppercase and underline '_' become empty
 *  space, e.g.: "SECTION HEAD".
 *  Use oyjlOption_s::value_type=oyjlOPTIONTYPE_CHOICE
 *  and place your string list into oyjlOptionChoice_s by filling it's
 *  members. Each choice is shown as own subsection.
 *  oyjlOptionChoice_s::nick is handled as a subsection headline and
 *  oyjlOptionChoice_s::name is shown as link or normal text.
 *  A "man-see_also" section oyjlOptionChoice_s::nick is scanned for
 *  MAN page cross references, e.g: "oyjl(1) oyjl(args(1)" and links are
 *  created appropriately.
 *  Translated section heads are "EXAMPLES, "SEE AS WELL", "HISTORY",
 *  "ENVIRONMENT VARIABLES", "EXIT-STATE" and "FILES".
 *
 *  @version Oyjl: 1.0.0
 *  @date    2020/04/15
 *  @since   2018/10/10 (OpenICC: 0.1.1)
 */
char *       oyjlUi_ToMan            ( oyjlUi_s          * ui,
                                       int                 flags OYJL_UNUSED )
{
  char * text = NULL, * tmp;
  const char * date = NULL,
             * desc = NULL,
             * mnft = NULL, * mnft_url = NULL,
             * copy = NULL, * lice = NULL, * lice_url = NULL,
             * bugs = NULL, * bugs_url = NULL,
             * vers = NULL;
  int i,n,ng;
  oyjlOptions_s * opts;
 
  if(!ui) return text;
  opts = ui->opts;

  n = oyjlUiHeaderSection_Count( ui->sections );
  for(i = 0; i < n; ++i)
  {
    oyjlUiHeaderSection_s * s = &ui->sections[i];
    if(strcmp(s->nick, "manufacturer") == 0) { mnft = s->name; mnft_url = s->description; }
    else if(strcmp(s->nick, "copyright") == 0) copy = s->name;
    else if(strcmp(s->nick, "license") == 0) { lice = s->name; lice_url = s->description; }
    else if(strcmp(s->nick, "url") == 0) continue;
    else if(strcmp(s->nick, "support") == 0) { bugs = s->name; bugs_url = s->description; }
    else if(strcmp(s->nick, "download") == 0) continue;
    else if(strcmp(s->nick, "sources") == 0) continue;
    else if(strcmp(s->nick, "development") == 0) continue;
    else if(strcmp(s->nick, "oyjl_module_author") == 0) continue;
    else if(strcmp(s->nick, "documentation") == 0) desc = s->description ? s->description : s->name;
    else if(strcmp(s->nick, "version") == 0) vers = s->name;
    else if(strcmp(s->nick, "date") == 0) date = s->description ? s->description : s->name;
  }

  ng = oyjlOptions_CountGroups(opts);
  if(!ng && !(flags & oyjlUI_STATE_NO_CHECKS)) return NULL;

  if((ui->app_type && ui->app_type[0]) || date || ui->nick)
  {
    int tool = ui->app_type && strcmp( ui->app_type, "tool" ) == 0;
    oyjlStringAdd( &text, malloc, free, ".TH \"%s\" %d \"%s\" \"%s\"\n", ui->nick,
                   tool?1:7, date?date:"", tool?"User Commands":"Misc" );
  }

  oyjlStringAdd( &text, malloc, free, ".SH %s\n%s%s%s%s \\- %s\n", _("NAME"), ui->nick, vers?" ":"", vers?"v":"", vers?vers:"", ui->name );

  if(ng)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n", _("SYNOPSIS") );
  for(i = 0; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    char * syn = oyjlOptions_PrintHelpSynopsis_( opts, g,
                         oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_MAN );
    if(syn && syn[0])
      oyjlStringAdd( &text, malloc, free, "%s\n%s", syn, (i < (ng-1)) ? ".br\n" : "" );
    free(syn);
  }

  if(desc)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s\n", _("DESCRIPTION"), desc );

  if(ng)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n", _("OPTIONS") );
  for(i = 0; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    int dn = 0,
        j;
    char ** d_list = oyjlStringSplit2( g->detail, "|,", 0, &dn, NULL, malloc );
    if(g->flags & OYJL_GROUP_FLAG_GENERAL_OPTS)
      oyjlStringAdd( &text, malloc, free, ".SH %s\n", _("GENERAL OPTIONS") );
    if(g->description)
      oyjlStringAdd( &text, malloc, free, ".SS\n%s\n", g->description  );
    else
    if(g->name)
      oyjlStringAdd( &text, malloc, free, ".SS\n%s\n", g->name );
    else
      oyjlStringPush( &text, "\n", malloc, free );
    if(g->mandatory && g->mandatory[0])
    {
      char * t = oyjlOptions_PrintHelpSynopsis_( opts, g, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_MAN );
      oyjlStringAdd( &text, malloc, free, "%s\n", t );
      free(t);
    }
    oyjlStringPush( &text, ".br\n", malloc, free );
    if(g->help)
    {
      oyjlStringAdd( &text, malloc, free, "%s\n.br\n.sp\n.br\n", g->help );
    }
    for(j = 0; j < dn; ++j)
    {
      const char * option = d_list[j];
      char * t;
      oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
      int mandatory_index = oyjlOption_MandatoryIndex_( o, g );
      int style = oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING | oyjlOPTIONSTYLE_MAN;
      if(mandatory_index == 0)
        style |= g->flags;

      if(!o)
      {
        fprintf(stdout, "%s %s: option not declared: %s\n", oyjlBT(0), g->name?g->name:"---", option);
        if(!getenv("OYJL_NO_EXIT")) exit(1);
      }
      switch(o->value_type)
      {
        case oyjlOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringPush( &text, t, malloc, free );
            free(t);
            if(o->name && !o->description)
              oyjlStringAdd( &text, malloc, free, "\t%s", o->name );
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s%s", o->description ? o->description:"", o->help?"\n.RS\n":"", o->help?o->help:"", o->help?"\n.RE\n":"\n.br\n" );
            while(o->values.choices.list && o->values.choices.list[n].nick && o->values.choices.list[n].nick[0] != '\000')
              ++n;
            for(l = 0; l < n; ++l)
            {
              tmp = oyjlOptionChoice_Print_( &o->values.choices.list[l], o, oyjlOPTIONSTYLE_MAN );
              oyjlStringPush( &text, tmp, malloc, free );
              free(tmp);
            }
          }
          break;
        case oyjlOPTIONTYPE_FUNCTION:
          {
            int n = 0,l;
            oyjlOptionChoice_s * list;
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringPush( &text, t, malloc, free );
            free(t);
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s%s", o->description ? o->description:"", o->help && o->help[0]?"\n.RS\n":"", o->help?o->help:"", o->help?"\n.RE\n":"\n.br\n" );
            if(o->flags & OYJL_OPTION_FLAG_EDITABLE)
              break;
            list = oyjlOption_GetChoices_(o, NULL, opts );
            if(list)
              while(list[n].nick && list[n].nick[0] != '\000')
                ++n;
            for(l = 0; l < n; ++l)
            {
              tmp = oyjlOptionChoice_Print_( &list[l], o, oyjlOPTIONSTYLE_MAN );
              oyjlStringPush( &text, tmp, malloc, free );
              free(tmp);
            }
            /* not possible, as the result of oyjlOption_GetChoices_() is cached - oyjlOptionChoice_Release( &list ); */
          }
          break;
        case oyjlOPTIONTYPE_DOUBLE:
          {
            char * desc = oyjlOption_PrintArg_Double_( o, oyjlDESCRIPTION );
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringPush( &text, t, malloc, free );
            free(t);
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s%s", desc, o->help?"\n.RS\n":"", o->help?o->help:"", o->help?"\n.RE\n":"\n.br\n" );
            free(desc);
          }
          break;
        case oyjlOPTIONTYPE_NONE:
          {
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringPush( &text, t, malloc, free );
            free(t);
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s%s", o->description ? o->description:"", o->help?"\n.RS\n":"", o->help?o->help:"", o->help?"\n.RE\n":"\n.br\n" );
          }
        break;
        case oyjlOPTIONTYPE_START: break;
        case oyjlOPTIONTYPE_END: break;
      }
    }
    oyjlStringListRelease( &d_list, dn, free );
  }

  tmp = oyjlExtraManSections_( opts, oyjlOPTIONSTYLE_MAN, NULL, NULL );
  if(tmp)
  {
    oyjlStringPush( &text, tmp, malloc, free );
    free(tmp);
  }

  if(mnft)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s %s\n", _("AUTHOR"), mnft, mnft_url?mnft_url:"" );

  if(lice || copy)
  {
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s\n", _("COPYRIGHT"), copy?copy:"" );
    if(lice)
      oyjlStringAdd( &text, malloc, free, ".br\n%s: %s %s\n", _("License"), lice?lice:"", lice_url?lice_url:"" );
  }

  if(bugs)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s %s\n", _("BUGS"), bugs, bugs_url?bugs_url:"" );
  else if(bugs_url)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s\n", _("BUGS"), bugs_url );

  return text;
}

/** @brief    Print help text to stderr
 *  @memberof oyjlOptions_s
 *
 *  @param   opts                      options to print
 *  @param   ui                        more info for e.g. from the documentation section for the description block; optional
 *  @param   verbose                   gives debug output
 *                                     - -1 : print help only for detected group
 *                                     - -2 : print only Synopsis 
 *                                     - -3 : print help only for detected group to stderr
 *  @param   out_file                  recommended output file
 *  @param   motto_format              prints a customised intoduction line
 *  @return                            help string for display on command line
 *
 *  @version Oyjl: 1.0.0
 *  @date    2022/07/02
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
char * oyjlOptions_PrintHelp         ( oyjlOptions_s     * opts,
                                       oyjlUi_s          * ui,
                                       int                 verbose,
                                       FILE             ** out_file,
                                       const char        * motto_format,
                                                           ... )
{
  int i,ng;
  char * text = NULL;
  oyjlUiHeaderSection_s * section = NULL;
  oyjlOptsPrivate_s * results = ui->opts->private_data;

  if(oyjl_help_zout == stdout || oyjl_help_zout == stderr || oyjl_help_zout == NULL)
    oyjl_help_zout = verbose >= -2 ? stdout : stderr;
  if(out_file)
    *out_file = oyjl_help_zout;

  if(verbose >= 0)
    oyjlStringPush( &text, "\n", 0,0 );
  if(verbose > 0)
  {
    for(i = 0; i < opts->argc; ++i)
      oyjlStringAdd( &text, 0,0, "\'%s\' ", oyjlTermColor( oyjlITALIC, opts->argv[i] ));
    oyjlStringAdd( &text, 0,0, "\n");
  }

  if(!((verbose == -1 || verbose == -3) && results && results->group > -1) && verbose >= 0)
  {
    if(!motto_format)
    {
      oyjlUiHeaderSection_s * version = oyjlUi_GetHeaderSection( ui,
                                                                 "version" );
      const char * prog = opts->argv[0];
      if(verbose == 0)
        prog = strrchr(prog,'/') ? strrchr(prog,'/') + 1 : prog;
      oyjlStringAdd( &text, 0,0, "%s v%s - %s", oyjlTermColor( oyjlBOLD, prog ),
                              version && version->name ? version->name : "",
                              ui->description ? ui->description : ui->name ? ui->name : "" );
      if(version && version->name && version->description && *oyjl_debug)
        oyjlStringAdd( &text, 0,0, "\n  %s", version->description );
    }
    else
    {
      char * t = NULL;
      OYJL_CREATE_VA_STRING(motto_format, t, malloc, return text)
      oyjlStringPush( &text, t, 0,0 );
      free(t);
    }
    oyjlStringPush( &text, "\n", 0,0 );
  }

  ng = oyjlOptions_CountGroups(opts);
  if(!ng) return text;

  if( ui && (section = oyjlUi_GetHeaderSection(ui, "documentation")) != NULL &&
      section->description &&
      !((verbose == -1 || verbose == -3) && results && results->group > -1) &&
      verbose >= 0
    )
    oyjlStringAdd( &text, 0,0, "\n%s:\n" OYJL_HELP_SUBSECTION "%s\n", oyjlTermColor(oyjlBOLD,_("Description")), section->description );

  if(!(verbose == -1 || verbose == -3))
  {
    oyjlStringAdd( &text, 0,0, "\n%s:\n", oyjlTermColor(oyjlBOLD,_("Synopsis")) );
    for(i = 0 ; i < ng; ++i)
    {
      oyjlOptionGroup_s * g = &opts->groups[i];
      char * t = oyjlOptions_PrintHelpSynopsis_( opts, g, oyjlOPTIONSTYLE_ONELETTER );
      oyjlStringAdd( &text, 0,0, OYJL_HELP_SUBSECTION "%s\n", t );
      free(t);
    }
  }
  if(verbose == -2)
  {
    oyjlStringPush( &text, "\n", 0,0 );
    return text;
  }

  oyjlStringAdd( &text, 0,0, "\n%s:\n", oyjlTermColor(oyjlBOLD,_("Usage"))  );
  for(i = ((verbose == -1 || verbose == -3) && results && results->group > -1) ? results->group : 0 ; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    int d = 0,
        j;
    char ** d_list = oyjlStringSplit2( g->detail, "|,", 0, &d, NULL, malloc );
    oyjlStringAdd( &text, 0,0, OYJL_HELP_SUBSECTION "%s\n", g->description?oyjlTermColor(oyjlUNDERLINE,g->description):"" );
    if(g->mandatory && g->mandatory[0])
    {
      char * t = oyjlOptions_PrintHelpSynopsis_( opts, g, oyjlOPTIONSTYLE_ONELETTER );
      oyjlStringAdd( &text, 0,0, OYJL_HELP_COMMAND "%s\n", t );
      free(t);
    }
    if(g->help)
    {
      oyjlStringAdd( &text, 0,0, OYJL_HELP_COMMAND "%s\n", g->help );
    }
    oyjlStringPush( &text, "\n", 0,0 );
    for(j = 0; j < d; ++j)
    {
      const char * option = d_list[j];
      oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
      int mandatory_index = oyjlOption_MandatoryIndex_( o, g );
      int style = oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING;
      if(mandatory_index == 0)
        style |= g->flags;
      if(!o)
      {
        oyjlStringAdd( &text, 0,0, "%s %s: option not declared: %s\n", oyjlBT(0), g->name?g->name:"---", &g->detail[j]);
        if(!getenv("OYJL_NO_EXIT")) exit(1);
      }
      switch(o->value_type)
      {
        case oyjlOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            if(o->value_name)
            {
              char * t = oyjlOption_PrintArg_(o, style);
              oyjlStringPush( &text, OYJL_HELP_OPTION, 0,0 );
              oyjlStringAdd( &text, 0,0, "%s", t );
              oyjlStringAdd( &text, 0,0, "\t%s\n", o->description ? o->description:"" );
              if(o->help)
                oyjlStringAdd( &text, 0,0,OYJL_HELP_HELP "%s\n", o->help );
              free(t);
            }
            while(o->values.choices.list && o->values.choices.list[n].nick && o->values.choices.list[n].nick[0] != '\000')
              ++n;
            for(l = 0; l < n; ++l)
            { char * t = oyjlOptionChoice_Print_( &o->values.choices.list[l], o, 0 ); oyjlStringPush( &text, t, 0,0 ); free(t); }
          }
          break;
        case oyjlOPTIONTYPE_FUNCTION:
          {
            int n = 0,l;
            oyjlOptionChoice_s * list;
            if(o->value_name)
            {
              char * t = oyjlOption_PrintArg_(o, style);
              oyjlStringPush( &text, OYJL_HELP_OPTION, 0,0 );
              oyjlStringAdd( &text, 0,0, "%s", t );
              oyjlStringAdd( &text, 0,0, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
              free(t);
            }
            if(o->flags & OYJL_OPTION_FLAG_EDITABLE)
              break;
            list = oyjlOption_GetChoices_(o, NULL, opts );
            if(list)
              while(list[n].nick && list[n].nick[0] != '\000')
                ++n;
            for(l = 0; l < n; ++l)
            { char * t = oyjlOptionChoice_Print_( &list[l], o, 0 ); oyjlStringPush( &text, t, 0,0 ); free(t); }
            /* not possible, as the result of oyjlOption_GetChoices_() is cached - oyjlOptionChoice_Release( &list ); */
          }
          break;
        case oyjlOPTIONTYPE_DOUBLE:
          {
            char * t = oyjlOption_PrintArg_(o, style);
            
            oyjlStringPush( &text, OYJL_HELP_OPTION, 0,0 );
            oyjlStringAdd( &text, 0,0, "%s", t );
            { char * desc = oyjlOption_PrintArg_Double_( o, oyjlDESCRIPTION | oyjlHELP ); oyjlStringAdd( &text, 0,0, "\t%s\n", desc ); free(desc); }
            free(t);
          }
          break;
        case oyjlOPTIONTYPE_NONE:
          {
            char * t = oyjlOption_PrintArg_(o, style);
            oyjlStringPush( &text, OYJL_HELP_OPTION, 0,0 );
            oyjlStringAdd( &text, 0,0, "%s", t?t:"" );
            oyjlStringAdd( &text, 0,0, "\t%s\n", o->description ? o->description:"" );
            if(o->help)
              oyjlStringAdd( &text, 0,0,OYJL_HELP_HELP "%s\n", o->help );
            free(t);
          }
        break;
        case oyjlOPTIONTYPE_START: break;
        case oyjlOPTIONTYPE_END: break;
      }
    }
    oyjlStringListRelease( &d_list, d, free );
    if(d) oyjlStringPush( &text, "\n", 0,0 );
    if((verbose == -1 || verbose == -3) && results && results->group > -1) break;
  }
  oyjlStringPush( &text, "\n", 0,0 );

  if(verbose > 0)
  {
    const char * mnft = NULL, * mnft_url = NULL,
             * copy = NULL, * lice = NULL, * lice_url = NULL,
             * bugs = NULL, * bugs_url = NULL;
    oyjlUiHeaderSection_s * s;
    char * tmp = oyjlExtraManSections_( opts, 0, NULL, NULL );
    if(tmp)
    {
      oyjlStringPush( &text, tmp, malloc, free );
      free(tmp);
    }

    s = oyjlUi_GetHeaderSection( ui, "manufacturer" );
    if(s) { mnft = s->name; mnft_url = s->description?s->description:""; }
    s = oyjlUi_GetHeaderSection( ui, "copyright" );
    if(s) copy = s->name;
    s = oyjlUi_GetHeaderSection( ui, "license" );
    if(s) { lice = s->name; lice_url = s->description; }
    s = oyjlUi_GetHeaderSection( ui, "support" );
    if(s) { bugs = s->name; bugs_url = s->description; }
    if(mnft)
      oyjlStringAdd( &text, malloc, free, "\n%s:\n  %s %s\n", oyjlTermColor(oyjlBOLD,_("Author")), mnft, mnft_url?mnft_url:"" );

    if(lice || copy)
    {
      oyjlStringAdd( &text, malloc, free, "\n%s:\n  %s\n", oyjlTermColor(oyjlBOLD,_("Copyright")), copy?copy:"" );
      if(lice)
        oyjlStringAdd( &text, malloc, free, "\n    %s:\n      %s %s\n", oyjlTermColor(oyjlUNDERLINE,_("License")), lice?lice:"", lice_url?lice_url:"" );
    }

    if(bugs)
      oyjlStringAdd( &text, malloc, free, "\n%s:\n  %s %s\n", oyjlTermColor(oyjlBOLD,_("Bugs")), bugs, bugs_url?bugs_url:"" );
    else if(bugs_url)
      oyjlStringAdd( &text, malloc, free, "\n%s:\n  %s\n", oyjlTermColor(oyjlBOLD,_("Bugs")), bugs_url );
  }

  if(oyjl_help_zout != stdout && oyjl_help_zout != stderr && oyjl_help_zout != NULL && !out_file)
  {
    fputs( text, oyjl_help_zout );
    free(text); text = NULL;
  }
  return text;
}

static void oyjlReplaceOutsideHTML_(const char * text OYJL_UNUSED, const char * start, const char * end, const char * search, const char ** replace, int * r_len OYJL_UNUSED, void * data)
{
  if(start < end)
  {
    const char * word = start;
    int * insideXML = (int*) data,
          inside_table = 0,
          inside_xml;

    word = start;
    while(word && (word = strstr(word+1,"<")) != NULL && word < end)
      ++insideXML[1];
    word = start;
    while(word && (word = strstr(word+1,">")) != NULL && word < end)
      --insideXML[1];
    inside_xml = insideXML[1];

    if(!inside_xml)
    {
      word = start;
      while(word && (word = strstr(word+1,"<")) != NULL && word && word[0] != '/' && word < end)
        ++insideXML[0];
      word = start;
      while(word && (word = strstr(word+1,"</")) != NULL && word < end)
        --insideXML[0];
      inside_table = insideXML[0];
    }


    if( inside_table || inside_xml )
      *replace = search;
    else
    {
      if(strcmp(search,"`") == 0)
        *replace = "\\`";
      if(strcmp(search,"-") == 0)
        *replace = "\\-";
      if(strcmp(search,"_") == 0)
        *replace = "\\_";
    }

  }
}

/** @brief    Return markdown formated text from options
 *  @memberof oyjlUi_s
 *
 *  This function supports extra sections in MAN page style.
 *  @see oyjlUi_ToMan()
 *
 *  @version Oyjl: 1.0.0
 *  @date    2019/08/02
 *  @since   2018/11/07 (OpenICC: 0.1.1)
 */
char *       oyjlUi_ToMarkdown       ( oyjlUi_s          * ui,
                                       int                 flags )
{
  char * text = NULL, * tmp, * doxy_link = NULL;
  const char * date = NULL,
             * desc = NULL,
             * mnft = NULL, * mnft_url = NULL,
             * copy = NULL, * lice = NULL, * lice_url = NULL,
             * bugs = NULL, * bugs_url = NULL,
             * vers = NULL, * t;
  char * country = NULL;
  int i,n,ng;
  oyjlOptions_s * opts;
  char ** sections_ = NULL, *** sections = &sections_;
  int sn_ = 0, * sn = &sn_;

  if( !ui ) return text;

  opts = ui->opts;
  n = oyjlUiHeaderSection_Count( ui->sections );
  for(i = 0; i < n; ++i)
  {
    oyjlUiHeaderSection_s * s = &ui->sections[i];
    if(strcmp(s->nick, "manufacturer") == 0) { mnft = s->name; mnft_url = s->description?s->description:""; }
    else if(strcmp(s->nick, "copyright") == 0) copy = s->name;
    else if(strcmp(s->nick, "license") == 0) { lice = s->name; lice_url = s->description; }
    else if(strcmp(s->nick, "url") == 0) continue;
    else if(strcmp(s->nick, "support") == 0) { bugs = s->name; bugs_url = s->description; }
    else if(strcmp(s->nick, "download") == 0) continue;
    else if(strcmp(s->nick, "sources") == 0) continue;
    else if(strcmp(s->nick, "development") == 0) continue;
    else if(strcmp(s->nick, "oyjl_module_author") == 0) continue;
    else if(strcmp(s->nick, "documentation") == 0) desc = s->description ? s->description : s->name;
    else if(strcmp(s->nick, "version") == 0) vers = s->name;
    else if(strcmp(s->nick, "date") == 0) date = s->description ? s->description : s->name;
  }

  ng = oyjlOptions_CountGroups(opts);
  if(!ng && !(flags & oyjlUI_STATE_NO_CHECKS)) return NULL;

#if defined(OYJL_HAVE_LANGINFO_H) && !defined(__ANDROID__)
  t = oyjlLang("");
  if(t && t[0])
    country = oyjlLanguage(t);
  if(!country)
  {
    t = nl_langinfo( _NL_ADDRESS_LANG_AB ); /* appears to depend on LANG environment variable */
    if(t)
      country = oyjlStringCopy(t,0);
  }
#endif
  if(flags & oyjlUI_STATE_VERBOSE || *oyjl_debug)
    fprintf(stderr, "country: \"%s\" (LANG=%s)\n", country?country:"", getenv("LANG"));

  oyjlStringAdd( &doxy_link, malloc, free, "{#%s%s}", ui->nick, country?country:"" );
  oyjlStringReplace( &doxy_link, "-", "", malloc, free );
  if(country) { free(country); country = NULL; }

  if(ui->app_type && ui->app_type[0])
  {
    int tool = strcmp( ui->app_type, "tool" ) == 0;
    oyjlStringAdd( &text, malloc, free, "<strong>\"%s\"</strong> *%d* <em>\"%s\"</em> \"%s\"\n", ui->nick,
                   tool?1:7, date?date:"", tool?"User Commands":"Misc" );
  }

  ADD_SECTION( _("NAME"), "name", "%s%s%s%s - %s\n", ui->nick, vers?" ":"", vers?"v":"", vers?vers:"", ui->name )

  if(ng)
  ADD_SECTION( _("SYNOPSIS"), "synopsis", "", "" )
  for(i = 0; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    char * syn = oyjlOptions_PrintHelpSynopsis_( opts, g,
                         oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_MARKDOWN | oyjlOPTIONSTYLE_LINK_GROUP );
    if(syn[0])
      oyjlStringAdd( &text, malloc, free, "%s\n%s", syn, (i < (ng-1)) ? "<br />\n" : "" );
    free(syn);
  }

  if(desc)
    ADD_SECTION( _("DESCRIPTION"), "description", "%s\n", desc )

  if(ng)
  ADD_SECTION( _("OPTIONS"), "options", "", "" )
  for(i = 0; i < ng; ++i)
  {
    oyjlOptionGroup_s * g = &opts->groups[i];
    int d = 0,
        j;
    char ** d_list = oyjlStringSplit2( g->detail, "|,", 0, &d, NULL, malloc ),
         * t;
    if(g->flags & OYJL_GROUP_FLAG_GENERAL_OPTS)
      ADD_SECTION( _("GENERAL OPTIONS"), "general_options", "", "" )
    if(g->description)
    {
      const char * group_id = oyjlOptions_GetGroupId_( opts, g );
      if(group_id)
        oyjlStringAdd( &text, malloc, free, "<h3 id=\"%s\">%s</h3>\n", group_id, g->description );
      else
        oyjlStringAdd( &text, malloc, free, "<h3>%s</h3>\n", g->description );
      oyjlStringPush( &text, "\n", malloc, free  );
    }
    if(g->mandatory && g->mandatory[0])
    {
      char * t = oyjlOptions_PrintHelpSynopsis_( opts, g, oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_MARKDOWN | oyjlOPTIONSTYLE_LINK_SYNOPSIS );
      oyjlStringAdd( &text, malloc, free, "&nbsp;&nbsp;%s\n", t );
      free(t);
    }
    oyjlStringPush( &text, "\n", malloc, free );
    if(g->help)
    {
      oyjlStringAdd( &text, malloc, free, "&nbsp;&nbsp;%s\n\n", g->help );
    }
    if(d)
      oyjlStringPush( &text, "<table style='width:100%'>\n", malloc, free );
    for(j = 0; j < d; ++j)
    {
      const char * option = d_list[j];
      oyjlOption_s * o = oyjlOptions_GetOptionL( opts, option, 0 );
      int mandatory_index = oyjlOption_MandatoryIndex_( o, g );
      int style = oyjlOPTIONSTYLE_ONELETTER | oyjlOPTIONSTYLE_STRING | oyjlOPTIONSTYLE_MARKDOWN;
      if(mandatory_index == 0)
        style |= g->flags;
      if(!o)
      {
        fprintf(stdout, "%s %s: option not declared: %s\n", oyjlBT(0), g->name?g->name:"---", option );
        if(!getenv("OYJL_NO_EXIT")) exit(1);
      }
#define OYJL_LEFT_TD_STYLE " style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%%'"
      switch(o->value_type)
      {
        case oyjlOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringAdd( &text, malloc, free, " <tr><td" OYJL_LEFT_TD_STYLE ">%s</td>", t );
            free(t);
            oyjlStringAdd( &text, malloc, free, " <td>%s%s%s", o->description ? o->description:"", o->help?"<br />":"", o->help?o->help :"" );
            while(o->values.choices.list && o->values.choices.list[n].nick && o->values.choices.list[n].nick[0] != '\000')
              ++n;
            if(n) oyjlStringPush( &text, "\n  <table>\n", malloc, free );
            for(l = 0; l < n; ++l)
            {
              tmp = oyjlOptionChoice_Print_( &o->values.choices.list[l], o, oyjlOPTIONSTYLE_MARKDOWN );
              oyjlStringPush( &text, tmp, malloc, free );
              free(tmp);
            }
            if(n) oyjlStringPush( &text, "  </table>\n", malloc, free );
            oyjlStringPush( &text, "  </td>\n", malloc, free );
          }
          break;
        case oyjlOPTIONTYPE_FUNCTION:
          {
            int n = 0,l;
            oyjlOptionChoice_s * list;
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringAdd( &text, malloc, free, " <tr><td" OYJL_LEFT_TD_STYLE ">%s</td>", t );
            free(t);
            oyjlStringAdd( &text, malloc, free, " <td>%s%s%s", o->description ? o->description:"", o->help?"<br />":"", o->help?o->help :"" );
            if(o->flags & OYJL_OPTION_FLAG_EDITABLE)
              break;
            list = oyjlOption_GetChoices_(o, NULL, opts );
            if(list)
              while(list[n].nick && list[n].nick[0] != '\000')
                ++n;
            if(n) oyjlStringPush( &text, "\n  <table>\n", malloc, free );
            for(l = 0; l < n; ++l)
            {
              tmp = oyjlOptionChoice_Print_( &list[l], o, oyjlOPTIONSTYLE_MARKDOWN );
              oyjlStringPush( &text, tmp, malloc, free );
              free(tmp);
            }
            if(n) oyjlStringPush( &text, "  </table>\n", malloc, free );
            oyjlStringPush( &text, "  </td>\n", malloc, free );
            /* not possible, as the result of oyjlOption_GetChoices_() is cached - oyjlOptionChoice_Release( &list ); */
          }
          break;
        case oyjlOPTIONTYPE_DOUBLE:
          {
            char * desc = oyjlOption_PrintArg_Double_( o, oyjlDESCRIPTION | oyjlHELP );
            t = oyjlOption_PrintArg_(o, style);
            oyjlStringAdd( &text, malloc, free, " <tr><td" OYJL_LEFT_TD_STYLE ">%s</td>", t );
            free(t);
            oyjlStringAdd( &text, malloc, free, " <td>%s</td>", desc );
            free( desc );
          }
          break;
        case oyjlOPTIONTYPE_NONE:
          t = oyjlOption_PrintArg_(o, style);
          oyjlStringAdd( &text, malloc, free, " <tr><td" OYJL_LEFT_TD_STYLE ">%s</td>", t );
          free(t);
          oyjlStringAdd( &text, malloc, free, " <td>%s%s%s</td>", o->description ? o->description:"", o->help?"<br />":"", o->help?o->help :"" );
        break;
        case oyjlOPTIONTYPE_START: break;
        case oyjlOPTIONTYPE_END: break;
      }
      oyjlStringPush( &text, " </tr>\n", malloc, free );
    }
    oyjlStringListRelease( &d_list, d, free );
    if(d)
      oyjlStringPush( &text, "</table>\n", malloc, free );
    oyjlStringPush( &text, "\n", malloc, free );
  }

  tmp = oyjlExtraManSections_( opts, oyjlOPTIONSTYLE_MARKDOWN, sections, sn );
  if(tmp)
  {
    oyjlStringPush( &text, tmp, malloc, free );
    free(tmp);
  }

  if(mnft)
    ADD_SECTION( _("AUTHOR"), "author", "%s %s\n", mnft, mnft_url )

  if(lice || copy)
  {
    ADD_SECTION( _("COPYRIGHT"), "copyright", "*%s*\n", copy?copy:"" )
    if(lice)
      oyjlStringAdd( &text, malloc, free, "\n\n<a name=\"license\"></a>\n### %s\n%s", _("License"), lice?lice:"" );
    if(lice_url)
      oyjlStringAdd( &text, malloc, free, " <a href=\"%s\">%s</a>", lice_url, lice_url );
    if(lice || lice_url)
      oyjlStringPush( &text, "\n", malloc, free );
  }

  if(bugs && bugs_url)
    ADD_SECTION( _("BUGS"), "bugs", "%s <a href=\"%s\">%s</a>\n", bugs, bugs_url, bugs_url )
  else if(bugs)
    ADD_SECTION( _("BUGS"), "bugs", "<a href=\"%s\">%s</a>\n", bugs, bugs )

  {
    char * txt = NULL;
    int i;

    oyjlStringAdd( &txt, malloc, free, "# %s%s%s%s %s\n<a name=\"toc\"></a>\n", ui->nick, vers?" ":"", vers?"v":"", vers?vers:"", doxy_link );
    for(i = 0; i < sn_/2; ++i)
      oyjlStringAdd( &txt, malloc, free, "[%s](#%s) ", sections_[2*i+0], sections_[2*i+1] );
    oyjlStringAdd( &txt, malloc, free, "\n\n%s", text );
    free(text);
    text = txt;
  }

  {
    const char * t;
    int insideXML[3] = {0,0,0};
    oyjl_str tmp = oyjlStr_New(10,0,0);
    oyjlStr_Push( tmp, text );
    oyjlStr_Replace( tmp, "`", "\\`", oyjlReplaceOutsideHTML_, insideXML );
    oyjlStr_Replace( tmp, "-", "\\-", oyjlReplaceOutsideHTML_, insideXML );
    oyjlStr_Replace( tmp, "_", "\\_", oyjlReplaceOutsideHTML_, insideXML );
    t = oyjlStr(tmp);
    text[0] = 0;
    oyjlStringPush( &text, t, malloc,free );
    oyjlStr_Release( &tmp );
  }

  free(doxy_link);
  oyjlStringListRelease( sections, sn_, free );

  return text;
}
#endif /* OYJL_ARGS_BASE */
#if defined (OYJL_ARGS_BASE) ||  !defined(OYJL_INTERNAL)
static const char * oyjlLibNameCreate_()
{
  static char * fn = NULL;
  if(fn) return fn;

#ifdef __APPLE__
    oyjlStringPush( &fn, "libOyjlCore.dyld", 0,0 );
#elif defined(_WIN32)
    oyjlStringPush( &fn, "OyjlCore.lib", 0,0 );
#else
    oyjlStringPush( &fn, "libOyjlCore.so.X", 0,0 );
    fn[strlen(fn)-1] = '\000';
    sprintf( &fn[strlen(fn)], "%d", OYJL_VERSION_A );
#endif
  return fn;
}
void * oyjl_core_lib_ArgsBase = NULL;
#ifdef HAVE_DL
# include <dlfcn.h>   /* dlopen() dlclose() */
#endif
#undef oyjlUi_ToText
//#undef oyjlTermColor
#endif /* OYJL_ARGS_BASE */

#if !defined (OYJL_ARGS_BASE) && defined (OYJL_INTERNAL)
char *             oyjlUi_ToText
#else /* OYJL_ARGS_BASE */
char *           (*oyjlUi_ToTextArgsBase_)
                                     ( oyjlUi_s          * ui,
                                       oyjlARGS_EXPORT_e   type,
                                       int                 flags ) = NULL;
char *             oyjlUi_ToTextArgsBase
#endif /* OYJL_ARGS_BASE */
                                     ( oyjlUi_s          * ui,
                                       oyjlARGS_EXPORT_e   type,
                                       int                 flags )
{
  char * text = NULL;
#if !defined(OYJL_ARGS_BASE)
  switch(type)
  {
    case oyjlARGS_EXPORT_HELP: text = oyjlOptions_PrintHelp( ui->opts, ui, flags, NULL, NULL ); break;
    case oyjlARGS_EXPORT_MAN: text = oyjlUi_ToMan( ui, flags ); break;
    case oyjlARGS_EXPORT_MARKDOWN: text = oyjlUi_ToMarkdown( ui, flags ); break;
#if defined(OYJL_INTERNAL)
    case oyjlARGS_EXPORT_JSON: text = oyjlUi_ToJson( ui, flags ); break;
    case oyjlARGS_EXPORT_EXPORT: text = oyjlUi_ExportToJson( ui, flags ); break;
#else /* OYJL_INTERNAL */
    case oyjlARGS_EXPORT_JSON:
    case oyjlARGS_EXPORT_EXPORT:
          oyjlArgsBaseLoadCore();
          if(oyjlUi_ToTextArgsBase_)
            text = oyjlUi_ToTextArgsBase_( ui, type, flags );
          else
            fprintf( stderr, OYJL_DBG_FORMAT "option -h and -X need load of %s but failed\n", OYJL_DBG_ARGS, oyjlLibNameCreate_() );
          if(!text)
            text = oyjlStringCopy( "{ \"error\": \"export JSON not supported\" }", 0);
          break;
#endif /* OYJL_INTERNAL */
  }
#else /* OYJL_ARGS_BASE */
  oyjlArgsBaseLoadCore();
  if(oyjlUi_ToTextArgsBase_)
    text = oyjlUi_ToTextArgsBase_( ui, type, flags );
  else
    fprintf( stderr, OYJL_DBG_FORMAT "option -h and -X need load of %s but failed\n", OYJL_DBG_ARGS, oyjlLibNameCreate_() );
#endif /* OYJL_ARGS_BASE */
  return text;
}

#if defined (OYJL_ARGS_BASE) || !defined (OYJL_INTERNAL)
int oyjlArgsBaseLoadCore_once_ = 0;
#if !defined (OYJL_INTERNAL) || defined (OYJL_ARGS_BASE)
# define oyjlTermColorArgsBase oyjlTermColor
# define oyjlBTArgsBase oyjlBT
#endif
void       oyjlArgsBaseLoadCore      ( )
{
  int verbose = 0;
  const char * libname;
  if(oyjlArgsBaseLoadCore_once_)
    return;
  ++oyjlArgsBaseLoadCore_once_;
  if(getenv("OYJL_DEBUG"))
    ++verbose;
#ifdef HAVE_DL
  libname = oyjlLibNameCreate_();
  if(!oyjl_core_lib_ArgsBase)
    oyjl_core_lib_ArgsBase = dlopen( libname, RTLD_LAZY );
  if(!oyjl_core_lib_ArgsBase)
  {
    if(verbose)
      fprintf( stderr, OYJL_DBG_FORMAT "load of %s failed\n", OYJL_DBG_ARGS, libname );
    return;
  }
  if(verbose)
    fprintf( stderr, OYJL_DBG_FORMAT "loaded %s\n", OYJL_DBG_ARGS, libname );
  if(!oyjlUi_ToTextArgsBase_)
    oyjlUi_ToTextArgsBase_ = (void*)dlsym( oyjl_core_lib_ArgsBase, "oyjlUi_ToText" );
#if defined (OYJL_INTERNAL) || defined (OYJL_ARGS_BASE)
  if(oyjlMessage_p == oyjlMessageFunc)
    oyjlMessage_p = (void*)dlsym( oyjl_core_lib_ArgsBase, "oyjlMessageFunc" );
  oyjlTermColorArgsBase = (void*)dlsym( oyjl_core_lib_ArgsBase, "oyjlTermColor" );
  oyjlBTArgsBase = (void*)dlsym( oyjl_core_lib_ArgsBase, "oyjlBT" );
#endif
  oyjlArgsRenderBase = (void*)dlsym( oyjl_core_lib_ArgsBase, "oyjlArgsRender" );
  if(verbose)
    fprintf( stderr, OYJL_DBG_FORMAT "functions %s\n", OYJL_DBG_ARGS, oyjlTermColorArgsBase(oyjlGREEN, "loaded") );
#else /* HAVE_DL */
#warning "HAVE_DL not defined (possibly dlfcn.h not found?): dynamic loading of libOyjlCore will not be possible"
#endif /* HAVE_DL */
}

const char *   oyjlSetLocale         ( int                 category OYJL_UNUSED,
                                       const char        * loc )
{
  const char * lang = getenv("LANG"),
             * language = getenv("LANGUAGE"),
             * dbg = getenv("OYJL_DEBUG"),
             * setloc = NULL;
  int debug = dbg?atoi(dbg):0;
  if((lang && lang[0] && language && language[0] && strcmp(lang,language) != 0 && !oyjlStringStartsWith(lang,language) && !oyjlStringStartsWith(lang,"C")) ||
     (!(lang && lang[0]) && language && language[0]))
  {
    setenv("LANG", language, 1);
    if(debug) fprintf(stderr, OYJL_DBG_FORMAT "LANG=%s (LANGUAGE=%s) ", OYJL_DBG_ARGS, getenv("LANG"), getenv("LANGUAGE") );
  } else {
    if(!(language && language[0]) && lang && lang[0])
    {
      setenv("LANGUAGE", lang, 1);
      if(debug) fprintf(stderr, OYJL_DBG_FORMAT "LANGUAGE=%s (LANG=%s) ", OYJL_DBG_ARGS, getenv("LANGUAGE"), getenv("LANG") );
    }
  }
#ifdef OYJL_HAVE_LOCALE_H
  setloc = setlocale( category, loc );
#else
  setloc = loc;
#endif
  if(debug) fprintf(stderr, OYJL_DBG_FORMAT "setlocale(loc: %s) = %s\n", OYJL_DBG_ARGS, loc, setloc );
  return setloc;
}
#endif /* OYJL_ARGS_BASE */


/** 
 *  @} *//* oyjl_args
 */

#endif /* OYJL_ARGS_C */
