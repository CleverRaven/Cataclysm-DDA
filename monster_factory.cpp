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

    mon_templates["MISSING_MONTYPE"] = m_missing_type;
}

monster_factory::~monster_factory()
{
    //dtor
}

void monster_factory::init()
{
    // init mdeath functions
    mdeath_function_list["NORMAL"] = &mdeath::normal;
    mdeath_function_list["ACID"] = &mdeath::acid;
    mdeath_function_list["BOOMER"] = &mdeath::boomer;
    mdeath_function_list["VINE_KILL"] = &mdeath::kill_vines;
    mdeath_function_list["VINE_CUT"] = &mdeath::vine_cut;
    mdeath_function_list["TRIFFID_HEART"] = &mdeath::triffid_heart;
    mdeath_function_list["FUNGUS"] = &mdeath::fungus;
    mdeath_function_list["FUNGUS_AWAKE"] = &mdeath::fungusawake;
    mdeath_function_list["DISINTEGRATE"] = &mdeath::disintegrate;
    //mdeath_function_list["SHRIEK"] = &mdeath::shriek;
    mdeath_function_list["WORM"] = &mdeath::worm;
    mdeath_function_list["DISAPPEAR"] = &mdeath::disappear;
    mdeath_function_list["GUILT"] = &mdeath::guilt;
    mdeath_function_list["SPLIT"] = &mdeath::blobsplit;
    mdeath_function_list["MELT"] = &mdeath::melt;
    mdeath_function_list["AMIGARA"] = &mdeath::amigara;
    mdeath_function_list["THING"] = &mdeath::thing;
    mdeath_function_list["EXPLODE"] = &mdeath::explode;
    mdeath_function_list["RAT_KING"] = &mdeath::ratking;
    mdeath_function_list["KILL_BREATHERS"] = &mdeath::kill_breathers;
    mdeath_function_list["SMOKE_BURST"] = &mdeath::smokeburst;
    mdeath_function_list["ZOMBIE"] = &mdeath::zombie;
    mdeath_function_list["GAMEOVER"] = &mdeath::gameover;

    // init mattack functions
    mattack_function_list["NONE"] = &mattack::none;
    mattack_function_list["ANT_QUEEN"] = &mattack::antqueen;
    mattack_function_list["SHRIEK"] = &mattack::shriek;
    mattack_function_list["ACID"] = &mattack::acid;
    mattack_function_list["SHOCK_STORM"] = &mattack::shockstorm;
    mattack_function_list["SMOKE_CLOUD"] = &mattack::smokecloud;
    mattack_function_list["BOOMER"] = &mattack::boomer;
    mattack_function_list["RESURRECT"] = &mattack::resurrect;
    mattack_function_list["SCIENCE"] = &mattack::science;
    mattack_function_list["GROW_PLANTS"] = &mattack::growplants;
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
    mattack_function_list["FORM_BLOB"] = &mattack::formblob;
    mattack_function_list["DOG_THING"] = &mattack::dogthing;
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
    mattack_function_list["RAT_KING"] = &mattack::ratking;
    mattack_function_list["GENERATOR"] = &mattack::generator;
    mattack_function_list["UPGRADE"] = &mattack::upgrade;
    mattack_function_list["BREATHE"] = &mattack::breathe;
    mattack_function_list["BITE"] = &mattack::bite;
    mattack_function_list["FLESH_GOLEM"] = &mattack::flesh_golem;
    mattack_function_list["PARROT"] = &mattack::parrot;
}

void monster_factory::load_monster_templates() throw(std::string)
{
    try
    {
        load_monster_templates_from("data/raw/monsters.json");
    }
    catch (std::string &error_message)
    {
        throw;
    }
}

void monster_factory::load_monster_templates_from(const std::string filename) throw(std::string)
{
    catajson all_monsters(filename);

    if(! json_good())
    	throw (std::string)"Could not open " + file_name;

    if (! all_items.is_array())
        throw file_name + (std::string)"is not an array of item_templates";

    for (all_monsters.set_begin(); all_monsters.has_curr(); all_monsters.next())
    {
        catajson entry = all_monsters.curr();

        if (!entry.has("id") || !entry.get("id").is_string())
        {
            debugmsg("monster definition skipped, no id found or id was malformed");
        }
        else
        {
            mtype *new_monster_template;

            /*
            collect data about mtype!
            data obtained
            <tag>:<value type>

            "id":"mon_",
            "name":"name",
            "species":"species_",
            "symbol":"symbol",
            "color":"c_",
            "size":"MS_",
            "material":"materialtype",
            "diff": int,
            "agression": int,
            "morale": int,
            "speed": int,
            "melee_skill": int,
            "melee_dice": int,
            "melee_sides": int,
            "melee_cut": int,
            "dodge": int,
            "arm_bash": int,
            "arm_cut": int,
            "item_chance": int,
            "HP": int,
            "special_frequency": int,
            "death_function": function,
            "special_attack_function": function,

            */

            std::string new_id = entry.get("id").as_string();
            std::string new_name,
                        new_species,
                        new_symbol,
                        new_color,
                        new_size,
                        new_material;
            int new_difficulty,
                new_aggression,
                new_morale,
                new_speed,
                new_melee_skill,
                new_melee_dice,
                new_melee_sides,
                new_melee_cut,
                new_dodge,
                new_arm_bash,
                new_arm_cut,
                new_item_chance,
                new_hp,
                new_spec_freq;
            std::string new_death_func;
            std::vector<std::string> new_attack_func,
                                     new_flags,
                                     new_categories,
                                     new_fears,
                                     new_angers,
                                     new_placates;

        }
    }
}
