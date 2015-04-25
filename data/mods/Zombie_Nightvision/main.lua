monster_types = game.get_monster_types()

-- gives night vision to all monsters whose species is "ZOMBIE"
for _, monster_type in ipairs(monster_types) do
    if game.monster_type(monster_type):in_species("ZOMBIE") then
        game.monster_type(monster_type).vision_night = game.monster_type(monster_type).vision_day
    end
end
