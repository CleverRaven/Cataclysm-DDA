function blacklist_item_from_spawns(id)
    local all_groups = game.get_item_groups()
    for _, group_id in ipairs(all_groups) do
        -- a chance of 0 removes the item
        game.add_item_to_group(group_id, id, 0)
    end
end

blacklist_item_from_spawns("jam_strawberries")
blacklist_item_from_spawns("jam_blueberries")

-- also remove rocks from spawn, since it's rather hard to test whether 
-- jams spawn or not ;) rock recipes should still exist though, leading
-- to hilarity
blacklist_item_from_spawns("rock")
