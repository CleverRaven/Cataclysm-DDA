#include "monster_factory.h"

monster_factory *monster_controller;

monster_factory::monster_factory()
{
    // Do Initialization of function lists!
    init();
    // init missing monster type
    m_missing_type = new mtype();
    m_missing_type->name = _("Error: Missing Monster Type");
    m_missing_type->description = _("There is only the space where a monster type should be, but isn't. No monster template of this type exists.");

    mon_templates["mon_null"] = m_missing_type;
}

monster_factory::~monster_factory()
{
    //dtor
}

void monster_factory::init()
{
    // init mdeath functions
    init_death_functions();

    // init mattack functions
    init_attack_functions();

    // init monster size list
    init_sizes();

    // init monster flags
    init_flags();

    // monster triggers
    init_triggers();
}

void monster_factory::init_death_functions()
{
    mdeath_function_list["NORMAL"] = &mdeath::normal;
    mdeath_function_list["ACID"] = &mdeath::acid;
    mdeath_function_list["BOOMER"] = &mdeath::boomer;
    mdeath_function_list["KILL_VINES"] = &mdeath::kill_vines;
    mdeath_function_list["VINE_CUT"] = &mdeath::vine_cut;
    mdeath_function_list["TRIFFID_HEART"] = &mdeath::triffid_heart;
    mdeath_function_list["FUNGUS"] = &mdeath::fungus;
    mdeath_function_list["FUNGUSAWAKE"] = &mdeath::fungusawake;
    mdeath_function_list["DISINTEGRATE"] = &mdeath::disintegrate;
    //mdeath_function_list["SHRIEK"] = &mdeath::shriek;
    mdeath_function_list["WORM"] = &mdeath::worm;
    mdeath_function_list["DISAPPEAR"] = &mdeath::disappear;
    mdeath_function_list["GUILT"] = &mdeath::guilt;
    mdeath_function_list["BLOBSPLIT"] = &mdeath::blobsplit;
    mdeath_function_list["MELT"] = &mdeath::melt;
    mdeath_function_list["AMIGARA"] = &mdeath::amigara;
    mdeath_function_list["THING"] = &mdeath::thing;
    mdeath_function_list["EXPLODE"] = &mdeath::explode;
    mdeath_function_list["RATKING"] = &mdeath::ratking;
    mdeath_function_list["KILL_BREATHERS"] = &mdeath::kill_breathers;
    mdeath_function_list["SMOKEBURST"] = &mdeath::smokeburst;
    mdeath_function_list["ZOMBIE"] = &mdeath::zombie;
    mdeath_function_list["GAMEOVER"] = &mdeath::gameover;
}

