#include "monster_factory.h"

#include "debug.h"

// Constructor
monster_factory::monster_factory()
{
    init();

    missing_monster_type = new mtype();
    missing_monster_type->name = _("Error: Missing Monster Type");
    missing_monster_type->description = _("There is only the space where a monster type should be, but isn't. No monster template of this type exists.");

    monsters["mon_null"] = missing_monster_type;

    missing_species_type = new species_type();
    missing_species_type->id = "NULL";
    species["NULL_SPECIES"] = missing_species_type;
}

void monster_factory::finalize_monsters()
{
    // matches up species temp values in mtypes, and sets bitflags

}

// Initializers
void monster_factory::init()
{
    init_death_functions();
    init_specattack_functions();
    init_sizes();
    init_flags();
    init_triggers();
    init_phases();
}
void monster_factory::init_death_functions()
{
    death_functions["NORMAL"] = &mdeath::normal;
    death_functions["ACID"] = &mdeath::acid;
    death_functions["BOOMER"] = &mdeath::boomer;
    death_functions["KILL_VINES"] = &mdeath::kill_vines;
    death_functions["VINE_CUT"] = &mdeath::vine_cut;
    death_functions["TRIFFID_HEART"] = &mdeath::triffid_heart;
    death_functions["FUNGUS"] = &mdeath::fungus;
    death_functions["FUNGUSAWAKE"] = &mdeath::fungusawake;
    death_functions["DISINTEGRATE"] = &mdeath::disintegrate;
    //death_functions["SHRIEK"] = &mdeath::shriek; // has no implementation!
    death_functions["WORM"] = &mdeath::worm;
    death_functions["DISAPPEAR"] = &mdeath::disappear;
    death_functions["GUILT"] = &mdeath::guilt;
    death_functions["BLOBSPLIT"] = &mdeath::blobsplit;
    death_functions["MELT"] = &mdeath::melt;
    death_functions["AMIGARA"] = &mdeath::amigara;
    death_functions["THING"] = &mdeath::thing;
    death_functions["EXPLODE"] = &mdeath::explode;
    death_functions["RATKING"] = &mdeath::ratking;
    death_functions["KILL_BREATHERS"] = &mdeath::kill_breathers;
    death_functions["SMOKEBURST"] = &mdeath::smokeburst;
    death_functions["ZOMBIE"] = &mdeath::zombie;

    death_functions["GAMEOVER"] = &mdeath::gameover;//
}
void monster_factory::init_specattack_functions()
{
    special_attack_functions["NONE"] = &mattack::none;
    special_attack_functions["ANTQUEEN"] = &mattack::antqueen;
    special_attack_functions["SHRIEK"] = &mattack::shriek;
    special_attack_functions["ACID"] = &mattack::acid;
    special_attack_functions["SHOCKSTORM"] = &mattack::shockstorm;
    special_attack_functions["SMOKECLOUD"] = &mattack::smokecloud;
    special_attack_functions["BOOMER"] = &mattack::boomer;
    special_attack_functions["RESURRECT"] = &mattack::resurrect;
    special_attack_functions["SCIENCE"] = &mattack::science;
    special_attack_functions["GROWPLANTS"] = &mattack::growplants;
    special_attack_functions["GROW_VINE"] = &mattack::grow_vine;
    special_attack_functions["VINE"] = &mattack::vine;
    special_attack_functions["SPIT_SAP"] = &mattack::spit_sap;
    special_attack_functions["TRIFFID_HEARTBEAT"] = &mattack::triffid_heartbeat;
    special_attack_functions["FUNGUS"] = &mattack::fungus;
    special_attack_functions["FUNGUS_SPROUT"] = &mattack::fungus_sprout;
    special_attack_functions["LEAP"] = &mattack::leap;
    special_attack_functions["DERMATIK"] = &mattack::dermatik;
    special_attack_functions["PLANT"] = &mattack::plant;
    special_attack_functions["DISAPPEAR"] = &mattack::disappear;
    special_attack_functions["FORMBLOB"] = &mattack::formblob;
    special_attack_functions["DOGTHING"] = &mattack::dogthing;
    special_attack_functions["TENTACLE"] = &mattack::tentacle;
    special_attack_functions["VORTEX"] = &mattack::vortex;
    special_attack_functions["GENE_STING"] = &mattack::gene_sting;
    special_attack_functions["STARE"] = &mattack::stare;
    special_attack_functions["FEAR_PARALYZE"] = &mattack::fear_paralyze;
    special_attack_functions["PHOTOGRAPH"] = &mattack::photograph;
    special_attack_functions["TAZER"] = &mattack::tazer;
    special_attack_functions["SMG"] = &mattack::smg;
    special_attack_functions["FLAMETHROWER"] = &mattack::flamethrower;
    special_attack_functions["COPBOT"] = &mattack::copbot;
    special_attack_functions["MULTI_ROBOT"] = &mattack::multi_robot;
    special_attack_functions["RATKING"] = &mattack::ratking;
    special_attack_functions["GENERATOR"] = &mattack::generator;
    special_attack_functions["UPGRADE"] = &mattack::upgrade;
    special_attack_functions["BREATHE"] = &mattack::breathe;
    special_attack_functions["BITE"] = &mattack::bite;
    special_attack_functions["FLESH_GOLEM"] = &mattack::flesh_golem;
    special_attack_functions["PARROT"] = &mattack::parrot;
}
void monster_factory::init_sizes()
{
    size_tags["TINY"]=MS_TINY;// = 0, // Rodent
    size_tags["SMALL"]=MS_SMALL;// // Half human
    size_tags["MEDIUM"]=MS_MEDIUM;// // Human
    size_tags["LARGE"]=MS_LARGE;// // Cow
    size_tags["HUGE"]=MS_HUGE;//  // TAAAANK
}
void monster_factory::init_flags()
{
    flag_tags["NULL"]=MF_NULL;// = 0, // Helps with setvector
    flag_tags["SEES"]=MF_SEES;// // It can see you (and will run/follow)
    flag_tags["VIS50"]=MF_VIS50;// //Vision -10
    flag_tags["VIS40"]=MF_VIS40;// //Vision -20
    flag_tags["VIS30"]=MF_VIS30;// //Vision -30
    flag_tags["VIS20"]=MF_VIS20;// //Vision -40
    flag_tags["VIS10"]=MF_VIS10;// //Vision -50
    flag_tags["HEARS"]=MF_HEARS;// // It can hear you
    flag_tags["GOODHEARING"]=MF_GOODHEARING;// // Pursues sounds more than most monsters
    flag_tags["SMELLS"]=MF_SMELLS;// // It can smell you
    flag_tags["KEENNOSE"]=MF_KEENNOSE;// //Keen sense of smell
    flag_tags["STUMBLES"]=MF_STUMBLES;// // Stumbles in its movement
    flag_tags["WARM"]=MF_WARM;// // Warm blooded
    flag_tags["NOHEAD"]=MF_NOHEAD;// // Headshots not allowed!
    flag_tags["HARDTOSHOOT"]=MF_HARDTOSHOOT;// // Some shots are actually misses
    flag_tags["GRABS"]=MF_GRABS;// // Its attacks may grab us!
    flag_tags["BASHES"]=MF_BASHES;// // Bashes down doors
    flag_tags["DESTROYS"]=MF_DESTROYS;// // Bashes down walls and more
    flag_tags["POISON"]=MF_POISON;// // Poisonous to eat
    flag_tags["VENOM"]=MF_VENOM;// // Attack may poison the player
    flag_tags["BADVENOM"]=MF_BADVENOM;// // Attack may SEVERELY poison the player
    flag_tags["BLEED"]=MF_BLEED;//       // Causes player to bleed
    flag_tags["WEBWALK"]=MF_WEBWALK;// // Doesn't destroy webs
    flag_tags["DIGS"]=MF_DIGS;// // Digs through the ground
    flag_tags["FLIES"]=MF_FLIES;// // Can fly (over water, etc)
    flag_tags["AQUATIC"]=MF_AQUATIC;// // Confined to water
    flag_tags["SWIMS"]=MF_SWIMS;// // Treats water as 50 movement point terrain
    flag_tags["ATTACKMON"]=MF_ATTACKMON;// // Attacks other monsters
    flag_tags["ANIMAL"]=MF_ANIMAL;// // Is an "animal" for purposes of the Animal Empath trait
    flag_tags["PLASTIC"]=MF_PLASTIC;// // Absorbs physical damage to a great degree
    flag_tags["SUNDEATH"]=MF_SUNDEATH;// // Dies in full sunlight
    flag_tags["ELECTRIC"]=MF_ELECTRIC;// // Shocks unarmed attackers
    flag_tags["ACIDPROOF"]=MF_ACIDPROOF;// // Immune to acid
    flag_tags["ACIDTRAIL"]=MF_ACIDTRAIL;// // Leaves a trail of acid
    flag_tags["SLUDGEPROOF"]=MF_SLUDGEPROOF;// // Ignores the effect of sludge trails
    flag_tags["SLUDGETRAIL"]=MF_SLUDGETRAIL;// // Causes monster to leave a sludge trap trail when moving
    flag_tags["FIREY"]=MF_FIREY;// // Burns stuff and is immune to fire
    flag_tags["QUEEN"]=MF_QUEEN;// // When it dies, local populations start to die off too
    flag_tags["ELECTRONIC"]=MF_ELECTRONIC;// // e.g. a robot; affected by emp blasts, and other stuff
    flag_tags["FUR"]=MF_FUR;//  // May produce fur when butchered
    flag_tags["LEATHER"]=MF_LEATHER;// // May produce leather when butchered
    flag_tags["FEATHER"]=MF_FEATHER;// // May produce feather when butchered
    flag_tags["CBM"]=MF_CBM;// // May produce a cbm or two when butchered
    flag_tags["BONES"]=MF_BONES;// // May produce bones and sinews when butchered
    flag_tags["IMMOBILE"]=MF_IMMOBILE;// // Doesn't move (e.g. turrets)
    flag_tags["FRIENDLY_SPECIAL"]=MF_FRIENDLY_SPECIAL;// // Use our special attack, even if friendly
    flag_tags["HIT_AND_RUN"]=MF_HIT_AND_RUN;// // Flee for several turns after a melee attack
    flag_tags["GUILT"]=MF_GUILT;// // You feel guilty for killing it
    flag_tags["HUMAN"]=MF_HUMAN;// // It's a live human
    flag_tags["NO_BREATHE"]=MF_NO_BREATHE;// //Provides immunity to inhalation effects from gas, smoke, and poison, and can't drown
    flag_tags["REGENERATES_50"]=MF_REGENERATES_50;// // Monster regenerates very quickly over time
    flag_tags["FLAMMABLE"]=MF_FLAMMABLE;// // Monster catches fire, burns, and passes the fire on to nearby objects
    flag_tags["REVIVES"]=MF_REVIVES;// // Monster corpse will revive after a short period of time
    flag_tags["HALLUCINATION"]=MF_HALLUCINATION;//
    flag_tags["CHITIN"]=MF_CHITIN;//  // May produce chitin when butchered
}
void monster_factory::init_triggers()
{
    trigger_tags["NULL"] = MTRIG_NULL;// = 0,
    trigger_tags["STALK"] = MTRIG_STALK;//  // Increases when following the player
    trigger_tags["MEAT"] = MTRIG_MEAT;//  // Meat or a corpse nearby
    trigger_tags["PLAYER_WEAK"] = MTRIG_PLAYER_WEAK;// // The player is hurt
    trigger_tags["PLAYER_CLOSE"] = MTRIG_PLAYER_CLOSE;// // The player gets within a few tiles
    trigger_tags["HURT"] = MTRIG_HURT;//  // We are hurt
    trigger_tags["FIRE"] = MTRIG_FIRE;//  // Fire nearby
    trigger_tags["FRIEND_DIED"] = MTRIG_FRIEND_DIED;// // A monster of the same type died
    trigger_tags["FRIEND_ATTACKED"] = MTRIG_FRIEND_ATTACKED;// // A monster of the same type attacked
    trigger_tags["SOUND"] = MTRIG_SOUND;//  // Heard a sound
}
void monster_factory::init_phases()
{
    phase_tags["PNULL"]=PNULL;
    phase_tags["SOLID"]=SOLID;
    phase_tags["LIQUID"]=LIQUID;
    phase_tags["GAS"]=GAS;
    phase_tags["PLASMA"]=PLASMA;
}

