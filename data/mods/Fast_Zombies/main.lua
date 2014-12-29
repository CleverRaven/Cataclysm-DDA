monster_types = game.get_monster_types()

-- double the speed of all monsters whose species is "ZOMBIE"
for _, monster_type in ipairs(monster_types) do
    if game.monster_type(monster_type):in_species("ZOMBIE") then
        game.monster_type(monster_type).speed = game.monster_type(monster_type).speed * 2
    end
end
