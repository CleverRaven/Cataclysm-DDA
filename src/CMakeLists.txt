# Cataclysm DDA client
cmake_minimum_required(VERSION 2.8.12)

SET(CATACLYSM_DDA_SRCS "")

SET(CATACLYSM_DDA_SOURCES
    ${CMAKE_SOURCE_DIR}/src/veh_interact.cpp
    ${CMAKE_SOURCE_DIR}/src/mondeath.cpp
    ${CMAKE_SOURCE_DIR}/src/path_info.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software_kitten.cpp
    ${CMAKE_SOURCE_DIR}/src/advanced_inv.cpp
    ${CMAKE_SOURCE_DIR}/src/player_activity.cpp
    ${CMAKE_SOURCE_DIR}/src/color.cpp
    ${CMAKE_SOURCE_DIR}/src/basecamp.cpp
    ${CMAKE_SOURCE_DIR}/src/tileray.cpp
    ${CMAKE_SOURCE_DIR}/src/npcmove.cpp
    ${CMAKE_SOURCE_DIR}/src/itype.cpp
    ${CMAKE_SOURCE_DIR}/src/options.cpp
    ${CMAKE_SOURCE_DIR}/src/field.cpp
    ${CMAKE_SOURCE_DIR}/src/main_menu.cpp
    ${CMAKE_SOURCE_DIR}/src/savegame_legacy.cpp
    ${CMAKE_SOURCE_DIR}/src/mutation_data.cpp
    ${CMAKE_SOURCE_DIR}/src/posix_time.cpp
    ${CMAKE_SOURCE_DIR}/src/tutorial.cpp
    ${CMAKE_SOURCE_DIR}/src/catacharset.cpp
    ${CMAKE_SOURCE_DIR}/src/item_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/activity_handlers.cpp
    ${CMAKE_SOURCE_DIR}/src/mongroup.cpp
    ${CMAKE_SOURCE_DIR}/src/mapgen_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/savegame_json.cpp
    ${CMAKE_SOURCE_DIR}/src/item_action.cpp
    ${CMAKE_SOURCE_DIR}/src/item_location.cpp
    ${CMAKE_SOURCE_DIR}/src/inventory.cpp
    ${CMAKE_SOURCE_DIR}/src/npctalk.cpp
    ${CMAKE_SOURCE_DIR}/src/calendar.cpp
    ${CMAKE_SOURCE_DIR}/src/rng.cpp
    ${CMAKE_SOURCE_DIR}/src/creature_tracker.cpp
    ${CMAKE_SOURCE_DIR}/src/mapgenformat.cpp
    ${CMAKE_SOURCE_DIR}/src/output.cpp
    ${CMAKE_SOURCE_DIR}/src/construction.cpp
    ${CMAKE_SOURCE_DIR}/src/overmapbuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp
    ${CMAKE_SOURCE_DIR}/src/item_group.cpp
    ${CMAKE_SOURCE_DIR}/src/monmove.cpp
    ${CMAKE_SOURCE_DIR}/src/debug.cpp
    ${CMAKE_SOURCE_DIR}/src/monstergenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/monfaction.cpp
    ${CMAKE_SOURCE_DIR}/src/catalua.cpp
    ${CMAKE_SOURCE_DIR}/src/map.cpp
    ${CMAKE_SOURCE_DIR}/src/action.cpp
    ${CMAKE_SOURCE_DIR}/src/nonplayer.cpp
    ${CMAKE_SOURCE_DIR}/src/start_location.cpp
    ${CMAKE_SOURCE_DIR}/src/mutation.cpp
    ${CMAKE_SOURCE_DIR}/src/mapgen.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/damage.cpp
    ${CMAKE_SOURCE_DIR}/src/activity_item_handling.cpp
    ${CMAKE_SOURCE_DIR}/src/ranged.cpp
    ${CMAKE_SOURCE_DIR}/src/skill.cpp
    ${CMAKE_SOURCE_DIR}/src/version.cpp
    ${CMAKE_SOURCE_DIR}/src/armor_layers.cpp
    ${CMAKE_SOURCE_DIR}/src/simplexnoise.cpp
    ${CMAKE_SOURCE_DIR}/src/translations.cpp
    ${CMAKE_SOURCE_DIR}/src/mission_start.cpp
    ${CMAKE_SOURCE_DIR}/src/mondefense.cpp
    ${CMAKE_SOURCE_DIR}/src/weather_gen.cpp
    ${CMAKE_SOURCE_DIR}/src/animation.cpp
    ${CMAKE_SOURCE_DIR}/src/mission.cpp
    ${CMAKE_SOURCE_DIR}/src/trapfunc.cpp
    ${CMAKE_SOURCE_DIR}/src/event.cpp
    ${CMAKE_SOURCE_DIR}/src/dependency_tree.cpp
    ${CMAKE_SOURCE_DIR}/src/monster.cpp
    ${CMAKE_SOURCE_DIR}/src/editmap.cpp
    ${CMAKE_SOURCE_DIR}/src/mapsharing.cpp
    ${CMAKE_SOURCE_DIR}/src/requirements.cpp
    ${CMAKE_SOURCE_DIR}/src/cursesport.cpp
    ${CMAKE_SOURCE_DIR}/src/sdltiles.cpp
    ${CMAKE_SOURCE_DIR}/src/artifact.cpp
    ${CMAKE_SOURCE_DIR}/src/mtype.cpp
    ${CMAKE_SOURCE_DIR}/src/input.cpp
    ${CMAKE_SOURCE_DIR}/src/speech.cpp
    ${CMAKE_SOURCE_DIR}/src/mod_manager_ui.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software_snake.cpp
    ${CMAKE_SOURCE_DIR}/src/savegame.cpp
    ${CMAKE_SOURCE_DIR}/src/lightmap.cpp
    ${CMAKE_SOURCE_DIR}/src/mapdata.cpp
    ${CMAKE_SOURCE_DIR}/src/submap.cpp
    ${CMAKE_SOURCE_DIR}/src/weather_data.cpp
    ${CMAKE_SOURCE_DIR}/src/martialarts.cpp
    ${CMAKE_SOURCE_DIR}/src/mod_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/vehicle.cpp
    ${CMAKE_SOURCE_DIR}/src/vehicle_group.cpp
    ${CMAKE_SOURCE_DIR}/src/mission_place.cpp
    ${CMAKE_SOURCE_DIR}/src/material.cpp
    ${CMAKE_SOURCE_DIR}/src/veh_type.cpp
    ${CMAKE_SOURCE_DIR}/src/melee.cpp
    ${CMAKE_SOURCE_DIR}/src/defense.cpp
    ${CMAKE_SOURCE_DIR}/src/npc.cpp
    ${CMAKE_SOURCE_DIR}/src/text_snippets.cpp
    ${CMAKE_SOURCE_DIR}/src/pickup.cpp
    ${CMAKE_SOURCE_DIR}/src/mapbuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/item.cpp
    ${CMAKE_SOURCE_DIR}/src/weather.cpp
    ${CMAKE_SOURCE_DIR}/src/mission_fail.cpp
    ${CMAKE_SOURCE_DIR}/src/ui.cpp
    ${CMAKE_SOURCE_DIR}/src/newcharacter.cpp
    ${CMAKE_SOURCE_DIR}/src/scenario.cpp
    ${CMAKE_SOURCE_DIR}/src/faction.cpp
    ${CMAKE_SOURCE_DIR}/src/wish.cpp
    ${CMAKE_SOURCE_DIR}/src/bionics.cpp
    ${CMAKE_SOURCE_DIR}/src/missiondef.cpp
    ${CMAKE_SOURCE_DIR}/src/wincurse.cpp
    ${CMAKE_SOURCE_DIR}/src/worldfactory.cpp
    ${CMAKE_SOURCE_DIR}/src/active_item_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/addiction.cpp
    ${CMAKE_SOURCE_DIR}/src/computer.cpp
    ${CMAKE_SOURCE_DIR}/src/trap.cpp
    ${CMAKE_SOURCE_DIR}/src/game.cpp
    ${CMAKE_SOURCE_DIR}/src/monattack.cpp
    ${CMAKE_SOURCE_DIR}/src/effect.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software_sokoban.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software.cpp
    ${CMAKE_SOURCE_DIR}/src/auto_pickup.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_actor.cpp
    ${CMAKE_SOURCE_DIR}/src/cata_tiles.cpp
	${CMAKE_SOURCE_DIR}/src/cata_utility.cpp
    ${CMAKE_SOURCE_DIR}/src/profession.cpp
    ${CMAKE_SOURCE_DIR}/src/init.cpp
    ${CMAKE_SOURCE_DIR}/src/sounds.cpp
    ${CMAKE_SOURCE_DIR}/src/filesystem.cpp
    ${CMAKE_SOURCE_DIR}/src/messages.cpp
    ${CMAKE_SOURCE_DIR}/src/clzones.cpp
    ${CMAKE_SOURCE_DIR}/src/character.cpp
    ${CMAKE_SOURCE_DIR}/src/live_view.cpp
    ${CMAKE_SOURCE_DIR}/src/iexamine.cpp
    ${CMAKE_SOURCE_DIR}/src/gamemode.cpp
    ${CMAKE_SOURCE_DIR}/src/creature.cpp
    ${CMAKE_SOURCE_DIR}/src/inventory_ui.cpp
    ${CMAKE_SOURCE_DIR}/src/char_validity_check.cpp
    ${CMAKE_SOURCE_DIR}/src/bodypart.cpp
    ${CMAKE_SOURCE_DIR}/src/line.cpp
    ${CMAKE_SOURCE_DIR}/src/overmap.cpp
    ${CMAKE_SOURCE_DIR}/src/ammo.cpp
    ${CMAKE_SOURCE_DIR}/src/mission_end.cpp
    ${CMAKE_SOURCE_DIR}/src/help.cpp
    ${CMAKE_SOURCE_DIR}/src/name.cpp
    ${CMAKE_SOURCE_DIR}/src/player.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse.cpp
    ${CMAKE_SOURCE_DIR}/src/crafting.cpp
    ${CMAKE_SOURCE_DIR}/src/pathfinding.cpp
    ${CMAKE_SOURCE_DIR}/src/mission_companion.cpp
    ${CMAKE_SOURCE_DIR}/src/recipe_dictionary.cpp
)

