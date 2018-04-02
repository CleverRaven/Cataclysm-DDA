local MOD = {}

mods["No_Cure"] = MOD

local NO_CURE_MESSAGE_START_NOTIFICATION = "Starting new game with <color_red>No Cure!</color> mod.  You will <color_red>die</color> in <color_yellow>72 hours</color> after getting <color_green>infected</color>."
local NO_CURE_MESSAGE_EFFECT_APPLIED = "You have been <color_green>infected</color> and will <color_red>die</color> in <color_yellow>72 hours</color>!"
local NO_CURE_MESSAGE_DEATH = "You have <color_red>died</color> due to being <color_green>infected</color> <color_yellow>72 hours</color> earlier."

local no_cure_effect = efftype_id("infected")
local no_cure_effect_duration = 72*60*60/10 --72 hours
local no_cure_effect_body_part = "bp_torso"
local no_cure_effect_body_part_damage = 666
local no_cure_player_variable = "No_Cure_DeathTurn"

function MOD.on_new_player_created()

  game.add_msg(NO_CURE_MESSAGE_START_NOTIFICATION)

  no_cure_process()

end

function MOD.on_minute_passed()

  no_cure_process()

end

function no_cure_process()

  local calendar = game.get_calendar_turn()
  local current_turn = calendar:get_turn()
  local expected_death_turn = tonumber(player:get_value(no_cure_player_variable))

  if expected_death_turn == nil and player:has_effect(no_cure_effect) then
    game.add_msg(NO_CURE_MESSAGE_EFFECT_APPLIED)
    player:set_value(no_cure_player_variable, tostring(current_turn + no_cure_effect_duration))
  else
    if current_turn >= expected_death_turn then
      game.add_msg(NO_CURE_MESSAGE_DEATH)
      player:apply_damage(player, no_cure_effect_body_part_damage, no_cure_effect_damage) -- player:die() is not working, so we apply high damage to selected player body part.
    end
  end

end
