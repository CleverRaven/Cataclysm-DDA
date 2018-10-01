function message(...)
    local s = string.format(...)
    game.add_msg(s)
end

function iuse_test_bionics_list()
    local num = player:num_bionics()
    if num == 0 then
        message("You installed no bionics.")
    else
        local i = 0
        while i < num do
            -- Get bionic reference
            local bio = player:bionic_at_index(i)
            -- Get bionic_data reference (use obj() function of bionic_id class)
            local bio_data = bio.id:obj()
            local color = "white"
            if bio_data.activated then
                color = "green"
            end
            message("bionics[%d]: <color_%s>%s</color>", i, color, bio_data.name)
            i = i + 1
        end
    end
end

game.register_iuse("TEST_BIONICS_LIST", iuse_test_bionics_list)