// Loading System
void monster_factory::load_monster(JsonObject &jo)
{
    if (jo.has_member("id"))
    {
        mtype *newmon = new mtype();
        try
        {
            set_mon_information(jo, newmon);
            set_mon_data(jo, newmon);
            set_mon_triggers(jo, newmon);
            set_mon_categories(jo, newmon);
            set_mon_functions(jo, newmon);

            // delete repeated monster object before insertion
            if (monsters.find(newmon->id) != monsters.end())
            {
                delete monsters[newmon->id];
            }
            monsters[newmon->id] = newmon;
        }
        catch(std::string err)
        {
            delete newmon;
        }
    }
}
void monster_factory::load_species(JsonObject &jo)
{

}
// Data Loaders
// -- MONSTERS
// Throw errors as std::string type to catch badly formed, but critical, information
void monster_factory::set_mon_information(JsonObject& jo, mtype* mon)
{
    // id, name, description, phase, material, species
    try
    {
        std::string id, name, desc, phase, msize;//, material, species;
        id = jo.get_string("id"); // already established that member: id exists
        name = jo.get_string("name", "");
        desc = jo.get_string("description", "");
        phase = jo.get_string("phase", ""); // will fail most of the time
        msize = jo.get_string("size", "");

        // material and species are arrays, and always arrays
        std::set<std::string> materials, m_speciess;
        get_tags(jo, "material", materials);
        get_tags(jo, "species", m_speciess);

        mon->id = id;
        mon->name = name;
        mon->description = desc;
        mon->phase = convert_tag(phase, phase_tags, SOLID);
        mon->size = convert_tag(msize, size_tags, MS_MEDIUM);

        std::set<std::string>::iterator it;
        // since mtypes can only have one material, check materials size, set to first material if any, set to "null" otherwise
        if (materials.size() > 0)
        {
            it = materials.begin();
            mon->mat = *it;
        }
        else
        {
            mon->mat = "null";
        }
        // monsters can have multiple species available to them
        if (m_speciess.size() > 0)
        {
            for (it = m_speciess.begin(); it != m_speciess.end(); ++it)
            {
                // need to change mon->s_species over to set<string> so that the details can be finalized later
            }
        }
        else
        {
            // species = NULL_SPECIES
        }
    }
    catch (std::string err)
    {
        DebugLog() << "set_mon_information: "<< err << "\n";
        throw err;
    }
}

