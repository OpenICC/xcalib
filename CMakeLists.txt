CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12)

PROJECT (xcalib C)

SET (XCALIB_VERSION_MAJOR 0)
SET (XCALIB_VERSION_MINOR 11)
SET (XCALIB_VERSION_MICRO 0)
SET (XCALIB_VERSION "${XCALIB_VERSION_MAJOR}.${XCALIB_VERSION_MINOR}.${XCALIB_VERSION_MICRO}")
SET (XCALIB_VERSION_MAJORMINOR "${XCALIB_VERSION_MAJOR}.${XCALIB_VERSION_MINOR}")

SET (PACKAGE_NAME "xcalib")
set (PACKAGE_DESCRIPTION "load 'vcgt'-tag of ICC profiles to X-server like MS-Windows or MacOS do it to calibrate your display. Versions 0.5 and higher are also usable with Microsoft Windows. They can be used as a free alternative to other calibration loaders.")
INCLUDE(GNUInstallDirs)

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

FIND_PACKAGE(Oyjl)
IF(HAVE_OYJL)
  INCLUDE_DIRECTORIES( ${OYJL_INCLUDE_DIR} )
  LINK_DIRECTORIES( ${OYJL_LIBRARY_DIRS} )
  SET( EXTRA_LIBS ${EXTRA_LIBS} ${OYJL_LIBRARIES} )
ELSE()
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DINCLUDE_OYJL_C=1 -DHAVE_DL" )
ENDIF()

INCLUDE(CheckLibraryExists)
INCLUDE(CheckIncludeFile)
CHECK_LIBRARY_EXISTS(m pow "math.h" HAVE_M)
IF(HAVE_M)
  SET( EXTRA_LIBS ${EXTRA_LIBS} m )
ENDIF(HAVE_M)

CHECK_INCLUDE_FILE(locale.h     HAVE_LOCALE_H)
CHECK_INCLUDE_FILE(libintl.h    HAVE_LIBINTL_H)

# define source file(s)
SET( ${PROJECT_NAME}_HEADERS
                ${PROJECT_NAME}.i18n.h )
              SET( ${PROJECT_NAME}_SRC
                oyjl_args.c )
SET( COMMON_CPPFILES
	xcalib.c
  ${${PROJECT_NAME}_HEADERS}
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


IF( NOT DOC_PATH )
  SET( DOC_PATH "${CMAKE_SOURCE_DIR}/docs" )
ENDIF( NOT DOC_PATH )

SET( USE_I18N TRUE )
IF(HAVE_OYJL)
  OYJL_DOCUMENT_TOOL( ${PROJECT_NAME} ${PROJECT_NAME}.c "de_DE.UTF8" ${DOC_PATH} )
  ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.i18n.h
      COMMAND oyjl json -w oiJS -W ${PROJECT_NAME}_i18n -i ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.i18n.json > ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.i18n.h
      DEPENDS ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.i18n.json
      COMMENT "generate ${PROJECT_NAME}.i18n.h" )
  ADD_CUSTOM_TARGET( ${PROJECT_NAME}_i18n ALL
      DEPENDS ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.i18n.json )
  OYJL_COMPLETION_TOOL( ${PROJECT_NAME} ${PROJECT_NAME}.c )
ELSE()
  IF(HAVE_LOCALE_H)
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOYJL_HAVE_LOCALE_H" )
  ENDIF(HAVE_LOCALE_H)
  IF(HAVE_LIBINTL_H)
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOYJL_HAVE_LIBINTL_H" )
  ENDIF(HAVE_LIBINTL_H)
ENDIF()

FILE( GLOB TEST_PROFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      *.icc
      *.icm
    )
FILE( GLOB MAN1_PAGES_${PROJECT_UP_NAME} ${DOC_PATH}/man/*.1 )
FILE( GLOB MAN1DE_PAGES_${PROJECT_UP_NAME} ${DOC_PATH}/man/de/*.1 )

INSTALL( TARGETS xcalib DESTINATION bin )
INSTALL( FILES ${TEST_PROFILES}
         DESTINATION share/color/icc/xcalib/test )
INSTALL( FILES ${MAN1_PAGES_${PROJECT_UP_NAME}} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )
INSTALL( FILES ${MAN1DE_PAGES_${PROJECT_UP_NAME}} DESTINATION ${CMAKE_INSTALL_MANDIR}/de/man1 )
INSTALL (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL( FILES extras/${PROJECT_NAME}.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
INSTALL( FILES extras/images/logo.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/ RENAME ${PROJECT_NAME}.png )
INSTALL( FILES extras/images/logo.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/share/pixmaps/hicolor/scalable/ RENAME ${PROJECT_NAME}.svg )

ADD_CUSTOM_TARGET( dist
                COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/ && git archive --prefix=${PACKAGE_NAME}-${XCALIB_VERSION}/ --format=tar HEAD > ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${XCALIB_VERSION}.tar 
                COMMAND echo "git:       git clone git://www.gitorious.org/xcalib/xcalib" > ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND echo "git sha1:  " | tr -d "\\n" >> ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/ && git rev-parse HEAD >> ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum 
                COMMAND rm -rf ${PACKAGE_NAME}-${XCALIB_VERSION} && tar xf ${PACKAGE_NAME}-${XCALIB_VERSION}.tar 
                COMMAND tar cf - ${PACKAGE_NAME}-${XCALIB_VERSION} | bzip2 > ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2
                COMMAND echo  "package:   http://sourceforge.net/projects/openicc/files/${PACKAGE_NAME}/${PACKAGE_NAME}%20${XCALIB_VERSION_MAJORMINOR}/${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2" >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND echo  "size:      " | tr -d "\\n" >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND du -b ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2 | sed s/[[:space:]][[:print:]]*// >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum 
                COMMAND echo  "md5sum:    " | tr -d "\\n" >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/ && md5sum ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2 >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND echo  "sha1sum:   " | tr -d "\\n" >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND sha1sum ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2 >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND echo  "sha256sum: " | tr -d "\\n" >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMAND sha256sum ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2 >> ${PACKAGE_NAME}-${XCALIB_VERSION}.tar.bz2.hashsum
                COMMENT Make a ${PACKAGE_NAME} source package by using git
                VERBATIM
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                )

INCLUDE(FeatureSummary)
FEATURE_SUMMARY(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
