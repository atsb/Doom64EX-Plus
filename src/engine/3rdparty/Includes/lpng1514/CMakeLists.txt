cmake_minimum_required(VERSION 3.6)

project(libpng
  VERSION 1.5.14
  LANGUAGES C
)

include(GNUInstallDirs)
enable_testing()

set(PNGLIB_MAJOR 1)
set(PNGLIB_MINOR 5)
set(PNGLIB_RELEASE 14)
set(PNGLIB_NAME libpng${PNGLIB_MAJOR}${PNGLIB_MINOR})
set(PNGLIB_VERSION ${PNGLIB_MAJOR}.${PNGLIB_MINOR}.${PNGLIB_RELEASE})
set(PNG_SOVERSION 15)

option(PNG_SHARED "Build shared library" ON)
option(PNG_STATIC "Build static library" ON)
option(PNG_TESTS  "Build libpng tests"   ON)
option(PNG_DEBUG  "Build with debug output" OFF)
option(PNGARG     "Disable ANSI-C prototypes" OFF)

set(CMAKE_DEBUG_POSTFIX d)

find_package(ZLIB REQUIRED)

if(NOT WIN32)
  find_library(M_LIBRARY m)
  if(NOT M_LIBRARY)
    message(STATUS "math library 'libm' not found - floating point support disabled")
  endif()
else()
  set(M_LIBRARY "")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt
  ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h
  COPYONLY
)

set(libpng_public_hdrs
  ${CMAKE_CURRENT_SOURCE_DIR}/png.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pngconf.h
  ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h
)

set(libpng_private_hdrs
  ${CMAKE_CURRENT_SOURCE_DIR}/pngdebug.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pnginfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pngpriv.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pngstruct.h
)

set(libpng_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/png.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngerror.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngget.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngpread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngrio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngrtran.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngrutil.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngset.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngtrans.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngwio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngwrite.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngwtran.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pngwutil.c
)

set(pngtest_sources ${CMAKE_CURRENT_SOURCE_DIR}/pngtest.c)
set(pngvalid_sources ${CMAKE_CURRENT_SOURCE_DIR}/contrib/libtests/pngvalid.c)

set(_libpng_common_defs PNG_CONFIGURE_LIBPNG)
if(PNG_DEBUG)
  list(APPEND _libpng_common_defs PNG_DEBUG)
endif()

set(_libpng_public_includes
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>
)

add_library(png_objects OBJECT
  ${libpng_public_hdrs}
  ${libpng_private_hdrs}
  ${libpng_sources}
)
target_compile_definitions(png_objects PRIVATE ${_libpng_common_defs})
target_include_directories(png_objects
  PUBLIC  ${_libpng_public_includes}
  PRIVATE ${ZLIB_INCLUDE_DIRS}
)
set_property(TARGET png_objects PROPERTY POSITION_INDEPENDENT_CODE ON)

if(PNG_SHARED)
  add_library(png_shared SHARED $<TARGET_OBJECTS:png_objects>)
  set_target_properties(png_shared PROPERTIES
    OUTPUT_NAME ${PNGLIB_NAME}
    VERSION     ${PNG_SOVERSION}.${PNGLIB_RELEASE}.0
    SOVERSION   ${PNG_SOVERSION}
  )
  if(MSVC)
    set_target_properties(png_shared PROPERTIES PREFIX "lib" IMPORT_PREFIX "lib")
  endif()
  if(WIN32)
    target_compile_definitions(png_shared PRIVATE PNG_BUILD_DLL)
  endif()
  target_link_libraries(png_shared
    PUBLIC  ${ZLIB_LIBRARIES}
    PUBLIC ${M_LIBRARY}
  )
  target_include_directories(png_shared
    PUBLIC ${_libpng_public_includes}
  )
endif()

if(PNG_STATIC)
  add_library(png_static STATIC $<TARGET_OBJECTS:png_objects>)
  set_target_properties(png_static PROPERTIES
    OUTPUT_NAME ${PNGLIB_NAME}
  )
  if(MSVC)
    set_target_properties(png_static PROPERTIES PREFIX "lib")
  endif()
  target_link_libraries(png_static
    PUBLIC  ${ZLIB_LIBRARIES}
    PUBLIC ${M_LIBRARY}
  )
  target_include_directories(png_static
    PUBLIC ${_libpng_public_includes}
  )
endif()

if(PNG_TESTS)
  set(_png_link_target "")
  if(TARGET png_shared)
    set(_png_link_target png_shared)
  elseif(TARGET png_static)
    set(_png_link_target png_static)
  endif()

  if(_png_link_target)
    add_executable(pngtest ${pngtest_sources})
    target_link_libraries(pngtest PRIVATE ${_png_link_target})
    add_test(NAME pngtest
             COMMAND pngtest ${CMAKE_CURRENT_SOURCE_DIR}/pngtest.png)

    add_executable(pngvalid ${pngvalid_sources})
    target_link_libraries(pngvalid PRIVATE ${_png_link_target})
    add_test(NAME pngvalid COMMAND pngvalid)
  else()
    message(STATUS "PNG_TESTS enabled, but no library target is available to link against.")
  endif()
endif()

install(FILES ${libpng_public_hdrs} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${libpng_public_hdrs} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PNGLIB_NAME})

set(_install_tgts)
if(TARGET png_shared)
  list(APPEND _install_tgts png_shared)
endif()
if(TARGET png_static)
  list(APPEND _install_tgts png_static)
endif()

if(_install_tgts)
  install(TARGETS ${_install_tgts}
    EXPORT libpngTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

set(PNG_MAN_DIR "share/man" CACHE STRING "Base manpage dir (will install to man3/man5 under this)")
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/libpng.3
  ${CMAKE_CURRENT_SOURCE_DIR}/libpngpf.3
  DESTINATION ${PNG_MAN_DIR}/man3
)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/png.5
  DESTINATION ${PNG_MAN_DIR}/man5
)

set(prefix       ${CMAKE_INSTALL_PREFIX})
set(exec_prefix  ${CMAKE_INSTALL_PREFIX})
set(libdir       ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir   ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(LIBS         "-lz")
if(M_LIBRARY)
  set(LIBS "${LIBS} -lm")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
  ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(EXPORT libpngTargets
  NAMESPACE libpng::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpng
  FILE lib${PNGLIB_NAME}.cmake
)

message(STATUS "libpng version       : ${PNGLIB_VERSION} (SOVERSION ${PNG_SOVERSION})")
message(STATUS "Build shared library : ${PNG_SHARED}")
message(STATUS "Build static library : ${PNG_STATIC}")
message(STATUS "Build tests          : ${PNG_TESTS}")
message(STATUS "Zlib includes        : ${ZLIB_INCLUDE_DIRS}")
message(STATUS "Zlib libs            : ${ZLIB_LIBRARIES}")
if(M_LIBRARY)
  message(STATUS "libm                 : ${M_LIBRARY}")
endif()
