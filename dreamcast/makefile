#----------------------
# | makefile
# | Description: KallistiOS build of the Legend of Mana Dreamcast port: converts the PS1 disc, compiles src/, the shared port/ sources and the decomp's main executable.
# |              Links lom.elf with dreamcast.ld, links the CHECKPS, TITLE, GNAME, WSEL, WMAP, MENU, SHOP, GOLEM and ZUKAN overlays against it and packs everything into lom.cdi.
# | Author: suinevere
# | Dependencies: $(KOS_BASE)/Makefile.rules, pre.makefile, dreamcast.ld, src, ../port, ../src, ../include, ../port/tools/overlay_link.py, ../port/tools/gen_pending.py, cd
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------

#----------------------
# | TARGET
# | Description: Output ELF binary name linked by kos-cc.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TARGET ?= lom.elf

#----------------------
# | OBJDIR / CDI
# | Description: Object-tree root and disc image name; test-image overrides both via a submake so the scripted build never shares objects or output with the shipping lom.cdi.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OBJDIR ?= build
CDI ?= lom.cdi

#----------------------
# | EXTRA_CFLAGS
# | Description: Extra defines test-image passes through a submake, e.g. -DLOM_PAD_SCRIPT; empty for the shipping build. Checked below (SOURCES) and folded into CFLAGS further down.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXTRA_CFLAGS ?=

#----------------------
# | PAD_SCRIPT_SOURCES
# | Description: Sources that only exist to run a compiled-in pad script or its watcher; excluded from the shipping SOURCES below and added back only when EXTRA_CFLAGS defines LOM_PAD_SCRIPT (test-image).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PAD_SCRIPT_SOURCES = src/input/pad_script.c src/input/pad_dc_script_map.c src/system/watch.c src/sound/sound_test_dc.c src/movie/movie_probe_dc.c

#----------------------
# | SOURCES
# | Description: Every C source under src except src/overlays and src/wmap (both compile into overlays instead of main) and PAD_SCRIPT_SOURCES (added back only for test-image).
# | Author: suinevere
# | Dependencies: PAD_SCRIPT_SOURCES
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SOURCES = $(filter-out $(PAD_SCRIPT_SOURCES),$(shell find src -name '*.c' -not -path 'src/overlays/*' -not -path 'src/wmap/*' -not -path 'src/field/*'))
ifneq (,$(findstring LOM_PAD_SCRIPT,$(EXTRA_CFLAGS)))
SOURCES += $(PAD_SCRIPT_SOURCES)
endif

#----------------------
# | KOS_SOURCES
# | Description: The platform sources that include KallistiOS headers, compiled without the decomp prefix because its PS1 types and macros would collide with KallistiOS's.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KOS_SOURCES = src/main.c src/boot/overlay_loader_dc.c src/input/pad_dc.c src/system/dc_exception.c src/system/dc_system.c src/system/vblank_dc.c src/video/pvr_backend_kos.c \
	src/sound/aica.c src/sound/akao_tick_dc.c

#----------------------
# | PORT_SOURCES
# | Description: The shared port/ sources the Dreamcast build uses.
# | Author: suinevere
# | Dependencies: ../port/boot, ../port/input, ../port/system, ../port/psyq, ../port/stubs, ../port/sound
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PORT_SOURCES = ../port/boot/overlay_header.c ../port/boot/resource_index.c ../port/input/pad_publish.c \
	../port/system/vsync_pacer.c ../port/psyq/gte_saturn.c ../port/psyq/libgte_tables.c ../port/stubs/main_stubs.c \
	../port/sound/spu_shadow.c \
	../port/sound/spu_sample_map.c \
	../port/sound/aica_convert.c \
	../port/sound/spu_long_voice.c \
	../port/sound/spu_noise.c \
	../port/sound/spu_voice_flush.c \
	../port/sound/libspu_shim.c

#----------------------
# | DECOMP_MAIN
# | Description: The decomp main-executable files the Saturn main link compiles (saturn/src/mainexe/*_unit.c), plus the PS1 stream decompressor cdrom_dc.c runs on raw resources and card_callbacks.c for CARDA's pet transfer.
# | Author: suinevere
# | Dependencies: saturn/src/mainexe
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_MAIN = ../src/main/main.c ../src/main/field_runtime.c ../src/main/screen_transition.c ../src/main/cdrom_decompress.c ../src/main/card_callbacks.c

#----------------------
# | DECOMP_AUDIO
# | Description: The decomp's AKAO sound driver and song loader, compiled unmodified into main.
# | Author: suinevere
# | Dependencies: ../src/main/audio
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_AUDIO = ../src/main/audio/akao_cmd.c ../src/main/audio/akao_control.c ../src/main/audio/akao_driver.c \
	../src/main/audio/akao_driver_boot.c ../src/main/audio/akao_driver_init_state.c ../src/main/audio/akao_sequencer.c \
	../src/main/audio/akao_voice.c ../src/main/audio/akao_xa_stream.c ../src/main/audio/game_audio.c

