# Cataclysm DDA client
cmake_minimum_required(VERSION 2.8.12)

set(CATACLYSM_DDA_SOURCES
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
    ${CMAKE_SOURCE_DIR}/src/itypedef.cpp
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
    ${CMAKE_SOURCE_DIR}/src/mongroupdef.cpp
    ${CMAKE_SOURCE_DIR}/src/mapgen_functions.cpp
    ${CMAKE_SOURCE_DIR}/src/savegame_json.cpp
    ${CMAKE_SOURCE_DIR}/src/item_action.cpp
    ${CMAKE_SOURCE_DIR}/src/inventory.cpp
    ${CMAKE_SOURCE_DIR}/src/npctalk.cpp
    ${CMAKE_SOURCE_DIR}/src/calendar.cpp
    ${CMAKE_SOURCE_DIR}/src/rng.cpp
    ${CMAKE_SOURCE_DIR}/src/creature_tracker.cpp
    ${CMAKE_SOURCE_DIR}/src/mapgenformat.cpp
    ${CMAKE_SOURCE_DIR}/src/disease.cpp
    ${CMAKE_SOURCE_DIR}/src/output.cpp
    ${CMAKE_SOURCE_DIR}/src/construction.cpp
    ${CMAKE_SOURCE_DIR}/src/overmapbuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp
    ${CMAKE_SOURCE_DIR}/src/item_group.cpp
    ${CMAKE_SOURCE_DIR}/src/monmove.cpp
    ${CMAKE_SOURCE_DIR}/src/debug.cpp
    ${CMAKE_SOURCE_DIR}/src/monstergenerator.cpp
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
    ${CMAKE_SOURCE_DIR}/src/mtypedef.cpp
    ${CMAKE_SOURCE_DIR}/src/input.cpp
    ${CMAKE_SOURCE_DIR}/src/speech.cpp
    ${CMAKE_SOURCE_DIR}/src/mod_manager_ui.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software_snake.cpp
    ${CMAKE_SOURCE_DIR}/src/savegame.cpp
    ${CMAKE_SOURCE_DIR}/src/lightmap.cpp
    ${CMAKE_SOURCE_DIR}/src/mapdata.cpp
    ${CMAKE_SOURCE_DIR}/src/weather_data.cpp
    ${CMAKE_SOURCE_DIR}/src/martialarts.cpp
    ${CMAKE_SOURCE_DIR}/src/mod_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/vehicle.cpp
    ${CMAKE_SOURCE_DIR}/src/mission_place.cpp
    ${CMAKE_SOURCE_DIR}/src/material.cpp
    ${CMAKE_SOURCE_DIR}/src/veh_typedef.cpp
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
    ${CMAKE_SOURCE_DIR}/src/trapdef.cpp
    ${CMAKE_SOURCE_DIR}/src/game.cpp
    ${CMAKE_SOURCE_DIR}/src/monattack.cpp
    ${CMAKE_SOURCE_DIR}/src/effect.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software_sokoban.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_software.cpp
    ${CMAKE_SOURCE_DIR}/src/auto_pickup.cpp
    ${CMAKE_SOURCE_DIR}/src/iuse_actor.cpp
    ${CMAKE_SOURCE_DIR}/src/cata_tiles.cpp
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
    ${CMAKE_SOURCE_DIR}/src/savegame.h
    ${CMAKE_SOURCE_DIR}/src/sounds.h
    ${CMAKE_SOURCE_DIR}/src/worldfactory.h
    ${CMAKE_SOURCE_DIR}/src/editmap.h
    ${CMAKE_SOURCE_DIR}/src/effect.h
    ${CMAKE_SOURCE_DIR}/src/creature.h
    ${CMAKE_SOURCE_DIR}/src/vehicle.h
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
    ${CMAKE_SOURCE_DIR}/src/char_validity_check.h
    ${CMAKE_SOURCE_DIR}/src/coordinates.h
    ${CMAKE_SOURCE_DIR}/src/field.h
    ${CMAKE_SOURCE_DIR}/src/monstergenerator.h
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
    ${CMAKE_SOURCE_DIR}/src/disease.h
    ${CMAKE_SOURCE_DIR}/src/item_stack.h
    ${CMAKE_SOURCE_DIR}/src/moraledata.h
    ${CMAKE_SOURCE_DIR}/src/itype.h
)