SET (CATACLYSM_DDA_HEADERS
    ${CMAKE_SOURCE_DIR}/src/weather.h
    ${CMAKE_SOURCE_DIR}/src/iuse_software.h
    ${CMAKE_SOURCE_DIR}/src/catacurse.h
    ${CMAKE_SOURCE_DIR}/src/ui.h
    ${CMAKE_SOURCE_DIR}/src/damage.h
    ${CMAKE_SOURCE_DIR}/src/cursesdef.h
    ${CMAKE_SOURCE_DIR}/src/color.h
    ${CMAKE_SOURCE_DIR}/src/debug.h
    ${CMAKE_SOURCE_DIR}/src/activity_handlers.h
    ${CMAKE_SOURCE_DIR}/src/bodypart.h
    ${CMAKE_SOURCE_DIR}/src/mapgen_functions.h
    ${CMAKE_SOURCE_DIR}/src/basecamp.h
    ${CMAKE_SOURCE_DIR}/src/overmapbuffer.h
    ${CMAKE_SOURCE_DIR}/src/monster.h
    ${CMAKE_SOURCE_DIR}/src/tileray.h
    ${CMAKE_SOURCE_DIR}/src/messages.h
    ${CMAKE_SOURCE_DIR}/src/veh_type.h
    ${CMAKE_SOURCE_DIR}/src/mapgenformat.h
    ${CMAKE_SOURCE_DIR}/src/mapbuffer.h
    ${CMAKE_SOURCE_DIR}/src/posix_time.h
    ${CMAKE_SOURCE_DIR}/src/item_action.h
    ${CMAKE_SOURCE_DIR}/src/item_location.h
    ${CMAKE_SOURCE_DIR}/src/map.h
    ${CMAKE_SOURCE_DIR}/src/npc.h
    ${CMAKE_SOURCE_DIR}/src/rng.h
    ${CMAKE_SOURCE_DIR}/src/compatibility.h
    ${CMAKE_SOURCE_DIR}/src/faction.h
    ${CMAKE_SOURCE_DIR}/src/speech.h
    ${CMAKE_SOURCE_DIR}/src/iuse_software_kitten.h
    ${CMAKE_SOURCE_DIR}/src/item_group.h
    ${CMAKE_SOURCE_DIR}/src/overmap.h
    ${CMAKE_SOURCE_DIR}/src/mongroup.h
    ${CMAKE_SOURCE_DIR}/src/calendar.h
    ${CMAKE_SOURCE_DIR}/src/mod_manager.h
    ${CMAKE_SOURCE_DIR}/src/catacharset.h
    ${CMAKE_SOURCE_DIR}/src/nonplayer.h
    ${CMAKE_SOURCE_DIR}/src/omdata.h
    ${CMAKE_SOURCE_DIR}/src/game_constants.h
    ${CMAKE_SOURCE_DIR}/src/mission.h
    ${CMAKE_SOURCE_DIR}/src/requirements.h
    ${CMAKE_SOURCE_DIR}/src/sounds.h
    ${CMAKE_SOURCE_DIR}/src/worldfactory.h
    ${CMAKE_SOURCE_DIR}/src/editmap.h
    ${CMAKE_SOURCE_DIR}/src/effect.h
    ${CMAKE_SOURCE_DIR}/src/creature.h
    ${CMAKE_SOURCE_DIR}/src/vehicle.h
    ${CMAKE_SOURCE_DIR}/src/vehicle_group.h
    ${CMAKE_SOURCE_DIR}/src/clzones.h
    ${CMAKE_SOURCE_DIR}/src/translations.h
    ${CMAKE_SOURCE_DIR}/src/skill.h
    ${CMAKE_SOURCE_DIR}/src/iuse_software_snake.h
    ${CMAKE_SOURCE_DIR}/src/bionics.h
    ${CMAKE_SOURCE_DIR}/src/pickup.h
    ${CMAKE_SOURCE_DIR}/src/enums.h
    ${CMAKE_SOURCE_DIR}/src/tutorial.h
    ${CMAKE_SOURCE_DIR}/src/simplexnoise.h
    ${CMAKE_SOURCE_DIR}/src/scenario.h
    ${CMAKE_SOURCE_DIR}/src/auto_pickup.h
    ${CMAKE_SOURCE_DIR}/src/cata_tiles.h
	${CMAKE_SOURCE_DIR}/src/cata_utility.h
    ${CMAKE_SOURCE_DIR}/src/char_validity_check.h
    ${CMAKE_SOURCE_DIR}/src/coordinates.h
    ${CMAKE_SOURCE_DIR}/src/field.h
    ${CMAKE_SOURCE_DIR}/src/monstergenerator.h
    ${CMAKE_SOURCE_DIR}/src/monfaction.h
    ${CMAKE_SOURCE_DIR}/src/event.h
    ${CMAKE_SOURCE_DIR}/src/ammo.h
    ${CMAKE_SOURCE_DIR}/src/mapgen.h
    ${CMAKE_SOURCE_DIR}/src/dialogue.h
    ${CMAKE_SOURCE_DIR}/src/computer.h
    ${CMAKE_SOURCE_DIR}/src/veh_interact.h
    ${CMAKE_SOURCE_DIR}/src/morale.h
    ${CMAKE_SOURCE_DIR}/src/game.h
    ${CMAKE_SOURCE_DIR}/src/help.h
    ${CMAKE_SOURCE_DIR}/src/lightmap.h
    ${CMAKE_SOURCE_DIR}/src/mutation.h
    ${CMAKE_SOURCE_DIR}/src/init.h
    ${CMAKE_SOURCE_DIR}/src/wdirent.h
    ${CMAKE_SOURCE_DIR}/src/item.h
    ${CMAKE_SOURCE_DIR}/src/iuse.h
    ${CMAKE_SOURCE_DIR}/src/creature_tracker.h
    ${CMAKE_SOURCE_DIR}/src/json.h
    ${CMAKE_SOURCE_DIR}/src/mondeath.h
    ${CMAKE_SOURCE_DIR}/src/mondefense.h
    ${CMAKE_SOURCE_DIR}/src/line.h
    ${CMAKE_SOURCE_DIR}/src/mtype.h
    ${CMAKE_SOURCE_DIR}/src/live_view.h
    ${CMAKE_SOURCE_DIR}/src/character.h
    ${CMAKE_SOURCE_DIR}/src/name.h
    ${CMAKE_SOURCE_DIR}/src/tile_id_data.h
    ${CMAKE_SOURCE_DIR}/src/crafting.h
    ${CMAKE_SOURCE_DIR}/src/output.h
    ${CMAKE_SOURCE_DIR}/src/material.h
    ${CMAKE_SOURCE_DIR}/src/action.h
    ${CMAKE_SOURCE_DIR}/src/dependency_tree.h
    ${CMAKE_SOURCE_DIR}/src/martialarts.h
    ${CMAKE_SOURCE_DIR}/src/iexamine.h
    ${CMAKE_SOURCE_DIR}/src/iuse_software_sokoban.h
    ${CMAKE_SOURCE_DIR}/src/iuse_software_minesweeper.h
    ${CMAKE_SOURCE_DIR}/src/options.h
    ${CMAKE_SOURCE_DIR}/src/get_version.h
    ${CMAKE_SOURCE_DIR}/src/mapsharing.h
    ${CMAKE_SOURCE_DIR}/src/catalua.h
    ${CMAKE_SOURCE_DIR}/src/trap.h
    ${CMAKE_SOURCE_DIR}/src/item_factory.h
    ${CMAKE_SOURCE_DIR}/src/weather_gen.h
    ${CMAKE_SOURCE_DIR}/src/platform_win.h
    ${CMAKE_SOURCE_DIR}/src/active_item_cache.h
    ${CMAKE_SOURCE_DIR}/src/construction.h
    ${CMAKE_SOURCE_DIR}/src/player.h
    ${CMAKE_SOURCE_DIR}/src/advanced_inv.h
    ${CMAKE_SOURCE_DIR}/src/pldata.h
    ${CMAKE_SOURCE_DIR}/src/start_location.h
    ${CMAKE_SOURCE_DIR}/src/inventory.h
    ${CMAKE_SOURCE_DIR}/src/gamemode.h
    ${CMAKE_SOURCE_DIR}/src/artifact.h
    ${CMAKE_SOURCE_DIR}/src/mapdata.h
    ${CMAKE_SOURCE_DIR}/src/submap.h
    ${CMAKE_SOURCE_DIR}/src/text_snippets.h
    ${CMAKE_SOURCE_DIR}/src/monattack.h
    ${CMAKE_SOURCE_DIR}/src/filesystem.h
    ${CMAKE_SOURCE_DIR}/src/player_activity.h
    ${CMAKE_SOURCE_DIR}/src/addiction.h
    ${CMAKE_SOURCE_DIR}/src/iuse_actor.h
    ${CMAKE_SOURCE_DIR}/src/wcwidth.h
    ${CMAKE_SOURCE_DIR}/src/uistate.h
    ${CMAKE_SOURCE_DIR}/src/profession.h
    ${CMAKE_SOURCE_DIR}/src/path_info.h
    ${CMAKE_SOURCE_DIR}/src/version.h
    ${CMAKE_SOURCE_DIR}/src/input.h
    ${CMAKE_SOURCE_DIR}/src/item_stack.h
    ${CMAKE_SOURCE_DIR}/src/itype.h
    ${CMAKE_SOURCE_DIR}/src/recipe_dictionary.h
)

