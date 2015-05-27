-- This is executed before any json data is loaded.
-- Place all iuse actions that shall be available for item here.

function hiccup(item, active, pos)
    item_store = item
    game.add_msg("You hiccup because of "..item:tname())
end

function tellstuff(item, active, pos)
    game.add_msg("Your foo tells you: str "..player.str_cur.."/"..player.str_max)
    -- Can't currently check specific bodyparts, sorry.
    -- Should be player:has_effect("hot", bp_leg_l) or so
    game.add_msg("Are you hot around the legs? "..tostring(player:has_effect("hot")))
    -- Can't currently add effects to specific bodyparts, sorry.
    -- Should be player:add_effect("hot", 10, bp_leg_l)
    player:add_effect("hot", 10)
    player.fatigue = 0
    game.add_msg("Fatigue: "..player.fatigue)
end

function custom_prozac(item, active, pos)
    if not player:has_effect("took_prozac") and player:morale_level() < 0 then
        player:add_effect("took_prozac", 7200)
    else
        player.stim = player.stim + 3
    end
end

function custom_sleep(item, active, pos)
    player.fatigue = player.fatigue + 40
    if not player:is_npc() then
        game.add_msg("You feel very sleepy...")
    end
end

function custom_iodine(item, active, pos)
    player:add_effect("iodine", 1200)
    if not player:is_npc() then
        game.add_msg("You take an iodine tablet.")
    end
end

function custom_flumed(item, active, pos)
    player:add_effect("took_flumed", 6000)
    if not player:is_npc() then
        game.add_msg("You take some "..item:tname()..".")
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

-- An iterator over all items on the map at a specific point.
function item_stack_iterator(pos)
    local start = map:i_at(pos)
    local b = start:cppbegin()
    return function ()
        if b == start:cppend() then
            return nil
        else
            local e = b:elem()
            b:inc()
            return e
        end
    end
end

function item_test()
    local pos = player:pos()
    for x in item_stack_iterator(pos) do
        game.popup( x:tname() )
    end
end

-- Goes through all the items on the map at the players location and tries to revive a corpse.
function necro()
    local pos = player:pos()
    -- Choose a random neighbor square, because the monster can not revive at pos as the player is in the way.
    local neighbor = tripoint(pos.x + game.rng(-1, 1), pos.y + game.rng(-1, 1), pos.z)
    game.add_msg("trying at " .. neighbor.x .. ", " .. neighbor.y .. ", " .. neighbor.z)
    for itm in item_stack_iterator(pos) do
        if itm:is_corpse() then
            game.add_msg("Found a corpe: " .. itm:tname())
            if game.revive_corpse(neighbor, itm) then
                game.remove_item(pos, itm)
                break
            end
        end
    end
end

function custom_iuse_test(item, active, pos)
    game.popup("use function called on an item " .. item:tname() .. ", active: " .. tostring(active) .. ", location is " .. pos.x .. ", " .. pos.y .. ", " .. pos.y)
    return 0
end

game.register_iuse("HICCUP", hiccup)
game.register_iuse("TELLSTUFF", tellstuff)
game.register_iuse("CUSTOM_PROZAC", custom_prozac)
game.register_iuse("CUSTOM_SLEEP", custom_sleep)
game.register_iuse("CUSTOM_FLUMED", custom_flumed)
game.register_iuse("CUSTOM_IODINE", custom_iodine)
game.register_iuse("CUSTOM_IUSE_TEST", custom_iuse_test)