void monster_factory::set_mon_triggers(JsonObject& jo, mtype* mon)
{
    std::set<std::string> anger, fear, placate;
    get_tags(jo, "anger_triggers", anger);
    get_tags(jo, "fear_triggers", fear);
    get_tags(jo, "placate_triggers", placate);

    monster_trigger temp;
    for (std::set<std::string>::iterator it = anger.begin(); it != anger.end(); ++it)
    {
        temp = convert_tag(*it, trigger_tags, MTRIG_NULL);
        if (temp != MTRIG_NULL)
        {
            mon->anger.push_back(temp);
        }
    }
    for (std::set<std::string>::iterator it = fear.begin(); it != fear.end(); ++it)
    {
        temp = convert_tag(*it, trigger_tags, MTRIG_NULL);
        if (temp != MTRIG_NULL)
        {
            mon->fear.push_back(temp);
        }
    }
    for (std::set<std::string>::iterator it = placate.begin(); it != placate.end(); ++it)
    {
        temp = convert_tag(*it, trigger_tags, MTRIG_NULL);
        if (temp != MTRIG_NULL)
        {
            mon->placate.push_back(temp);
        }
    }
}

void monster_factory::set_mon_categories(JsonObject& jo, mtype* mon)
{
    std::set<std::string> _categories;

    get_tags(jo, "categories", _categories);
    for (std::set<std::string>::iterator it = _categories.begin(); it != _categories.end(); ++it)
    {
        // if the category doesn't exist, make it exist in the category set
        if (categories.find(*it) == categories.end())
        {
            categories.insert(*it);
        }
        mon->s_categories.insert(*it);
    }
}

