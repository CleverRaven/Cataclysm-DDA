local milk = game.get_comestible_type("milk")

-- if milk spoils in less than a day, make it last at least a day
if milk.spoils < 24 then
    milk.spoils = 24
end

-- set the volume to half, just to test that inheritance really works
milk.volume = milk.volume / 2