void monster_factory::init_attack_functions()
{
    mattack_function_list["NONE"] = &mattack::none;
    mattack_function_list["ANTQUEEN"] = &mattack::antqueen;
    mattack_function_list["SHRIEK"] = &mattack::shriek;
    mattack_function_list["ACID"] = &mattack::acid;
    mattack_function_list["SHOCKSTORM"] = &mattack::shockstorm;
    mattack_function_list["SMOKECLOUD"] = &mattack::smokecloud;
    mattack_function_list["BOOMER"] = &mattack::boomer;
    mattack_function_list["RESURRECT"] = &mattack::resurrect;
    mattack_function_list["SCIENCE"] = &mattack::science;
    mattack_function_list["GROWPLANTS"] = &mattack::growplants;
    mattack_function_list["GROW_VINE"] = &mattack::grow_vine;
    mattack_function_list["VINE"] = &mattack::vine;
    mattack_function_list["SPIT_SAP"] = &mattack::spit_sap;
    mattack_function_list["TRIFFID_HEARTBEAT"] = &mattack::triffid_heartbeat;
    mattack_function_list["FUNGUS"] = &mattack::fungus;
    mattack_function_list["FUNGUS_SPROUT"] = &mattack::fungus_sprout;
    mattack_function_list["LEAP"] = &mattack::leap;
    mattack_function_list["DERMATIK"] = &mattack::dermatik;
    mattack_function_list["PLANT"] = &mattack::plant;
    mattack_function_list["DISAPPEAR"] = &mattack::disappear;
    mattack_function_list["FORMBLOB"] = &mattack::formblob;
    mattack_function_list["DOGTHING"] = &mattack::dogthing;
    mattack_function_list["TENTACLE"] = &mattack::tentacle;
    mattack_function_list["VORTEX"] = &mattack::vortex;
    mattack_function_list["GENE_STING"] = &mattack::gene_sting;
    mattack_function_list["STARE"] = &mattack::stare;
    mattack_function_list["FEAR_PARALYZE"] = &mattack::fear_paralyze;
    mattack_function_list["PHOTOGRAPH"] = &mattack::photograph;
    mattack_function_list["TAZER"] = &mattack::tazer;
    mattack_function_list["SMG"] = &mattack::smg;
    mattack_function_list["FLAMETHROWER"] = &mattack::flamethrower;
    mattack_function_list["COPBOT"] = &mattack::copbot;
    mattack_function_list["MULTI_ROBOT"] = &mattack::multi_robot;
    mattack_function_list["RATKING"] = &mattack::ratking;
    mattack_function_list["GENERATOR"] = &mattack::generator;
    mattack_function_list["UPGRADE"] = &mattack::upgrade;
    mattack_function_list["BREATHE"] = &mattack::breathe;
    mattack_function_list["BITE"] = &mattack::bite;
    mattack_function_list["FLESH_GOLEM"] = &mattack::flesh_golem;
    mattack_function_list["PARROT"] = &mattack::parrot;
}

void monster_factory::init_sizes()
{
    mon_size_list["TINY"] = MS_TINY;
    mon_size_list["SMALL"] = MS_SMALL;
    mon_size_list["MEDIUM"] = MS_MEDIUM;
    mon_size_list["LARGE"] = MS_LARGE;
    mon_size_list["HUGE"] = MS_HUGE;
}

void monster_factory::init_flags()
{
    mon_flags["NULL"]= MF_NULL;
    mon_flags["REVIVES"] = MF_REVIVES;
    mon_flags["SEES"] = MF_SEES;
    mon_flags["VIS50"] = MF_VIS50;
    mon_flags["VIS40"] = MF_VIS40;
    mon_flags["VIS30"] = MF_VIS30;
    mon_flags["VIS20"] = MF_VIS20;
    mon_flags["VIS10"] = MF_VIS10;
    mon_flags["HEARS"] = MF_HEARS;
    mon_flags["GOODHEARING"] = MF_GOODHEARING;
    mon_flags["SMELLS"] = MF_SMELLS;
    mon_flags["KEENNOSE"] = MF_KEENNOSE;
    mon_flags["STUMBLES"] = MF_STUMBLES;
    mon_flags["WARM"] = MF_WARM;
    mon_flags["NOHEAD"] = MF_NOHEAD;
    mon_flags["HARDTOSHOOT"] = MF_HARDTOSHOOT;
    mon_flags["GRABS"] = MF_GRABS;
    mon_flags["BASHES"] = MF_BASHES;
    mon_flags["DESTROYS"] = MF_DESTROYS;
    mon_flags["POISON"] = MF_POISON;
    mon_flags["VENOM"] = MF_VENOM;
    mon_flags["BADVENOM"] = MF_BADVENOM;
    mon_flags["BLEED"] = MF_BLEED;
    mon_flags["WEBWALK"] = MF_WEBWALK;
    mon_flags["DIGS"] = MF_DIGS;
    mon_flags["FLIES"] = MF_FLIES;
    mon_flags["AQUATIC"] = MF_AQUATIC;
    mon_flags["SWIMS"] = MF_SWIMS;
    mon_flags["ATTACKMON"] = MF_ATTACKMON;
    mon_flags["ANIMAL"] = MF_ANIMAL;
    mon_flags["PLASTIC"] = MF_PLASTIC;
    mon_flags["SUNDEATH"] = MF_SUNDEATH;
    mon_flags["ELECTRIC"] = MF_ELECTRIC;
    mon_flags["ACIDPROOF"] = MF_ACIDPROOF;
    mon_flags["ACIDTRAIL"] = MF_ACIDTRAIL;
    mon_flags["SLUDGEPROOF"] = MF_SLUDGEPROOF;
    mon_flags["SLUDGETRAIL"] = MF_SLUDGETRAIL;
    mon_flags["FIREY"] = MF_FIREY;
    mon_flags["QUEEN"] = MF_QUEEN;
    mon_flags["ELECTRONIC"] = MF_ELECTRONIC;
    mon_flags["FUR"] = MF_FUR;
    mon_flags["LEATHER"] = MF_LEATHER;
    mon_flags["FEATHER"] = MF_FEATHER;
    mon_flags["CBM"] = MF_CBM;
    mon_flags["BONES"] = MF_BONES;
    mon_flags["IMMOBILE"] = MF_IMMOBILE;
    mon_flags["FRIENDLY_SPECIAL"] = MF_FRIENDLY_SPECIAL;
    mon_flags["HIT_AND_RUN"] = MF_HIT_AND_RUN;
    mon_flags["GUILT"] = MF_GUILT;
    mon_flags["HUMAN"] = MF_HUMAN;
    mon_flags["NO_BREATHE"] = MF_NO_BREATHE;
    mon_flags["REGENERATES_50"] = MF_REGENERATES_50;
    mon_flags["FLAMMABLE"] = MF_FLAMMABLE;
}

