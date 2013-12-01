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
                writable = false
            },
            dex_cur = {
                type = "int",
                writable = false
            },
            int_cur = {
                type = "int",
                writable = false
            },
            per_cur = {
                type = "int",
                writable = false
            },
            
            str_max = {
                type = "int",
                writable = false
            },
            dex_max = {
                type = "int",
                writable = false
            },
            int_max = {
                type = "int",
                writable = false
            },
            per_max = {
                type = "int",
                writable = false
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
                args = { "game" },
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
    monster = {
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
    }
        
}

global_functions = {
    add_msg = {
        cpp_name = "g->add_msg",
        args = { "cstring" },
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
}
