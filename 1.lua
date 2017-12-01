function AddMessage(message_text)

  print(message_text)
  game.add_msg(message_text)

end

function CheckItem(item, x, y, z, item_searchmask, omt_id)

  local item_display_name = item:display_name()
  --local item_weight = item.type.weight
  --local item_volume = item.type.volume:value()
  local item_birthday = item:birthday()
  local item_age = item:age()

  if (string.match(item_display_name, item_searchmask) ~= nil) then

    local message_text = string.format("Item: <color_blue>%s</color> [B:<color_green>%d</color>, A:<color_cyan>%d</color>] at [<color_red>%d</color>,<color_red>%d</color>,<color_red>%d</color>(<color_yellow>%s</color>)]", item_display_name, item_birthday, item_age, x, y, z, omt_id)
    AddMessage(message_text)

  end

end

function CheckMapItems(item_searchmask)

  local REALITY_BUBBLE_WIDTH = 24
  local ScanRadius = REALITY_BUBBLE_WIDTH * 5
  local center = player:pos()

  for x = -ScanRadius, ScanRadius do
    for y = -ScanRadius, ScanRadius do
      local z = 0 --only current Z-level
  
      local point = tripoint (center.x + x, center.y + y, center.z + z)
      local distance = game.trig_dist(point.x, point.y, center.x, center.y)     

      if (distance <= ScanRadius) then
        if map:i_at(point):size() > 0 then
          player:setpos(point)
		  local omt_id = "unknown"
		  if game.get_omt_id then
			omt_id = game.get_omt_id (g:get_cur_om(), player:global_omt_location())
		  end
          local item_stack_iterator =  map:i_at(point):cppbegin()
          for _ = 1, map:i_at(point):size() do
            CheckItem(item_stack_iterator:elem(), point.x, point.y, point.z, item_searchmask, omt_id)
            item_stack_iterator:inc()
          end
        end
      end
  
    end
  end

  player:setpos(center)

end

CheckMapItems("fresh")

local calendar = game.get_calendar_turn()
local calendar_turn = calendar:get_turn()
local message_text = string.format("Calendar turn: <color_yellow>%d</color>", calendar_turn)
AddMessage(message_text)
