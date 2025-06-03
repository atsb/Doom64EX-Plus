CC ?= gcc

libs := sdl3 libpng gl fluidsynth

CFLAGS += $(foreach lib,$(libs),$(shell pkg-config --cflags $(lib)))
LDFLAGS := -lm $(foreach lib,$(libs),$(shell pkg-config --libs $(lib)))

OBJDIR=src/engine
OUTPUT=DOOM64EX-Plus

OBJS_SRC = i_system.o am_draw.o am_map.o info.o md5.o tables.o con_console.o con_cvar.o d_devstat.o d_main.o d_net.o f_finale.o in_stuff.o g_actions.o g_demo.o g_game.o g_settings.o wi_stuff.o m_cheat.o m_menu.o m_misc.o m_fixed.o m_keys.o m_password.o m_random.o m_shift.o net_client.o net_common.o net_dedicated.o net_io.o net_loop.o net_packet.o net_query.o net_server.o net_structure.o dgl.o gl_draw.o gl_main.o gl_texture.o sc_main.o p_ceilng.o p_doors.o p_enemy.o p_user.o p_floor.o p_inter.o p_lights.o p_macros.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o r_clipper.o r_drawlist.o r_lights.o r_main.o r_scene.o r_bsp.o r_sky.o r_things.o r_wipe.o s_sound.o st_stuff.o i_audio.o i_main.o i_png.o i_video.o w_file.o w_merge.o w_wad.o z_zone.o i_sdlinput.o deh_io.o deh_ptr.o deh_ammo.o deh_doom.o deh_main.o deh_misc.o deh_frame.o deh_thing.o deh_weapon.o deh_mapping.o deh_str.o sha1.o i_xinput.o

OBJS := $(addprefix $(OBJDIR)/, $(OBJS_SRC))

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(OUTPUT) $(OUTPUT).gdb $(OUTPUT).map $(OBJS)