void monster_factory::init_triggers()
{
    mon_triggers["NULL"] = MTRIG_NULL;
    mon_triggers["STALK"] = MTRIG_STALK;
    mon_triggers["MEAT"] = MTRIG_MEAT;
    mon_triggers["PLAYER_WEAK"] = MTRIG_PLAYER_WEAK;
    mon_triggers["PLAYER_CLOSE"] = MTRIG_PLAYER_CLOSE;
    mon_triggers["HURT"] = MTRIG_HURT;
    mon_triggers["FIRE"] = MTRIG_FIRE;
    mon_triggers["FRIEND_DIED"] = MTRIG_FRIEND_DIED;
    mon_triggers["FRIEND_ATTACKED"] = MTRIG_FRIEND_ATTACKED;
    mon_triggers["SOUND"] = MTRIG_SOUND;
}

void monster_factory::init_phases()
{
    phase_map["NULL"] = PNULL;
    phase_map["SOLID"] = SOLID;
    phase_map["LIQUID"] = LIQUID;
    phase_map["GAS"] = GAS;
    phase_map["PLASMA"] = PLASMA;
}

void monster_factory::load_monster_templates() throw(std::string)
{
    try
    {
        load_monster_templates_from("data/raw/monsters.json");

        finalize_monsters();
    }
    catch (std::string &error_message)
    {
        throw;
    }
}