# Get GIT version strings
ADD_CUSTOM_TARGET(
	get_version
	DEPENDS ${CMAKE_SOURCE_DIR}/src/version.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

ADD_CUSTOM_COMMAND (
	OUTPUT ${CMAKE_SOURCE_DIR}/src/version.h
	COMMAND ${CMAKE_COMMAND}
		-D SRC=${CMAKE_SOURCE_DIR}/src/version.h.in
		-D DST=${CMAKE_SOURCE_DIR}/src/version.h
		-D GIT_EXECUTABLE=${GIT_EXECUTABLE}
		-P ${CMAKE_SOURCE_DIR}/src/version.cmake
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

IF(WIN32)
	IF(RC_COMPILER_PATH)
		ADD_CUSTOM_TARGET(
			compile_res
			DEPENDS ${CMAKE_BINARY_DIR}/resource.rc.o
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_BINARY_DIR}/resource.rc.o
			COMMAND	${RC_COMPILER_PATH}
				${CMAKE_SOURCE_DIR}/src/resource.rc
				${CMAKE_BINARY_DIR}/resource.rc.o
			WORKING_DIRECTORY
				${CMAKE_SOURCE_DIR}/src
		)
		SET(CATACLYSM_DDA_SRCS
			${CMAKE_SOURCE_DIR}/src/main.cpp
			${CMAKE_BINARY_DIR}/resource.rc.o
		)
	ENDIF(RC_COMPILER_PATH)
ENDIF(WIN32)

# Build tiles version if requested
IF(TILES)
	IF(WIN32)
		ADD_EXECUTABLE(cataclysm-tiles WIN32
			${CATACLYSM_DDA_SOURCES}
			${CATACLYSM_DDA_HEADERS}
			${CATACLYSM_DDA_SRCS}
		)
	ELSE(WIN32)
		ADD_EXECUTABLE(cataclysm-tiles
			${CATACLYSM_DDA_SOURCES}
			${CATACLYSM_DDA_HEADERS}
		)
	ENDIF(WIN32)

	IF(RC_COMPILER_PATH)
		ADD_DEPENDENCIES(cataclysm-tiles compile_res)
	ENDIF(RC_COMPILER_PATH)

	ADD_DEPENDENCIES(cataclysm-tiles get_version)

	IF (LUA)
		ADD_DEPENDENCIES(cataclysm-tiles lua_bindings)
		target_include_directories(cataclysm-tiles PUBLIC ${LUA_INCLUDE_DIR})
		target_link_libraries(cataclysm-tiles ${LUA_LIBRARIES})
	ENDIF(LUA)

	IF (LOCALIZE)
		target_include_directories(cataclysm-tiles PUBLIC
			${LIBINTL_INCLUDE_DIR}
			${ICONV_INCLUDE_DIR}
		)
		target_link_libraries(cataclysm-tiles
			${LIBINTL_LIBRARIES}
			${ICONV_LIBRARIES}
		)
	ENDIF (LOCALIZE)

	IF(CMAKE_USE_PTHREADS_INIT)
		set_property(TARGET cataclysm-tiles PROPERTY COMPILE_OPTIONS "-pthread")
		set_property(TARGET cataclysm-tiles PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
	ENDIF(CMAKE_USE_PTHREADS_INIT)

	IF(CMAKE_THREAD_LIBS_INIT)
		target_link_libraries(cataclysm-tiles ${CMAKE_THREAD_LIBS_INIT})
	ENDIF(CMAKE_THREAD_LIBS_INIT)

	IF (NOT DYNAMIC_LINKING)
		# SDL, SDL_Image, SDL_ttf deps are required for static build
		target_include_directories(cataclysm-tiles PUBLIC
			${FREETYPE_INCLUDE_DIRS}
			${PNG_INCLUDE_DIRS}
			${JPEG_INCLUDE_DIR}
			${ZLIB_INCLUDE_DIRS}
			${BZIP2_INCLUDE_DIR}
		)
		target_link_libraries(cataclysm-tiles
			${FREETYPE_LIBRARIES}
			${PNG_LIBRARIES}
			${JPEG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${BZIP2_LIBRARIES}
		)
	ENDIF (NOT DYNAMIC_LINKING)

	target_include_directories(cataclysm-tiles PUBLIC
		${SDL2_INCLUDE_DIR}
		${SDL2_IMAGE_INCLUDE_DIRS}
		${SDL2_TTF_INCLUDE_DIRS}
	)
	target_link_libraries(cataclysm-tiles
		${SDL2_LIBRARY}
		${SDL2_IMAGE_LIBRARIES}
		${SDL2_TTF_LIBRARIES}
	)

	IF(SOUND)
		set_target_properties(cataclysm-tiles PROPERTIES COMPILE_DEFINITIONS
			"TILES;SDL_SOUND" )

		target_include_directories(cataclysm-tiles PUBLIC ${OGGVORBIS_INCLUDE_DIR})
		target_link_libraries(cataclysm-tiles ${OGG_LIBRARY})
		target_link_libraries(cataclysm-tiles ${VORBIS_LIBRARY})
		target_link_libraries(cataclysm-tiles ${VORBISFILE_LIBRARY})

		target_include_directories(cataclysm-tiles PUBLIC ${SDL2_MIXER_INCLUDE_DIRS})
		target_link_libraries(cataclysm-tiles ${SDL2_MIXER_LIBRARIES})
	ELSE(SOUND)
		# Set additional build props
		set_target_properties(cataclysm-tiles PROPERTIES COMPILE_DEFINITIONS
			"TILES" )
	ENDIF(SOUND)

	IF(WIN32)
		# Global settings for Windows targets (at end)
		target_link_libraries(cataclysm-tiles gdi32.lib)
		target_link_libraries(cataclysm-tiles winmm.lib)
		target_link_libraries(cataclysm-tiles imm32.lib)
		target_link_libraries(cataclysm-tiles ole32.lib)
		target_link_libraries(cataclysm-tiles oleaut32.lib)
		target_link_libraries(cataclysm-tiles version.lib)
	ENDIF(WIN32)

	IF(RELEASE)
		install(TARGETS cataclysm-tiles DESTINATION ${BIN_PREFIX})
	ENDIF(RELEASE)
ENDIF(TILES)

# Build curses version if requested
IF(CURSES)
	IF(WIN32)
		ADD_EXECUTABLE(cataclysm
			${CATACLYSM_DDA_SOURCES}
			${CATACLYSM_DDA_HEADERS}
			${CATACLYSM_DDA_SRCS}
		)
	ELSE(WIN32)
		ADD_EXECUTABLE(cataclysm
			${CATACLYSM_DDA_SOURCES}
			${CATACLYSM_DDA_HEADERS}
		)
	ENDIF(WIN32)

	IF(RC_COMPILER_PATH)
		ADD_DEPENDENCIES(cataclysm compile_res)
	ENDIF(RC_COMPILER_PATH)

	ADD_DEPENDENCIES(cataclysm get_version)

	IF (LUA)
		ADD_DEPENDENCIES(cataclysm lua_bindings)
		target_include_directories(cataclysm PUBLIC ${LUA_INCLUDE_DIR})
		target_link_libraries(cataclysm ${LUA_LIBRARIES})
	ENDIF(LUA)

	IF (LOCALIZE)
		target_include_directories(cataclysm PUBLIC
			${LIBINTL_INCLUDE_DIR}
			${ICONV_INCLUDE_DIR}
		)
		target_link_libraries(cataclysm
			${LIBINTL_LIBRARIES}
			${ICONV_LIBRARIES}
		)
	ENDIF (LOCALIZE)

	target_include_directories(cataclysm PUBLIC ${CURSES_INCLUDE_DIR})
	target_link_libraries(cataclysm ${CURSES_LIBRARIES})

	IF(CMAKE_USE_PTHREADS_INIT)
		set_property(TARGET cataclysm PROPERTY COMPILE_OPTIONS "-pthread")
		set_property(TARGET cataclysm PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
	ENDIF(CMAKE_USE_PTHREADS_INIT)

	IF(CMAKE_THREAD_LIBS_INIT)
		target_link_libraries(cataclysm ${CMAKE_THREAD_LIBS_INIT})
	ENDIF(CMAKE_THREAD_LIBS_INIT)

	IF(WIN32)
		# Global settings for Windows targets (at end)
		target_link_libraries(cataclysm gdi32.lib)
		target_link_libraries(cataclysm winmm.lib)
		target_link_libraries(cataclysm imm32.lib)
		target_link_libraries(cataclysm ole32.lib)
		target_link_libraries(cataclysm oleaut32.lib)
		target_link_libraries(cataclysm version.lib)
	ENDIF(WIN32)

	IF(RELEASE)
		install(TARGETS cataclysm DESTINATION ${BIN_PREFIX})
	ENDIF(RELEASE)


ENDIF(CURSES)

IF(MINGW AND NOT RELEASE)
	# Try to Install shared libraries for dev builds
	#
	# Note: It is specific to MSYS2 and uses hardcoded versions so
	# probly it will fail if you run it :)

	# GCC-specific libraries
	find_library(RuntimeLib_GCC_S_DW2_1  "gcc_s_dw2-1")
	find_library(RuntimeLib_STDC_PP_6    "stdc++-6")
	find_library(RuntimeLib_WINPTHREAD_1 "winpthread-1")
	SET(RuntimeLib_GCC_ALL
		${RuntimeLib_GCC_S_DW2_1}
		${RuntimeLib_STDC_PP_6}
		${RuntimeLib_WINPTHREAD_1}
	)
	IF (LUA)
		SET(RuntimeLib_LUA
			${LUA_LIBRARIES}
		)
	ENDIF (LUA)
	IF (LOCALIZE)
		find_library(RuntimeLib_iconv  "libiconv-2")
		find_library(RuntimeLib_intl  "libintl-8")
		SET(RuntimeLib_LOCALIZE
			${RuntimeLib_iconv}
			${RuntimeLib_intl}
		)
	ENDIF (LOCALIZE)
	IF (TILES)
		# SDL2 can have a varoius deps. Here you are the MSYS2 ones...
		find_library(RuntimeLib_SDL2  "SDL2")
		find_library(RuntimeLib_SDL2_IMG  "SDL2_image")
		find_library(RuntimeLib_png  "libpng16-16")
		find_library(RuntimeLib_jpeg  "libjpeg-8")
		find_library(RuntimeLib_jbig  "libjbig-0")
		find_library(RuntimeLib_tiff  "libtiff-5")
		find_library(RuntimeLib_webp  "libwebp-5")
		find_library(RuntimeLib_lzma  "liblzma-5")
		find_library(RuntimeLib_bz2  "libbz2-1")
		find_library(RuntimeLib_zlib  "zlib1")
		find_library(RuntimeLib_hb  "libharfbuzz-0")
		find_library(RuntimeLib_SDL2_TTF  "SDL2_ttf")
		find_library(RuntimeLib_ft  "libfreetype-6")
		find_library(RuntimeLib_glib  "libglib-2.0-0")
		SET(RuntimeLib_SDL
			${RuntimeLib_SDL2}
			${RuntimeLib_SDL2_IMG}
			${RuntimeLib_png}
			${RuntimeLib_jpeg}
			${RuntimeLib_jbig}
			${RuntimeLib_tiff}
			${RuntimeLib_webp}
			${RuntimeLib_lzma}
			${RuntimeLib_bz2}
			${RuntimeLib_zlib}
			${RuntimeLib_hb}
			${RuntimeLib_SDL2_TTF}
			${RuntimeLib_ft}
			${RuntimeLib_glib}
		)
		IF(SOUND)
			find_library(RuntimeLib_SDL_SND  "SDL2_mixer")
			find_library(RuntimeLib_flak  "libFLAC-8")
			find_library(RuntimeLib_ogg  "libogg-0")
			find_library(RuntimeLib_flu  "libfluidsynth-1")
			find_library(RuntimeLib_port  "libportaudio-2")
			find_library(RuntimeLib_snd  "libsndfile-1")
			find_library(RuntimeLib_vorb  "libvorbis-0")
			find_library(RuntimeLib_vorb_enc  "libvorbisenc-2")
			find_library(RuntimeLib_vorb_f  "libvorbisfile-3")
			find_library(RuntimeLib_mod  "libmodplug-1")
			find_library(RuntimeLib_mpeg  "smpeg2")
		SET(RuntimeLib_SDL_SOUND
			${RuntimeLib_SDL_SND}
			${RuntimeLib_flak}
			${RuntimeLib_ogg}
			${RuntimeLib_flu}
			${RuntimeLib_port}
			${RuntimeLib_snd}
			${RuntimeLib_vorb}
			${RuntimeLib_vorb_enc}
			${RuntimeLib_vorb_f}
			${RuntimeLib_mod}
			${RuntimeLib_mpeg}
		)
		ENDIF(SOUND)
	ENDIF (TILES)
	install(FILES ${RuntimeLib_GCC_ALL}
			${RuntimeLib_LOCALIZE}
			${RuntimeLib_LUA}
			${RuntimeLib_SDL}
			${RuntimeLib_SDL_SOUND}
		DESTINATION ${BIN_PREFIX}
	)
ENDIF(MINGW AND NOT RELEASE)
