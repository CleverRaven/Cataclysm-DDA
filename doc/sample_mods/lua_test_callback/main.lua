local MOD = {
  id = "lua_test_callback",
  version = "2018-09-10"
}
mods[MOD.id] = MOD

MOD.MessageWithLog = function( s )
    if (log.message) then
      log.message( s )
    end
    if (game.add_msg) then
      game.add_msg( s )
    end
end

MOD.on_game_loaded = function() 
  MOD.DisplayCallbackMessages("on_game_loaded")
end

MOD.on_savegame_loaded = function()
  MOD.DisplayCallbackMessages("on_savegame_loaded")
end

MOD.on_new_player_created = function()
  MOD.DisplayCallbackMessages("on_new_player_created")
end

MOD.on_turn_passed = function()
  MOD.DisplayCallbackMessages("on_turn_passed")
end

MOD.on_second_passed = function()
  MOD.DisplayCallbackMessages("on_second_passed")
end

MOD.on_minute_passed = function()
  MOD.DisplayCallbackMessages("on_minute_passed")
end

MOD.on_hour_passed = function()
  MOD.DisplayCallbackMessages("on_hour_passed")
end

MOD.on_day_passed = function()
  MOD.DisplayCallbackMessages("on_day_passed")
end

MOD.on_year_passed = function()
  MOD.DisplayCallbackMessages("on_year_passed")
end

MOD.on_player_skill_increased = function(skill_increased_source, skill_increased_id, skill_increased_level)
  MOD.DisplayCallbackMessages("on_player_skill_increased", skill_increased_source, skill_increased_id, skill_increased_level)
end

MOD.on_player_dodge = function(source_dodge,difficulty_dodge)
  MOD.DisplayCallbackMessages("on_player_dodge", source_dodge, difficulty_dodge)
end

MOD.on_player_hit = function(source_hit, body_part_hit) --[[dealt_projectile_attack]]--
  MOD.DisplayCallbackMessages("on_player_hit")
end

MOD.on_player_hurt = function(source_hurt, disturb)
  MOD.DisplayCallbackMessages("on_player_hurt", source_hurt, disturb)
end

MOD.on_player_mutation_gain = function(mutation_gained)
  MOD.DisplayCallbackMessages("on_player_mutation_gain", mutation_gained)
end

MOD.on_player_mutation_loss = function(mutation_lost)
  MOD.DisplayCallbackMessages("on_player_mutation_loss", mutation_lost)
end

MOD.on_player_stat_change = function(stat_changed,stat_value)
  MOD.DisplayCallbackMessages("on_player_stat_change", stat_changed,stat_value)
end

MOD.on_player_item_wear = function(item_last_worn)
  MOD.DisplayCallbackMessages("on_player_item_wear", item_last_worn)
end

MOD.on_player_item_takeoff = function(item_last_taken_off)
  MOD.DisplayCallbackMessages("on_player_item_takeoff", item_last_taken_off)
end

MOD.on_player_effect_int_changes = function(effect_changed, effect_intensity, effect_bodypart)
  MOD.DisplayCallbackMessages("on_player_effect_int_change", effect_changed, effect_intensity, effect_bodypart)
end

MOD.on_player_mission_assignment = function(mission_assigned)
  MOD.DisplayCallbackMessages("on_player_mission_assignment", mission_assigned)
end

MOD.on_player_mission_finished = function(mission_finished)
  MOD.DisplayCallbackMessages("on_player_mission_finished", mission_finished)
end

MOD.on_mapgen_finished = function(mapgen_generator_type, mapgen_terrain_type_id, mapgen_terrain_coordinates) 
  MOD.DisplayCallbackMessages("on_mapgen_finished", mapgen_generator_type, mapgen_terrain_type_id, mapgen_terrain_coordinates)
end

MOD.on_weather_changed = function(weather_new, weather_old)
  MOD.DisplayCallbackMessages("on_weather_changed", weather_new, weather_old)
end

MOD.DisplayCallbackMessages = function( callback_name, ... )

  MOD.MessageWithLog ("Callback name is <color_cyan>"..tostring(callback_name).."</color>")
  local callback_args = {...}
  local callback_args_count = #callback_args
  if callback_args_count > 0 then
    MOD.MessageWithLog ("Callback data length is <color_blue>"..tostring(callback_args_count).."</color>")
    for i = 1, callback_args_count do
      local callback_arg_name = "callback_arg_"..i
      local callback_arg_data = callback_args[i]
      local callback_arg_type = type(callback_arg_data)
      MOD.MessageWithLog ("Callback arg <color_yellow>"..tostring(callback_arg_name).."</color> is <color_green>"..tostring(callback_arg_data).."</color> of type <color_pink>"..tostring(callback_arg_type).."</color>")
    end
  else
    MOD.MessageWithLog ("Callback args are <color_red>empty</color>")
  end
end

MOD.on_game_loaded()
