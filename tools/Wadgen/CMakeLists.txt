##-----------------------------------------------------------------------------
##
## Root CMake file.
##
##-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.6)

project(WadGen)
set(VERSION_MAJOR "2")
set(VERSION_MINOR "4")
set(VERSION_PATCH "0")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
IF(VERSION_SUFFIX)
    SET(VERSION "${VERSION}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)

include(CheckCCompilerFlag)

##----------------------------------------------------------------------------|
## Functions                                                                  |
##

##
## TRY_C_COMPILER_FLAG
##
function(TRY_C_COMPILER_FLAG flag name)
   CHECK_C_COMPILER_FLAG(${flag} ${name})

   if(${name})
	   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
   endif()
endfunction()


##----------------------------------------------------------------------------|
## Environment Detection                                                      |
##

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
   TRY_C_COMPILER_FLAG(-Wall    FLAG_C_Wall   )
   TRY_C_COMPILER_FLAG(-Wextra  FLAG_C_Wextra )
   TRY_C_COMPILER_FLAG(-Wshadow FLAG_C_Wshadow)
   TRY_C_COMPILER_FLAG(-std=c11 FLAG_C_C11)
   if(WIN32) # MinGW should build WadGen as a GUI program
       TRY_C_COMPILER_FLAG(-mwindows FLAG_GUI_WINDOWS)
   endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
   TRY_C_COMPILER_FLAG(/Wall    FLAG_C_Wall   )
endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

add_subdirectory(src)

#eof
