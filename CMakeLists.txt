CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (xcalib)

SET (XCALIB_VERSION_MAJOR 0)
SET (XCALIB_VERSION_MINOR 9)
SET (XCALIB_VERSION_MICRO 0)
SET (XCALIB_VERSION "${XCALIB_VERSION_MAJOR}.${XCALIB_VERSION_MINOR}.${XCALIB_VERSION_MICRO}")

SET (PACKAGE_NAME "xcalib")
set (PACKAGE_DESCRIPTION "load 'vcgt'-tag of ICC profiles to X-server like MS-Windows or MacOS do it to calibrate your display. Versions 0.5 and higher are also usable with Microsoft Windows. They can be used as a free alternative to other calibration loaders.")

SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXCALIB_VERSION=\\\"${XCALIB_VERSION}\\\"" )

# additional targets
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET( uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Uninstall.cmake")


IF (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF (NOT CMAKE_BUILD_TYPE)

# detect dependency packages
FIND_PACKAGE( X11 )
IF(X11_FOUND)
  INCLUDE_DIRECTORIES( ${X11_INCLUDE_DIRS} )
  LINK_DIRECTORIES( ${X11_LIBRARY_DIRS} )
  SET( HAVE_X11 TRUE )
  IF(X11_Xrandr_FOUND)
    SET( HAVE_XRANDR TRUE )
  ENDIF()
  IF(X11_xf86vmode_FOUND)
    SET( HAVE_XF86VMODE TRUE )
  ENDIF()
ENDIF()

INCLUDE(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m pow "math.h" HAVE_M)
IF(HAVE_M)
  SET( EXTRA_LIBS ${EXTRA_LIBS} m )
ENDIF(HAVE_M)

# define source file(s)
SET( COMMON_CPPFILES
	xcalib.c
   )
SET( xcalib_SRCS
     ${COMMON_CPPFILES}
   )

ADD_EXECUTABLE( xcalib ${xcalib_SRCS} )
TARGET_LINK_LIBRARIES ( xcalib
                 ${EXTRA_LIBS}
                 ${X11_X11_LIB}
                 ${X11_Xrandr_LIB}
                 ${X11_Xxf86vm_LIB} )

FILE( GLOB TEST_PROFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      *.icc
      *.icm
    )

INSTALL( TARGETS xcalib DESTINATION bin )
INSTALL( FILES xcalib.1 DESTINATION share/man/man1 )
INSTALL( FILES ${TEST_PROFILES}
         DESTINATION share/color/icc/xcalib/test )