# Get GIT version strings
ADD_CUSTOM_TARGET(
	get_version
	DEPENDS ${CMAKE_SOURCE_DIR}/src/version.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

ADD_CUSTOM_COMMAND (
	OUTPUT ${CMAKE_SOURCE_DIR}/src/version.h
	COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/src/version.h.in
	-D DST=${CMAKE_SOURCE_DIR}/src/version.h
	-P ${CMAKE_SOURCE_DIR}/src/version.cmake
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Build tiles version if requested
IF(TILES)
	ADD_EXECUTABLE(cataclysm-tiles ${CATACLYSM_DDA_SOURCES} ${CATACLYSM_DDA_HEADERS})


	ADD_DEPENDENCIES(cataclysm-tiles get_version)
	IF (LUA)
		ADD_DEPENDENCIES(cataclysm-tiles lua_bindings)
	ENDIF(LUA)

	target_include_directories(cataclysm-tiles PUBLIC ${SDL2_INCLUDE_DIR})
	target_link_libraries(cataclysm-tiles ${SDL2_LIBRARY})

	target_include_directories(cataclysm-tiles PUBLIC ${SDL2_TTF_INCLUDE_DIRS})
	target_link_libraries(cataclysm-tiles ${SDL2_TTF_LIBRARIES})

	target_include_directories(cataclysm-tiles PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
	target_link_libraries(cataclysm-tiles ${SDL2_IMAGE_LIBRARIES})
	IF(SOUND)
		set_target_properties(cataclysm-tiles PROPERTIES COMPILE_DEFINITIONS
			"SDLTILES;TILES;SDL_SOUND" )

		target_include_directories(cataclysm-tiles PUBLIC ${OGGVORBIS_INCLUDE_DIR})
		target_link_libraries(cataclysm-tiles ${OGG_LIBRARY})
		target_link_libraries(cataclysm-tiles ${VORBIS_LIBRARY})
		target_link_libraries(cataclysm-tiles ${VORBISFILE_LIBRARY})
	
		target_include_directories(cataclysm-tiles PUBLIC ${SDL2_MIXER_INCLUDE_DIRS})
		target_link_libraries(cataclysm-tiles ${SDL2_MIXER_LIBRARIES})
	ELSE(SOUND)
		# Set additional build props
		set_target_properties(cataclysm-tiles PROPERTIES COMPILE_DEFINITIONS
			"SDLTILES;TILES" )
	ENDIF(SOUND)
	IF(RELEASE)
		install(TARGETS cataclysm-tiles DESTINATION ${BIN_PREFIX})
	ENDIF(RELEASE)
ENDIF(TILES)

# Build curses version if requested
IF(CURSES)
	ADD_EXECUTABLE(cataclysm ${CATACLYSM_DDA_SOURCES} ${CATACLYSM_DDA_HEADERS})

	ADD_DEPENDENCIES(cataclysm get_version)
	IF (LUA)
		ADD_DEPENDENCIES(cataclysm lua_bindings)
	ENDIF(LUA)

	target_include_directories(cataclysm PUBLIC ${CURSES_INCLUDE_PATH})
	target_link_libraries(cataclysm ${CURSES_LIBRARIES})
	#-L/usr/lib64 -lncursesw)
	#${CURSES_LIBRARIES} -L/usr/lib64)

	IF(WIN32)
        	target_link_libraries(cataclysm winmm.lib)
	ENDIF(WIN32)
	IF(RELEASE)
		install(TARGETS cataclysm DESTINATION ${BIN_PREFIX})
	ENDIF(RELEASE)
ENDIF(CURSES)

