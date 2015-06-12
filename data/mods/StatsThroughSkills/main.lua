--Increasing stats by skill
local MOD = {}

mods["StatsThoughSkills"] = MOD

function MOD.on_day_passed()
    game.add_msg("Calculating new stats based off skills")
    int_bonus = 8
    str_bonus = 8
    dex_bonus = 8
    per_bonus = 8

    --based skills
    barter_skill = player:get_skill_level("barter")
    if (barter_skill >= 3) then
        int_bonus = int_bonus + 1
        if (barter_skill >= 6) then
            int_bonus = int_bonus + 1
            if (barter_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    computer_skill = player:get_skill_level("computer")
    if (computer_skill >= 3) then
        int_bonus = int_bonus + 1
        if (computer_skill >= 6) then
            int_bonus = int_bonus + 1
            if (computer_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    cooking_skill = player:get_skill_level("cooking")
    if (cooking_skill >= 3) then
        int_bonus = int_bonus + 1
        if (cooking_skill >= 6) then
            int_bonus = int_bonus + 1
            if (cooking_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    electronics_skill = player:get_skill_level("electronics")
    if (electronics_skill >= 3) then
        int_bonus = int_bonus + 1
        if (electronics_skill >= 6) then
            int_bonus = int_bonus + 1
            if (electronics_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    fabrication_skill = player:get_skill_level("fabrication")
    if (fabrication_skill >= 3) then
        int_bonus = int_bonus + 1
        if (fabrication_skill >= 6) then
            int_bonus = int_bonus + 1
            if (fabrication_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    firstaid_skill = player:get_skill_level("firstaid")
    if (firstaid_skill >= 3) then
        int_bonus = int_bonus + 1
        if (firstaid_skill >= 6) then
            int_bonus = int_bonus + 1
            if (firstaid_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    speech_skill = player:get_skill_level("speech")
    if (speech_skill >= 3) then
        int_bonus = int_bonus + 1
        if (speech_skill >= 6) then
            int_bonus = int_bonus + 1
            if (speech_skill >= 9) then
                int_bonus = int_bonus + 1
            end
        end
    end

    --Str based skills
    carpentry_skill = player:get_skill_level("carpentry")
    if (carpentry_skill >= 3) then
        str_bonus = str_bonus + 1
        if (carpentry_skill >= 6) then
            str_bonus = str_bonus + 1
            if (carpentry_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    mechanics_skill = player:get_skill_level("mechanics")
    if (mechanics_skill >= 3) then
        str_bonus = str_bonus + 1
        if (mechanics_skill >= 6) then
            str_bonus = str_bonus + 1
            if (mechanics_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    swimming_skill = player:get_skill_level("swimming")
    if (swimming_skill >= 3) then
        str_bonus = str_bonus + 1
        if (swimming_skill >= 6) then
            str_bonus = str_bonus + 1
            if (swimming_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    bashing_skill = player:get_skill_level("bashing")
    if (bashing_skill >= 3) then
        str_bonus = str_bonus + 1
        if (bashing_skill >= 6) then
            str_bonus = str_bonus + 1
            if (bashing_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    cutting_skill = player:get_skill_level("cutting")
    if (cutting_skill >= 3) then
        str_bonus = str_bonus + 1
        if (cutting_skill >= 6) then
            str_bonus = str_bonus + 1
            if (cutting_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    melee_skill = player:get_skill_level("melee")
    if (melee_skill >= 3) then
        str_bonus = str_bonus + 1
        if (melee_skill >= 6) then
            str_bonus = str_bonus + 1
            if (melee_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    throw_skill = player:get_skill_level("throw")
    if (throw_skill >= 3) then
        str_bonus = str_bonus + 1
        if (throw_skill >= 6) then
            str_bonus = str_bonus + 1
            if (throw_skill >= 9) then
                str_bonus = str_bonus + 1
            end
        end
    end

    --Dex based skills
    driving_skill = player:get_skill_level("driving")
    if (driving_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (driving_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (driving_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    survival_skill = player:get_skill_level("survival")
    if (survival_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (survival_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (survival_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    tailor_skill = player:get_skill_level("tailor")
    if (tailor_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (tailor_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (tailor_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    traps_skill = player:get_skill_level("traps")
    if (traps_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (traps_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (traps_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    dodge_skill = player:get_skill_level("dodge")
    if (dodge_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (dodge_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (dodge_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    stabbing_skill = player:get_skill_level("stabbing")
    if (stabbing_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (stabbing_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (stabbing_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    unarmed_skill = player:get_skill_level("unarmed")
    if (unarmed_skill >= 3) then
        dex_bonus = dex_bonus + 1
        if (unarmed_skill >= 6) then
            dex_bonus = dex_bonus + 1
            if (unarmed_skill >= 9) then
                dex_bonus = dex_bonus + 1
            end
        end
    end

    --Per based skills
    archery_skill = player:get_skill_level("archery")
    if (archery_skill >= 3) then
        per_bonus = per_bonus + 1
        if (archery_skill >= 6) then
            per_bonus = per_bonus + 1
            if (archery_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

    gun_skill = player:get_skill_level("gun")
    if (gun_skill >= 3) then
        per_bonus = per_bonus + 1
        if (gun_skill >= 6) then
            per_bonus = per_bonus + 1
            if (gun_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

    launcher_skill = player:get_skill_level("launcher")
    if (launcher_skill >= 3) then
        per_bonus = per_bonus + 1
        if (launcher_skill >= 6) then
            per_bonus = per_bonus + 1
            if (launcher_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

    pistol_skill = player:get_skill_level("pistol")
    if (pistol_skill >= 3) then
        per_bonus = per_bonus + 1
        if (pistol_skill >= 6) then
            per_bonus = per_bonus + 1
            if (pistol_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

    rifle_skill = player:get_skill_level("rifle")
    if (rifle_skill >= 3) then
        per_bonus = per_bonus + 1
        if (rifle_skill >= 6) then
            per_bonus = per_bonus + 1
            if (rifle_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

    shotgun_skill = player:get_skill_level("shotgun")
    if (shotgun_skill >= 3) then
        per_bonus = per_bonus + 1
        if (shotgun_skill >= 6) then
            per_bonus = per_bonus + 1
            if (shotgun_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

    smg_skill = player:get_skill_level("smg")
    if (smg_skill >= 3) then
        per_bonus = per_bonus + 1
        if (smg_skill >= 6) then
            per_bonus = per_bonus + 1
            if (smg_skill >= 9) then
                per_bonus = per_bonus + 1
            end
        end
    end

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
    if player:has_trait("STR_UP2") then str_bonus = str_bonus + 2 end
    if player:has_trait("STR_UP3") then str_bonus = str_bonus + 4 end
    if player:has_trait("STR_UP4") then str_bonus = str_bonus + 7 end
    if player:has_trait("DEX_UP") then dex_bonus = dex_bonus + 1 end
    if player:has_trait("DEX_UP2") then dex_bonus = dex_bonus + 2 end
    if player:has_trait("DEX_UP3") then dex_bonus = dex_bonus + 4 end
    if player:has_trait("DEX_UP4") then dex_bonus = dex_bonus + 7 end
    if player:has_trait("INT_UP") then int_bonus = int_bonus + 1 end
    if player:has_trait("INT_UP2") then int_bonus = int_bonus + 2 end
    if player:has_trait("INT_UP3") then int_bonus = int_bonus + 4 end
    if player:has_trait("INT_UP4") then int_bonus = int_bonus + 7 end
    if player:has_trait("PER_UP") then per_bonus = per_bonus + 1 end
    if player:has_trait("PER_UP2") then per_bonus = per_bonus + 2 end
    if player:has_trait("PER_UP3") then per_bonus = per_bonus + 4 end
    if player:has_trait("PER_UP4") then per_bonus = per_bonus + 7 end
    if player:has_trait("PER_SLIME") then per_bonus = per_bonus - 8 end
    if player:has_trait("PER_SLIME_OK") then per_bonus = per_bonus + 5 end

    if (player.int_max < int_bonus) then
        game.add_msg("Raising Int to "..tostring(int_bonus))
    elseif (player.int_max > int_bonus) then
        game.add_msg("Lowering Int to "..tostring(int_bonus))
    end
    player.int_max = int_bonus

    if (player.str_max < str_bonus) then
        game.add_msg("Raising Str to "..tostring(str_bonus))
    elseif (player.str_max > str_bonus) then
        game.add_msg("Lowering Str to "..tostring(str_bonus))
    end
    player.str_max = str_bonus

    if (player.dex_max < dex_bonus) then
        game.add_msg("Raising Dex to "..tostring(dex_bonus))
    elseif (player.dex_max > dex_bonus) then
        game.add_msg("Lowering Dex to "..tostring(dex_bonus))
    end
    player.dex_max = dex_bonus

    if (player.per_max < per_bonus) then
        game.add_msg("Raising Per to "..tostring(per_bonus))
    elseif (player.per_max > per_bonus) then
        game.add_msg("Lowering Per to "..tostring(per_bonus))
    end
    player.per_max = per_bonus

    player:recalc_hp()
end
