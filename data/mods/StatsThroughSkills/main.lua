--Increasing stats by skill
local MOD = {}

local version = 2

local str_skills = { "carpentry", "mechanics", "swimming", "bashing", "cutting", "melee", "throw" }
local dex_skills = { "driving", "survival", "tailor", "traps", "dodge", "stabbing", "unarmed" }
local int_skills = { "barter", "computer", "cooking", "electronics", "fabrication", "firstaid", "speech" }
local per_skills = { "archery", "gun", "launcher", "pistol", "rifle", "shotgun", "smg" }

mods["StatsThroughSkills"] = MOD

function MOD.on_new_player_created()
    game.add_msg("New player starting with StatsThroughSkills")
    player:set_value("StatsThoughSkills", tostring(version))
    calculate_bonuses()
end

function MOD.on_skill_increased()
    calculate_bonuses()
end

function calculate_bonuses()
    game.add_msg("Calculating new stats based off skills")

    handle_previous_version()

    prev_str = player.str_max
    prev_dex = player.dex_max
    prev_int = player.int_max
    prev_per = player.per_max

    remove_existing_bonuses()

    str_bonus = 0
    dex_bonus = 0
    int_bonus = 0
    per_bonus = 0

    str_bonus = calc_bonus(str_skills)
    dex_bonus = calc_bonus(dex_skills)
    int_bonus = calc_bonus(dex_skills)
    per_bonus = calc_bonus(per_skills)

    player:set_value("str_bonus", tostring(str_bonux))
    player:set_value("dex_bonus", tostring(dex_bonus))
    player:set_value("int_bonus", tostring(int_bonus))
    player:set_value("per_bonus", tostring(per_bonus))

    player.str_max = player.str_max + str_bonus
    player.dex_max = player.dex_max + dex_bonus
    player.int_max = player.int_max + int_bonus
    player.per_max = player.per_max + per_bonus

    print_results(player.str_max,"Str",prev_str)
    print_results(player.dex_max,"Dex",prev_dex)
    print_results(player.int_max,"Int",prev_int)
    print_results(player.per_max,"Per",prev_per)

    player:recalc_hp()
end

function remove_existing_bonuses() 
    str_bonus = tonumber(player:get_value("str_bonus"))
    dex_bonus = tonumber(player:get_value("dex_bonus"))
    int_bonus = tonumber(player:get_value("int_bonus"))
    per_bonus = tonumber(player:get_value("per_bonus"))

    player.str_max = player.str_max - str_bonus
    player.dex_max = player.dex_max - dex_bonus
    player.int_max = player.int_max - int_bonus
    player.per_max = player.per_max - per_bonus
end

function calc_bonus(skills_set)
    skill_total = 0
    for _,s in ipairs(skills_set) do
        skill_total += player:get_skill_level(skill_id(s))
    end

    skill_total /= 4

    if (skill_total >= 21) then
        skill_bonus = 6
    elseif (skill_total >= 15) then
        skill_bonus = 5
    elseif (skill_total >= 10) then
        skill_bonus = 4
    elseif (skill_total >=6) then
        skill_bonus = 3
    elseif (skill_total >=3) then
        skill_bonus = 2
    elseif (skill_total >=1) then
        skill_bonus = 1
    else
        skill_bonus = 0
    end

    return skill_bonus
end

function print_results(cur_stat,stat,prev_stat)
    if (prev_stat < cur_stat) then
        game.add_msg("Raising "..stat.." to "..tostring(cur_stat))
    elseif (prev_stat > cur_stat) then
        game.add_msg("Lowering "..stat.." to "..tostring(cur_stat))
    end
end

function handle_previous_version()
    loaded_version = tonumber(player:get_value("StatsThroughSkills"))
    is_older_than_day = get_calendar_turn:days() >= 1
    if(loaded_version < 2 && is_older_than_day) then --handle upgrading from original StatsThroughSkills to version 2
        game.add_msg("Migrating from version 1")

        str_bonus = get_stat_bonus_for_skills_version_1(str_skills)
        dex_bonus = get_stat_bonus_for_skills_version_1(dex_skills)
        int_bonus = get_stat_bonus_for_skills_version_1(int_skills)
        per_bonus = get_stat_bonus_for_skills_version_1(per_skills)

        player.str_max = player.str_max - str_bonus
        player.dex_max = player.dex_max - dex_bonus
        player.int_max = player.int_max - int_bonus
        player.per_max = player.per_max - per_bonus

        player:set_value("StatsThoughSkills", tostring(2))
    end
end

function get_stat_bonus_for_skills_version_1(skill_set)
    total_bonus = 0
    for _,v in ipairs(skill_set) do
        skill_level = player:get_skill_level(skill_id(skill))
        if (skill_level / 3 < 3) then
            stat_bonus = stat_bonus + skill_level / 3
        else
            stat_bonus = stat_bonus + 3
        end
        total_bonus = math.floor(stat_bonus)
    end
    return total_bonus
end