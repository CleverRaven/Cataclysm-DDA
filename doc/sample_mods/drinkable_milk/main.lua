local milk = game.get_comestible_type("milk")

-- if milk spoils in less than a day, make it last at least a day
if milk.spoils_in < 24 then
    milk.spoils_in = 24
end

-- set the volume to half, just to test that inheritance really works
milk.volume = milk.volume / 2

-- access a few other things to test the wrappers
local pipe_rifle = game.get_gun_type("rifle_22")
pipe_rifle.ammo = "battery" -- hell yeah

local welder = game.get_tool_type("welder")
welder.max_charges = 1000 -- why not?
welder.def_charges = 500
