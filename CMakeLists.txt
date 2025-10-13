cmake_minimum_required(VERSION 3.11)

project(DOOM64 LANGUAGES C VERSION 5.1.0.0)

if(NOT DEFINED ENV{FMOD_STUDIO_SDK_ROOT})
	message(
		FATAL_ERROR
		"FMOD_STUDIO_SDK_ROOT environment variable is not set.\n"
		"Point it to the root of the FMOD studio SDK that can be downloaded on https://www.fmod.com/download#fmodstudio"
	)
endif()

if(
	CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR
	CMAKE_SYSTEM_PROCESSOR STREQUAL "EM64T" OR
	CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
)
	set(CPU_ARCH "x86_64")
elseif(
	CMAKE_SYSTEM_PROCESSOR STREQUAL "X86" OR
	CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" OR
	CMAKE_SYSTEM_PROCESSOR STREQUAL "i686"
)
	set(CPU_ARCH "x86")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
	set(CPU_ARCH "arm")
elseif(
	CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64" OR
	CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64"
)
	set(CPU_ARCH "arm64")
else()
	message(FATAL_ERROR "FMOD: Unsupported CPU architecture")
endif()

set(FMOD_INC_DIR "$ENV{FMOD_STUDIO_SDK_ROOT}/api/core/inc")
set(FMOD_LIB_DIR "$ENV{FMOD_STUDIO_SDK_ROOT}/api/core/lib/${CPU_ARCH}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunknown-pragmas -fwrapv -MD")
set(CMAKE_EXE_LINKER_FLAGS "-L${FMOD_LIB_DIR}")

set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-s -O2")

find_package(SDL3 REQUIRED)
find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
include_directories(
	${libpng_INCLUDE_DIRS}
	${OpenGL_INCLUDE_DIRS}
	${FMOD_INC_DIR}
)

set(SOURCE_DIR "src/engine")

add_executable(
	${PROJECT_NAME}
	${SOURCE_DIR}/i_system.c
	${SOURCE_DIR}/am_draw.c
	${SOURCE_DIR}/am_map.c
	${SOURCE_DIR}/info.c
	${SOURCE_DIR}/kpf.c
	${SOURCE_DIR}/md5.c
	${SOURCE_DIR}/tables.c
	${SOURCE_DIR}/con_console.c
	${SOURCE_DIR}/con_cvar.c
	${SOURCE_DIR}/d_devstat.c
	${SOURCE_DIR}/d_main.c
	${SOURCE_DIR}/d_net.c
	${SOURCE_DIR}/f_finale.c
	${SOURCE_DIR}/in_stuff.c
	${SOURCE_DIR}/g_actions.c
	${SOURCE_DIR}/g_demo.c
	${SOURCE_DIR}/g_game.c
	${SOURCE_DIR}/g_settings.c
	${SOURCE_DIR}/wi_stuff.c
	${SOURCE_DIR}/m_cheat.c
	${SOURCE_DIR}/m_menu.c
	${SOURCE_DIR}/m_misc.c
	${SOURCE_DIR}/m_fixed.c
	${SOURCE_DIR}/m_keys.c
	${SOURCE_DIR}/m_password.c
	${SOURCE_DIR}/m_random.c
	${SOURCE_DIR}/m_shift.c
	${SOURCE_DIR}/net_client.c
	${SOURCE_DIR}/net_common.c
	${SOURCE_DIR}/net_io.c
	${SOURCE_DIR}/net_loop.c
	${SOURCE_DIR}/net_packet.c
	${SOURCE_DIR}/net_query.c
	${SOURCE_DIR}/net_server.c
	${SOURCE_DIR}/net_structure.c
	${SOURCE_DIR}/dgl.c
	${SOURCE_DIR}/gl_draw.c
	${SOURCE_DIR}/gl_main.c
	${SOURCE_DIR}/gl_texture.c
	${SOURCE_DIR}/sc_main.c
	${SOURCE_DIR}/p_ceilng.c
	${SOURCE_DIR}/p_doors.c
	${SOURCE_DIR}/p_enemy.c
	${SOURCE_DIR}/p_user.c
	${SOURCE_DIR}/p_floor.c
	${SOURCE_DIR}/p_inter.c
	${SOURCE_DIR}/p_lights.c
	${SOURCE_DIR}/p_macros.c
	${SOURCE_DIR}/p_map.c
	${SOURCE_DIR}/p_maputl.c
	${SOURCE_DIR}/p_mobj.c
	${SOURCE_DIR}/p_plats.c
	${SOURCE_DIR}/p_pspr.c
	${SOURCE_DIR}/p_saveg.c
	${SOURCE_DIR}/p_setup.c
	${SOURCE_DIR}/p_sight.c
	${SOURCE_DIR}/p_spec.c
	${SOURCE_DIR}/p_switch.c
	${SOURCE_DIR}/p_telept.c
	${SOURCE_DIR}/p_tick.c
	${SOURCE_DIR}/r_clipper.c
	${SOURCE_DIR}/r_drawlist.c
	${SOURCE_DIR}/r_lights.c
	${SOURCE_DIR}/r_main.c
	${SOURCE_DIR}/r_scene.c
	${SOURCE_DIR}/r_bsp.c
	${SOURCE_DIR}/r_sky.c
	${SOURCE_DIR}/r_things.c
	${SOURCE_DIR}/r_wipe.c
	${SOURCE_DIR}/s_sound.c
	${SOURCE_DIR}/st_stuff.c
	${SOURCE_DIR}/i_audio.c
	${SOURCE_DIR}/i_main.c
	${SOURCE_DIR}/i_png.c
	${SOURCE_DIR}/i_video.c
	${SOURCE_DIR}/w_file.c
	${SOURCE_DIR}/w_merge.c
	${SOURCE_DIR}/w_wad.c
	${SOURCE_DIR}/z_zone.c
	${SOURCE_DIR}/i_sdlinput.c
	${SOURCE_DIR}/sha1.c
	${SOURCE_DIR}/steam.c
	${SOURCE_DIR}/p_mapinfo.c
	${SOURCE_DIR}/i_shaders.c
	${SOURCE_DIR}/i_sectorcombiner.c
)

set_property(TARGET ${PROJECT_NAME} PROPERTY BUILD_RPATH ".")

target_link_libraries(
	${PROJECT_NAME}
	SDL3::SDL3
	png
	${OPENGL_LIBRARY}
	fmod
	z
	m
)
