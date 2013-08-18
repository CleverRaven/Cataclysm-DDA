#ifndef TILE_ID_DATA
#define TILE_ID_DATA

#define toString(x) #x

const std::string terrain_names[num_terrain_types] =
{
    // t_null just because we need a value here to occupy index 0
    toString(t_null),
    // Real nothingness; makes you fall a z-level
    toString(t_hole),
    // Ground
    toString(t_dirt),
    toString(t_sand),
    toString(t_dirtmound),
    toString(t_pit_shallow),
    toString(t_pit),
    toString(t_pit_corpsed),
    toString(t_pit_covered),
    toString(t_pit_spiked),
    toString(t_pit_spiked_covered),
    toString(t_rock_floor),
    toString(t_rubble),
    toString(t_ash),
    toString(t_metal),
    toString(t_wreckage),
    toString(t_grass),
    toString(t_metal_floor),
    toString(t_pavement),
    toString(t_pavement_y),
    toString(t_sidewalk),
    toString(t_floor),
    toString(t_dirtfloor),//Dirt floor(Has roof)
    toString(t_grate),
    toString(t_slime),
    toString(t_bridge),
    // Lighting related
    toString(t_skylight),
    toString(t_emergency_light_flicker),
    toString(t_emergency_light),
    // Walls
    toString(t_wall_log_half),
    toString(t_wall_log),
    toString(t_wall_log_chipped),
    toString(t_wall_log_broken),
    toString(t_palisade),
    toString(t_palisade_gate),
    toString(t_palisade_gate_o),
    toString(t_wall_half),
    toString(t_wall_wood),
    toString(t_wall_wood_chipped),
    toString(t_wall_wood_broken),
    toString(t_wall_v),
    toString(t_wall_h),
    toString(t_concrete_v),
    toString(t_concrete_h),
    toString(t_wall_metal_v),
    toString(t_wall_metal_h),
    toString(t_wall_glass_v),
    toString(t_wall_glass_h),
    toString(t_wall_glass_v_alarm),
    toString(t_wall_glass_h_alarm),
    toString(t_reinforced_glass_v),
    toString(t_reinforced_glass_h),
    toString(t_bars),
    toString(t_door_c),
    toString(t_door_b),
    toString(t_door_o),
    toString(t_door_locked_interior),
    toString(t_door_locked),
    toString(t_door_locked_alarm),
    toString(t_door_frame),
    toString(t_chaingate_l),
    toString(t_fencegate_c),
    toString(t_fencegate_o),
    toString(t_chaingate_c),
    toString(t_chaingate_o),
    toString(t_door_boarded),
    toString(t_door_metal_c),
    toString(t_door_metal_o),
    toString(t_door_metal_locked),
    toString(t_door_bar_c),
    toString(t_door_bar_o),
    toString(t_door_bar_locked),
    toString(t_door_glass_c),
    toString(t_door_glass_o),
    toString(t_portcullis),
    toString(t_recycler),
    toString(t_window),
    toString(t_window_taped),
    toString(t_window_domestic),
    toString(t_window_domestic_taped),
    toString(t_window_open),
    toString(t_curtains),
    toString(t_window_alarm),
    toString(t_window_alarm_taped),
    toString(t_window_empty),
    toString(t_window_frame),
    toString(t_window_boarded),
    toString(t_window_stained_green),
    toString(t_window_stained_red),
    toString(t_window_stained_blue),
    toString(t_rock),
    toString(t_fault),
    toString(t_paper),
    // Tree
    toString(t_tree),
    toString(t_tree_young),
    toString(t_tree_apple),
    toString(t_underbrush),
    toString(t_shrub),
    toString(t_shrub_blueberry),
    toString(t_shrub_strawberry),
    toString(t_trunk),
    toString(t_root_wall),
    toString(t_wax),
    toString(t_floor_wax),
    toString(t_fence_v),
    toString(t_fence_h),
    toString(t_chainfence_v),
    toString(t_chainfence_h),
    toString(t_chainfence_posts),
    toString(t_fence_post),
    toString(t_fence_wire),
    toString(t_fence_barbed),
    toString(t_fence_rope),
    toString(t_railing_v),
    toString(t_railing_h),
    // Nether
    toString(t_marloss),
    toString(t_fungus),
    toString(t_tree_fungal),
    // Water, lava, etc.
    toString(t_water_sh),
    toString(t_water_dp),
    toString(t_water_pool),
    toString(t_sewage),
    toString(t_lava),
    // More embellishments than you can shake a stick at.
    toString(t_sandbox),
    toString(t_slide),
    toString(t_monkey_bars),
    toString(t_backboard),
    toString(t_gas_pump),
    toString(t_gas_pump_smashed),
    toString(t_generator_broken),
    toString(t_missile),
    toString(t_missile_exploded),
    toString(t_radio_tower),
    toString(t_radio_controls),
    toString(t_console_broken),
    toString(t_console),
    toString(t_gates_mech_control),
    toString(t_gates_control_concrete),
    toString(t_barndoor),
    toString(t_palisade_pulley),
    toString(t_sewage_pipe),
    toString(t_sewage_pump),
    toString(t_centrifuge),
    toString(t_column),
    toString(t_vat),
    // Staircases etc.
    toString(t_stairs_down),
    toString(t_stairs_up),
    toString(t_manhole),
    toString(t_ladder_up),
    toString(t_ladder_down),
    toString(t_slope_down),
     toString(t_slope_up),
    toString(t_rope_up),
    toString(t_manhole_cover),
    // Special
    toString(t_card_science),
    toString(t_card_military),
    toString(t_card_reader_broken),
    toString(t_slot_machine),
     toString(t_elevator_control),
    toString(t_elevator_control_off),
    toString(t_elevator),
    toString(t_pedestal_wyrm),
     toString(t_pedestal_temple),
    // Temple tiles
    toString(t_rock_red),
    toString(t_rock_green),
    toString(t_rock_blue),
    toString(t_floor_red),
    toString(t_floor_green),
    toString(t_floor_blue),
     toString(t_switch_rg),
    toString(t_switch_gb),
    toString(t_switch_rb),
    toString(t_switch_even)
};
const std::string trap_names[num_trap_types] =
{
    toString(tr_null),
    toString(tr_bubblewrap),
    toString(tr_cot),
    toString(tr_brazier),
    toString(tr_funnel),
    toString(tr_rollmat),
    toString(tr_beartrap),
    toString(tr_beartrap_buried),
    toString(tr_snare),
    toString(tr_nailboard),
    toString(tr_tripwire),
    toString(tr_crossbow),
    toString(tr_shotgun_2),
    toString(tr_shotgun_1),
    toString(tr_engine),
    toString(tr_blade),
    toString(tr_light_snare),
    toString(tr_heavy_snare),
    toString(tr_landmine),
    toString(tr_landmine_buried),
    toString(tr_telepad),
    toString(tr_goo),
    toString(tr_dissector),
    toString(tr_sinkhole),
    toString(tr_pit),
    toString(tr_spike_pit),
    toString(tr_lava),
    toString(tr_portal),
    toString(tr_ledge),
    toString(tr_boobytrap),
    toString(tr_temple_flood),
    toString(tr_temple_toggle),
    toString(tr_glow),
    toString(tr_hum),
    toString(tr_shadow),
    toString(tr_drain),
    toString(tr_snake)
};
const std::string field_names[num_fields] =
{
    toString(fd_null),
    toString(fd_blood),
    toString(fd_bile),
    toString(fd_gibs_flesh),
    toString(fd_gibs_veggy),
    toString(fd_web),
    toString(fd_slime),
    toString(fd_acid),
    toString(fd_sap),
    toString(fd_sludge),
    toString(fd_fire),
    toString(fd_rubble),
    toString(fd_smoke),
    toString(fd_toxic_gas),
    toString(fd_tear_gas),
    toString(fd_nuke_gas),
    toString(fd_gas_vent),
    toString(fd_fire_vent),
    toString(fd_flame_burst),
    toString(fd_electricity),
    toString(fd_fatigue),
    toString(fd_push_items),
    toString(fd_shock_vent),
    toString(fd_acid_vent)
};
const std::string monster_names[num_monsters] =
{
    toString(mon_null),

    // Wildlife
    toString(mon_squirrel),
    toString(mon_rabbit),
    toString(mon_deer),
    toString(mon_moose),
    toString(mon_wolf),
    toString(mon_coyote),
    toString(mon_bear),
    toString(mon_cougar),
    toString(mon_crow),

    // Friendly animals
    toString(mon_dog),
    toString(mon_cat),

    // Ants
    toString(mon_ant_larva),
    toString(mon_ant),
    toString(mon_ant_soldier),
    toString(mon_ant_queen),
    toString(mon_ant_fungus),

    // Bees
    toString(mon_fly),
    toString(mon_bee),
    toString(mon_wasp),

    // Worms
    toString(mon_graboid),
    toString(mon_worm),
    toString(mon_halfworm),

    // Wild Mutants
    toString(mon_sludge_crawler),

    // Zombies
    toString(mon_zombie),
    toString(mon_zombie_cop),
    toString(mon_zombie_shrieker),
    toString(mon_zombie_spitter),
    toString(mon_zombie_electric),

    toString(mon_zombie_smoker),

    toString(mon_zombie_fast),
    toString(mon_zombie_brute),
    toString(mon_zombie_hulk),
    toString(mon_zombie_fungus),

    toString(mon_boomer),
    toString(mon_boomer_fungus),
    toString(mon_skeleton),
    toString(mon_zombie_necro),

    toString(mon_zombie_scientist),
    toString(mon_zombie_soldier),
    toString(mon_zombie_grabber),

    toString(mon_zombie_master),
    toString(mon_beekeeper),
    toString(mon_zombie_child),

    // Flesh Golem
    toString(mon_jabberwock),

    // Triffids
    toString(mon_triffid),
    toString(mon_triffid_young),
    toString(mon_triffid_queen),
    toString(mon_creeper_hub),

    toString(mon_creeper_vine),
    toString(mon_biollante),
    toString(mon_vinebeast),
    toString(mon_triffid_heart),

    // Fungaloids
    toString(mon_fungaloid),
    toString(mon_fungaloid_dormant),
    toString(mon_fungaloid_young),
    toString(mon_spore),

    toString(mon_fungaloid_queen),
    toString(mon_fungal_wall),

    // Blobs
    toString(mon_blob),
    toString(mon_blob_small),

    // Sewer mutants
    toString(mon_chud),
    toString(mon_one_eye),
    toString(mon_crawler),

    // Sewer animals
    toString(mon_sewer_fish),
    toString(mon_sewer_snake),
    toString(mon_sewer_rat),
    toString(mon_rat_king),

    // Swamp monsters
    toString(mon_mosquito),
    toString(mon_dragonfly),
    toString(mon_centipede),
    toString(mon_frog),
    toString(mon_slug),

    toString(mon_dermatik_larva),
    toString(mon_dermatik),

    // SPIDERS
    toString(mon_spider_wolf),
    toString(mon_spider_web),
    toString(mon_spider_jumping),
    toString(mon_spider_trapdoor),

    toString(mon_spider_widow),

    // Unearthed Horrors
    toString(mon_dark_wyrm),
    toString(mon_amigara_horror),
    toString(mon_dog_thing),
    toString(mon_headless_dog_thing),

    toString(mon_thing),

    // Spiral monsters
    toString(mon_human_snail),
    toString(mon_twisted_body),
    toString(mon_vortex),

    // Subspace monsters
    toString(mon_flying_polyp),
    toString(mon_hunting_horror),
    toString(mon_mi_go),
    toString(mon_yugg),
    toString(mon_gelatin),

    toString(mon_flaming_eye),
    toString(mon_kreck),
    toString(mon_gracke),
    toString(mon_blank),
    toString(mon_gozu),
    toString(mon_shadow),
    toString(mon_breather_hub),

    toString(mon_breather),
    toString(mon_shadow_snake),

    // Cult, lobotomized creatures that are human/undead hybrids
    toString(mon_dementia),
    toString(mon_homunculus),
    toString(mon_blood_sacrifice),
    toString(mon_flesh_angel),

    // Robots
    toString(mon_eyebot),
    toString(mon_manhack),
    toString(mon_skitterbot),
    toString(mon_secubot),
    toString(mon_hazmatbot),
    toString(mon_copbot),
    toString(mon_molebot),

    toString(mon_tripod),
    toString(mon_chickenbot),
    toString(mon_tankbot),
    toString(mon_turret),
    toString(mon_exploder),

    // Hallucinations
    toString(mon_hallu_zom),
    toString(mon_hallu_bee),
    toString(mon_hallu_ant),
    toString(mon_hallu_mom),

    // Special monsters
    toString(mon_generator)
};
const std::string furn_names[num_furniture_types] =
{
    toString(f_null),
    toString(f_hay),
    toString(f_bulletin),
    toString(f_indoor_plant),
    toString(f_bed),
    toString(f_toilet),
    toString(f_makeshift_bed),
    toString(f_sink),
    toString(f_oven),
    toString(f_woodstove),
    toString(f_fireplace),
    toString(f_bathtub),
    toString(f_chair),
    toString(f_armchair),
    toString(f_sofa),
    toString(f_cupboard),
    toString(f_trashcan),
    toString(f_desk),
    toString(f_exercise),
    toString(f_bench),
    toString(f_table),
    toString(f_pool_table),
    toString(f_counter),
    toString(f_fridge),
    toString(f_glass_fridge),
    toString(f_dresser),
    toString(f_locker),
    toString(f_rack),
    toString(f_bookcase),
    toString(f_washer),
    toString(f_dryer),
    toString(f_dumpster),
    toString(f_crate_c),
    toString(f_crate_o),
    toString(f_canvas_wall),
    toString(f_canvas_door),
    toString(f_canvas_door_o),
    toString(f_groundsheet),
    toString(f_fema_groundsheet),
    toString(f_skin_wall),
    toString(f_skin_door),
    toString(f_skin_door_o),
    toString(f_skin_groundsheet),
    toString(f_mutpoppy)
};
const std::string veh_part_names[num_vparts] =
{
    toString(vp_null),

    // external parts
    toString(vp_seat),
    toString(vp_saddle),
    toString(vp_bed),
    toString(vp_frame_h),
    toString(vp_frame_v),
    toString(vp_frame_c),
    toString(vp_frame_y),
    toString(vp_frame_u),
    toString(vp_frame_n),//10
    toString(vp_frame_b),
    toString(vp_frame_h2),
    toString(vp_frame_v2),
    toString(vp_frame_cover),
    toString(vp_frame_handle),
    toString(vp_board_h),
    toString(vp_board_v),
    toString(vp_board_y),
    toString(vp_board_u),
    toString(vp_board_n),//20
    toString(vp_board_b),
    toString(vp_aisle_h2),
    toString(vp_aisle_v2),
    toString(vp_floor_trunk),
    toString(vp_roof),
    toString(vp_door),
    toString(vp_door_o),
    toString(vp_door_i),
    toString(vp_window),
    toString(vp_blade_h),//30
    toString(vp_blade_v),
    toString(vp_spike_h),

    toString(vp_wheel),
    toString(vp_wheel_wide),
    toString(vp_wheel_bicycle),
    toString(vp_wheel_motorbike),
    toString(vp_wheel_small),

    toString(vp_engine_gas_1cyl),
    toString(vp_engine_gas_v2),
    toString(vp_engine_gas_i4),//40
    toString(vp_engine_gas_v6),
    toString(vp_engine_gas_v8),
    toString(vp_engine_motor),
    toString(vp_engine_motor_large),
    toString(vp_engine_plasma),
    toString(vp_engine_foot_crank),
    toString(vp_fuel_tank_gas),
    toString(vp_fuel_tank_batt),
    toString(vp_fuel_tank_plut),
    toString(vp_fuel_tank_hydrogen),//50
    toString(vp_fuel_tank_water),
    toString(vp_cargo_trunk), // over
    toString(vp_cargo_box),   // over

    // pure internal parts
    toString(vp_controls),
    toString(vp_muffler),
    toString(vp_seatbelt),
    toString(vp_solar_panel),
    toString(vp_kitchen_unit),
    toString(vp_m249),
    toString(vp_flamethrower),//60
    toString(vp_plasmagun),

    // plating -- special case. mounted as internal, work as first line
    // of defence and gives color to external part
    toString(vp_steel_plate),
    toString(vp_superalloy_plate),
    toString(vp_spiked_plate),
    toString(vp_hard_plate),

    toString(vp_head_light)
};

#endif // TILE_ID_DATA
