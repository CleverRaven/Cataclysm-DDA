-- Defines which attributes are exported by our C++ classes.

classes = {
    player = {
        attributes = {
            hunger = {
                type = "int",
                writable = true
            },
            thirst = {
                type = "int",
                writable = true
            },
            fatigue = {
                type = "int",
                writable = true
            },

            name = {
                type = "string",
                writable = false
            },

            male = {
                type = "bool",
                writable = false
            },

            str_cur = {
                type = "int",
                writable = true
            },
            dex_cur = {
                type = "int",
                writable = true
            },
            int_cur = {
                type = "int",
                writable = true
            },
            per_cur = {
                type = "int",
                writable = true
            },

            str_max = {
                type = "int",
                writable = true
            },
            dex_max = {
                type = "int",
                writable = true
            },
            int_max = {
                type = "int",
                writable = true
            },
            per_max = {
                type = "int",
                writable = true
            },

            stim = {
                type = "int",
                writable = true
            }
        },
        functions = {
            posx = {
                args = {},
                rval = "int"
            },
            posy = {
                args = {},
                rval = "int"
            },
            pos = {
                args = {},
                rval = "tripoint"
            },
            morale_level = {
                args = { },
                rval = "int"
            },
            is_npc = {
                args = {},
                rval = "bool"
            },
            weight_carried = {
                args = {},
                rval = "int"
            },
            weight_capacity = {
                args = {},
                rval = "int"
            },
            volume_carried = {
                args = {},
                rval = "int"
            },
            volume_capacity = {
                args = {},
                rval = "int"
            }
        }
    },
    item = {
        attributes = {
            fridge = { type = "int", writable = true },
            type = { type = "itype", writable = true },
            invlet = { type = "int", writable = true },
            charges = { type = "int", writable = true },
            active = { type = "bool", writable = true },
            damage = { type = "int", writable = true },
            burnt = { type = "int", writable = true },
            bday = { type = "int", writable = true },
            item_counter = { type = "int", writable = true },
            mission_id = { type = "int", writable = true },
            player_id = { type = "int", writable = true }
        },
        functions = {
            make_corpse = { rval = nil, args = { "mtype", "int" } },
            make_corpse = { rval = nil, args = { "mtype", "int", "string" } },
            make_corpse = { rval = nil, args = { "string", "int" } },
            make_corpse = { rval = nil, args = { } },
            get_mtype = { rval = "mtype", args = { } },
            set_mtype = { rval = nil, args = { "mtype" } },
            is_corpse = { rval = "bool", args = { } },
            init = { rval = nil, args = { } },
            make = { rval = nil, args = { "string" } },
            clear = { rval = nil, args = { } },
            color = { rval = "int", args = { "player" } },
            color_in_inventory = { rval = "int", args = { } },
            tname = { rval = "string", args = { } },
            display_name = { rval = "string", args = { "int" } },
            use = { rval = nil, args = { } },
            burn = { rval = "bool", args = { "int" } },
            reload_time = { rval = "int", args = { "player" } },
            clip_size = { rval = "int", args = { } },
            sight_dispersion = { rval = "int", args = { "int" } },
            aim_speed = { rval = "int", args = { "int" } },
            noise = { rval = "int", args = { } },
            burst_size = { rval = "int", args = { } },
            pick_reload_ammo = { rval = "int", args = { "player", "bool" } },
            reload = { rval = "bool", args = { "player", "int" } },
            skill = { rval = "string", args = { } },
            load_info = { rval = nil, args = { "string" } },
            info = { rval = "string", args = { "bool" } },
            symbol = { rval = "int", args = { } },
            color = { rval = "int", args = { } },
            price = { rval = "int", args = { } },
            butcher_factor = { rval = "int", args = { } },
            invlet_is_okay = { rval = "bool", args = { } },
            stacks_with = { rval = "bool", args = { "item" } },
            merge_charges = { rval = "bool", args = { "item" } },
            add_rain_to_container = { rval = nil, args = { "bool", "int" } },
            weight = { rval = "int", args = { } },
            precise_unit_volume = { rval = "int", args = { } },
            volume = { rval = "int", args = { "bool", "bool" } },
            volume_contained = { rval = "int", args = { } },
            attack_time = { rval = "int", args = { } },
            damage_bash = { rval = "int", args = { } },
            damage_cut = { rval = "int", args = { } },
            has_flag = { rval = "bool", args = { "string" } },
            contains_with_flag = { rval = "bool", args = { "string" } },
            has_quality = { rval = "bool", args = { "string" } },
            has_quality = { rval = "bool", args = { "string", "int" } },
            active_gunmod = { rval = "item", args = { } },
            active_gunmod = { rval = "item", args = { } },
            goes_bad = { rval = "bool", args = { } },
            is_going_bad = { rval = "bool", args = { } },
            count_by_charges = { rval = "bool", args = { } },
            craft_has_charges = { rval = "bool", args = { } },
            num_charges = { rval = "int", args = { } },
            reduce_charges = { rval = "bool", args = { "int" } },
            rotten = { rval = "bool", args = { } },
            calc_rot = { rval = nil, args = { "tripoint" } },
            has_rotten_away = { rval = "bool", args = { } },
            get_relative_rot = { rval = "float", args = { } },
            set_relative_rot = { rval = nil, args = { "float" } },
            get_rot = { rval = "int", args = { } },
            brewing_time = { rval = "int", args = { } },
            ready_to_revive = { rval = "bool", args = { "tripoint" } },
            detonate = { rval = nil, args = { "tripoint" } },
            can_revive = { rval = "bool", args = { } },
            getlight_emit = { rval = "int", args = { } },
            getlight_emit_active = { rval = "int", args = { } },
            weapon_value = { rval = "int", args = { "player" } },
            melee_value = { rval = "int", args = { "player" } },
            bash_resist = { rval = "int", args = { } },
            cut_resist = { rval = "int", args = { } },
            acid_resist = { rval = "int", args = { } },
            is_two_handed = { rval = "bool", args = { "player" } },
            made_of = { rval = "bool", args = { "string" } },
            conductive = { rval = "bool", args = { } },
            flammable = { rval = "bool", args = { } },
            already_used_by_player = { rval = "bool", args = { "player" } },
            mark_as_used_by_player = { rval = nil, args = { "player" } },
            process = { rval = "bool", args = { "player", "tripoint", "bool" } },
            reset_cable = { rval = nil, args = { "player" } },
            needs_processing = { rval = "bool", args = { } },
            processing_speed = { rval = "int", args = { } },
            process_artifact = { rval = "bool", args = { "player", "tripoint" } },
            get_free_mod_locations = { rval = "int", args = { "string" } },
            destroyed_at_zero_charges = { rval = "bool", args = { } },
            is_null = { rval = "bool", args = { } },
            is_food = { rval = "bool", args = { "player" } },
            is_food_container = { rval = "bool", args = { "player" } },
            is_food = { rval = "bool", args = { } },
            is_food_container = { rval = "bool", args = { } },
            is_ammo_container = { rval = "bool", args = { } },
            is_drink = { rval = "bool", args = { } },
            is_weap = { rval = "bool", args = { } },
            is_bashing_weapon = { rval = "bool", args = { } },
            is_cutting_weapon = { rval = "bool", args = { } },
            is_gun = { rval = "bool", args = { } },
            is_silent = { rval = "bool", args = { } },
            is_gunmod = { rval = "bool", args = { } },
            is_bionic = { rval = "bool", args = { } },
            is_ammo = { rval = "bool", args = { } },
            is_armor = { rval = "bool", args = { } },
            is_book = { rval = "bool", args = { } },
            is_container = { rval = "bool", args = { } },
            is_watertight_container = { rval = "bool", args = { } },
            is_salvageable = { rval = "bool", args = { } },
            is_disassemblable = { rval = "bool", args = { } },
            is_container_empty = { rval = "bool", args = { } },
            is_container_full = { rval = "bool", args = { } },
            is_emissive = { rval = "bool", args = { } },
            is_tool = { rval = "bool", args = { } },
            is_tool_reversible = { rval = "bool", args = { } },
            is_software = { rval = "bool", args = { } },
            is_var_veh_part = { rval = "bool", args = { } },
            is_artifact = { rval = "bool", args = { } },
            set_snippet = { rval = nil, args = { "string" } },
            get_remaining_capacity_for_liquid = { rval = "int", args = { "item" } },
            components_to_string = { rval = "string", args = { } },
            get_curammo = { rval = "itype", args = { } },
            has_curammo = { rval = "bool", args = { } },
            unset_curammo = { rval = nil, args = { } },
            set_curammo = { rval = nil, args = { "item" } },
            on_wear = { rval = nil, args = { "player" } },
            on_wield = { rval = nil, args = { "player" } },
            type_name = { rval = "string", args = { "int" } },
            liquid_charges = { rval = "int", args = { "int" } },
            liquid_units = { rval = "int", args = { "int" } },
            set_var = { rval = nil, args = { "string", "int" } },
            get_var = { rval = "int", args = { "string", "int" } },
            set_var = { rval = nil, args = { "string", "int" } },
            get_var = { rval = "int", args = { "string", "int" } },
            set_var = { rval = nil, args = { "string", "string" } },
            get_var = { rval = "string", args = { "string", "string" } },
            get_var = { rval = "string", args = { "string" } },
            has_var = { rval = "bool", args = { "string" } },
            erase_var = { rval = nil, args = { "string" } },
            is_seed = { rval = "bool", args = { } },
            get_plant_epoch = { rval = "int", args = { } },
            get_plant_name = { rval = "string", args = { } },
            get_warmth = { rval = "int", args = { } },
            get_thickness = { rval = "int", args = { } },
            get_coverage = { rval = "int", args = { } },
            get_encumber = { rval = "int", args = { } },
            get_storage = { rval = "int", args = { } },
            get_env_resist = { rval = "int", args = { } },
            is_power_armor = { rval = "bool", args = { } },
            get_chapters = { rval = "int", args = { } },
            get_remaining_chapters = { rval = "int", args = { "player" } },
            mark_chapter_as_read = { rval = nil, args = { "player" } },
            deactivate_charger_gun = { rval = "bool", args = { } },
            activate_charger_gun = { rval = "bool", args = { "player" } },
            update_charger_gun_ammo = { rval = "bool", args = { } },
            is_charger_gun = { rval = "bool", args = { } },
            is_auxiliary_gunmod = { rval = "bool", args = { } },
            is_in_auxiliary_mode = { rval = "bool", args = { } },
            set_auxiliary_mode = { rval = nil, args = { } },
            get_gun_mode = { rval = "string", args = { } },
            set_gun_mode = { rval = nil, args = { "string" } },
            next_mode = { rval = nil, args = { } },
            gun_range = { rval = "int", args = { "player" } },
            gun_range = { rval = "int", args = { "bool" } },
            gun_recoil = { rval = "int", args = { "bool" } },
            gun_damage = { rval = "int", args = { "bool" } },
            gun_pierce = { rval = "int", args = { "bool" } },
            gun_dispersion = { rval = "int", args = { "bool" } },
            gun_skill = { rval = "string", args = { } },
            weap_skill = { rval = "string", args = { } },
            spare_mag_size = { rval = "int", args = { } },
            get_usable_item = { rval = "item", args = { "string" } },
            quiver_store_arrow = { rval = "int", args = { "item" } },
            max_charges_from_flag = { rval = "int", args = { "string" } }
        }
    },
    point = {
        by_value = true,
        attributes = {
            x = {
                type = "int",
                writable = true
            },
            y = {
                type = "int",
                writable = true
            }
        },
        functions = {
        }
    },
    tripoint = {
        by_value = true,
        attributes = {
            x = {
                type = "int",
                writable = true
            },
            y = {
                type = "int",
                writable = true
            },
            z = {
                type = "int",
                writable = true
            }
        },
        functions = {
        }
    },
    uimenu = {
        attributes = {
            title = {
                type = "string",
                writable = true
            },
            selected = {
                type = "int",
                writable = false
            }
        },
        functions = {
            show = {
                args = {},
                rval = nil
            },
            query = {
                args = { "bool" },
                rval = nil
            },
            addentry = {
                args = { "string" },
                rval = nil
            }
        }
    },
    map = {
        attributes = {
        },
        functions = {
            ter = {
                args = {"int", "int"},
                rval = "int"
            },
            ter_set = {
                args = {"int", "int", "string"},
                rval = nil
            },
            ter_iset = {
                cpp_name = "ter_set",
                args = {"int", "int", "int"},
                rval = nil
            },

            furn = {
                args = {"int", "int"},
                rval = "int"
            },
            furn_set = {
                args = {"int", "int", "string"},
                rval = nil
            },
            line_ter = {
                cpp_name = "draw_line_ter",
                args = {"string", "int", "int", "int", "int"},
                rval = nil
            },
            line_furn = {
                cpp_name = "draw_line_furn",
                args = {"string", "int", "int", "int", "int"},
                rval = nil
            },
            fill_background = {
                cpp_name = "draw_fill_background",
                args = {"string"},
                rval = nil
            },
            square_ter = {
                cpp_name = "draw_square_ter",
                args = {"string", "int", "int", "int", "int"},
                rval = nil
            },
            square_furn = {
                cpp_name = "draw_square_furn",
                args = {"string", "int", "int", "int", "int"},
                rval = nil
            },
            rough_circle = {
                cpp_name = "draw_rough_circle",
                args = {"string", "int", "int", "int"},
                rval = nil
            },
            place_items = {
                args = {"string", "int", "int", "int", "int",  "int", "bool", "int"},
                rval = nil
            }
        }
    },
    ter_t = {
        attributes = {
            name = {
                type = "string",
                writable = false
            },
            id = {
                type = "string",
                writable = false
            },
            loadid = {
                type = "int",
                writable = false
            },
            movecost = {
                type = "int",
                writable = false
            }
        },
        functions = {
        }
    },
    Creature = {
        attributes = {

        },
        functions = {
            get_speed = {
                args = {},
                rval = "int"
            }
        }
    },
    monster = {
        parent = "Creature",
        attributes = {
            moves = {
                type = "int",
                writable = true
            },
            friendly = {
                type = "int",
                writable = true
            }
        },
        functions = {
            name = {
                args = {},
                rval = "string"
            },
            make_friendly = {
                args = {},
                rval = nil
            },
            get_hp = {
                args = {},
                rval = "int"
            },
            get_hp = {
                args = {},
                rval = "int"
            },
            pos = {
                args = {},
                rval = "tripoint"
            },
            setpos = {
                args = { "tripoint" },
                rval = nil
            },
            posx = {
                args = {},
                rval = "int"
            },
            posy = {
                args = {},
                rval = "int"
            }
        }
    },
    mtype = {
        attributes = {
            speed = {
                type = "int",
                writable = true
            },
            hp = {
                type = "int",
                writable = true
            },
            melee_skill = {
                type = "int",
                writable = true
            },
            melee_dice = {
                type = "int",
                writable = true
            },
            melee_sides = {
                type = "int",
                writable = true
            },
            melee_cut = {
                type = "int",
                writable = true
            },
            sk_dodge = {
                type = "int",
                writable = true
            },
            armor_bash = {
                type = "int",
                writable = true
            },
            armor_cut = {
                type = "int",
                writable = true
            },
            sk_dodge = {
                type = "int",
                writable = true
            },
            difficulty = {
                type = "int",
                writable = true
            },
            agro = {
                type = "int",
                writable = true
            },
            morale = {
                type = "int",
                writable = true
            },
            vision_day = {
                type = "int",
                writable = true
            },
            vision_night = {
                type = "int",
                writable = true
            },
        },
        functions = {
            in_category = {
                args = {"string"},
                rval = "bool"
            },
            in_species = {
                args = {"string"},
                rval = "bool"
            },
            has_flag = {
                args = {"string"},
                rval = "bool"
            },
            set_flag = {
                args = {"string", "bool"},
                rval = nil
            }
        }
    },
    mongroup = {
        attributes = {
            type = {
                type = "string",
                writable = true
            },
            posx = {
                type = "int",
                writable = true
            },
            posy = {
                type = "int",
                writable = true
            },
            posz = {
                type = "int",
                writable = true
            },
            tx = {
                type = "int",
                writable = false
            },
            ty = {
                type = "int",
                writable = false
            },
            dying = {
                type = "bool",
                writable = true
            },
            horde = {
                type = "bool",
                writable = true
            },
            diffuse = {
                type = "bool",
                writable = true
            },
            radius = {
                type = "int",
                writable = true
            },
            population = {
                type = "int",
                writable = true
            }
        },
        functions = {
            set_target = {
                args = {"int", "int"},
                rval = nil
            },
            inc_interest = {
                args = {"int"},
                rval = nil
            },
            dec_interest = {
                args = {"int"},
                rval = nil
            },
            set_interest = {
                args = {"int"},
                rval = nil
            }
        }
    },
    overmap = {
        attributes = {
        },
        functions = {
            get_bottom_border = {
                args = {},
                rval = "int"
            },
            get_top_border = {
                args = {},
                rval = "int"
            },
            get_left_border = {
                args = {},
                rval = "int"
            },
            get_right_border = {
                args = {},
                rval = "int"
            },
        }
    },
    itype = {
        attributes = {
            id = {
                type = "string",
                writable = false,
                desc = "The unique string identifier of the item type, as defined in the JSON."
            },
            description = {
                type = "string",
                writable = true
            },
            volume = {
                type = "int",
                writable = true
            },
            stack_size = {
                type = "int",
                writable = true
            },
            weight = {
                type = "int",
                writable = true
            },
            bashing = {
                type = "int",
                cpp_name = "melee_dam",
                writable = true
            },
            cutting = {
                type = "int",
                cpp_name = "melee_cut",
                writable = true
            },
            to_hit = {
                type = "int",
                cpp_name = "m_to_hit",
                writable = true
            },
            price = {
                type = "int",
                writable = true
            }
        },
        functions = {
            nname = {
                args = { "int" },
                argnames = { "quantity" },
                rval = "string",
                desc = "Get a translated name for the item with the given quantity."
            }
        }
    },
    it_comest = {
        parent = "itype",
        attributes = {
            quench = {
                type = "int",
                writable = true
            },
            nutrition = {
                type = "int",
                cpp_name = "nutr",
                writable = true
            },
            spoils_in = {
                type = "int",
                cpp_name = "spoils",
                writable = true
            },
            addiction_potential = {
                type = "int",
                cpp_name = "addict",
                writable = true
            },
            def_charges = {
                type = "int",
                writable = true
            },
            stim = {
                type = "int",
                writable = true
            },
            healthy = {
                type = "int",
                writable = true
            },
            brewtime = {
                type = "int",
                writable = true
            },
            fun = {
                type = "int",
                writable = true
            },
            tool = {
                type = "string",
                writable = true
            },
            comestible_type = {
                type = "string",
                cpp_name = "comesttype",
                writable = true
            }
        },
        functions = {
        }
    },
    it_tool = {
        parent = "itype",
        attributes = {
            ammo_id = {
                type = "string",
                writable = true
            },
            max_charges = {
                type = "int",
                writable = true
            },
            def_charges = {
                type = "int",
                writable = true
            },
            charges_per_use = {
                type = "int",
                writable = true
            },
            turns_per_charge = {
                type = "int",
                writable = true
            }
        },
        functions = {
        }
    }
}

