monster_types = game.get_monster_types()

-- remove revivivication flag from zombies.
for _, monster_type in ipairs(monster_types) do
    local mtype = monster_type:obj()
    if mtype:in_species(species_id("ZOMBIE")) then
        mtype:set_flag("REVIVES", false)
    end
end
