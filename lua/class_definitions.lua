-- Defines which attributes are exported by our C++ classes.

classes = {
    player = {
        attributes = {
            posx = {
                type = "int",
                writable = false
            },
            posy = {
                type = "int",
                writable = false
            },

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
            health = {
                type = "int",
                writable = false
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
            has_disease = {
                args = { "string" },
                rval = "bool"
            },
            rem_disease = {
                args = { "string" },
                rval = nil
            },
            add_disease = {
                args = { "string", "int", "int", "int" },
                rval = nil
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
                args = { "bool" },
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
            hp = {
                type = "int",
                writable = true
            },
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
            attack_at = {
                args = {"int", "int"},
                rval = "int"
            },
            make_friendly = {
                args = {},
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
            item_chance = {
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
        },
        functions = {
            in_category = {
                args = {"string"},
                rval = "bool"
            },
            in_species = {
                args = {"string"},
                rval = "bool"
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
            name = {
                type = "string",
                writable = true
            },
            description = {
                type = "string",
                writable = true
            },
            m1 = {
                type = "string",
                writable = true
            },
            m2 = {
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
            melee_dam = {
                type = "int",
                writable = true
            },
            melee_cut = {
                type = "int",
                writable = true
            },
            m_to_hit = {
                type = "int",
                writable = true
            },
            price = {
                type = "int",
                writable = true
            }
        },
        functions = {
        }
    },
    it_comest = {
        parent = "itype",
        attributes = {
            quench = {
                type = "int",
                writable = true
            },
            nutr = {
                type = "int",
                writable = true
            },
            spoils = {
                type = "int",
                writable = true
            },
            addict = {
                type = "int",
                writable = true
            },
            charges = {
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
            }
        },
        functions = {
        }
    },
    it_gun = {
        parent = "itype",
        attributes = {
            ammo = {
                type = "string",
                writable = true
            },
            dmg_bonus = {
                type = "int",
                writable = true
            },
            pierce = {
                type = "int",
                writable = true
            },
            range = {
                type = "int",
                writable = true
            },
            dispersion = {
                type = "int",
                writable = true
            },
            recoil = {
                type = "int",
                writable = true
            },
            durability = {
                type = "int",
                writable = true
            },
            burst = {
                type = "int",
                writable = true
            },
            clip = {
                type = "int",
                writable = true
            }
        },
        functions = {
        }
    },
    it_gunmod = {
        parent = "itype",
        attributes = {
            dispersion = {
                type = "int",
                writable = true
            },
            damage = {
                type = "int",
                writable = true
            },
            loudness = {
                type = "int",
                writable = true
            },
            clip = {
                type = "int",
                writable = true
            },
            recoil = {
                type = "int",
                writable = true
            },
            burst = {
                type = "int",
                writable = true
            },
            newtype = {
                type = "string",
                writable = true
            },
            location = {
                type = "string",
                writable = true
            }
        },
        functions = {
        }
    },
    it_tool = {
        parent = "itype",
        attributes = {
            ammo = {
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
        cpp_name = "g->add_msg",
        args = { "cstring" },
        rval = nil
    },
    revive_corpse = {
        cpp_name = "g->revive_corpse",
        args = { "int", "int", "int" },
        rval = nil
    },
    popup = {
        cpp_name = "popup",
        args = { "cstring" },
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
    create_monster_group = {
        cpp_name = "create_monster_group",
        args = {"overmap", "string", "int", "int", "int", "int", "int"},
        rval = "mongroup"
    },
    get_comestible_type = {
        cpp_name = "get_comestible_type",
        args = { "string" },
        rval = "it_comest"
    },
    get_gun_type = {
        cpp_name = "get_gun_type",
        args = { "string" },
        rval = "it_gun"
    },
    get_gunmod_type = {
        cpp_name = "get_gunmod_type",
        args = { "string" },
        rval = "it_gunmod"
    },
    get_tool_type = {
        cpp_name = "get_tool_type",
        args = { "string" },
        rval = "it_tool"
    },
    add_item_to_group = {
        cpp_name = "item_controller->add_item_to_group",
        args = { "string", "string", "int" },
        rval = "bool"
    }
}
