function message(...)
    local s = string.format(...)
    game.add_msg(s)
end

function iuse_test_gain_morale()
    -- Create select menu
    local um = game.create_uimenu()
    um.title = "Gain morale menu"
    um:addentry("Gain morale")
    um:addentry("Remove morale")
    um:addentry("Cancel")
    -- Wait for player selection
    um:query(true)

    if um.selected == 0 then
        -- "Gain morale" is selected
        if player:has_morale(morale_type("morale_test")) == 0 then
            -- Add morale boost (+100/10 min.)
            player:add_morale(morale_type("morale_test"), 100, 100, MINUTES(10))
            message("You gained big morale boost!")
        else
            message("You already gained morale boost.")
        end
    elseif um.selected == 1 then
        -- "Remove morale" is selected
        player:rem_morale(morale_type("morale_test"))
        message("Your morale boost is finished.")
    end

    return 0
end

game.register_iuse("TEST_GAIN_MORALE", iuse_test_gain_morale)
