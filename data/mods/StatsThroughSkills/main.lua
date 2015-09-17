--Increasing stats by skill
local MOD = {}

mods["StatsThoughSkills"] = MOD

function MOD.on_day_passed()
    game.add_msg("Calculating new stats based off skills")
    str_bonus = 8
    dex_bonus = 8
    int_bonus = 8
    per_bonus = 8

    --Str based skills
    str_bonus = calc_bonus(str_bonus,"carpentry")
    str_bonus = calc_bonus(str_bonus,"mechanics")
    str_bonus = calc_bonus(str_bonus,"swimming")
    str_bonus = calc_bonus(str_bonus,"bashing")
    str_bonus = calc_bonus(str_bonus,"cutting")
    str_bonus = calc_bonus(str_bonus,"melee")
    str_bonus = calc_bonus(str_bonus,"throw")

    --Dex based skills
    dex_bonus = calc_bonus(dex_bonus,"driving")
    dex_bonus = calc_bonus(dex_bonus,"survival")
    dex_bonus = calc_bonus(dex_bonus,"tailor")
    dex_bonus = calc_bonus(dex_bonus,"traps")
    dex_bonus = calc_bonus(dex_bonus,"dodge")
    dex_bonus = calc_bonus(dex_bonus,"stabbing")
    dex_bonus = calc_bonus(dex_bonus,"unarmed")

    --Int based skills
    int_bonus = calc_bonus(int_bonus,"barter")
    int_bonus = calc_bonus(int_bonus,"computer")
    int_bonus = calc_bonus(int_bonus,"cooking")
    int_bonus = calc_bonus(int_bonus,"electronics")
    int_bonus = calc_bonus(int_bonus,"fabrication")
    int_bonus = calc_bonus(int_bonus,"firstaid")
    int_bonus = calc_bonus(int_bonus,"speech")

    --Per based skills
    per_bonus = calc_bonus(per_bonus,"archery")
    per_bonus = calc_bonus(per_bonus,"gun")
    per_bonus = calc_bonus(per_bonus,"launcher")
    per_bonus = calc_bonus(per_bonus,"pistol")
    per_bonus = calc_bonus(per_bonus,"rifle")
    per_bonus = calc_bonus(per_bonus,"shotgun")
    per_bonus = calc_bonus(per_bonus,"smg")

    --Checking for mutations
    if player:has_trait("BENDY1") then dex_bonus = dex_bonus + 1 end
    if player:has_trait("BENDY2") then 
        dex_bonus = dex_bonus + 3 
        str_bonus = str_bonus - 2 
    end
    if player:has_trait("BENDY3") then 
        dex_bonus = dex_bonus + 4 
        str_bonus = str_bonus - 4 
    end
    if player:has_trait("STOCKY_TROGLO") then 
        str_bonus = str_bonus + 2 
        dex_bonus = dex_bonus - 2 
    end
    if player:has_trait("PRED3") then int_bonus = int_bonus - 1 end
    if player:has_trait("PRED4") then int_bonus = int_bonus - 3 end
    if player:has_trait("LARGE") then str_bonus = str_bonus + 2 end
    if player:has_trait("LARGE_OK") then str_bonus = str_bonus + 2 end
    if player:has_trait("HUGE") then str_bonus = str_bonus + 4 end
    if player:has_trait("HUGE_OK") then str_bonus = str_bonus + 4 end
    if player:has_trait("STR_UP") then str_bonus = str_bonus + 1 end
    if player:has_trait("STR_UP_2") then str_bonus = str_bonus + 2 end
    if player:has_trait("STR_UP_3") then str_bonus = str_bonus + 4 end
    if player:has_trait("STR_UP_4") then str_bonus = str_bonus + 7 end
    if player:has_trait("DEX_UP") then dex_bonus = dex_bonus + 1 end
    if player:has_trait("DEX_UP_2") then dex_bonus = dex_bonus + 2 end
    if player:has_trait("DEX_UP_3") then dex_bonus = dex_bonus + 4 end
    if player:has_trait("DEX_UP_4") then dex_bonus = dex_bonus + 7 end
    if player:has_trait("INT_UP") then int_bonus = int_bonus + 1 end
    if player:has_trait("INT_UP_2") then int_bonus = int_bonus + 2 end
    if player:has_trait("INT_UP_3") then int_bonus = int_bonus + 4 end
    if player:has_trait("INT_UP_4") then int_bonus = int_bonus + 7 end
    if player:has_trait("PER_UP") then per_bonus = per_bonus + 1 end
    if player:has_trait("PER_UP_2") then per_bonus = per_bonus + 2 end
    if player:has_trait("PER_UP_3") then per_bonus = per_bonus + 4 end
    if player:has_trait("PER_UP_4") then per_bonus = per_bonus + 7 end
    if player:has_trait("PER_SLIME") then per_bonus = per_bonus - 8 end
    if player:has_trait("PER_SLIME_OK") then per_bonus = per_bonus + 5 end

    print_results(str_bonus,"Str",player.str_max)
    player.str_max = str_bonus

    print_results(dex_bonus,"Dex",player.dex_max)
    player.dex_max = dex_bonus

    print_results(int_bonus,"Int",player.int_max)
    player.int_max = int_bonus

    print_results(per_bonus,"Per",player.per_max)
    player.per_max = per_bonus

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
