local MOD = {}

mods["ElfMod"] = MOD

function MOD.on_day_passed()
    game.add_msg("A day has passed! New elves are spawning!")

    -- spawn monster group in top left corner
    local overmap = game.get_current_overmap()
    local top = overmap:get_top_border()
    local left = overmap:get_left_border()
    local newgroup = game.create_monster_group(overmap, "GROUP_ELF", left + 5, top + 5, 0, 10, 2000)
    newgroup.horde = true

    -- Elves multiply
    for _, mongroup in pairs(game.monstergroups(overmap)) do
        if mongroup.type == "GROUP_ELF" then
            mongroup.population = mongroup.population + 1
            game.add_msg("The elves multiply, oh noes! Current elf count: "..tostring(mongroup.population))
        end
    end
end