void monster_factory::set_mon_flags(JsonObject &jo, mtype *mon)
{
    std::set<std::string> flags;

    get_tags(jo, "flags", flags);

    m_flag flag;
    for (std::set<std::string>::iterator it = flags.begin(); it != flags.end(); ++it)
    {
        flag = convert_tag(*it, flag_tags, MF_NULL);
        if (flag != MF_NULL)
        {
            mon->flags.push_back(flag);
        }
    }
}

void monster_factory::set_mon_data(JsonObject& jo, mtype* mon)
{
    mon->sym = jo.get_string("symbol", "?")[0];
    mon->color = color_from_string(jo.get_string("color", "white"));
    mon->difficulty = jo.get_int("difficulty");
    mon->agro = jo.get_int("aggression");
    mon->morale = jo.get_int("morale");
    mon->speed = jo.get_int("speed");
    mon->melee_skill = jo.get_int("melee_skill");
    mon->melee_dice = jo.get_int("melee_dice");
    mon->melee_sides = jo.get_int("melee_dice_sides");
    mon->melee_cut = jo.get_int("melee_cut");
    mon->sk_dodge = jo.get_int("dodge_skill");
    mon->armor_bash = jo.get_int("bash_resist");
    mon->armor_cut = jo.get_int("cut_resist");
    mon->item_chance = jo.get_int("item_drop_chance");
    mon->hp = jo.get_int("hp");
    mon->sp_freq = jo.get_int("special_attack_frequency");
}

