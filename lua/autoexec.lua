--dofile("./class_definitions.lua")

function generate_metatable(name)
    return {
        __index = function(userdata, key)
            -- iterate over the class inheritance hierarchy
            local current_name = name
            while current_name do
                local class = classes[current_name]
                local attribute = class.attributes[key]
                if attribute then
                    return game[name.."_get_"..key](userdata)
                elseif class.functions[key] then
                    return game[name.."_"..key]
                else
                    current_name = class.parent
                end
            end
            error("Unknown "..name.." attribute: "..key)
        end,

        __newindex = function(userdata, key, value)
            -- iterate over the class inheritance hierarchy
            local current_name = name
            while current_name do
                local class = classes[current_name]
                local attribute = class.attributes[key]
                if attribute then
                    if not attribute.writable then
                        error("Attempting to set read-only item attribute: "..key)
                    end
                    return game[name.."_set_"..key](userdata, value)
                else
                    current_name = class.parent
                end
            end
            error("Unknown "..name.." attribute: "..key)
        end,

        __gc = game.__gc
    }
end

for key, _ in pairs(classes) do
    _G[key.."_metatable"] = generate_metatable(key)
end

outdated_metatable = {
    __index = function(userdata, key)
        error("Attempt to access outdated gamedata.")
    end,

    __newindex = function(table, key, value)
        error("Attempt to access outdated gamedata.")
    end
}

-- table containing our mods
mods = { }

function mod_callback(callback_name)
    for modname, mod_instance in pairs(mods) do
        if type(mod_instance[callback_name]) == "function" then
            mod_instance[callback_name]()
        end
    end
end
