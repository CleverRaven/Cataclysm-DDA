local MOD = {}

mods["LUAPlayerTakeoff"] = MOD

function playertakeoff()
        if (player:i_at(-2):display_name() ~= "none") then
                game.add_msg("Taking off: "..player:i_at(-2):display_name())
                player:takeoff(player:i_at(-2))
        else
                game.add_msg("Nothing in that slot!")
        end
end

game.register_iuse("PLAYERTAKEOFF", playertakeoff)
