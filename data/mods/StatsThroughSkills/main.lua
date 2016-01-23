--Increasing stats by skill
local MOD = {}

mods["StatsThoughSkills"] = MOD

function initialize_new_player()
    initialized = player:get_value("StatsThoughSkills")

    if(initialized == "true") {
        return
    }

    base_str = player.str_max
    base_dex = player.dex_max
    base_int = player.int_max
    base_per = player.per_max

    player:set_value("base_str", tostring(base_str))
    player:set_value("base_dex", tostring(base_dex))
    player:set_value("base_int", tostring(base_int))
    player:set_value("base_per", tostring(base_per))

    player:set_value("StatsThoughSkills", "true")
end

-- function MOD.new_player_created()
--     initialize_new_player();
-- end

function MOD.on_day_passed()
    initialize_new_player()
    game.add_msg("Calculating new stats based off skills")

    str_bonus = 0
    dex_bonus = 0
    int_bonus = 0
    per_bonus = 0

    --Str based skills
    str_bonus = calc_bonus(str_bonus,"carpentry")
    str_bonus = calc_bonus(str_bonus,"mechanics")
    str_bonus = calc_bonus(str_bonus,"swimming")
    str_bonus = calc_bonus(str_bonus,"bashing")
    str_bonus = calc_bonus(str_bonus,"cutting")
    str_bonus = calc_bonus(str_bonus,"melee")
    str_bonus = calc_bonus(str_bonus,"throw")
    if (str_bonus >= 21) then
        str_bonus = 6
    elseif (str_bonus >= 15) then
        str_bonus = 5
    elseif (str_bonus >= 10) then
        str_bonus = 4
    elseif (str_bonus >=6) then
        str_bonus = 3
    elseif (str_bonus >=3) then
        str_bonus = 2
    elseif (str_bonus >=1) then
        str_bonus = 1
    else
        str_bonus = 0
    end
    
    --Dex based skills
    dex_bonus = calc_bonus(dex_bonus,"driving")
    dex_bonus = calc_bonus(dex_bonus,"survival")
    dex_bonus = calc_bonus(dex_bonus,"tailor")
    dex_bonus = calc_bonus(dex_bonus,"traps")
    dex_bonus = calc_bonus(dex_bonus,"dodge")
    dex_bonus = calc_bonus(dex_bonus,"stabbing")
    dex_bonus = calc_bonus(dex_bonus,"unarmed")
    if (dex_bonus >= 21) then
        dex_bonus = 6
    elseif (dex_bonus >= 15) then
        dex_bonus = 5
    elseif (dex_bonus >= 10) then
        dex_bonus = 4
    elseif (dex_bonus >=6) then
        dex_bonus = 3
    elseif (dex_bonus >=3) then
        dex_bonus = 2
    elseif (dex_bonus >=1) then
        dex_bonus = 1
    else
        dex_bonus = 0
    end

    --Int based skills
    int_bonus = calc_bonus(int_bonus,"barter")
    int_bonus = calc_bonus(int_bonus,"computer")
    int_bonus = calc_bonus(int_bonus,"cooking")
    int_bonus = calc_bonus(int_bonus,"electronics")
    int_bonus = calc_bonus(int_bonus,"fabrication")
    int_bonus = calc_bonus(int_bonus,"firstaid")
    int_bonus = calc_bonus(int_bonus,"speech")
    if (int_bonus >= 21) then
        int_bonus = 6
    elseif (int_bonus >= 15) then
        int_bonus = 5
    elseif (int_bonus >= 10) then
        int_bonus = 4
    elseif (int_bonus >=6) then
        int_bonus = 3
    elseif (int_bonus >=3) then
        int_bonus = 2
    elseif (int_bonus >=1) then
        int_bonus = 1
    else
        int_bonus = 0
    end

    --Per based skills
    per_bonus = calc_bonus(per_bonus,"archery")
    per_bonus = calc_bonus(per_bonus,"gun")
    per_bonus = calc_bonus(per_bonus,"launcher")
    per_bonus = calc_bonus(per_bonus,"pistol")
    per_bonus = calc_bonus(per_bonus,"rifle")
    per_bonus = calc_bonus(per_bonus,"shotgun")
    per_bonus = calc_bonus(per_bonus,"smg")
    if (per_bonus >= 21) then
        per_bonus = 6
    elseif (per_bonus >= 15) then
        per_bonus = 5
    elseif (per_bonus >= 10) then
        per_bonus = 4
    elseif (per_bonus >=6) then
        per_bonus = 3
    elseif (per_bonus >=3) then
        per_bonus = 2
    elseif (per_bonus >=1) then
        per_bonus = 1
    else
        per_bonus = 0
    end

    player.str_max = tonumber(player:get_value("base_str")) + str_bonus
    player.dex_max = tonumber(player:get_value("base_dex")) + dex_bonus
    player.int_max = tonumber(player:get_value("base_int")) + int_bonus
    player.per_max = tonumber(player:get_value("base_per")) + per_bonus
    print_results(str_bonus,"Str",player.str_max)
    print_results(dex_bonus,"Dex",player.dex_max)
    print_results(int_bonus,"Int",player.int_max)
    print_results(per_bonus,"Per",player.per_max)

    player:recalc_hp()
end

function calc_bonus(stat_bonus,skill)
    skill_level = player:get_skill_level(skill_id(skill))
    if (skill_level / 3 < 3) then
        stat_bonus = stat_bonus + skill_level / 3
    else
        stat_bonus = stat_bonus + 3
    end
    return math.floor(stat_bonus)
end

function print_results(stat_bonus,stat,player_stat)
    if (player_stat < stat_bonus) then
        game.add_msg("Raising "..stat.." to "..tostring(stat_bonus))
    elseif (player_stat > stat_bonus) then
        game.add_msg("Lowering "..stat.." to "..tostring(stat_bonus))
    end
end
