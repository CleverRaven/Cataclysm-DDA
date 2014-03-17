monster_types = game.get_monster_types()

-- double the speed of all monsters that start with mon_zombie
for _, monster_type in ipairs(monster_types) do
    if monster_type:find("mon_zombie") then
        game.monster_type(monster_type).speed = game.monster_type(monster_type).speed * 2
    end
end
