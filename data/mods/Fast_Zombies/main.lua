monster_types = game.get_monster_types()

-- double the speed of all monsters whose species is "ZOMBIE"
for _, monster_type in ipairs(monster_types) do
    local mtype = monster_type:obj()
    if mtype:in_species(species_id("ZOMBIE")) then
        mtype.speed = mtype.speed * 2
    end
end
