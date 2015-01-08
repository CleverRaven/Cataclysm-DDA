monster_types = game.get_monster_types()

-- halve the speed of all monsters whose species is "ZOMBIE"
for _, monster_type in ipairs(monster_types) do
    if game.monster_type(monster_type):in_species("ZOMBIE") then
        game.monster_type(monster_type).vision_min = 60 - game.monster_type(monster_type).vision_dec
    end
end
