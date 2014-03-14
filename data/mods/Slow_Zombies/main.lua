monster_types = {
    "mon_zombie",
    "mon_zombie_fat",
    "mon_zombie_tough",
    "mon_zombie_crawler",
    "mon_zombie_hazmat",
    "mon_zombie_fireman",
    "mon_zombie_survivor",
    "mon_zombie_rot",
    "mon_zombie_cop",
    "mon_zombie_shrieker",
    "mon_zombie_spitter",
    "mon_zombie_electric",
    "mon_zombie_smoker",
    "mon_zombie_swimmer",
    "mon_zombie_dog",
    "mon_zombie_brute",
    "mon_zombie_hulk"
}

-- halve the speed of all monsters in the above list
for _, monster_type in ipairs(monster_types) do
    game.monster_type(monster_type).speed = game.monster_type(monster_type).speed / 2
end