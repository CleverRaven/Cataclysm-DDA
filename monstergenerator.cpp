#include "monstergenerator.h"
#include "color.h"
#include "translations.h"
#include "rng.h"
#include "output.h"

MonsterGenerator::MonsterGenerator()
{
    mon_templates["mon_null"] = new mtype();
    mon_species["spec_null"] = new species_type();
    //ctor
    init_phases();
    init_attack();
    init_death();
    init_flags();
    init_trigger();
    init_sizes();
}

MonsterGenerator::~MonsterGenerator()
{
    //dtor
    if (mon_templates.size() > 0){
        for (std::map<std::string, mtype*>::iterator types = mon_templates.begin(); types != mon_templates.end(); ++types){
            delete types->second;
        }
        mon_templates.clear();
    }
    if (mon_species.size() > 0){
        for (std::map<std::string, species_type*>::iterator specs = mon_species.begin(); specs != mon_species.end(); ++specs){
            delete specs->second;
        }
        mon_species.clear();
    }
}

void MonsterGenerator::finalize_mtypes()
{
    for (std::map<std::string, mtype*>::iterator monentry = mon_templates.begin(); monentry != mon_templates.end(); ++monentry){
        mtype *mon = monentry->second;
        apply_species_attributes(mon);
        set_mtype_flags(mon);
    }
}

void MonsterGenerator::apply_species_attributes(mtype *mon)
{
    for (std::set<std::string>::iterator spec = mon->species.begin(); spec != mon->species.end(); ++spec){
        if (mon_species.find(*spec) != mon_species.end()){
            species_type *mspec = mon_species[*spec];

            // apply species flags/triggers
            apply_set_to_set(mspec->flags, mon->flags);
            apply_set_to_set(mspec->anger_trig, mon->anger);
            apply_set_to_set(mspec->fear_trig, mon->fear);
            apply_set_to_set(mspec->placate_trig, mon->placate);
        }
    }
}
void MonsterGenerator::set_mtype_flags(mtype *mon)
{
    // The flag vectors are slow, given how often has_flags() is called,
    // so instead we'll use bitsets and initialize them here.
    m_flag nflag;
    for (std::set<m_flag>::iterator flag = mon->flags.begin(); flag != mon->flags.end(); ++flag){
        nflag = m_flag(*flag);
        mon->bitflags[nflag] = true;
    }
    monster_trigger ntrig;
    for (std::set<monster_trigger>::iterator trig = mon->anger.begin(); trig != mon->anger.end(); ++trig){
        ntrig = monster_trigger(*trig);
        mon->bitanger[*trig] = true;
    }
    for (std::set<monster_trigger>::iterator trig = mon->fear.begin(); trig != mon->fear.end(); ++trig){
        ntrig = monster_trigger(*trig);
        mon->bitfear[ntrig] = true;
    }
    for (std::set<monster_trigger>::iterator trig = mon->placate.begin(); trig != mon->placate.end(); ++trig){
        ntrig = monster_trigger(*trig);
        mon->bitplacate[ntrig] = true;
    }
}

template <typename T>
void MonsterGenerator::apply_set_to_set(std::set<T> from, std::set<T> &to)
{
    for (typename std::set<T>::iterator entry = from.begin(); entry != from.end(); ++entry){
        to.insert(*entry);
    }
}

void MonsterGenerator::init_phases()
{
    phase_map["NULL"]=PNULL;
    phase_map["SOLID"]=SOLID;
    phase_map["LIQUID"]=LIQUID;
    phase_map["GAS"]=GAS;
    phase_map["PLASMA"]=PLASMA;
}

void MonsterGenerator::init_sizes()
{
    size_map["TINY"]=MS_TINY;// Rodent
    size_map["SMALL"]=MS_SMALL;// Half human
    size_map["MEDIUM"]=MS_MEDIUM;// Human
    size_map["LARGE"]=MS_LARGE;// Cow
    size_map["HUGE"]=MS_HUGE;// TAAAANK
}

