--dofile("./class_definitions.lua")

-- table containing our mods
mods = { }

function mod_callback(callback_name)
    for modname, mod_instance in pairs(mods) do
        if type(mod_instance[callback_name]) == "function" then
            mod_instance[callback_name]()
        end
    end
end