#----------------------
# | DECOMP_FIELD
# | Description: Every decomp file under src/overlays/field, linked into FIELD's own Dreamcast overlay instead of main.
# | Author: suinevere
# | Dependencies: src/overlays/field
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_FIELD = ../src/overlays/field/actors/field_actor_action_defaults.c \
	../src/overlays/field/actors/field_actor_behavior.c \
	../src/overlays/field/actors/field_actor_camera.c ../src/overlays/field/actors/field_actor_effects.c \
	../src/overlays/field/actors/field_actor_hud_effects.c \
	../src/overlays/field/actors/field_actor_idle_ops.c ../src/overlays/field/actors/field_actor_input_actions.c \
	../src/overlays/field/actors/field_actor_input_map_init.c ../src/overlays/field/actors/field_actor_key_ops.c \
	../src/overlays/field/actors/field_actor_lifecycle.c ../src/overlays/field/actors/field_actor_motion.c \
	../src/overlays/field/actors/field_actor_reactions.c ../src/overlays/field/actors/field_actor_record_ops.c \
	../src/overlays/field/actors/field_actor_resource_unpack.c ../src/overlays/field/actors/field_actor_runtime.c \
	../src/overlays/field/actors/field_actor_script_ops.c ../src/overlays/field/actors/field_actor_slot_resources.c \
	../src/overlays/field/actors/field_actor_state_updates.c ../src/overlays/field/actors/field_actor_target_queries.c \
	../src/overlays/field/actors/field_actor_templates.c ../src/overlays/field/actors/field_actor_transition_reset.c \
	../src/overlays/field/actors/field_animation.c ../src/overlays/field/actors/field_command_history.c \
	../src/overlays/field/actors/field_path_interpolation.c ../src/overlays/field/overlay_header.c \
	../src/overlays/field/records/field_active_record_ops.c \
	../src/overlays/field/records/field_equipment_combination_rules.c \
	../src/overlays/field/records/field_generated_record_ops.c \
	../src/overlays/field/records/field_golem_logic_blocks.c \
	../src/overlays/field/records/field_group_derived_stats.c ../src/overlays/field/records/field_group_layout_ops.c \
	../src/overlays/field/records/field_group_stat_transfer.c ../src/overlays/field/records/field_layout_slot_state.c \
	../src/overlays/field/records/field_pair_rule_lookup.c ../src/overlays/field/records/field_party_state.c \
	../src/overlays/field/records/field_progression_ops.c ../src/overlays/field/records/field_record_buffer_ops.c \
	../src/overlays/field/records/field_record_effect_ops.c ../src/overlays/field/records/field_record_growth_ops.c \
	../src/overlays/field/records/field_record_lookup_ops.c \
	../src/overlays/field/records/field_record_position_queries.c \
	../src/overlays/field/records/field_record_setup_ops.c ../src/overlays/field/records/field_record_stat_ops.c \
	../src/overlays/field/records/field_record_table_ops.c ../src/overlays/field/records/field_resource_table_ops.c \
	../src/overlays/field/records/field_reward_command_ops.c ../src/overlays/field/records/field_saved_slot_ops.c \
	../src/overlays/field/records/field_select_distance_bucket.c \
	../src/overlays/field/records/field_stat_counter_ops.c ../src/overlays/field/records/field_status_ticks.c \
	../src/overlays/field/render/field_draw_state.c ../src/overlays/field/render/field_effect_update.c \
	../src/overlays/field/render/field_fade.c \
	../src/overlays/field/render/field_mesh_part_animation.c ../src/overlays/field/render/field_mesh_render.c \
	../src/overlays/field/render/field_mesh_transform.c ../src/overlays/field/runtime/field_audio_runtime.c \
	../src/overlays/field/runtime/field_block_allocator.c ../src/overlays/field/runtime/field_card_clock.c \
	../src/overlays/field/runtime/field_character_name_flags.c ../src/overlays/field/runtime/field_resource_load.c \
	../src/overlays/field/runtime/field_slot_pool_ops.c ../src/overlays/field/runtime/field_subsystem_init.c \
	../src/overlays/field/scene/field_collision.c ../src/overlays/field/scene/field_contact_geometry.c \
	../src/overlays/field/scene/field_interaction_start.c \
	../src/overlays/field/scene/field_scene_build.c ../src/overlays/field/scene/field_scene_control.c \
	../src/overlays/field/scene/field_scene_load.c ../src/overlays/field/scene/field_scene_transition.c \
	../src/overlays/field/scripts/field_action_descriptors.c ../src/overlays/field/scripts/field_action_modifiers.c \
	../src/overlays/field/scripts/field_action_setup.c ../src/overlays/field/scripts/field_event_dispatch.c \
	../src/overlays/field/scripts/field_frame_commands.c ../src/overlays/field/scripts/field_script_arith_ops.c \
	../src/overlays/field/scripts/field_script_commands.c ../src/overlays/field/scripts/field_script_operands.c \
	../src/overlays/field/scripts/field_script_ops.c ../src/overlays/field/ui/field_choice_labels.c \
	../src/overlays/field/ui/field_coordinate_icon.c ../src/overlays/field/ui/field_dialog_screens.c \
	../src/overlays/field/ui/field_item_selection.c ../src/overlays/field/ui/field_menu_ops.c \
	../src/overlays/field/ui/field_menu_windows.c ../src/overlays/field/ui/field_modal_runtime.c \
	../src/overlays/field/ui/field_modal_stream_start.c ../src/overlays/field/ui/field_numeric_sprites.c \
	../src/overlays/field/ui/field_pair_indicators.c ../src/overlays/field/ui/field_ring_selection.c \
	../src/overlays/field/ui/field_text.c ../src/overlays/field/ui/field_text_quad.c \
	../src/overlays/field/ui/field_text_session.c ../src/overlays/field/ui/field_text_window_api.c

#----------------------
# | FIELD_TEST_OBJS
# | Description: field_watch_unit.o, compiled into FIELD only for the test image (EXTRA_CFLAGS defines LOM_PAD_SCRIPT).
# | Author: suinevere
# | Dependencies: src/overlays/field_watch_unit.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELD_TEST_OBJS = build/overlays/field_watch_unit.o

#----------------------
# | DECOMP_CHECKPS
# | Description: The decomp CHECKPS files saturn/src/checkps/*_unit.c compile; init.c goes through src/overlays/checkps_init_unit.c for its render-state size check.
# | Author: suinevere
# | Dependencies: saturn/src/checkps, src/overlays/checkps_init_unit.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_CHECKPS = ../src/overlays/checkps/cdrom.c ../src/overlays/checkps/font.c ../src/overlays/checkps/kanji.c ../src/overlays/checkps/pattern.c

#----------------------
# | DECOMP_TITLE
# | Description: The decomp TITLE files saturn/src/title/*_unit.c compile; the Dreamcast keeps title.c's own song loaders.
# | Author: suinevere
# | Dependencies: saturn/src/title
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_TITLE = ../src/overlays/title/title.c ../src/overlays/title/title_save.c

#----------------------
# | DECOMP_GNAME
# | Description: The decomp GNAME file saturn/src/gname/gname_unit.c compiles; the Dreamcast links it alone, so its fade globals keep their names.
# | Author: suinevere
# | Dependencies: saturn/src/gname
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_GNAME = ../src/overlays/gname/gname.c

#----------------------
# | DECOMP_MENU
# | Description: The decomp MENU files the Dreamcast MENU overlay compiles; its PS1 overlay header is replaced by the generated vectors.
# | Author: suinevere
# | Dependencies: src/overlays/menu
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_MENU = ../src/overlays/menu/menu.c ../src/overlays/menu/menu_screens.c ../src/overlays/menu/menu_lists.c ../src/overlays/menu/menu_actions.c ../src/overlays/menu/menu_card.c

#----------------------
# | DECOMP_SHOP / DECOMP_GOLEM / DECOMP_ZUKAN / DECOMP_GOVER / DECOMP_MOVIE / DECOMP_ADDHERO / DECOMP_NIKI / DECOMP_CLOAD / DECOMP_CARDA / DECOMP_GOSUB
# | Description: The decomp files each secondary overlay compiles; their PS1 overlay headers are replaced by the generated vectors.
# | Author: suinevere
# | Dependencies: src/overlays/shop, src/overlays/golem, src/overlays/zukan
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_SHOP = ../src/overlays/shop/shop.c ../src/overlays/shop/shop_render.c ../src/overlays/shop/shop_text.c ../src/overlays/shop/shop_trade.c
DECOMP_GOLEM = ../src/overlays/golem/golem.c
DECOMP_ZUKAN = ../src/overlays/zukan/zukan.c ../src/overlays/zukan/zukan_category.c
DECOMP_GOVER = ../src/overlays/gover/gover.c
DECOMP_MOVIE = ../src/overlays/movie/movie.c ../src/overlays/movie/movie_stream.c
DECOMP_ADDHERO = ../src/overlays/addhero/addhero.c ../src/overlays/addhero/addhero_card.c ../src/overlays/addhero/addhero_glyph.c ../src/overlays/addhero/addhero_widgets.c
DECOMP_NIKI = ../src/overlays/niki/niki.c ../src/overlays/niki/niki_io.c
DECOMP_CLOAD = ../src/overlays/cload/cload.c ../src/overlays/cload/cload_card.c ../src/overlays/cload/cload_glyph.c ../src/overlays/cload/cload_widgets.c
DECOMP_CARDA = ../src/overlays/carda/carda.c ../src/overlays/carda/carda_card.c ../src/overlays/carda/carda_glyph.c ../src/overlays/carda/carda_save.c ../src/overlays/carda/carda_widgets.c
DECOMP_GOSUB = ../src/overlays/gosub/gosub.c ../src/overlays/gosub/gosub_dialogs.c ../src/overlays/gosub/gosub_elements.c ../src/overlays/gosub/gosub_helpers.c ../src/overlays/gosub/gosub_lists.c ../src/overlays/gosub/gosub_render.c ../src/overlays/gosub/gosub_runtime.c ../src/overlays/gosub/gosub_selection.c