void MonsterGenerator::init_death()
{
    death_map["NORMAL"] = &mdeath::normal;// Drop a body
    death_map["ACID"] = &mdeath::acid;// Acid instead of a body
    death_map["BOOMER"] = &mdeath::boomer;// Explodes in vomit :3
    death_map["KILL_VINES"] = &mdeath::kill_vines;// Kill all nearby vines
    death_map["VINE_CUT"] = &mdeath::vine_cut;// Kill adjacent vine if it's cut
    death_map["TRIFFID_HEART"] = &mdeath::triffid_heart;// Destroy all roots
    death_map["FUNGUS"] = &mdeath::fungus;// Explodes in spores D:
    death_map["DISINTEGRATE"] = &mdeath::disintegrate;// Falls apart
    death_map["WORM"] = &mdeath::worm;// Spawns 2 half-worms
    death_map["DISAPPEAR"] = &mdeath::disappear;// Hallucination disappears
    death_map["GUILT"] = &mdeath::guilt;// Morale penalty
    death_map["BLOBSPLIT"] = &mdeath::blobsplit;// Creates more blobs
    death_map["MELT"] = &mdeath::melt;// Normal death, but melts
    death_map["AMIGARA"] = &mdeath::amigara;// Removes hypnosis if last one
    death_map["THING"] = &mdeath::thing;// Turn into a full thing
    death_map["EXPLODE"] = &mdeath::explode;// Damaging explosion
    death_map["RATKING"] = &mdeath::ratking;// Cure verminitis
    death_map["KILL_BREATHERS"] = &mdeath::kill_breathers;// All breathers die
    death_map["SMOKEBURST"] = &mdeath::smokeburst;// Explode like a huge smoke bomb.
    death_map["ZOMBIE"] = &mdeath::zombie;// generate proper clothing for zombies
    death_map["GAMEOVER"] = &mdeath::gameover;// Game over!  Defense mode

    /* Currently Unimplemented */
    //death_map["SHRIEK"] = &mdeath::shriek;// Screams loudly
    //death_map["RATTLE"] = &mdeath::rattle;// Rattles like a rattlesnake
}

void MonsterGenerator::init_attack()
{
    attack_map["NONE"] = &mattack::none;
    attack_map["ANTQUEEN"] = &mattack::antqueen;
    attack_map["SHRIEK"] = &mattack::shriek;
    attack_map["RATTLE"] = &mattack::rattle;
    attack_map["ACID"] = &mattack::acid;
    attack_map["SHOCKSTORM"] = &mattack::shockstorm;
    attack_map["SMOKECLOUD"] = &mattack::smokecloud;
    attack_map["BOOMER"] = &mattack::boomer;
    attack_map["RESURRECT"] = &mattack::resurrect;
    attack_map["SCIENCE"] = &mattack::science;
    attack_map["GROWPLANTS"] = &mattack::growplants;
    attack_map["GROW_VINE"] = &mattack::grow_vine;
    attack_map["VINE"] = &mattack::vine;
    attack_map["SPIT_SAP"] = &mattack::spit_sap;
    attack_map["TRIFFID_HEARTBEAT"] = &mattack::triffid_heartbeat;
    attack_map["FUNGUS"] = &mattack::fungus;
    attack_map["FUNGUS_GROWTH"] = &mattack::fungus_growth;
    attack_map["FUNGUS_SPROUT"] = &mattack::fungus_sprout;
    attack_map["LEAP"] = &mattack::leap;
    attack_map["DERMATIK"] = &mattack::dermatik;
    attack_map["DERMATIK_GROWTH"] = &mattack::dermatik_growth;
    attack_map["PLANT"] = &mattack::plant;
    attack_map["DISAPPEAR"] = &mattack::disappear;
    attack_map["FORMBLOB"] = &mattack::formblob;
    attack_map["DOGTHING"] = &mattack::dogthing;
    attack_map["TENTACLE"] = &mattack::tentacle;
    attack_map["VORTEX"] = &mattack::vortex;
    attack_map["GENE_STING"] = &mattack::gene_sting;
    attack_map["PARA_STING"] = &mattack::para_sting;
    attack_map["TRIFFID_GROWTH"] = &mattack::triffid_growth;
    attack_map["STARE"] = &mattack::stare;
    attack_map["FEAR_PARALYZE"] = &mattack::fear_paralyze;
    attack_map["PHOTOGRAPH"] = &mattack::photograph;
    attack_map["TAZER"] = &mattack::tazer;
    attack_map["SMG"] = &mattack::smg;
    attack_map["FLAMETHROWER"] = &mattack::flamethrower;
    attack_map["COPBOT"] = &mattack::copbot;
    attack_map["MULTI_ROBOT"] = &mattack::multi_robot;
    attack_map["RATKING"] = &mattack::ratking;
    attack_map["GENERATOR"] = &mattack::generator;
    attack_map["UPGRADE"] = &mattack::upgrade;
    attack_map["BREATHE"] = &mattack::breathe;
    attack_map["BITE"] = &mattack::bite;
    attack_map["FLESH_GOLEM"] = &mattack::flesh_golem;
    attack_map["PARROT"] = &mattack::parrot;
}

