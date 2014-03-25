local json = require("lua/dkjson")
local lfs = require("lfs")

local exit_code = 0

-- function to read a file completely into a string
function read_file(filename)
    local f = io.open(filename, "r")
    local content = f:read("*all")
    f:close()
    return content
end

-- parse the JSON of an entire cataclysm file
function parse_cata_json(filename, handler)
    local content = read_file(filename)

    local root, pos, err = json.decode(content, 1, nil)
    if err then
        print("Error in ", filename ,":", err)
        os.exit(1)
    else
        -- top level should be a json array
        if type(root) ~= "table" then
            print("Wrong root element to JSON file ", filename, " :", type(root))
        end

        for _, entry in ipairs(root) do
            if not entry.type then
                print("Invalid entry type in ", filename, ": ", entry.type)
            end
            if handler[entry.type] then
                handler[entry.type](entry, filename)
            end
        end
    end
end

local definitions = {}
function load_item_definition(entry, filename)
    -- store that this item was defined
    definitions[entry.id] = true
end

-- define load_definition handlers
local load_definition = {}
load_definition.BOOK        = load_item_definition
load_definition.TOOL        = load_item_definition
load_definition.GUN         = load_item_definition
load_definition.GUNMOD      = load_item_definition
load_definition.TOOL_ARMOR  = load_item_definition
load_definition.ARMOR       = load_item_definition
load_definition.BIONIC_ITEM = load_item_definition
load_definition.GENERIC     = load_item_definition
load_definition.AMMO        = load_item_definition
load_definition.CONTAINER   = load_item_definition
load_definition.COMESTIBLE  = load_item_definition

local recipe_handler = {}

function ensure_definition(id, filename, parent_id)
    if not definitions[id] then
        -- signify that something went wrong
        exit_code = 1

        print("Trying to access non-existent item id ", id, " in ", filename, "(", parent_id, ")")
    end
end

recipe_handler.recipe = function(entry, filename)
    ensure_definition(entry.result, filename, entry.result)
    for _, alternatives in ipairs(entry.components) do
        for _, item in ipairs(alternatives) do
            ensure_definition(item[1], filename, entry.result)
        end
    end
end

function string.endswith(mystr, myend)
   return myend=="" or string.sub(mystr,string.len(mystr)-string.len(myend)+1)==myend
end


function load_all_jsons_recursive(path, handler)
    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            local f = path..'/'..file

            local attr = lfs.attributes(f)
            if attr.mode == "directory" then
                load_all_jsons_recursive(f, handler)
            elseif attr.mode == "file" and string.endswith(f, ".json") then
                parse_cata_json(f, handler)
            end
        end
    end
end

function load_all_jsons(handler)
    load_all_jsons_recursive("data/json", handler)
    load_all_jsons_recursive("data/mods", handler)
end

-- first load all item definitions
load_all_jsons(load_definition)

-- then verify recipes
load_all_jsons(recipe_handler)

os.exit(exit_code)