#----------------------
# | DECOMP_WMAP
# | Description: The decomp WMAP files saturn/src/wmap/*_unit.c compile, compiled here by path.
# | Author: suinevere
# | Dependencies: saturn/src/wmap
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_WMAP = ../src/overlays/wmap/overlay_header.c ../src/overlays/wmap/effects/wmap_effect_backdrop.c \
	../src/overlays/wmap/effects/wmap_effect_primitives.c ../src/overlays/wmap/effects/wmap_effect_resources.c \
	../src/overlays/wmap/render/wmap_frame_render.c ../src/overlays/wmap/effects/wmap_land_effect_00.c ../src/overlays/wmap/effects/wmap_land_effect_01.c \
	../src/overlays/wmap/effects/wmap_land_effect_02.c ../src/overlays/wmap/effects/wmap_land_effect_03.c \
	../src/overlays/wmap/effects/wmap_land_effect_04.c ../src/overlays/wmap/effects/wmap_land_effect_05.c \
	../src/overlays/wmap/effects/wmap_land_effect_07.c ../src/overlays/wmap/effects/wmap_land_effect_08.c \
	../src/overlays/wmap/effects/wmap_land_effect_09.c ../src/overlays/wmap/effects/wmap_land_effect_10.c \
	../src/overlays/wmap/effects/wmap_land_effect_11.c ../src/overlays/wmap/effects/wmap_land_effect_12.c \
	../src/overlays/wmap/effects/wmap_land_effect_13.c ../src/overlays/wmap/effects/wmap_land_effect_15.c \
	../src/overlays/wmap/effects/wmap_land_effect_16.c ../src/overlays/wmap/effects/wmap_land_effect_17.c \
	../src/overlays/wmap/effects/wmap_land_effect_18.c ../src/overlays/wmap/effects/wmap_land_effect_19.c \
	../src/overlays/wmap/effects/wmap_land_effect_21.c ../src/overlays/wmap/effects/wmap_land_effect_22.c \
	../src/overlays/wmap/effects/wmap_land_effect_23.c ../src/overlays/wmap/effects/wmap_land_effect_24.c \
	../src/overlays/wmap/effects/wmap_land_effect_25.c ../src/overlays/wmap/effects/wmap_land_effect_26.c \
	../src/overlays/wmap/effects/wmap_land_effect_27.c ../src/overlays/wmap/effects/wmap_land_effect_30.c \
	../src/overlays/wmap/effects/wmap_land_effect_31.c ../src/overlays/wmap/effects/wmap_land_effect_32.c \
	../src/overlays/wmap/effects/wmap_land_effect_33.c ../src/overlays/wmap/effects/wmap_land_effect_loader.c \
	../src/overlays/wmap/effects/wmap_land_event_00_a.c ../src/overlays/wmap/effects/wmap_land_event_00_b.c \
	../src/overlays/wmap/effects/wmap_land_event_17.c ../src/overlays/wmap/map/wmap_land_layout.c ../src/overlays/wmap/map/wmap_land_preview_lines.c \
	../src/overlays/wmap/map/wmap_land_preview.c ../src/overlays/wmap/map/wmap_land_selection.c ../src/overlays/wmap/map/wmap_land_transition.c \
	../src/overlays/wmap/wmap_main.c ../src/overlays/wmap/map/wmap_map_display.c ../src/overlays/wmap/map/wmap_map_events.c \
	../src/overlays/wmap/render/wmap_map_labels.c ../src/overlays/wmap/render/wmap_model_render.c ../src/overlays/wmap/travel/wmap_party_travel.c \
	../src/overlays/wmap/travel/wmap_pathfinding.c ../src/overlays/wmap/map/wmap_resource_support.c ../src/overlays/wmap/travel/wmap_sequence_runtime.c \
	../src/overlays/wmap/effects/wmap_special_effect_34.c ../src/overlays/wmap/effects/wmap_special_effect_35.c \
	../src/overlays/wmap/render/wmap_sprite_render.c ../src/overlays/wmap/travel/wmap_travel_sequences.c ../src/overlays/wmap/render/wmap_view_effects.c

#----------------------
# | OBJS
# | Description: $(TARGET)'s objects: src/ under $(OBJDIR)/, port/ under $(OBJDIR)/port/, the decomp main executable under build/decomp/, and the generated pending stubs.
# |              Then each overlay's objects and all of them together; WSEL compiles wsel.c through src/overlays/wsel_unit.c, FIELD adds its function tables and (test-image only) its watch entry, and WMAP adds its function tables and controller-port pointer.
# | Author: suinevere
# | Dependencies: SOURCES, PORT_SOURCES, DECOMP_MAIN, DECOMP_CHECKPS, DECOMP_TITLE, DECOMP_GNAME, DECOMP_FIELD, DECOMP_WMAP, src/overlays, src/field, src/wmap
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_OBJ = $(patsubst ../src/%.c,build/decomp/%.o,$(1))
MAIN_OBJS = $(patsubst src/%.c,$(OBJDIR)/%.o,$(SOURCES)) $(patsubst ../port/%.c,$(OBJDIR)/port/%.o,$(PORT_SOURCES)) $(call DECOMP_OBJ,$(DECOMP_MAIN)) $(call DECOMP_OBJ,$(DECOMP_AUDIO)) \
	$(OBJDIR)/gen/akao_data.o
OBJS = $(MAIN_OBJS) $(OBJDIR)/gen/dc_pending.o
CHECKPS_OBJS = build/ovl/checkps_vectors.o $(call DECOMP_OBJ,$(DECOMP_CHECKPS)) build/overlays/checkps_init_unit.o
TITLE_OBJS = build/ovl/title_vectors.o $(call DECOMP_OBJ,$(DECOMP_TITLE))
GNAME_OBJS = build/ovl/gname_vectors.o $(call DECOMP_OBJ,$(DECOMP_GNAME))
MENU_OBJS = build/ovl/menu_vectors.o $(call DECOMP_OBJ,$(DECOMP_MENU))
SHOP_OBJS = build/ovl/shop_vectors.o $(call DECOMP_OBJ,$(DECOMP_SHOP))
GOLEM_OBJS = build/ovl/golem_vectors.o $(call DECOMP_OBJ,$(DECOMP_GOLEM))
ZUKAN_OBJS = build/ovl/zukan_vectors.o $(call DECOMP_OBJ,$(DECOMP_ZUKAN)) build/overlays/zukan_run_dc.o
GOVER_OBJS = build/ovl/gover_vectors.o $(call DECOMP_OBJ,$(DECOMP_GOVER))
MOVIE_OBJS = build/ovl/movie_vectors.o $(call DECOMP_OBJ,$(DECOMP_MOVIE))
ADDHERO_OBJS = build/ovl/addhero_vectors.o $(call DECOMP_OBJ,$(DECOMP_ADDHERO))
NIKI_OBJS = build/ovl/niki_vectors.o $(call DECOMP_OBJ,$(DECOMP_NIKI))
CLOAD_OBJS = build/ovl/cload_vectors.o $(call DECOMP_OBJ,$(DECOMP_CLOAD))
CARDA_OBJS = build/ovl/carda_vectors.o $(call DECOMP_OBJ,$(DECOMP_CARDA))
GOSUB_OBJS = build/ovl/gosub_vectors.o $(call DECOMP_OBJ,$(DECOMP_GOSUB))
WSEL_OBJS = build/ovl/wsel_vectors.o build/overlays/wsel_unit.o