void monster_factory::set_mon_functions(JsonObject& jo, mtype* mon)
{
    std::string monattack = "NONE", mondeath = "NORMAL";

    if (jo.has_member("special_attack_function"))
    {
        monattack = jo.get_string("special_attack_function");
    }
    if (jo.has_member("death_function"))
    {
        mondeath = jo.get_string("death_function");
    }

    mon->sp_attack = special_attack_functions[monattack];
    mon->dies = death_functions[mondeath];
}

// -- SPECIES
void monster_factory::set_species_triggers(JsonObject& jo, species_type& type)
{
    std::set<std::string> anger, fear, placate;
    get_tags(jo, "anger_triggers", anger);
    get_tags(jo, "fear_triggers", fear);
    get_tags(jo, "placate_triggers", placate);

    monster_trigger temp;
    for (std::set<std::string>::iterator it = anger.begin(); it != anger.end(); ++it)
    {
        temp = convert_tag(*it, trigger_tags, MTRIG_NULL);
        if (temp != MTRIG_NULL)
        {
            type.anger_triggers.insert(*it);
        }
    }
    for (std::set<std::string>::iterator it = fear.begin(); it != fear.end(); ++it)
    {
        temp = convert_tag(*it, trigger_tags, MTRIG_NULL);
        if (temp != MTRIG_NULL)
        {
            type.fear_triggers.insert(*it);
        }
    }
    for (std::set<std::string>::iterator it = placate.begin(); it != placate.end(); ++it)
    {
        temp = convert_tag(*it, trigger_tags, MTRIG_NULL);
        if (temp != MTRIG_NULL)
        {
            type.placate_triggers.insert(*it);
        }
    }
}

void monster_factory::set_species_flags(JsonObject& jo, species_type& type)
{
    std::set<std::string> t_flags;

    get_tags(jo, "flags", t_flags);

    m_flag temp;
    for (std::set<std::string>::iterator it = t_flags.begin(); it != t_flags.end(); ++it)
    {
        temp = convert_tag(*it, flag_tags, MF_NULL);
        if (temp != MF_NULL)
        {
            type.flags.insert(*it);
        }
    }
}



// Data Extractors
// Will not alter the dataset if there JsonArray is empty.
void monster_factory::get_tags(JsonObject &jo, std::string membername, std::set<std::string> &data)
{
    JsonArray arr = jo.get_array(membername);

    if (arr.size() > 0)
    {
        while (arr.has_more())
        {
            data.insert(arr.next_string());
        }
    }
}

// Accessors!
mtype *monster_factory::get_mon(Mon_Tag tag)
{
    if (monsters.find(tag) != monsters.end())
    {
        return monsters[tag];
    }
    return monsters["mon_null"];
}

// This should get rid of the need for a lot of overloaded string->enum functions
template<typename T>
T monster_factory::convert_tag(std::string tag, std::map<Mon_Tag, T> tag_map, T fallback)
{
    if (tag_map.find(tag) != tag_map.end())
    {
        return tag_map[tag];
    }
    return fallback;
}


phase_id monster_factory::convert_tag(std::string tag, phase_id fallback)
{
    if (phase_tags.find(tag) != phase_tags.end())
    {
        return phase_tags[tag];
    }
    return fallback;
}

m_size monster_factory::convert_tag(std::string tag, m_size fallback)
{
    if (size_tags.find(tag) != size_tags.end())
    {
        return size_tags[tag];
    }
    return fallback;
}

m_flag monster_factory::convert_tag(std::string tag, m_flag fallback)
{
    if (flag_tags.find(tag) != flag_tags.end())
    {
        return flag_tags[tag];
    }
    return fallback;
}

monster_trigger monster_factory::convert_tag(std::string tag, monster_trigger fallback)
{
    if (trigger_tags.find(tag) != trigger_tags.end())
    {
        return trigger_tags[tag];
    }
    return fallback;
}
