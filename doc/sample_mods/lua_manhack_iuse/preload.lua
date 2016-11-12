function pick_from_list(list)
	local rand = math.random(1, #list)
	return list[rand]
end

function iuse_manhack(item, active)
	-- find a bunch of locations where we might spawn the manhack
	local locs = {}
	for delta_x = -1, 1 do
		for delta_y = -1, 1 do
			local point = player:pos()
			point.x = point.x + delta_x
			point.y = point.y + delta_y
			if g:is_empty(point) then
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
	local monster = game.create_monster(mtype_id("mon_manhack"), loc)
	return 1 -- 1 charge used
end

game.register_iuse("IUSE_LUA_MANHACK", iuse_manhack)
