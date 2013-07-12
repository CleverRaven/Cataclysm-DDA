-- Main mod file that will be executed by the game.

function hiccup(player, item, active)
    item_store = item
    game.add_msg("You hiccup because of "..item.id)
end

function makehungry(player, item, active)
    game.add_msg("You suddenly feel hungry.. Your hunger is now "..player.hunger)
    player.hunger = player.hunger + 10
end

function tellstuff(player, item, active)
    game.add_msg("Your "..item.name.." tells you: str "..player.str_cur.."/"..player.str_max)
    game.add_msg("Are you hot around the legs? "..tostring(player:has_disease("hot_legs")))
    player:add_disease("hot_feet", 10, 1, 3)
    player.fatigue = 0
    game.add_msg("Fatigue: "..player.fatigue)
end

function custom_prozac(player, item, active)
    if not player:has_disease("took_prozac") and player:morale_level() < 0 then
        player:add_disease("took_prozac", 7200, 0, -1)
    else
        player.stim = player.stim + 3
    end
end

function custom_sleep(player, item, active)
    player.fatigue = player.fatigue + 40
    if not player:is_npc() then
        game.add_msg("You feel very sleepy...")
    end
end

function custom_iodine(player, item, active)
    player:add_disease("iodine", 1200, 0, -1)
    if not player:is_npc() then
        game.add_msg("You take an iodine tablet.")
    end
end

function custom_flumed(player, item, active)
    player:add_disease("took_flumed", 6000, 0, -1)
    if not player:is_npc() then
        game.add_msg("You take some "..item.name..".")
    end
end



game.register_iuse("HICCUP", hiccup)
game.register_iuse("TELLSTUFF", tellstuff)
game.register_iuse("CUSTOM_PROZAC", custom_prozac)
game.register_iuse("CUSTOM_SLEEP", custom_sleep)
game.register_iuse("CUSTOM_FLUMED", custom_flumed)
