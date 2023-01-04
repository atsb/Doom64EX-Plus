##-----------------------------------------------------------------------------
##
## Source Level CMake file.
##
##-----------------------------------------------------------------------------

include_directories(${PNG_INCLUDE_DIR})

add_executable(wadgen
    WIN32
    MACOSX_BUNDLE
    deflate-N64.c
    files.c
    gfx.c
    level.c
    mem.c
    png.c
    MD5.c
    rom.c
    sndfont.c
    sound.c
    sprite.c
    texture.c
    wad.c
    wadgen.c
)

if(WIN32)
    target_link_libraries(wadgen ${PNG_LIBRARY} ${ZLIB_LIBRARY} comctl32)
else()
    target_link_libraries(wadgen ${PNG_LIBRARY} ${ZLIB_LIBRARY} m)
endif()

##----------------------------------------------------------------------------|
## CPack Info                                                                 |
##

INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Wad Generator for Doom64 EX")
SET(CPACK_PACKAGE_VENDOR "Team Kex")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
IF(VERSION_SUFFIX)
    SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}-${VERSION_SUFFIX}")
ENDIF(VERSION_SUFFIX)

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "WadGen ${VERSION}")

##----------------------------------------------------------------------------|
## System-specific CPack Variables                                            |
##

IF(WIN32)
    # TODO
    #SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/data\\\\nsis_branding.bmp")
    #SET(CPACK_NSIS_MUI_ICON
    #    "${CMAKE_SOURCE_DIR}/src/win32/resources/\\\\doom64.ico")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "wadgen.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "WadGen for Doom64")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.doom64ex.wordpress.com/")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.doom64ex.wordpress.com/")
    SET(CPACK_NSIS_CONTACT "MP2E@archlinux.us")
    SET(CPACK_PACKAGE_EXECUTABLES "wadgen.exe;Tool for Doom64EX")
    SET(CPACK_NSIS_MENU_LINKS "wadgen.exe;Tool for Doom64EX")
    SET(CPACK_SOURCE_GENERATOR "ZIP")
ELSEIF(APPLE)
	# set how it shows up in the Info.plist file
    SET(MACOSX_BUNDLE_INFO_STRING "Doom64 Wad Generator for Mac")
	SET(MACOSX_BUNDLE_ICON_FILE "res/apple/icon.icns")
    SET(MACOSX_BUNDLE_GUI_IDENTIFIER "WadGen")
	SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}")
    SET(MACOSX_BUNDLE_BUNDLE_NAME "WadGen")
	SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
	SET(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
	SET(MACOSX_BUNDLE_COPYRIGHT "Team Kex, 2014")

    # set where in the bundle to put the icns file
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/res/apple/icon.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	# include the icns file in the target
    LIST(APPEND EXTRA_SOURCES ${CMAKE_SOURCE_DIR}/res/apple/icon.icns)
ENDIF()

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "wadgen-${VERSION}")
SET(CPACK_SOURCE_IGNORE_FILES ".*~;\\\\.git;\\\\.svn;${CMAKE_BINARY_DIR}")

# Installation configuration
SET(plugin_dest_dir bin)
SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/WadGen")
IF(APPLE)
    SET(plugin_dest_dir WadGen.app/Contents/MacOS)
    SET(APPS "\${CMAKE_INSTALL_PREFIX}/WadGen.app")
ENDIF()
IF(WIN32)
    SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/WadGen.exe")
ENDIF()

#------------------------------------------------------------------------------
# Install application. On Mac OS X, the bundle is the root of the
# install tree and on other platforms it goes in the bin directory
install(TARGETS wadgen
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime)

# Install Content folder into the Mac OS X bundle
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Content" DESTINATION 
    WadGen.app/Contents/MacOS)

#------------------------------------------------------------------------------
# Use BundleUtilities to get all other dependencies for the application to work.
# It takes a bundle or executable along with possible plugins and inspects it
# for dependencies.  If they are not system dependencies, they are copied.

# directories to look for dependencies
SET(DIRS ${PNG_LIBRARY_DIRS})

# Now the work of copying dependencies into the bundle/package
# The quotes are escaped and variables to use at install time are escaped 
INSTALL(CODE "
    file(GLOB_RECURSE QTPLUGINS
      \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
    set(BU_CHMOD_BUNDLE_ITEMS ON)
    include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"\${PNG_LIBRARY}\" \"${DIRS}\")
    " COMPONENT Runtime)
# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well
set(CPACK_BINARY_DRAGNDROP ON)
INCLUDE(CPack)

# eof