void MonsterGenerator::init_trigger()
{
    trigger_map["NULL"] = MTRIG_NULL;// = 0,
    trigger_map["STALK"] = MTRIG_STALK;//  // Increases when following the player
    trigger_map["MEAT"] = MTRIG_MEAT;//  // Meat or a corpse nearby
    trigger_map["PLAYER_WEAK"] = MTRIG_PLAYER_WEAK;// // The player is hurt
    trigger_map["PLAYER_CLOSE"] = MTRIG_PLAYER_CLOSE;// // The player gets within a few tiles
    trigger_map["HURT"] = MTRIG_HURT;//  // We are hurt
    trigger_map["FIRE"] = MTRIG_FIRE;//  // Fire nearby
    trigger_map["FRIEND_DIED"] = MTRIG_FRIEND_DIED;// // A monster of the same type died
    trigger_map["FRIEND_ATTACKED"] = MTRIG_FRIEND_ATTACKED;// // A monster of the same type attacked
    trigger_map["SOUND"] = MTRIG_SOUND;//  // Heard a sound
}

void MonsterGenerator::init_flags()
{
    flag_map["NULL"] = MF_NULL;// = 0, // Helps with setvector
    flag_map["SEES"] = MF_SEES;// // It can see you (and will run/follow)
    flag_map["VIS50"] = MF_VIS50;// //Vision -10
    flag_map["VIS40"] = MF_VIS40;// //Vision -20
    flag_map["VIS30"] = MF_VIS30;// //Vision -30
    flag_map["VIS20"] = MF_VIS20;// //Vision -40
    flag_map["VIS10"] = MF_VIS10;// //Vision -50
    flag_map["HEARS"] = MF_HEARS;// // It can hear you
    flag_map["GOODHEARING"] = MF_GOODHEARING;// // Pursues sounds more than most monsters
    flag_map["SMELLS"] = MF_SMELLS;// // It can smell you
    flag_map["KEENNOSE"] = MF_KEENNOSE;// //Keen sense of smell
    flag_map["STUMBLES"] = MF_STUMBLES;// // Stumbles in its movement
    flag_map["WARM"] = MF_WARM;// // Warm blooded
    flag_map["NOHEAD"] = MF_NOHEAD;// // Headshots not allowed!
    flag_map["HARDTOSHOOT"] = MF_HARDTOSHOOT;// // Some shots are actually misses
    flag_map["GRABS"] = MF_GRABS;// // Its attacks may grab us!
    flag_map["BASHES"] = MF_BASHES;// // Bashes down doors
    flag_map["DESTROYS"] = MF_DESTROYS;// // Bashes down walls and more
    flag_map["POISON"] = MF_POISON;// // Poisonous to eat
    flag_map["VENOM"] = MF_VENOM;// // Attack may poison the player
    flag_map["BADVENOM"] = MF_BADVENOM;// // Attack may SEVERELY poison the player
    flag_map["PARALYZEVENOM"] = MF_PARALYZE;// // Attack may paralyze the player with venom
    flag_map["BLEED"] = MF_BLEED;//       // Causes player to bleed
    flag_map["WEBWALK"] = MF_WEBWALK;// // Doesn't destroy webs
    flag_map["DIGS"] = MF_DIGS;// // Digs through the ground
    flag_map["CAN_DIG"] = MF_CAN_DIG;// // Digs through the ground
    flag_map["FLIES"] = MF_FLIES;// // Can fly (over water, etc)
    flag_map["AQUATIC"] = MF_AQUATIC;// // Confined to water
    flag_map["SWIMS"] = MF_SWIMS;// // Treats water as 50 movement point terrain
    flag_map["ATTACKMON"] = MF_ATTACKMON;// // Attacks other monsters
    flag_map["ANIMAL"] = MF_ANIMAL;// // Is an "animal" for purposes of the Animal Empath trait
    flag_map["PLASTIC"] = MF_PLASTIC;// // Absorbs physical damage to a great degree
    flag_map["SUNDEATH"] = MF_SUNDEATH;// // Dies in full sunlight
    flag_map["ELECTRIC"] = MF_ELECTRIC;// // Shocks unarmed attackers
    flag_map["ACIDPROOF"] = MF_ACIDPROOF;// // Immune to acid
    flag_map["ACIDTRAIL"] = MF_ACIDTRAIL;// // Leaves a trail of acid
    flag_map["SLUDGEPROOF"] = MF_SLUDGEPROOF;// // Ignores the effect of sludge trails
    flag_map["SLUDGETRAIL"] = MF_SLUDGETRAIL;// // Causes monster to leave a sludge trap trail when moving
    flag_map["FIREY"] = MF_FIREY;// // Burns stuff and is immune to fire
    flag_map["QUEEN"] = MF_QUEEN;// // When it dies, local populations start to die off too
    flag_map["ELECTRONIC"] = MF_ELECTRONIC;// // e.g. a robot; affected by emp blasts, and other stuff
    flag_map["FUR"] = MF_FUR;//  // May produce fur when butchered
    flag_map["LEATHER"] = MF_LEATHER;// // May produce leather when butchered
    flag_map["FEATHER"] = MF_FEATHER;// // May produce feather when butchered
    flag_map["CBM"] = MF_CBM;// // May produce a cbm or two when butchered
    flag_map["BONES"] = MF_BONES;// // May produce bones and sinews when butchered
    flag_map["IMMOBILE"] = MF_IMMOBILE;// // Doesn't move (e.g. turrets)
    flag_map["FRIENDLY_SPECIAL"] = MF_FRIENDLY_SPECIAL;// // Use our special attack, even if friendly
    flag_map["HIT_AND_RUN"] = MF_HIT_AND_RUN;// // Flee for several turns after a melee attack
    flag_map["GUILT"] = MF_GUILT;// // You feel guilty for killing it
    flag_map["HUMAN"] = MF_HUMAN;// // It's a live human
    flag_map["NO_BREATHE"] = MF_NO_BREATHE;// //Provides immunity to inhalation effects from gas, smoke, and poison, and can't drown
    flag_map["REGENERATES_50"] = MF_REGENERATES_50;// // Monster regenerates very quickly over time
    flag_map["FLAMMABLE"] = MF_FLAMMABLE;// // Monster catches fire, burns, and passes the fire on to nearby objects
    flag_map["REVIVES"] = MF_REVIVES;// // Monster corpse will revive after a short period of time
    flag_map["CHITIN"] = MF_CHITIN;//  // May produce chitin when butchered
}


