function pick_from_list(list)
	local rand = math.random(1, #list)
	return list[rand]
end

function iuse_manhack(item, active)
	-- find a bunch of locations where we might spawn the manhack
	local locs = {}
	for delta_x = -1, 1 do
		for delta_y = -1, 1 do
			local point = { x = player.posx + delta_x, y = player.posy + delta_y }
			if game.is_empty(point.x, point.y) then
				table.insert(locs, point )
			end
		end
	end

	if #locs == 0 then
		game.add_msg("You can't use this here!")
		return 0 -- 0 charges used
	end

	-- okay, we got a bunch of locations, pick one and spawn a manhack there
	local loc = pick_from_list(locs)
	local monster = game.create_monster("mon_manhack", loc.x, loc.y)
	return 1 -- 1 charge used
end

game.register_iuse("IUSE_LUA_MANHACK", iuse_manhack)
