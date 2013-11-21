dofile("lua/class_definitions.lua")

function generate_metatable(name, class)
    return {
        __index = function(userdata, key)
            local attribute = class.attributes[key]
            if attribute then
                return game[name.."_get_"..key](userdata)
            elseif class.functions[key] then
                return game[name.."_"..key]
            else
                error("Unknown "..name.." attribute: "..key)
            end
        end,

        __newindex = function(userdata, key, value)
            local attribute = class.attributes[key]
            if attribute then
                if not attribute.writable then
                    error("Attempting to set read-only item attribute: "..key)
                end
                return game[name.."_set_"..key](userdata, value)
            else
                error("Unknown "..name.." attribute: "..key)
            end
        end,

        __gc = game.__gc
    }
end

item_metatable = generate_metatable("item", classes.item)
player_metatable = generate_metatable("player", classes.player)
uimenu_metatable = generate_metatable("uimenu", classes.uimenu)
map_metatable = generate_metatable("map", classes.map)
ter_t_metatable = generate_metatable("ter_t", classes.ter_t)
monster_metatable = generate_metatable("monster", classes.monster)

outdated_metatable = {
    __index = function(userdata, key)
        error("Attempt to access outdated gamedata.")
    end,

    __newindex = function(table, key, value)
        error("Attempt to access outdated gamedata.")
    end
}