#----------------------
# | field_watch_entry.o OBJDIR scoping
# | Description: Unlike FIELD's other extra objects, field_watch_entry.o's body depends on EXTRA_CFLAGS (LOM_PAD_SCRIPT), so it must not share the build/overlays/%.o path the shipping and test-image links would otherwise reuse stale across each other.
# | Author: suinevere
# | Dependencies: src/overlays/field_watch_entry.c, EXTRA_CFLAGS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELD_OBJS = build/ovl/field_vectors.o $(call DECOMP_OBJ,$(DECOMP_FIELD)) build/ovl/field_function_tables.o build/overlays/field_globals_dc.o $(OBJDIR)/overlays/field_watch_entry.o
ifneq (,$(findstring LOM_PAD_SCRIPT,$(EXTRA_CFLAGS)))
FIELD_OBJS += $(FIELD_TEST_OBJS)
endif
WMAP_OBJS = build/ovl/wmap_vectors.o $(call DECOMP_OBJ,$(DECOMP_WMAP)) build/ovl/wmap_function_tables.o build/overlays/wmap_controller_ports.o
OVERLAY_OBJS = $(CHECKPS_OBJS) $(TITLE_OBJS) $(GNAME_OBJS) $(WSEL_OBJS) $(FIELD_OBJS) $(WMAP_OBJS) $(MENU_OBJS) $(SHOP_OBJS) $(GOLEM_OBJS) $(ZUKAN_OBJS) $(GOSUB_OBJS) $(CARDA_OBJS) $(CLOAD_OBJS) $(NIKI_OBJS) $(ADDHERO_OBJS) $(GOVER_OBJS) $(MOVIE_OBJS)

#----------------------
# | OVERLAY_NAMES / OVERLAY_GROUP_*
# | Description: The linked overlays and each one's objects, for recipes that must judge every overlay against main alone, as its own link does.
# | Author: suinevere
# | Dependencies: CHECKPS_OBJS, TITLE_OBJS, GNAME_OBJS, WSEL_OBJS, FIELD_OBJS, WMAP_OBJS, MENU_OBJS, SHOP_OBJS, GOLEM_OBJS, ZUKAN_OBJS, GOVER_OBJS, ADDHERO_OBJS, NIKI_OBJS, CLOAD_OBJS, CARDA_OBJS, GOSUB_OBJS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_NAMES = checkps title gname wsel field wmap menu shop golem zukan gosub carda cload niki addhero gover movie
OVERLAY_GROUP_checkps = $(CHECKPS_OBJS)
OVERLAY_GROUP_title = $(TITLE_OBJS)
OVERLAY_GROUP_gname = $(GNAME_OBJS)
OVERLAY_GROUP_wsel = $(WSEL_OBJS)
OVERLAY_GROUP_field = $(FIELD_OBJS)
OVERLAY_GROUP_wmap = $(WMAP_OBJS)
OVERLAY_GROUP_menu = $(MENU_OBJS)
OVERLAY_GROUP_shop = $(SHOP_OBJS)
OVERLAY_GROUP_golem = $(GOLEM_OBJS)
OVERLAY_GROUP_zukan = $(ZUKAN_OBJS)
OVERLAY_GROUP_gover = $(GOVER_OBJS)
OVERLAY_GROUP_movie = $(MOVIE_OBJS)
OVERLAY_GROUP_addhero = $(ADDHERO_OBJS)
OVERLAY_GROUP_niki = $(NIKI_OBJS)
OVERLAY_GROUP_cload = $(CLOAD_OBJS)
OVERLAY_GROUP_carda = $(CARDA_OBJS)
OVERLAY_GROUP_gosub = $(GOSUB_OBJS)

#----------------------
# | OVERLAY_SYMBOLS / OVERLAY_CODE
# | Description: The converter's per-overlay data symbol scripts, and this build's packed overlay code files.
# | Author: suinevere
# | Dependencies: PRE_BUILD_STAMP, OBJDIR
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_SYMBOLS = gen/checkps_symbols.ld gen/title_symbols.ld gen/gname_symbols.ld gen/wsel_symbols.ld gen/field_symbols.ld gen/wmap_symbols.ld gen/menu_symbols.ld gen/shop_symbols.ld gen/golem_symbols.ld gen/zukan_symbols.ld gen/gosub_symbols.ld gen/carda_symbols.ld gen/cload_symbols.ld gen/niki_symbols.ld gen/addhero_symbols.ld gen/gover_symbols.ld gen/movie_symbols.ld
OVERLAY_CODE = $(OBJDIR)/ovl/OVL15C.BIN $(OBJDIR)/ovl/OVL04C.BIN $(OBJDIR)/ovl/OVL05C.BIN $(OBJDIR)/ovl/OVL14C.BIN $(OBJDIR)/ovl/OVL02C.BIN $(OBJDIR)/ovl/OVL03C.BIN $(OBJDIR)/ovl/OVL06C.BIN $(OBJDIR)/ovl/OVL07C.BIN $(OBJDIR)/ovl/OVL09C.BIN $(OBJDIR)/ovl/OVL08C.BIN $(OBJDIR)/ovl/OVL13C.BIN $(OBJDIR)/ovl/OVL12C.BIN $(OBJDIR)/ovl/OVL16C.BIN $(OBJDIR)/ovl/OVL17C.BIN $(OBJDIR)/ovl/OVL18C.BIN $(OBJDIR)/ovl/OVL10C.BIN $(OBJDIR)/ovl/OVL11C.BIN

#----------------------
# | CFLAGS
# | Description: US region and Dreamcast-target defines, plus an include path for every src subfolder, the port/ folders, the decomp's include, the generated gen/ headers and EXTRA_CFLAGS.
# |              Raises RESOURCE_INDEX_MAX past Saturn's 256 default to 6144, covering the table's 5811 entries.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CFLAGS += -std=gnu11 -O2 -Wall -DVERSION_US -DLOM_DREAMCAST -DRESOURCE_INDEX_MAX=6144 $(patsubst %,-I%,$(shell find src -type d)) -I../port/boot -I../port/input -I../port/system -I../port/sound -I../include -Igen $(EXTRA_CFLAGS)

#----------------------
# | DECOMP_FLAGS / DECOMP_STD
# | Description: The decomp prefix and include paths (the Psy-Q headers after newlib's, so its <strings.h> keeps libc's prototypes), and the Saturn DECOMP_STD warning set minus -Wreturn-mismatch, which GCC 13 lacks, for every source that sees PS1 types.
# | Author: suinevere
# | Dependencies: ../port/psyq/lom_prefix.h, saturn/makefile (DECOMP_STD)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_FLAGS = -idirafter ../include/sdk -I../src -I../port/psyq -include ../port/psyq/lom_prefix.h
DECOMP_STD = -fno-builtin -Wno-builtin-declaration-mismatch -Wno-error=incompatible-pointer-types

include $(KOS_BASE)/Makefile.rules

#----------------------
# | KOS_LDFLAGS
# | Description: KallistiOS's link flags with its shlelf.xc swapped for dreamcast.ld, exported so kos-cc links with the overlay arena.
# | Author: suinevere
# | Dependencies: dreamcast.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
export KOS_LDFLAGS := $(filter-out -T%,$(KOS_LDFLAGS)) -Tdreamcast.ld