// monster json has following 'groups' to deal with:
//species: [{id:string, defaults...},...],
//categories: used for world gen option bitset to load monsters as needed
//monsters:[{id:string, everything else an mtype requires},...]
void monster_factory::load_monster_templates_from(const std::string filename) throw(std::string)
{
    catajson data(filename);

    if(! json_good())
    {
    	throw (std::string)"Could not open " + filename;
    }
    // load all of the species data!
    if (data.has("species"))
    {
        catajson all_species = data.get("species");

        for (all_species.set_begin(); all_species.has_curr(); all_species.next())
        {
            catajson entry = all_species.curr();

            if (!entry.has("id") || !entry.get("id").is_string())
            {
                // skip because malformed or missing its id
                continue;
            }

            species_type *new_species = new species_type;

            new_species->id = entry.get("id").as_string();

            if (entry.has("flags"))
            {
                tags_from_json(entry.get("flags"), new_species->flags);
            }
            if (entry.has("anger_triggers"))
            {
                tags_from_json(entry.get("anger_triggers"), new_species->anger_triggers);
            }
            if (entry.has("fear_triggers"))
            {
                tags_from_json(entry.get("fear_triggers"), new_species->fear_triggers);
            }
            if (entry.has("placate_triggers"))
            {
                tags_from_json(entry.get("placate_triggers"), new_species->placate_triggers);
            }
            // finally save the species data to the master list
            mon_species[new_species->id] = new_species;
        }
    }
    // load any categories! Not really necessary, will be used to control modes (probably)
    if (data.has("categories"))
    {
        catajson all_categories = data.get("categories");

        for (all_categories.set_begin(); all_categories.has_curr(); all_categories.next())
        {
            if (!all_categories.curr().is_string())
            {
                // skip, malformed
                continue;
            }
            mon_categories.push_back(all_categories.curr().as_string());
        }
    }
    if (data.has("monsters") && data.get("monsters").is_array())
    {
        // now the meat of the system!
        catajson all_monsters = data.get("monsters");

        // go through each monster
        for (all_monsters.set_begin(); all_monsters.has_curr(); all_monsters.next())
        {
            catajson entry = all_monsters.curr();

            if (!entry.has("id") || !entry.get("id").is_string())
            {
                // skip, malformed
                continue;
            }
            mtype *newmon = new mtype(entry.get("id").as_string());
            // get description
            newmon->description = _(entry.get("description").as_string());
            // get name
            newmon->name = _(entry.get("name").as_string());
            // get phase
            if (entry.has("phase") && entry.get("phase").is_string())
            {
                newmon->phase = phase_from_tag(entry.get("phase").as_string());
            }
            else
            {
                newmon->phase = SOLID;
            }
            // get size
            if (entry.has("size") && entry.get("size").is_string())
            {
                newmon->size = size_from_tag(entry.get("size").as_string());
            }
            else
            {
                newmon->size = MS_MEDIUM;
            }
            // get symbol
            newmon->sym = entry.get("symbol").as_char();
            // get color
            newmon->color = color_from_string(entry.get("color").as_string());
            // get material
            if (entry.has("material") && entry.get("material").is_string())
            {
                newmon->mat = entry.get("material").as_string();
            }
            else
            {
                newmon->mat = "null";
            }

            // get species(s?)
            std::set<std::string> m_species;
            if (entry.has("species"))
            {
                tags_from_json(entry.get("species"), m_species);
            }
            else
            {
                m_species.insert("UNKNOWN"); // no species given, so set to unknown!
            }
            set_species_defaults(m_species, newmon);

            // grab mon specific flags/triggers
            std::set<std::string> flags, angers, placates, fears;
            if (entry.has("flags"))
            {
                tags_from_json(entry.get("flags"), flags);
            }
            if (entry.has("anger_triggers"))
            {
                tags_from_json(entry.get("anger_triggers"), angers);
            }
            if (entry.has("placate_triggers"))
            {
                tags_from_json(entry.get("placate_triggers"), placates);
            }
            if (entry.has("fear_triggers"))
            {
                tags_from_json(entry.get("fear_triggers"), fears);
            }
            // apply mon flags/triggers
            set_monster_flags(flags, newmon->s_flags);
            set_monster_triggers(angers, newmon->s_anger);
            set_monster_triggers(placates, newmon->s_placate);
            set_monster_triggers(fears, newmon->s_fear);

            // get categories
            if (entry.has("categories"))
            {
                tags_from_json(entry.get("categories"), newmon->s_categories);
            }

            newmon->difficulty = entry.get("difficulty").as_int();
            newmon->agro = entry.get("aggression").as_int();
            newmon->morale = entry.get("morale").as_int();

            newmon->speed = entry.get("speed").as_int();
            newmon->melee_skill = entry.get("melee_skill").as_int();
            newmon->melee_dice = entry.get("melee_dice").as_int();
            newmon->melee_sides = entry.get("melee_dice_sides").as_int();
            newmon->melee_cut = entry.get("melee_cut").as_int();
            newmon->sk_dodge = entry.get("dodge_skill").as_int();
            newmon->armor_bash = entry.get("bash_resist").as_int();
            newmon->armor_cut = entry.get("cut_resist").as_int();
            newmon->item_chance = entry.get("item_drop_chance").as_int();
            newmon->hp = entry.get("hp").as_int();
            newmon->sp_freq = entry.get("special_frequency").as_int();

            if (entry.has("death_function") && entry.get("death_function").is_string())
            {
                if (mdeath_function_list.find(entry.get("death_function").as_string()) != mdeath_function_list.end())
                {
                    newmon->dies = mdeath_function_list[entry.get("death_function").as_string()];
                }
                else
                {
                    newmon->dies = mdeath_function_list["NORMAL"];
                }
            }
            else
            {
                newmon->dies = mdeath_function_list["NORMAL"];
            }

            if (entry.has("special_attack_function") && entry.get("special_attack_function").is_string())
            {
                if (mattack_function_list.find(entry.get("special_attack_function").as_string()) != mattack_function_list.end())
                {
                    newmon->sp_attack = mattack_function_list[entry.get("special_attack_function").as_string()];
                }
                else
                {
                    newmon->sp_attack = mattack_function_list["NONE"];
                }
            }
            else
            {
                newmon->sp_attack = mattack_function_list["NONE"];
            }

            // we have completed the definition of this mtype*, add it to the list!
            mon_templates[newmon->sid] = newmon;
        }
    }
}

