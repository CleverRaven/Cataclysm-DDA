monster_types = game.get_monster_types()

-- gives pathfinding to all monsters whose species is "ZOMBIE"
for _, monster_type in ipairs(monster_types) do
    if game.monster_type(monster_type):in_species("ZOMBIE") then
        game.monster_type(monster_type):set_flag("PATHFINDS", true)
    end
end
