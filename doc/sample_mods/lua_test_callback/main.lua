local MOD = {
  id = "lua_test_callback",
  version = "2019-01-06"
}
mods[MOD.id] = MOD

MOD.MessageWithLog = function(s)
    if log.message then
      log.message(s)
    end
    if game.add_msg then
      game.add_msg(s)
    end
end

MOD.on_game_loaded = function()
  MOD.DisplayCallbackMessages("on_game_loaded")
end

MOD.on_savegame_loaded = function()
  MOD.DisplayCallbackMessages("on_savegame_loaded")
end

MOD.on_new_player_created = function(player_id)
  MOD.DisplayCallbackMessages("on_new_player_created", player_id)
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

MOD.on_weather_changed = function(weather_new, weather_old)
  MOD.DisplayCallbackMessages("on_weather_changed", weather_new, weather_old)
end

MOD.on_player_skill_increased = function(player_id, source, skill_id, level)
  MOD.DisplayCallbackMessages("on_player_skill_increased", player_id, source, skill_id, level)
end

MOD.on_player_dodge = function(player_id, source, difficulty)
  MOD.DisplayCallbackMessages("on_player_dodge", player_id, source, difficulty)
end

MOD.on_player_hit = function(player_id, source, bodypart) --[[dealt_projectile_attack]]--
  MOD.DisplayCallbackMessages("on_player_hit", player_id, source, bodypart)
end

MOD.on_player_hurt = function(player_id, source, disturb)
  MOD.DisplayCallbackMessages("on_player_hurt", player_id, source, disturb)
end

MOD.on_player_mutation_gain = function(player_id, mutation_id)
  MOD.DisplayCallbackMessages("on_player_mutation_gain", player_id, mutation_id)
end

MOD.on_player_mutation_loss = function(player_id, mutation_id)
  MOD.DisplayCallbackMessages("on_player_mutation_loss", player_id, mutation_id)
end

MOD.on_player_stat_change = function(player_id, stat_id, stat_value)
  MOD.DisplayCallbackMessages("on_player_stat_change", player_id, stat_id, stat_value)
end

MOD.on_player_item_wear = function(player_id, item_id)
  MOD.DisplayCallbackMessages("on_player_item_wear", player_id, item_id)
end

MOD.on_player_item_takeoff = function(player_id, item_id)
  MOD.DisplayCallbackMessages("on_player_item_takeoff", player_id, item_id)
end

MOD.on_player_effect_int_changes = function(player_id, effect_id, intensity, bodypart)
  MOD.DisplayCallbackMessages("on_player_effect_int_change", player_id, effect_id, intensity, bodypart)
end

MOD.on_player_mission_assignment = function(player_id, mission_id)
  MOD.DisplayCallbackMessages("on_player_mission_assignment", player_id, mission_id)
end

MOD.on_player_mission_finished = function(player_id, mission_id)
  MOD.DisplayCallbackMessages("on_player_mission_finished", player_id, mission_id)
end

MOD.on_activity_call_do_turn_started = function(act_id, player_id)
  MOD.DisplayCallbackMessages("on_activity_call_do_turn_started", act_id, player_id)
end

MOD.on_activity_call_do_turn_finished = function(act_id, player_id)
  MOD.DisplayCallbackMessages("on_activity_call_do_turn_finished", act_id, player_id)
end

MOD.on_activity_call_finish_started = function(act_id, player_id)
  MOD.DisplayCallbackMessages("on_activity_call_finish_started", act_id, player_id)
end

MOD.on_activity_call_finish_finished = function(act_id, player_id)
  MOD.DisplayCallbackMessages("on_activity_call_finish_finished", act_id, player_id)
end

MOD.on_mapgen_finished = function(mapgen_type, mapgen_id, mapgen_coord)
  MOD.DisplayCallbackMessages("on_mapgen_finished", mapgen_type, mapgen_id, mapgen_coord)
end

MOD.DisplayCallbackMessages = function(callback_name, ...)

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