void monster_factory::finalize_monsters()
{
    set_flags();
    set_triggers();
    set_bitflags();

}

void monster_factory::set_flags()
{
    for (std::map<Mon_Tag, mtype*>::iterator it = mon_templates.begin(); it != mon_templates.end(); ++it)
    {
        mtype *mon = it->second;

        for (std::map<Mon_Tag, m_flag>::iterator flag = mon->s_flags.begin(); flag != mon->s_flags.end(); ++flag)
        {
            mon->flags.push_back(flag->second);
        }
    }
}
void monster_factory::set_triggers()
{
    for (std::map<Mon_Tag, mtype*>::iterator it = mon_templates.begin(); it != mon_templates.end(); ++it)
    {
        mtype *mon = it->second;

        for (std::map<Mon_Tag, monster_trigger>::iterator trigger = mon->s_anger.begin(); trigger != mon->s_anger.end(); ++trigger)
        {
            mon->anger.push_back(trigger->second);
        }
        for (std::map<Mon_Tag, monster_trigger>::iterator trigger = mon->s_fear.begin(); trigger != mon->s_fear.end(); ++trigger)
        {
            mon->fear.push_back(trigger->second);
        }
        for (std::map<Mon_Tag, monster_trigger>::iterator trigger = mon->s_placate.begin(); trigger != mon->s_placate.end(); ++trigger)
        {
            mon->placate.push_back(trigger->second);
        }
    }
}
void monster_factory::set_bitflags()
{
    for (std::map<Mon_Tag, mtype*>::iterator it = mon_templates.begin(); it != mon_templates.end(); ++it)
    {
        mtype *mon = it->second;

        for (std::map<std::string, m_flag>::iterator flag = mon->s_flags.begin(); flag != mon->s_flags.end(); ++flag)
        {
            mon->bitflags[flag->second] = true;
        }
    }
}