global_functions = {
    add_msg = {
        cpp_name = "add_msg_wrapper",
        args     = { "string" },
        argnames = { "message" },
        rval = nil,
        desc = "Write a message to the game's standard message window."
    },
    popup = {
        cpp_name = "popup_wrapper",
        args = { "string" },
        rval = nil
    },
    string_input_popup = {
        cpp_name = "string_input_popup",
        args = { "string", "int", "string" },
        rval = "string"
    },
    create_uimenu = {
        cpp_name = "create_uimenu",
        args = {},
        rval = "uimenu"
    },
    get_terrain_type = {
        cpp_name = "get_terrain_type",
        args = {"int"},
        rval = "ter_t"
    },
    rng = {
        cpp_name = "rng",
        args = {"int", "int"},
        rval = "int"
    },
    one_in = {
        cpp_name = "one_in",
        args = {"int"},
        rval = "bool"
    },
    distance = {
        cpp_name = "rl_dist",
        args = {"int", "int", "int", "int"},
        rval = "int"
    },
    trig_dist = {
        cpp_name = "trig_dist",
        args = {"int", "int", "int", "int"},
        rval = "int"
    },
    remove_item = {
        cpp_name = "game_remove_item",
        args = {"int", "int", "item"},
        rval = nil
    },
    get_current_overmap = {
        cpp_name = "get_current_overmap",
        args = { },
        rval = "overmap"
    },
    add_item_to_group = {
        cpp_name = "item_controller->add_item_to_group",
        args = { "string", "string", "int" },
        rval = "bool"
    },
    get_comestible_type = {
        cpp_name = "get_comestible_type",
        args = { "string" },
        rval = "it_comest"
    },
    zombie = {
        cpp_name = "g->zombie",
        args = { "int" },
        rval = "monster",
        desc = "Returns a reference to the zombie of given index (starts at 0), use num_zombies to get the number of zombies. Parameters must be in the range [0, num_zombies-1]"
    },
    num_zombies = {
        cpp_name = "g->num_zombies",
        args = {},
        rval = "int",
        desc = "Returns the number of monsters currently existing in the reailty bubble."
    },
    create_monster = {
        cpp_name = "create_monster",
        args = { "string", "tripoint" },
        rval = "monster",
        desc = "Creates and spawns a new monster of given type. Returns a refernce to it, *or* nil if it could not be spawned."
    },
    get_tool_type = {
        cpp_name = "get_tool_type",
        args = { "string" },
        rval = "it_tool"
    }
}
