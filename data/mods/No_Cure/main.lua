local MOD = {}

mods[ "No_Cure" ] = MOD

function MOD.on_new_player_created()

  game.add_msg( "Starting new game with <color_red>No Cure!</color> mod." )
  game.add_msg( "You will <color_red>die</color> in <color_yellow>24 hours</color> after getting <color_green>infected</color>." )

  doom()

end

function MOD.on_minute_passed()

  doom()

end

function doom()

  local calendar = game.get_calendar_turn()
  local current_turn = calendar:get_turn()

  if get_death_turn() ~= nil then
    if current_turn >= get_death_turn() then
      game.add_msg( "You have <color_red>died</color> due to being <color_green>infected</color> <color_yellow>24 hours</color> earlier." )
      player:apply_damage(player, "bp_torso", 666)  -- :die() is not working, so we apply high damage to player torso.
    end
  else
    if player:has_effect(efftype_id("infected")) then
      set_death_turn(current_turn + 24*60*60/10)
    end
  end

end

function get_death_turn()

  return tonumber(player:get_value("No_Cure_DeathTurn"))

end

function set_death_turn(death_turn)

  player:set_value("No_Cure_DeathTurn", tostring(death_turn))
  if get_death_turn() ~= nil then
    game.add_msg( "You have have been <color_green>infected</color> and will <color_red>die</color> in <color_yellow>24 hours</color>!" )
  end

end
