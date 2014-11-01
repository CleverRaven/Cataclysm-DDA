-- Main mod file that will be executed by the game.

function testui()
    local ui = game.create_uimenu()

    local selections = {"Foo", "Bar", "Baz", "Too many options"}
    for _, value in ipairs(selections) do
        ui:addentry(value)
    end
    ui:query()
    game.add_msg("You selected: "..selections[ui.selected+1])
end

function testter()
    local ter_id = map:ter(1, 1)
    local ter = game.get_terrain_type(ter_id)
    game.add_msg("Terrain name: "..ter.name)
end

function testmon()
    local monsters = game.get_monsters()
    game.add_msg("There are "..#monsters.." monsters loaded.")
    if #monsters > 0 then
        game.add_msg("The first of which is a "..monsters[1]:name())
    end
end

function makehungry(player, item, active)
    game.add_msg("You suddenly feel hungry.. Your hunger is now "..player.hunger)
    player.hunger = player.hunger + 10
end

monster_move = {}
function monster_move.dog(monster)
    monster.friendly = 1000
    local hp_percentage = 100 * monster.hp / monster:max_hp()
    local master_distance = game.distance(monster.posx, monster.posy, player.posx, player.posy)
    
    -- Is there edible meat at our current location?
    if monster.hp < monster:max_hp() then
        local items = game.items_at(monster.posx, monster.posy)
        for _, item in ipairs(items) do
            if item:made_of("flesh") then
                game.add_msg("The dog eats the "..item:tname())
                monster.hp = monster.hp + 30
                game.remove_item(monster.posx, monster.posy, item)
            end
            
            if monster.hp >= monster:max_hp() then
                break
            end
        end
    end
    
    -- Our offensiveness depends on our current health.
    local max_offensive_distance = math.floor(20 * hp_percentage / 100)
    local retreat_mode = false -- retreat mode means we're not fighting at all
                               -- except to protect our master
                               
    -- We're guarding the player so get the closest enemy to that.
    local closest_enemy = monster:get_closest_enemy_monster(player.posx, player.posy)
    
    local closest_enemy_strong = false
    if closest_enemy then
        -- todo: check for more things than just hp
        local factor = monster.hp * monster:max_hp() / (closest_enemy.hp * closest_enemy:max_hp())
        closest_enemy_strong = factor < 0.1
    end
                               
    -- a few possible conditions for retreat mode:
    -- 1. our master went missing, try to catch up to them
    -- 2. very low HP
    -- 3. closest enemy significantly stronger than self
    -- 4. too many enemies in area
    if hp_percentage <= 40 or (not monster:can_see(player.posx, player.posy)) or closest_enemy_strong or monster:get_num_visible_enemies() > 8 then
        retreat_mode = true
    end
    
    -- if the closest enemy is fighting the player right now, that overrides retreat mode
    if closest_enemy and game.distance(player.posx, player.posy, closest_enemy.posx, closest_enemy.posy) <= 1 then
        retreat_mode = false
    end
    
    -- retreat mode may make the dog whimper
    if retreat_mode and game.rng(0, 20) == 0 then
        game.add_msg("The dog whimpers.")
    end
    
    local distance 
    if closest_enemy then
        distance = game.distance(monster.posx, monster.posy, closest_enemy.posx, closest_enemy.posy)
    end
    
    if closest_enemy and distance < max_offensive_distance and not retreat_mode then
        -- maybe we can attack outright?
        if distance == 1 then
            monster:attack_at(closest_enemy.posx, closest_enemy.posy)
            monster.moves = monster.moves - 100
        else
            monster:step_towards(closest_enemy.posx, closest_enemy.posy)
        end
    -- possibly add: monster:can_see(player.posx, player.posy)
    -- but we'll just pretend the dog remembered where the player went
    elseif master_distance > 3 or retreat_mode then
        local ox = 0
        local oy = 0
        if game.rng(0, 3) == 0 then
            ox = game.rng(-1, 1)
            oy = game.rng(-1, 1)
        end
        monster:step_towards(player.posx + ox, player.posy + oy)
    else
        -- Avoid infinite loop
        monster.moves = monster.moves - 100
    end
end
