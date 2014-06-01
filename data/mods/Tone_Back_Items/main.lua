-- Nerf survivor armor to be jacks of all trades but masters of none.
-- Also give lightstrips a lot less charges.

-- commented stuff: masks already encumber enough
local armors = {
    --"mask_survivor",
    --"mask_survivorxl",
    "survivor_vest",
    "survivor_pack",
    "survivor_rucksack",
    "trenchcoat_survivor",
    "pants_survivor",
    "lsurvivor_suit",
    "survivor_suit",
    "hsurvivor_suit",
    "wsurvivor_suit",
    "boots_wsurvivor",
    "gloves_wsurvivor",
    "hood_wsurvivor",
    --"mask_wsurvivor",
    --"mask_wsurvivorxl",
    "fsurvivor_suit",
    "boots_fsurvivor",
    "gloves_fsurvivor",
    "hood_fsurvivor",
    --"mask_fsurvivor",
    --"mask_fsurvivorxl",
    "h20survivor_suit",
    "boots_h20survivor",
    "gloves_h20survivor",
    "hood_h20survivor",
    "boots_lsurvivor",
    "boots_survivor",
    "boots_hsurvivor",
    "gloves_lsurvivor",
    "gloves_survivor",
    "gloves_hsurvivor",
    "hood_lsurvivor",
    "hood_survivor",
    "helmet_survivor",
    "helmet_hsurvivor",
    "xlsurvivor_suit",
    "gloves_xlsurvivor",
    "boots_xlsurvivor",
    "hood_xlsurvivor",
    "helmet_xlsurvivor",
    --"mask_h20survivor",
    --"mask_h20survivor_on",
    --"mask_h20survivorxl",
    --"mask_h20survivorxl_on",
}

for _, i in ipairs(armors) do
    local a = game.get_armor_type(i)
    a.encumberance = a.encumberance + 1
    a.storage = a.storage * 7 / 10
    -- A little less protection.
    if a.material_thickness > 6 then
        a.material_thickness = a.material_thickness - 1
    end
    if a.material_thickness > 3 then
        a.material_thickness = a.material_thickness - 1
    end
end

-- Lightstrips consume 120 charges per day,
-- so with 8000 charges they last nearly 5 seasons!
-- Now they'll only last a little over a week.
local lightstrip = game.get_tool_type("lightstrip")
lightstrip.max_charges = 900
lightstrip.def_charges = 900
local lightstrip_inactive = game.get_tool_type("lightstrip_inactive")
lightstrip_inactive.max_charges = 900
lightstrip_inactive.def_charges = 900