void MonsterGenerator::load_monster(JsonObject &jo)
{
    // id
    std::string mid;
    if (jo.has_member("id")){
        mid = jo.get_string("id");

        mtype *newmon = new mtype;

        newmon->id = mid;
        newmon->name = _(jo.get_string("name","").c_str());
        newmon->description = _(jo.get_string("description").c_str());

        newmon->mat = jo.get_string("material");

        newmon->species = get_tags(jo, "species");
        newmon->categories = get_tags(jo, "categories");

        newmon->sym = jo.get_string("symbol")[0]; // will fail here if there is no symbol
        newmon->color = color_from_string(jo.get_string("color"));
        newmon->size = get_from_string(jo.get_string("size", "MEDIUM"), size_map, MS_MEDIUM);
        newmon->phase = get_from_string(jo.get_string("phase", "SOLID"), phase_map, SOLID);

        newmon->difficulty = jo.get_int("diff", 0);
        newmon->agro = jo.get_int("aggression", 0);
        newmon->morale = jo.get_int("morale", 0);
        newmon->speed = jo.get_int("speed", 0);
        newmon->melee_skill = jo.get_int("melee_skill", 0);
        newmon->melee_dice = jo.get_int("melee_dice", 0);
        newmon->melee_sides = jo.get_int("melee_dice_sides", 0);
        newmon->melee_cut = jo.get_int("melee_cut", 0);
        newmon->sk_dodge = jo.get_int("dodge", 0);
        newmon->armor_bash = jo.get_int("armor_bash", 0);
        newmon->armor_cut = jo.get_int("armor_cut", 0);
        newmon->item_chance = jo.get_int("item_chance", 0);
        newmon->hp = jo.get_int("hp", 0);
        newmon->sp_freq = jo.get_int("special_freq", 0);
        newmon->luminance = jo.get_float("luminance", 0);

        newmon->dies = get_death_function(jo, "death_function");
        newmon->sp_attack = get_attack_function(jo, "special_attack");

        std::set<std::string> flags, anger_trig, placate_trig, fear_trig, cats;
        flags = get_tags(jo, "flags");
        anger_trig = get_tags(jo, "anger_triggers");
        placate_trig = get_tags(jo, "placate_triggers");
        fear_trig = get_tags(jo, "fear_triggers");

        newmon->flags = get_set_from_tags(flags, flag_map, MF_NULL);
        newmon->anger = get_set_from_tags(anger_trig, trigger_map, MTRIG_NULL);
        newmon->fear = get_set_from_tags(fear_trig, trigger_map, MTRIG_NULL);
        newmon->placate = get_set_from_tags(placate_trig, trigger_map, MTRIG_NULL);

        mon_templates[mid] = newmon;
    }
}
void MonsterGenerator::load_species(JsonObject &jo)
{
    // id, flags, triggers (anger, placate, fear)
    std::string sid;
    if (jo.has_member("id")){
        sid = jo.get_string("id");

        std::set<std::string> sflags, sanger, sfear, splacate;
        sflags = get_tags(jo, "flags");
        sanger = get_tags(jo, "anger_triggers");
        sfear  = get_tags(jo, "fear_triggers");
        splacate=get_tags(jo, "placate_triggers");

        std::set<m_flag> flags = get_set_from_tags(sflags, flag_map, MF_NULL);
        std::set<monster_trigger> anger, fear, placate;
        anger = get_set_from_tags(sanger, trigger_map, MTRIG_NULL);
        fear = get_set_from_tags(sfear, trigger_map, MTRIG_NULL);
        placate = get_set_from_tags(splacate, trigger_map, MTRIG_NULL);

        species_type *new_species = new species_type(sid, flags, anger, fear, placate);

        mon_species[sid] = new_species;
    }
}

