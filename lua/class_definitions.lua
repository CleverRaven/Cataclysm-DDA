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
            charges = {
                type = "int",
                writable = true
            }
        },
        functions = {
            tname = {
                args = { },
                rval = "string"
            },
            made_of = {
                args = { "string" },
                rval = "bool"
            }
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
