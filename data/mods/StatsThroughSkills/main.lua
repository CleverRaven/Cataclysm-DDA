--Increasing stats by skill
local MOD = {}

local version = 2

local str_skills = { "mechanics", "swimming", "bashing", "cutting", "melee", "throw" }
local dex_skills = { "driving", "survival", "tailor", "traps", "dodge", "stabbing", "unarmed" }
local int_skills = { "barter", "computer", "cooking", "electronics", "fabrication", "firstaid", "speech" }
local per_skills = { "archery", "gun", "launcher", "pistol", "rifle", "shotgun", "smg" }

mods["StatsThroughSkills"] = MOD

function MOD.on_new_player_created()
    game.add_msg("New player starting with StatsThroughSkills")
    player:set_value("StatsThroughSkills", tostring(version))
    calculate_bonuses()
end

function MOD.on_skill_increased()
    calculate_bonuses()
end

function calculate_bonuses()
    game.add_msg("Calculating new stats based off skills")

    handle_previous_version()

    local prev_str = player.str_max
    local prev_dex = player.dex_max
    local prev_int = player.int_max
    local prev_per = player.per_max

    remove_existing_bonuses()

    local str_bonus = calc_bonus(str_skills)
    local dex_bonus = calc_bonus(dex_skills)
    local int_bonus = calc_bonus(int_skills)
    local per_bonus = calc_bonus(per_skills)

    player:set_value("str_bonus", tostring(str_bonus))
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

    local hp_prev = {}
    for k,v in pairs(enums.hp_part) do
        if string.match(v, "num_hp_parts") == nil then
            hp_prev[v] = player:get_hp(v)
        end
    end

    player:recalc_hp()

    local hp_new = {}
    for k,v in pairs(enums.hp_part) do
        if string.match(v, "num_hp_parts") == nil then
            hp_new[v] = player:get_hp(v)
        end
    end

    for k,v in pairs(enums.hp_part) do
        if string.match(v, "num_hp_parts") == nil then
            if hp_prev[v] ~= hp_new[v] then
                player:heal(v, math.abs(hp_prev[v] - hp_new[v]))
            end
        end
    end

end

function remove_existing_bonuses()
    local str_bonus = tonumber(player:get_value("str_bonus"))
    local dex_bonus = tonumber(player:get_value("dex_bonus"))
    local int_bonus = tonumber(player:get_value("int_bonus"))
    local per_bonus = tonumber(player:get_value("per_bonus"))

    if(str_bonus) then player.str_max = player.str_max - str_bonus end
    if(dex_bonus) then player.dex_max = player.dex_max - dex_bonus end
    if(int_bonus) then player.int_max = player.int_max - int_bonus end
    if(per_bonus) then player.per_max = player.per_max - per_bonus end
end

function calc_bonus(skills_set)
    local skill_total = 0
    for _,s in ipairs(skills_set) do
        skill_total = skill_total + player:get_skill_level(skill_id(s))
    end

    return (skill_total > 3 and math.floor(math.pow((skill_total - 3), (1 / 2.46))) or 0)
end

function print_results(cur_stat,stat,prev_stat)
    if (prev_stat < cur_stat) then
        game.add_msg("Raising "..stat.." to "..tostring(cur_stat))
    elseif (prev_stat > cur_stat) then
        game.add_msg("Lowering "..stat.." to "..tostring(cur_stat))
    end
end

function handle_previous_version()
    local loaded_version = tonumber(player:get_value("StatsThroughSkills"))
    local character_eligible_for_migration = false
    local calendar = game.get_calendar_turn()
    if( function_exists( "calendar.days" ) ) then
        character_eligible_for_migration = calendar:days() >= 1
    else
        character_eligible_for_migration = calendar:years() >= 1
    end
    if( (not loaded_version or loaded_version < 2) and character_eligible_for_migration ) then --handle upgrading from original StatsThroughSkills to version 2
        game.add_msg("Migrating from version 1")

        local str_bonus = get_stat_bonus_for_skills_version_1(str_skills)
        local dex_bonus = get_stat_bonus_for_skills_version_1(dex_skills)
        local int_bonus = get_stat_bonus_for_skills_version_1(int_skills)
        local per_bonus = get_stat_bonus_for_skills_version_1(per_skills)

        player.str_max = player.str_max - str_bonus
        player.dex_max = player.dex_max - dex_bonus
        player.int_max = player.int_max - int_bonus
        player.per_max = player.per_max - per_bonus

        player:set_value("StatsThroughSkills", tostring(2))
    end
end

function get_stat_bonus_for_skills_version_1(skill_set)
    local total_bonus = 0
    for _,v in ipairs(skill_set) do
        local skill_level = player:get_skill_level(skill_id(v))
        local stat_bonus = 0
        if (skill_level / 3 < 3) then
            stat_bonus = stat_bonus + skill_level / 3
        else
            stat_bonus = stat_bonus + 3
        end
        total_bonus = total_bonus + math.floor(stat_bonus)
    end
    return total_bonus
end
