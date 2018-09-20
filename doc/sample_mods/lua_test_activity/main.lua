local MOD = {}
mods["lua_test_activity"] = MOD

function message(...)
    local s = string.format(...)
    game.add_msg(s)
end

function MOD.on_minute_passed()
    local act = player.activity
    if not act:is_null() then
        -- Show your current activity
        message("activity_id: %s", act:id():str())
        message("moves_left: %d", act.moves_left)
    end
end
