-- This is executed before any json data is loaded.
-- Place all iuse actions that shall be available for item here.

function hiccup(item, active)
    item_store = item
    game.add_msg("You hiccup because of "..item.id)
end

function tellstuff(item, active)
    game.add_msg("Your foo tells you: str "..player.str_cur.."/"..player.str_max)
    game.add_msg("Are you hot around the legs? "..tostring(player:has_disease("hot_legs")))
    player:add_disease("hot_feet", 10, 1, 3)
    player.fatigue = 0
    game.add_msg("Fatigue: "..player.fatigue)
end

function custom_prozac(item, active)
    if not player:has_disease("took_prozac") and player:morale_level() < 0 then
        player:add_disease("took_prozac", 7200, 0, -1)
    else
        player.stim = player.stim + 3
    end
end

function custom_sleep(item, active)
    player.fatigue = player.fatigue + 40
    if not player:is_npc() then
        game.add_msg("You feel very sleepy...")
    end
end

function custom_iodine(item, active)
    player:add_disease("iodine", 1200, 0, -1)
    if not player:is_npc() then
        game.add_msg("You take an iodine tablet.")
    end
end

function custom_flumed(item, active)
    player:add_disease("took_flumed", 6000, 0, -1)
    if not player:is_npc() then
        game.add_msg("You take some "..item.name..".")
    end
end

-- Test the tripoint metatable: accessing functions, reading and writing members.
function tripoint_mt_test()
    local trp = tripoint(4, 77, 2)
    -- Function call hard coded in catabindings.cpp
    local a = trp:serialize()
    -- You may try this (it should fail, Lua should complain, but it should *not* crash):
    -- trp:non_existing_function()
    -- trp.non_existing_member = 10
    game.popup("a: " .. a) -- Should be a json string [9, 3, 1]
    -- Read a member, this is dispatched via the READ_MEMBERS in C++
    local b = trp.x
    game.popup("a: " .. a .. ", b: " .. b)
    -- Write a member, this is dispatched via the WRITE_MEMBERS in C++
    trp.x = 1000;
    game.popup("b: " .. b .. ", trp.x: " .. trp.x) -- b should be the same as before
end

game.register_iuse("HICCUP", hiccup)
game.register_iuse("TELLSTUFF", tellstuff)
game.register_iuse("CUSTOM_PROZAC", custom_prozac)
game.register_iuse("CUSTOM_SLEEP", custom_sleep)
game.register_iuse("CUSTOM_FLUMED", custom_flumed)
game.register_iuse("CUSTOM_IODINE", custom_iodine)
