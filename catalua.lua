dofile("catalua/class_definitions.lua")

item_metatable = {
    __index = function(userdata, key)
        if key == "name" then
            return game.item_tname(userdata)
        end

    
        local attribute = item.attributes[key]
        if attribute then
            return game["item_get_"..key](userdata)
        elseif item.functions[key] then
            return game["item_"..key]
        else
            error("Unknown item attribute: "..key)
        end
    end,

    __newindex = function(userdata, key, value)
        local attribute = item.attributes[key]
        if attribute then
            if not attribute.writable then
                error("Attempting to set read-only item attribute: "..key)
            end
            return game["item_set_"..key](userdata, value)
        else
            error("Unknown item attribute: "..key)
        end
    end
}

player_metatable = {
    __index = function(userdata, key)
        local attribute = player.attributes[key]
        if attribute then
            return game["player_get_"..key](userdata)
        elseif player.functions[key] then
            return game["player_"..key]
        else
            error("Unknown player attribute: "..key)
        end
    end,

    __newindex = function(userdata, key, value)
        local attribute = player.attributes[key]
        if attribute then
            if not attribute.writable then
                error("Attempting to set read-only player attribute: "..key)
            end
            return game["player_set_"..key](userdata, value)
        else
            error("Unknown player attribute: "..key)
        end
    end
}

outdated_metatable = {
    __index = function(userdata, key)
        error("Attempt to access outdated gamedata.")
    end,

    __newindex = function(table, key, value)
        error("Attempt to access outdated gamedata.")
    end
}
