ifndef FMOD_STUDIO_SDK_ROOT
$(error FMOD_STUDIO_SDK_ROOT environment variable is not set. Point it to the root of the FMOD studio SDK that can be downloaded on https://www.fmod.com/download#fmodstudio)
endif

ARCH=$(shell uname -m)

ifeq ($(ARCH),x86_64)
FMOD_ARCH=x86_64
endif

ifeq ($(ARCH),aarch64)
FMOD_ARCH=arm64
endif

ifeq ($(ARCH),i586)
FMOD_ARCH=x86
endif

ifeq ($(ARCH),i686)
FMOD_ARCH=x86
endif

ifeq ($(findstring arm,$ARCH),arm)
FMOD_ARCH=arm
endif

ifndef FMOD_ARCH
$(error Unsupported arch: $(ARCH))
endif

libs := sdl3 libpng gl

# dir containing fmod.h
FMOD_INC_DIR=$(FMOD_STUDIO_SDK_ROOT)/api/core/inc

# dir containing libfmod.so
FMOD_LIB_DIR=$(FMOD_STUDIO_SDK_ROOT)/api/core/lib/$(FMOD_ARCH)

CC ?= gcc

OPTFLAGS ?=-O2

# optional: -DDOOM_UNIX_SYSTEM_DATADIR=\"/usr/share/games/doom64ex-plus\"
CFLAGS := $(shell pkg-config --cflags $(libs)) -I$(FMOD_INC_DIR) $(OPTFLAGS) -Wunknown-pragmas -fwrapv -MD $(CFLAGS)

# put current directory (.) in the rpath so DOOM64Ex-Plus can also find libfmod.so.xx in the current directory
LDFLAGS += $(shell pkg-config --libs $(libs)) -lm -L$(FMOD_LIB_DIR) -lfmod -lz -Wl,-rpath=.

OBJDIR=src/engine
OUTPUT=DOOM64

OBJS_SRC = i_system.o am_draw.o am_map.o info.o md5.o tables.o con_console.o con_cvar.o d_devstat.o d_main.o d_net.o f_finale.o in_stuff.o g_actions.o g_demo.o g_game.o g_settings.o wi_stuff.o m_cheat.o m_menu.o m_misc.o m_fixed.o m_keys.o m_password.o m_random.o m_shift.o net_client.o net_common.o net_io.o net_loop.o net_packet.o net_query.o net_server.o net_structure.o dgl.o gl_draw.o gl_main.o gl_texture.o sc_main.o p_ceilng.o p_doors.o p_enemy.o p_user.o p_floor.o p_inter.o p_lights.o p_macros.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o r_clipper.o r_drawlist.o r_lights.o r_main.o r_scene.o r_bsp.o r_sky.o r_things.o r_wipe.o s_sound.o st_stuff.o i_audio.o i_main.o i_png.o i_video.o w_file.o w_merge.o w_wad.o z_zone.o i_sdlinput.o  sha1.o steam.o kpf.o p_mapinfo.o i_shaders.o i_sectorcombiner.o

OBJS := $(addprefix $(OBJDIR)/, $(OBJS_SRC))

OBJS_DEPS := $(OBJS:.o=.d)

all: $(OUTPUT)
	cp $(FMOD_LIB_DIR)/libfmod.so.?? .

$(OUTPUT): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(OUTPUT) $(OUTPUT).gdb $(OUTPUT).map $(OBJS) $(OBJS_DEPS) libfmod.so.??

appimage:
	(cd AppImage && ./build.sh $(PLATFORM))

appimage-clean:
	(cd AppImage && ./build.sh clean)

asan-build:
	LDFLAGS="-fsanitize=address" CFLAGS="-fsanitize=address -fsanitize-recover=address -g -fno-omit-frame-pointer" make OPTFLAGS="-O0" -j

asan-run: asan-build
# __GL_CONSTANT_FRAME_RATE_HINT needed for NVIDIA to not explode with ASAN, do nothing otherwise
	__GL_CONSTANT_FRAME_RATE_HINT=3 ./$(OUTPUT) $(GAME_OPTS) || true

-include $(OBJS_DEPS)
