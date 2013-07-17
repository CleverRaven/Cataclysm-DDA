dofile("catalua/class_definitions.lua")

function create_metatable(classname, class)
    return {
        __index = function(userdata, key)
            local attribute = class.attributes[key]
            if attribute then
                return game[classname.."_get_"..key](userdata)
            elseif class.functions[key] then
                return game[classname.."_"..key]
            else
                error("Unknown "..classname.." attribute: "..key)
            end
        end,

        __newindex = function(userdata, key, value)
            local attribute = class.attributes[key]
            if attribute then
                if not attribute.writable then
                    error("Attempting to set read-only "..classname.." attribute: "..key)
                end
                return game[classname.."_set_"..key](userdata, value)
            else
                error("Unknown "..classname.." attribute: "..key)
            end
        end
    }
end

item_metatable = create_metatable("item", __metatables.item)
player_metatable = create_metatable("player", __metatables.player)

outdated_metatable = {
    __index = function(userdata, key)
        error("Attempt to access outdated gamedata.")
    end,

    __newindex = function(table, key, value)
        error("Attempt to access outdated gamedata.")
    end
}