std::set<std::string> MonsterGenerator::get_tags(JsonObject &jo, std::string member)
{
    std::set<std::string> ret;

    if (jo.has_member(member)){
        json_value_type jvt = jo.get_member_type(member);
        if (jvt == JVT_STRING){
            ret.insert(jo.get_string(member));
        }else if (jvt == JVT_ARRAY){
            JsonArray jarr = jo.get_array(member);
            while (jarr.has_more()){
                ret.insert(jarr.next_string());
            }
        }
    }

    return ret;
}
mtype *MonsterGenerator::get_mtype(std::string mon)
{
    static mtype *default_montype = mon_templates["mon_null"];

    if (mon_templates.find(mon) != mon_templates.end())
    {
        return mon_templates[mon];
    }
    debugmsg("Could not find monster with type %s", mon.c_str());
    return default_montype;
}
mtype *MonsterGenerator::get_mtype(int mon)
{
    int count = 0;
    for (std::map<std::string, mtype*>::iterator monit = mon_templates.begin(); monit != mon_templates.end(); ++monit){
        if (count == mon){
            return monit->second;
        }
        ++count;
    }
    return mon_templates["mon_null"];
}

std::map<std::string, mtype*> MonsterGenerator::get_all_mtypes() const
{
    return mon_templates;
}
std::vector<std::string> MonsterGenerator::get_all_mtype_ids() const
{
    static std::vector<std::string> hold;
    if (hold.size() == 0){
        for (std::map<std::string, mtype*>::const_iterator mon = mon_templates.begin(); mon != mon_templates.end(); ++mon){
            hold.push_back(mon->first);
        }
    }
    const std::vector<std::string> ret = hold;
    return ret;
}

mtype *MonsterGenerator::get_valid_hallucination()
{
    static std::vector<mtype*> potentials;
    if (potentials.size() == 0){
        for (std::map<std::string, mtype*>::iterator mon = mon_templates.begin(); mon != mon_templates.end(); ++mon){
            if (mon->first != "mon_null" && mon->first != "mon_generator"){
                potentials.push_back(mon->second);
            }
        }
    }

    return potentials[rng(0, potentials.size())];
}

MonDeathFunction MonsterGenerator::get_death_function(JsonObject& jo, std::string member)
{
    static MonDeathFunction default_death = death_map["NORMAL"];

    if (death_map.find(jo.get_string(member, "")) != death_map.end())
    {
        return death_map[jo.get_string(member)];
    }

    return default_death;
}

MonAttackFunction MonsterGenerator::get_attack_function(JsonObject& jo, std::string member)
{
    static MonAttackFunction default_attack = attack_map["NONE"];

    if (attack_map.find(jo.get_string(member, "")) != attack_map.end())
    {
        return attack_map[jo.get_string(member)];
    }

    return default_attack;
}
template <typename T>
std::set<T> MonsterGenerator::get_set_from_tags(std::set<std::string> tags, std::map<std::string, T> conversion_map, T fallback)
{
    std::set<T> ret;

    if (tags.size() > 0){
        for (std::set<std::string>::iterator it = tags.begin(); it != tags.end(); ++it){
            if (conversion_map.find(*it) != conversion_map.end()){
                ret.insert(conversion_map[*it]);
            }
        }
    }
    if (ret.size() == 0){
        ret.insert(fallback);
    }

    return ret;
}

template <typename T>
T MonsterGenerator::get_from_string(std::string tag, std::map<std::string, T> conversion_map, T fallback)
{
    T ret = fallback;
    if (conversion_map.find(tag) != conversion_map.end()){
        ret = conversion_map[tag];
    }
    return ret;
}