#----------------------
# | KOS_CDROM_PATCH_DIR / KOS_CDROM_PATCH_MEMBERS / KOS_CDROM_PATCH_OBJS / KOS_LIBKALLISTI
# | Description: The decomp's cdrom_init (src/cd/cdrom_dc.c) collides by name with libkallisti's own GD-ROM bring-up; these are local, renamed copies of the two libkallisti members that define or call it, linked ahead of -lkallisti so KallistiOS's real hardware init keeps running under its own symbol instead of being shadowed by ours.
# | Author: suinevere
# | Dependencies: src/cd/cdrom_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KOS_CDROM_PATCH_DIR = build/kos_patch
KOS_CDROM_PATCH_MEMBERS = cdrom.o hardware.o
KOS_CDROM_PATCH_OBJS = $(patsubst %,$(KOS_CDROM_PATCH_DIR)/%,$(KOS_CDROM_PATCH_MEMBERS))
KOS_LIBKALLISTI = $(KOS_BASE)/lib/dreamcast/libkallisti.a

#----------------------
# | TOOLCHAIN_LIBS
# | Description: The libc, libm and libgcc archives main links, which the pending generator counts as defined.
# | Author: suinevere
# | Dependencies: kos-cc
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TOOLCHAIN_LIBS = $(shell kos-cc -print-file-name=libc.a) $(shell kos-cc -print-file-name=libm.a) $(shell kos-cc -print-libgcc-file-name)

.PHONY: all clean pending test-image

#----------------------
# | all
# | Description: Builds the bootable disc image.
# | Author: suinevere
# | Dependencies: $(CDI)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
all: $(CDI)

include pre.makefile

$(MAIN_OBJS) $(OVERLAY_OBJS): | $(PRE_BUILD_STAMP)

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

