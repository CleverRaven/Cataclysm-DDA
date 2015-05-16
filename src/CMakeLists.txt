set(SRCS
	action.cpp
	action.h
	active_item_cache.cpp
	active_item_cache.h
	activity_handlers.cpp
	activity_handlers.h
	activity_item_handling.cpp
	addiction.cpp
	addiction.h
	advanced_inv.cpp
	advanced_inv.h
	ammo.cpp
	ammo.h
	animation_curses.cpp
	animation_tiles.cpp
	armor_layers.cpp
	armor_layers.h
	artifact.cpp
	artifact.h
	auto_pickup.cpp
	auto_pickup.h
	basecamp.cpp
	basecamp.h
	bionics.cpp
	bionics.h
	bodypart.cpp
	bodypart.h
	calendar.cpp
	calendar.h
	catacharset.cpp
	catacharset.h
	catacurse.h
	catalua.cpp
	catalua.h
	cata_tiles.cpp
	cata_tiles.h
	character.cpp
	character.h
	char_validity_check.cpp
	char_validity_check.h
	clzones.cpp
	clzones.h
	color.cpp
	color.h
	compatibility.h
	computer.cpp
	computer.h
	construction.cpp
	construction.h
	coordinates.h
	crafting.cpp
	crafting.h
	creature.cpp
	creature.h
	creature_tracker.cpp
	creature_tracker.h
	cursesdef.h
	cursesport.cpp
	damage.cpp
	damage.h
	debug.cpp
	debug.h
	defense.cpp
	dependency_tree.cpp
	dependency_tree.h
	dialogue.h
	editmap.cpp
	editmap.h
	effect.cpp
	effect.h
	enums.h
	event.cpp
	event.h
	faction.cpp
	faction.h
	field.cpp
	field.h
	filesystem.cpp
	filesystem.h
	game_constants.h
	game.cpp
	game.h
	gamemode.cpp
	gamemode.h
	get_version.h
	help.cpp
	help.h
	iexamine.cpp
	iexamine.h
	init.cpp
	init.h
	input.cpp
	input.h
	inventory.cpp
	inventory.h
	inventory_ui.cpp
	item_action.cpp
	item_action.h
	item.cpp
	item_factory.cpp
	item_factory.h
	item_group.cpp
	item_group.h
	item.h
	item_stack.h
	itype.cpp
	itype.h
	iuse_actor.cpp
	iuse_actor.h
	iuse.cpp
	iuse.h
	iuse_software.cpp
	iuse_software.h
	iuse_software_kitten.cpp
	iuse_software_kitten.h
	iuse_software_snake.cpp
	iuse_software_snake.h
	iuse_software_sokoban.cpp
	iuse_software_sokoban.h
	json.cpp
	json.h
	lightmap.cpp
	lightmap.h
	line.cpp
	line.h
	live_view.cpp
	live_view.h
	main.cpp
	main_menu.cpp
	mapbuffer.cpp
	mapbuffer.h
	map.cpp
	mapdata.cpp
	mapdata.h
	mapgen.cpp
	mapgenformat.cpp
	mapgenformat.h
	mapgen_functions.cpp
	mapgen_functions.h
	mapgen.h
	map.h
	mapsharing.cpp
	mapsharing.h
	martialarts.cpp
	martialarts.h
	material.cpp
	material.h
	melee.cpp
	messages.cpp
	messages.h
	mission.cpp
	missiondef.cpp
	mission_end.cpp
	mission_fail.cpp
	mission.h
	mission_place.cpp
	mission_start.cpp
	mod_manager.cpp
	mod_manager.h
	mod_manager_ui.cpp
	monattack.cpp
	monattack.h
	mondeath.cpp
	mondeath.h
	mondefense.cpp
	mondefense.h
	mongroupdef.cpp
	mongroup.h
	monmove.cpp
	monster.cpp
	monstergenerator.cpp
	monstergenerator.h
	monster.h
	moraledata.h
	morale.h
	mtype.cpp
	mtype.h
	mutation.cpp
	mutation_data.cpp
	mutation.h
	name.cpp
	name.h
	newcharacter.cpp
	nonplayer.cpp
	nonplayer.h
	npc.cpp
	npc.h
	npcmove.cpp
	npctalk.cpp
	omdata.h
	options.cpp
	options.h
	output.cpp
	output.h
	overmapbuffer.cpp
	overmapbuffer.h
	overmap.cpp
	overmap.h
	path_info.cpp
	path_info.h
	pickup.cpp
	pickup.h
	platform_win.h
	player_activity.cpp
	player_activity.h
	player.cpp
	player.h
	pldata.h
	posix_time.cpp
	posix_time.h
	profession.cpp
	profession.h
	ranged.cpp
	requirements.cpp
	requirements.h
	resource.rc
	rng.cpp
	rng.h
	savegame.cpp
	savegame.h
	savegame_json.cpp
	savegame_legacy.cpp
	scenario.cpp
	scenario.h
	sdltiles.cpp
	simplexnoise.cpp
	simplexnoise.h
	skill.cpp
	skill.h
	sounds.cpp
	sounds.h
	speech.cpp
	speech.h
	start_location.cpp
	start_location.h
	text_snippets.cpp
	text_snippets.h
	tile_id_data.h
	tileray.cpp
	tileray.h
	translations.cpp
	translations.h
	trapdef.cpp
	trapfunc.cpp
	trap.h
	tutorial.cpp
	tutorial.h
	ui.cpp
	ui.h
	uistate.h
	vehicle.cpp
	vehicle.h
	vehicle_factory.cpp
	vehicle_factory.h
	veh_interact.cpp
	veh_interact.h
	veh_typedef.cpp
	veh_type.h
	version.cpp
	wcwidth.h
	wdirent.h
	weather.cpp
	weather_data.cpp
	weather_gen.cpp
	weather_gen.h
	weather.h
	wincurse.cpp
	wish.cpp
	worldfactory.cpp
	worldfactory.h
)