void monster_factory::set_species_defaults(std::set<std::string> tags, mtype *mon)
{
    for (std::set<std::string>::iterator it = tags.begin(); it != tags.end(); ++it)
    {
        // check to make sure that the species given exists in the species map
        if (mon_species.find(*it) != mon_species.end())
        {
            species_type *species = mon_species[*it];

            mon->s_species.insert(species);

            for (std::set<std::string>::iterator flag = species->flags.begin(); flag != species->flags.end(); ++flag)
            {
                if (mon_flags.find(*flag) != mon_flags.end())
                {
                    mon->s_flags[*flag] = mon_flags[*flag];
                }
            }
            for (std::set<std::string>::iterator a_trig = species->anger_triggers.begin(); a_trig != species->anger_triggers.end(); ++a_trig)
            {
                if (mon_triggers.find(*a_trig) != mon_triggers.end())
                {
                    mon->s_anger[*a_trig] = mon_triggers[*a_trig];
                }
            }
            for (std::set<std::string>::iterator p_trig = species->placate_triggers.begin(); p_trig != species->placate_triggers.end(); ++p_trig)
            {
                if (mon_triggers.find(*p_trig) != mon_triggers.end())
                {
                    mon->s_placate[*p_trig] = mon_triggers[*p_trig];
                }
            }
            for (std::set<std::string>::iterator f_trig = species->fear_triggers.begin(); f_trig != species->fear_triggers.end(); ++f_trig)
            {
                if (mon_triggers.find(*f_trig) != mon_triggers.end())
                {
                    mon->s_fear[*f_trig] = mon_triggers[*f_trig];
                }
            }
        }
    }
}

m_size monster_factory::size_from_tag(Mon_Tag name)
{
    if (mon_size_list.find(name) != mon_size_list.end())
    {
        return mon_size_list[name];
    }
    return MS_MEDIUM;
}

phase_id monster_factory::phase_from_tag(Mon_Tag name)
{
    if (phase_map.find(name) != phase_map.end())
    {
        return phase_map[name];
    }
    return SOLID;
}


void monster_factory::set_monster_flags(std::set<std::string> tags, std::map<Mon_Tag, m_flag>& target)
{
    for (std::set<std::string>::iterator tag = tags.begin(); tag != tags.end(); ++tag)
    {
        std::string opword = *tag;
        if (opword[0] == '-')
        {
            // check to see if target contains the opword (sans '-')
            std::string s_op = opword.substr(1);
            if (target.find(s_op) != target.end())
            {
                target.erase(s_op);
            }
        }
        else
        {
            // only insert if we do not already have it, and the flag exists in the flags list!
            if (target.find(opword) == target.end() && mon_flags.find(opword) != mon_flags.end())
            {
                target[opword] = mon_flags[opword];
            }
        }
    }
}

void monster_factory::set_monster_triggers(std::set<std::string> tags, std::map<Mon_Tag, monster_trigger>& target)
{
    for (std::set<std::string>::iterator tag = tags.begin(); tag != tags.end(); ++tag)
    {
        std::string opword = *tag;
        if (opword[0] == '-')
        {
            // check to see if target contains the opword (sans '-')
            std::string s_op = opword.substr(1);
            if (target.find(s_op) != target.end())
            {
                target.erase(s_op);
            }
        }
        else
        {
            // only insert if we do not already have it, and the flag exists in the flags list!
            if (target.find(opword) == target.end() && mon_triggers.find(opword) != mon_triggers.end())
            {
                target[opword] = mon_triggers[opword];
            }
        }
    }
}


// copied wholesale from item_factory::tags_from_json
void monster_factory::tags_from_json(catajson tag_list, std::set<std::string> &tags)
{
    if (tag_list.is_array())
    {
        for (tag_list.set_begin(); tag_list.has_curr(); tag_list.next())
        {
            tags.insert( tag_list.curr().as_string() );
        }
    }
    else
    {
        //we should have gotten a string, if not an array, and catajson will do error checking
        tags.insert( tag_list.as_string() );
    }
}