$(patsubst src/%.c,$(OBJDIR)/%.o,$(filter-out src/main.c,$(KOS_SOURCES))): $(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@

#----------------------
# | build/main.o
# | Description: Compiles main.c, then renames its KOS_INIT_FLAGS reference to KallistiOS's cdrom_init (from <dc/cdrom.h>, included through kos.h) to kos_real_cdrom_init, so it binds to the real GD-ROM bring-up and not to the decomp's identically-named cdrom_init (src/cd/cdrom_dc.c); main.c therefore never calls the decomp's cdrom_init itself.
# | Author: suinevere
# | Dependencies: sh-elf-objcopy, src/cd/cdrom_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/main.o: src/main.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@
	sh-elf-objcopy --redefine-sym _cdrom_init=_kos_real_cdrom_init $@

$(OBJDIR)/port/%.o: ../port/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

#----------------------
# | build/port/psyq/gte_saturn.o
# | Description: Strict C11 hides newlib's BSD <strings.h> from gte_saturn.c's <string.h>, whose bcopy prototype clashes with the Psy-Q one lom_prefix.h declares.
# | Author: suinevere
# | Dependencies: ../port/psyq/gte_saturn.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/port/psyq/gte_saturn.o: DECOMP_STD += -std=c11

#----------------------
# | build/port/sound/%.o
# | Description: The shared sound sources include <string.h> too, so they take gte_saturn.o's strict C11 for the same bcopy clash.
# | Author: suinevere
# | Dependencies: ../port/sound, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/port/sound/%.o: DECOMP_STD += -std=c11

#----------------------
# | DECOMP_AUDIO objects
# | Description: AKAO's root-counter, event and CD-mix calls are renamed to the sound tick's own, so its timer event never reaches the card code's OpenEvent.
# | Author: suinevere
# | Dependencies: src/sound/akao_tick_dc.c, ../src/main/audio/akao_driver_boot.c, ../src/main/audio/akao_sequencer.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(call DECOMP_OBJ,$(DECOMP_AUDIO)): DECOMP_STD += -DSetRCnt=lom_SetRCnt -DStartRCnt=lom_StartRCnt -DGetRCnt=lom_GetRCnt -DOpenEvent=lom_sound_OpenEvent -DEnableEvent=lom_sound_EnableEvent \
	-DDisableEvent=lom_sound_DisableEvent -DStopRCnt=lom_StopRCnt -DUnDeliverEvent=lom_sound_UnDeliverEvent -DCdMix=lom_sound_CdMix

#----------------------
# | build/cd/cdrom_dc.o
# | Description: Strict C11 hides newlib's BSD <strings.h> from cdrom_dc.c's <string.h>, whose bcopy prototype clashes with the Psy-Q one lom_prefix.h declares; same fix as gte_saturn.o.
# | Author: suinevere
# | Dependencies: src/cd/cdrom_dc.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/cd/cdrom_dc.o: DECOMP_STD += -std=c11

#----------------------
# | build/card/card_vmu.o / build/card/card_store_kos.o
# | Description: Strict C11 for the card backend's <string.h>, for the same bcopy clash as cdrom_dc.o.
# | Author: suinevere
# | Dependencies: src/card/card_vmu.c, src/card/card_store_kos.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/card/card_vmu.o $(OBJDIR)/card/card_store_kos.o: DECOMP_STD += -std=c11

#----------------------
# | build/decomp/overlays/checkps/kanji.o
# | Description: kanji.c includes <strings.h> directly, so -std=c11 cannot hide its legacy bcopy the way it does for <string.h> callers (newlib still shows bcopy when __POSIX_VISIBLE is unset); raising _POSIX_C_SOURCE past 200112 hides it instead, clashing with lom_prefix.h's Psy-Q bcopy.
# | Author: suinevere
# | Dependencies: ../src/overlays/checkps/kanji.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/decomp/overlays/checkps/kanji.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L

#----------------------
# | $(OBJDIR)/movie/%.o / $(OBJDIR)/cd/cd_stream.o / $(OBJDIR)/cd/cd_stream_kos.o / $(OBJDIR)/video/vram_mirror.o
# | Description: The movie codec, sector stream and VRAM mirror use <string.h>; the same _POSIX_C_SOURCE fix as kanji.o hides the legacy newlib bcopy from the Psy-Q one in lom_prefix.h.
# | Author: suinevere
# | Dependencies: src/movie/*.c, src/cd/cd_stream.c, src/cd/cd_stream_kos.c, src/video/vram_mirror.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/movie/%.o $(OBJDIR)/cd/cd_stream.o $(OBJDIR)/cd/cd_stream_kos.o $(OBJDIR)/video/vram_mirror.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L

#----------------------
# | $(OBJDIR)/movie/mdec.o / $(OBJDIR)/movie/str_vlc.o
# | Description: The software STR decoder at -O3: the decode probe measured 49 ms a frame at -O2 and 38 ms at -O3, against 66.7 ms per movie frame.
# | Author: suinevere
# | Dependencies: src/movie/mdec.c, src/movie/str_vlc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/movie/mdec.o $(OBJDIR)/movie/str_vlc.o: DECOMP_STD += -O3

#----------------------
# | $(call DECOMP_OBJ,$(DECOMP_MOVIE))
# | Description: MOVIE's AKAO ring-stream calls go to movie_audio_dc.c, which plays the ring through the AICA stream channel; AKAO's own ring refills on SPU IRQs the Dreamcast does not raise.
# | Author: suinevere
# | Dependencies: src/movie/movie_audio_dc.c, ../src/overlays/movie/movie.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(call DECOMP_OBJ,$(DECOMP_MOVIE)): DECOMP_STD += -Dakao_start_xa_stream=movie_audio_start -Dakao_xa_advance_frame=movie_audio_advance \
	-Dakao_xa_get_position=movie_audio_position -Dakao_set_xa_volume=movie_audio_volume

#----------------------
# | build/decomp/overlays/field/actors/field_command_history.o / build/decomp/overlays/field/runtime/field_character_name_flags.o
# | Description: Both include <strings.h> directly, same legacy-bcopy clash as kanji.o; same fix.
# | Author: suinevere
# | Dependencies: ../src/overlays/field/actors/field_command_history.c, ../src/overlays/field/runtime/field_character_name_flags.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/decomp/overlays/field/actors/field_command_history.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L
build/decomp/overlays/field/runtime/field_character_name_flags.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L
build/decomp/overlays/field/runtime/field_subsystem_init.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L
build/decomp/overlays/field/ui/field_ring_selection.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L

#----------------------
# | MENU objects
# | Description: menu.h includes <strings.h>, so every MENU file hits the same legacy-bcopy clash as kanji.o; same fix.
# | Author: suinevere
# | Dependencies: ../include/overlays/menu/menu.h, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(call DECOMP_OBJ,$(DECOMP_MENU)): DECOMP_STD += -D_POSIX_C_SOURCE=200809L

#----------------------
# | GOVER objects
# | Description: gover.c splits its frame pair into g_gover_frame_header and g_gover_frame_tail and relies on them being adjacent in source order; gcc otherwise emits globals reversed.
# | Author: suinevere
# | Dependencies: ../src/overlays/gover/gover.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(call DECOMP_OBJ,$(DECOMP_GOVER)): DECOMP_STD += -fno-common -fno-toplevel-reorder

#----------------------
# | GOSUB objects
# | Description: gosub_internal.h defines helpers as GNU89 "extern inline" (inline-only, real copy elsewhere), which gnu11 reads as a second definition.
# | Author: suinevere
# | Dependencies: ../src/overlays/gosub/internal/gosub_internal.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(call DECOMP_OBJ,$(DECOMP_GOSUB)): DECOMP_STD += -fgnu89-inline

#----------------------
# | Secondary overlay objects
# | Description: These secondary overlays pull in <strings.h> too, so they need MENU's legacy-bcopy fix.
# | Author: suinevere
# | Dependencies: ../src/overlays/golem/golem.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(call DECOMP_OBJ,$(DECOMP_GOLEM)) $(call DECOMP_OBJ,$(DECOMP_ZUKAN)) $(call DECOMP_OBJ,$(DECOMP_GOSUB)) $(call DECOMP_OBJ,$(DECOMP_CARDA)) $(call DECOMP_OBJ,$(DECOMP_CLOAD)) $(call DECOMP_OBJ,$(DECOMP_NIKI)) $(call DECOMP_OBJ,$(DECOMP_ADDHERO)) $(call DECOMP_OBJ,$(DECOMP_GOVER)) $(call DECOMP_OBJ,$(DECOMP_MOVIE)): DECOMP_STD += -D_POSIX_C_SOURCE=200809L

#----------------------
# | build/decomp/overlays/field/actors/field_actor_hud_effects.o
# | Description: field_actor_hud_effects.c declares its own bcopy prototype, clashing with lom_prefix.h's Psy-Q one. LOM_SKIP_PREFIX_BCOPY hides lom_prefix.h's so only the file's own applies, still resolving to port/stubs/main_stubs.c at link time.
# | Author: suinevere
# | Dependencies: ../src/overlays/field/actors/field_actor_hud_effects.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/decomp/overlays/field/actors/field_actor_hud_effects.o: DECOMP_STD += -DLOM_SKIP_PREFIX_BCOPY

build/decomp/%.o: ../src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

#----------------------
# | build/overlays/%.o
# | Description: Compiles the overlay wrappers in src/overlays into build/overlays whatever OBJDIR is, since every build links the same overlay objects.
# | Author: suinevere
# | Dependencies: src/overlays
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/overlays/%.o: src/overlays/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

build/ovl/%_vectors.o: gen/%_vectors.s
	@mkdir -p $(dir $@)
	kos-cc -c $< -o $@

#----------------------
# | build/ovl/wmap_function_tables.o
# | Description: Compiles the converter's WMAP function tables into the WMAP overlay, never main, so main holds no WMAP-owned data.
# | Author: suinevere
# | Dependencies: src/wmap/wmap_function_tables.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/ovl/wmap_function_tables.o: src/wmap/wmap_function_tables.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

#----------------------
# | build/ovl/field_function_tables.o
# | Description: Compiles the converter's FIELD function tables into the FIELD overlay, never main, so main holds no FIELD-owned data.
# | Author: suinevere
# | Dependencies: src/field/field_function_tables.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/ovl/field_function_tables.o: src/field/field_function_tables.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

$(OBJDIR)/gen/dc_pending.o: gen/dc_pending.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@

#----------------------
# | build/gen/akao_data.o
# | Description: AKAO's PS1 data and BSS blocks, generated from the user's SLUS by the pre-build step; gen/dc_pending.ld aliases AKAO's data symbols into them.
# | Author: suinevere
# | Dependencies: gen/akao_data.c, ../port/tools/akao_data.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/gen/akao_data.o: gen/akao_data.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@

gen/akao_data.c: | $(PRE_BUILD_STAMP)

#----------------------
# | DEPFILES
# | Description: Pulls in every object's -MMD -MP dependency file, so editing a header these objects include (lom_prefix.h, gpu_tag.h, lom_platform_memory.h, …) rebuilds them; mirrors saturn/makefile's -include of C_OBJECTS/PORT_OBJECTS .d files.
# | Author: suinevere
# | Dependencies: OBJS, OVERLAY_OBJS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
-include $(OBJS:.o=.d) $(OVERLAY_OBJS:.o=.d)

#----------------------
# | $(KOS_CDROM_PATCH_DIR)/.stamp
# | Description: Extracts the two colliding libkallisti members and renames their cdrom_init symbol to kos_real_cdrom_init, so our decomp cdrom_init stays the only cdrom_init in the final link.
# | Author: suinevere
# | Dependencies: sh-elf-ar, sh-elf-objcopy, $(KOS_LIBKALLISTI)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(KOS_CDROM_PATCH_DIR)/.stamp: $(KOS_LIBKALLISTI)
	@mkdir -p $(KOS_CDROM_PATCH_DIR)
	cd $(KOS_CDROM_PATCH_DIR) && sh-elf-ar x $(abspath $(KOS_LIBKALLISTI)) $(KOS_CDROM_PATCH_MEMBERS)
	for o in $(KOS_CDROM_PATCH_MEMBERS); do sh-elf-objcopy --redefine-sym _cdrom_init=_kos_real_cdrom_init $(KOS_CDROM_PATCH_DIR)/$$o; done
	touch $@

$(KOS_CDROM_PATCH_OBJS): $(KOS_CDROM_PATCH_DIR)/.stamp

#----------------------
# | $(TARGET)
# | Description: Links lom.elf, failing on any section dreamcast.ld does not place.
# |              gen/dc_pending.ld (pending data aliases) and $(OBJDIR)/ovl/overlay_externs.ld ride along as implicit linker scripts, and the overlay entry script relinks it when the overlays move.
# |              -u roots AKAO's data blocks for --gc-sections, since only gen/dc_pending.ld's address aliases refer to them.
# | Author: suinevere
# | Dependencies: OBJS, KOS_CDROM_PATCH_OBJS, dreamcast.ld, gen/dc_pending.ld, gen/overlay_entries.ld, $(OBJDIR)/ovl/overlay_externs.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(TARGET): $(OBJS) $(KOS_CDROM_PATCH_OBJS) dreamcast.ld gen/dc_pending.ld gen/overlay_entries.ld $(OBJDIR)/ovl/overlay_externs.ld
	kos-cc -o $@ $(OBJS) $(KOS_CDROM_PATCH_OBJS) gen/dc_pending.ld $(OBJDIR)/ovl/overlay_externs.ld -Wl,--orphan-handling=error \
		-Wl,-u,_lom_akao_data -Wl,-u,_lom_akao_bss

#----------------------
# | $(OBJDIR)/ovl/overlay_externs.ld
# | Description: EXTERN()s every symbol an overlay references that neither its own objects nor its own data script define.
# |              So --gc-sections keeps it in lom.elf and libc helpers only an overlay calls get linked into main.
# |              Per-OBJDIR (not shared "build/ovl/...") since FIELD_OBJS gains field_watch_unit.o only under test-image's EXTRA_CFLAGS, changing OVERLAY_GROUP_field's own unresolved set between the two builds.
# | Author: suinevere
# | Dependencies: sh-elf-nm, OVERLAY_NAMES, OVERLAY_GROUP_*, OVERLAY_SYMBOLS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/ovl/overlay_externs.ld: $(OVERLAY_OBJS) $(OVERLAY_SYMBOLS)
	@mkdir -p $(dir $@)
	rm -f $(OBJDIR)/ovl/overlay_unresolved.txt
	$(foreach o,$(OVERLAY_NAMES),\
		{ sh-elf-nm -g --defined-only $(OVERLAY_GROUP_$(o)) | awk 'NF == 3 { print $$3 }'; \
		  awk '$$2 == "=" { print $$1 }' gen/$(o)_symbols.ld; } | sort -u > $(OBJDIR)/ovl/$(o)_defined.txt && \
		sh-elf-nm -u $(OVERLAY_GROUP_$(o)) | awk 'NF == 2 { print $$2 }' | sort -u \
			| comm -23 - $(OBJDIR)/ovl/$(o)_defined.txt >> $(OBJDIR)/ovl/overlay_unresolved.txt &&) true
	sort -u $(OBJDIR)/ovl/overlay_unresolved.txt | sed 's/.*/EXTERN(&)/' > $@

#----------------------
# | $(OBJDIR)/ovl/OVL15C.BIN / $(OBJDIR)/ovl/OVL04C.BIN / $(OBJDIR)/ovl/OVL02C.BIN / $(OBJDIR)/ovl/OVL05C.BIN / $(OBJDIR)/ovl/OVL14C.BIN / $(OBJDIR)/ovl/OVL03C.BIN
# | Description: Links CHECKPS, TITLE, FIELD, GNAME, WSEL, WMAP, MENU, SHOP, GOLEM and ZUKAN at their arena code regions against $(TARGET), per-OBJDIR since lom.elf and lom_test.elf differ.
# |              GNAME links after FIELD and also against FIELD's own elf (--links field), as saturn/makefile's OVERLAY_LINKS_gname = field does.
# | Author: suinevere
# | Dependencies: ../port/tools/overlay_link.py, gen/checkps.ld, gen/title.ld, gen/field.ld, gen/gname.ld, gen/wsel.ld, gen/wmap.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/ovl/OVL15C.BIN: $(TARGET) $(CHECKPS_OBJS) gen/checkps.ld gen/checkps_pending.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link checkps $(TARGET) $(OBJDIR)/ovl $@ $(CHECKPS_OBJS)

$(OBJDIR)/ovl/OVL04C.BIN: $(TARGET) $(TITLE_OBJS) gen/title.ld gen/title_pending.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link title $(TARGET) $(OBJDIR)/ovl $@ $(TITLE_OBJS)

$(OBJDIR)/ovl/OVL02C.BIN: $(TARGET) $(FIELD_OBJS) gen/field.ld gen/field_pending.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link field $(TARGET) $(OBJDIR)/ovl $@ $(FIELD_OBJS)

$(OBJDIR)/ovl/OVL05C.BIN: $(TARGET) $(GNAME_OBJS) gen/gname.ld gen/gname_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link gname $(TARGET) $(OBJDIR)/ovl $@ $(GNAME_OBJS) --links field

$(OBJDIR)/ovl/OVL14C.BIN: $(TARGET) $(WSEL_OBJS) gen/wsel.ld gen/wsel_pending.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link wsel $(TARGET) $(OBJDIR)/ovl $@ $(WSEL_OBJS)

$(OBJDIR)/ovl/OVL03C.BIN: $(TARGET) $(WMAP_OBJS) gen/wmap.ld gen/wmap_pending.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link wmap $(TARGET) $(OBJDIR)/ovl $@ $(WMAP_OBJS)

$(OBJDIR)/ovl/OVL06C.BIN: $(TARGET) $(MENU_OBJS) gen/menu.ld gen/menu_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link menu $(TARGET) $(OBJDIR)/ovl $@ $(MENU_OBJS) --links field

$(OBJDIR)/ovl/OVL07C.BIN: $(TARGET) $(SHOP_OBJS) gen/shop.ld gen/shop_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link shop $(TARGET) $(OBJDIR)/ovl $@ $(SHOP_OBJS) --links field

$(OBJDIR)/ovl/OVL09C.BIN: $(TARGET) $(GOLEM_OBJS) gen/golem.ld gen/golem_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link golem $(TARGET) $(OBJDIR)/ovl $@ $(GOLEM_OBJS) --links field

$(OBJDIR)/ovl/OVL08C.BIN: $(TARGET) $(ZUKAN_OBJS) gen/zukan.ld gen/zukan_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link zukan $(TARGET) $(OBJDIR)/ovl $@ $(ZUKAN_OBJS) --links field

$(OBJDIR)/ovl/OVL10C.BIN: $(TARGET) $(GOVER_OBJS) gen/gover.ld gen/gover_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link gover $(TARGET) $(OBJDIR)/ovl $@ $(GOVER_OBJS) --links field

$(OBJDIR)/ovl/OVL11C.BIN: $(TARGET) $(MOVIE_OBJS) gen/movie.ld gen/movie_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link movie $(TARGET) $(OBJDIR)/ovl $@ $(MOVIE_OBJS) --links field

$(OBJDIR)/ovl/OVL18C.BIN: $(TARGET) $(ADDHERO_OBJS) gen/addhero.ld gen/addhero_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link addhero $(TARGET) $(OBJDIR)/ovl $@ $(ADDHERO_OBJS) --links field

$(OBJDIR)/ovl/OVL17C.BIN: $(TARGET) $(NIKI_OBJS) gen/niki.ld gen/niki_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link niki $(TARGET) $(OBJDIR)/ovl $@ $(NIKI_OBJS) --links field

$(OBJDIR)/ovl/OVL16C.BIN: $(TARGET) $(CLOAD_OBJS) gen/cload.ld gen/cload_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link cload $(TARGET) $(OBJDIR)/ovl $@ $(CLOAD_OBJS) --links field

$(OBJDIR)/ovl/OVL12C.BIN: $(TARGET) $(CARDA_OBJS) gen/carda.ld gen/carda_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link carda $(TARGET) $(OBJDIR)/ovl $@ $(CARDA_OBJS) --links field

$(OBJDIR)/ovl/OVL13C.BIN: $(TARGET) $(GOSUB_OBJS) gen/gosub.ld gen/gosub_pending.ld $(OBJDIR)/ovl/OVL02C.BIN
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link gosub $(TARGET) $(OBJDIR)/ovl $@ $(GOSUB_OBJS) --links field

#----------------------
# | $(OBJDIR)/cd-data/.stamp
# | Description: Stages a per-build copy of the shared cd/data tree (the converter's output, via $(PRE_BUILD_STAMP)) with that build's own packed overlay code laid on top.
# |              So packing never mixes one build's data with another build's overlay code.
# | Author: suinevere
# | Dependencies: PRE_BUILD_STAMP, OVERLAY_CODE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/cd-data/.stamp: $(PRE_BUILD_STAMP) $(OVERLAY_CODE)
	rm -rf $(OBJDIR)/cd-data
	cp -a cd/data $(OBJDIR)/cd-data
	cp -f $(OVERLAY_CODE) $(OBJDIR)/cd-data/OVL/
	touch $@

#----------------------
# | $(CDI)
# | Description: Packs the linked ELF and this build's staged cd-data tree into a bootable MIL-CD image, so KallistiOS sees them under /cd (overlays in /cd/OVL).
# |              -N drops mkdcdisc's data-track padding, which only speeds up loading on real GD-ROM drives and costs nothing this build needs.
# |              Flycast boots the unpadded disc; GDEMU behavior on real hardware is unverified.
# | Author: suinevere
# | Dependencies: $(TARGET), mkdcdisc, $(OBJDIR)/cd-data/.stamp
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(CDI): $(TARGET) $(OBJDIR)/cd-data/.stamp
	mkdcdisc -e $(TARGET) -o $(CDI) -D $(OBJDIR)/cd-data -n "Legend of Mana" -N --allow-overwrite

#----------------------
# | clean
# | Description: Removes build output, the linked ELF, the packed overlay code files and both disc images, including the test-image build directory.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
clean:
	rm -rf build build-test
	rm -f lom.elf lom.cdi lom_test.elf lom_test.cdi $(PADSEL_IMAGES) cd/data/OVL/OVL*C.BIN

#----------------------
# | gen/pad_script_table.h
# | Description: Compiles every tests/pad_scripts/*.pad, new_game.pad first, into the pad_script_scripts[] table test-image's pad_dc.o includes under LOM_PAD_SCRIPT.
# | Author: suinevere
# | Dependencies: tools/gen_pad_script.py, PAD_SCRIPTS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PAD_SCRIPTS = tests/pad_scripts/new_game.pad $(filter-out tests/pad_scripts/new_game.pad,$(sort $(wildcard tests/pad_scripts/*.pad)))
gen/pad_script_table.h: $(PAD_SCRIPTS) tools/gen_pad_script.py
	@mkdir -p $(dir $@)
	$(PYTHON) tools/gen_pad_script.py $(PAD_SCRIPTS) $@

#----------------------
# | PADSEL_IMAGES / lom_<script>_test.cdi
# | Description: Extra scripted discs from lom_test.elf and its disc data, each with a root PADSEL.TXT naming the pad script to run: the card save and load discs, the sound sweep and sound effects discs, the movie probe and intro discs, the FIELD movie disc and the PAL 50 Hz boot disc.
# | Author: suinevere
# | Dependencies: test-image, mkdcdisc, src/input/pad_dc.c (PADSEL.TXT)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PADSEL_SCRIPTS = card_save card_load sound_sweep sound_effects movie_probe movie_intro field_movie pal_50hz
PADSEL_IMAGES = $(patsubst %,lom_%_test.cdi,$(PADSEL_SCRIPTS))
lom_%_test.cdi: lom_test.elf build-test/cd-data/.stamp
	@mkdir -p build-test/padsel/$*
	printf '%s\n' $* > build-test/padsel/$*/PADSEL.TXT
	mkdcdisc -e lom_test.elf -o $@ -D build-test/cd-data -f build-test/padsel/$*/PADSEL.TXT -n "Legend of Mana" -N --allow-overwrite

#----------------------
# | test-image
# | Description: Builds dreamcast/lom_test.cdi from a separate build-test/ object tree with -DLOM_PAD_SCRIPT and the compiled new_game.pad table, so the scripted build never touches the shipping lom.cdi's objects.
# | Author: suinevere
# | Dependencies: gen/pad_script_table.h, OBJDIR, TARGET, CDI, EXTRA_CFLAGS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
test-image: gen/pad_script_table.h
	$(MAKE) OBJDIR=build-test TARGET=lom_test.elf CDI=lom_test.cdi EXTRA_CFLAGS=-DLOM_PAD_SCRIPT all
	$(MAKE) $(PADSEL_IMAGES)

#----------------------
# | PENDING_DEFINED
# | Description: What defines symbols for main's link outside the compiled objects: the toolchain and patched KallistiOS libraries and main's linker scripts.
# | Author: suinevere
# | Dependencies: TOOLCHAIN_LIBS, KOS_LIBKALLISTI, KOS_CDROM_PATCH_OBJS, dreamcast.ld, gen/overlay_entries.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PENDING_DEFINED = $(KOS_LIBKALLISTI) $(KOS_CDROM_PATCH_OBJS) $(TOOLCHAIN_LIBS) dreamcast.ld gen/overlay_entries.ld

#----------------------
# | pending
# | Description: Compiles main, CHECKPS, TITLE, FIELD, GNAME, WSEL, WMAP and the secondary overlays, then regenerates gen/dc_pending.c (halting stubs), gen/dc_pending.ld (data aliases) and each overlay's gen/<overlay>_pending.ld (its unnamed own data).
# |              Each overlay links against main alone, so main's and every overlay's references count only what that link defines, except every secondary overlay's group also resolves against FIELD's own objects (--group-links <name>:field), matching their real --links field; overlays share PS1 addresses.
# | Author: suinevere
# | Dependencies: ../port/tools/gen_pending.py, sh-elf-nm, ../config/us/symbols/*.txt, PENDING_DEFINED, OVERLAY_SYMBOLS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
pending: $(PRE_BUILD_STAMP) $(MAIN_OBJS) $(OVERLAY_OBJS) $(KOS_CDROM_PATCH_OBJS)
	$(PYTHON) ../port/tools/gen_pending.py --nm sh-elf-nm --overlay dreamcast \
		--objects $(MAIN_OBJS) \
		$(foreach o,$(OVERLAY_NAMES),--overlay-group $(OVERLAY_GROUP_$(o)) gen/$(o)_symbols.ld) \
		--group-links gname:field menu:field shop:field golem:field zukan:field gosub:field carda:field cload:field niki:field addhero:field gover:field movie:field \
		--symbols $(wildcard ../config/us/symbols/*.txt) \
		--defined $(PENDING_DEFINED) --alias-defined gen/overlay_entries.ld \
		--out-c gen/dc_pending.c --out-ld gen/dc_pending.ld --group-out-dir gen

#----------------------
# | gen/dc_pending.c / gen/dc_pending.ld / gen/<overlay>_pending.ld
# | Description: Generated by the pending recipe when missing, since dreamcast/gen is not committed; make pending refreshes them.
# | Author: suinevere
# | Dependencies: pending
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
gen/dc_pending.c: | $(PRE_BUILD_STAMP)
	$(MAKE) pending

gen/dc_pending.ld $(foreach o,$(OVERLAY_NAMES),gen/$(o)_pending.ld): gen/dc_pending.c ;
