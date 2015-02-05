monster_types = game.get_monster_types()

-- remove revivivication flag from zombies.
for _, monster_type in ipairs(monster_types) do
    if game.monster_type(monster_type):in_species("ZOMBIE") then
        game.monster_type(monster_type):set_flag("REVIVES", false)
    end
end