# Add the actual executable
add_executable(cataclysm ${SRCS})

# Generate version information
if (WIN32)
	add_custom_target(gen_version
		COMMAND for /F \"tokens=*\" %%i in \('git describe --tags --always --dirty --match \"[0-9]*.[0-9]*\"'\) do set VERSION=%%i
		COMMAND echo \#define VERSION \"%VERSION%\" > ${CMAKE_SOURCE_DIR}/src/version.h
	)
else()
	add_custom_target(gen_version
		COMMAND echo -n \"\#define VERSION \" > ${CMAKE_SOURCE_DIR}/src/version.h
		COMMAND git describe --tags --always --dirty --match \"[0-9]*.[0-9]*\" >> ${CMAKE_SOURCE_DIR}/src/version.h
	)
endif()
add_dependencies(cataclysm gen_version)

# Required library linkage
if(TILES)
	target_include_directories(cataclysm PUBLIC ${SDL2_INCLUDE_DIR})
	target_link_libraries(cataclysm ${SDL2_LIBRARY})

	target_include_directories(cataclysm PUBLIC ${SDL2_TTF_INCLUDE_DIRS})
	target_link_libraries(cataclysm ${SDL2_TTF_LIBRARIES})

	target_include_directories(cataclysm PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
	target_link_libraries(cataclysm ${SDL2_IMAGE_LIBRARIES})
else()
	target_include_directories(cataclysm PUBLIC ${CURSES_INCLUDE_DIRS})
	target_link_libraries(cataclysm ${CURSES_LIBRARIES})

    if(WIN32)
        target_link_libraries(cataclysm winmm.lib)
    endif()
endif()

if(SOUND)
	target_include_directories(cataclysm PUBLIC ${OGGVORBIS_INCLUDE_DIR})
	target_link_libraries(cataclysm ${OGG_LIBRARY})
	target_link_libraries(cataclysm ${VORBIS_LIBRARY})
	target_link_libraries(cataclysm ${VORBISFILE_LIBRARY})

	target_include_directories(cataclysm PUBLIC ${SDL2_MIXER_INCLUDE_DIRS})
	target_link_libraries(cataclysm ${SDL2_MIXER_LIBRARIES})
endif()
