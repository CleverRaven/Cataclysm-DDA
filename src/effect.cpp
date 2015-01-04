#include "effect.h"
#include "rng.h"
#include "output.h"
#include "player.h"

#include <map>
#include <sstream>
#include <type_traits>
#include <algorithm>
#include <iterator>

std::map<std::string, effect_type> effect_types;

static std::string const eff_mods_base    = {"base_mods"};
static std::string const eff_mode_scaling = {"scaling_mods"};

static std::string const eff_arg_min        = {"min"};
static std::string const eff_arg_max        = {"max"};
static std::string const eff_arg_amount     = {"amount"};
static std::string const eff_arg_min_val    = {"min_val"};
static std::string const eff_arg_max_val    = {"max_val"};
static std::string const eff_arg_chance_top = {"chance_top"};
static std::string const eff_arg_chance_bot = {"chance_bot"};
static std::string const eff_arg_tick       = {"tick"};

static std::string const eff_arg_min_value = {"min_value"};
static std::string const eff_arg_max_value = {"max_value"};

static std::string const eff_type_str     {"STR"};
static std::string const eff_type_dex   = {"DEX"};
static std::string const eff_type_per   = {"PER"};
static std::string const eff_type_int   = {"INT"};
static std::string const eff_type_speed = {"SPEED"};

static std::string const eff_type_pain    = {"PAIN"};
static std::string const eff_type_hurt    = {"HURT"};
static std::string const eff_type_sleep   = {"SLEEP"};
static std::string const eff_type_pkill   = {"PKILL"};
static std::string const eff_type_stim    = {"STIM"};
static std::string const eff_type_health  = {"HEALTH"};
static std::string const eff_type_h_mod   = {"H_MOD"};
static std::string const eff_type_rad     = {"RAD"};
static std::string const eff_type_hunger  = {"HUNGER"};
static std::string const eff_type_thirst  = {"THIRST"};
static std::string const eff_type_fatigue = {"FATIGUE"};
static std::string const eff_type_cough   = {"COUGH"};
static std::string const eff_type_vomit   = {"VOMIT"};

/**
 * clamp n to [lo, hi]
 */
template <typename T>
inline T clamp(T const n, T const lo, T const hi) noexcept {
    return (n < lo) ? lo :
           (n > hi) ? hi :
           n;
}


// This should probably be moved elsewhere if it is to be kept so that it can be more widely used
namespace detail {

template <typename To, typename From>
struct select_result_type {
    using type = typename std::conditional<
        std::is_same<void, To>::value, From, To
    >::type;
};

template <typename To, typename From>
using select_result_type_t = typename select_result_type<To, From>::type;

} //namespace detail

/**
 * do a round followed by a cast
 *
 * @tparam To optional cast type, the results will otherwise fallback to decltype(value)
 */
template <
    typename To = void
  , typename From
  , typename Result = detail::select_result_type_t<To, From>
  , typename std::enable_if<std::is_floating_point<From>::value>::type* = nullptr
>
auto round_to(From const value) -> Result {
    using std::round;
    return static_cast<To>(round(value));
}

/**
 * mapping from hash value to T
 */
template <typename T>
using hash_pair = std::pair<int, T>;

/**
 * find key in an array
 *
 * @return a pair of the form {value at key, key was found}
 */
template <typename T, typename K, size_t N>
std::pair<T, bool>
find_mapping(hash_pair<T> const (&arr)[N], K const& key) {
    auto const beg = std::begin(arr);
    auto const end = std::end(arr);

    auto const it = std::find_if(beg, end, [&](hash_pair<T> const& value) {
        return key == value.first;
    });
    
    auto const found = (it != end);

    return std::make_pair(it->second, found);
}

/**
 * get a value (enumeration) from a string value
 *
 * @return a pair of the form {value, string was valid}
 */
template <typename T>
std::pair<T, bool> from_string(std::string const& s);

/**
 * get a value (enumeration) from a string value, otherwise @p fallback
 */
template <typename T>
T from_string(std::string const& s, T const fallback) {
    auto const result = from_string<T>(s);
    return result.second ? result.first : fallback;
}

/**
 * string -> game_message_type
 */
template <>
std::pair<game_message_type, bool> from_string<game_message_type>(std::string const& s) {
    //TODO perhaps move this elsewhere and complete for other game_message_type values?

    using type = game_message_type;
    
    static hash_pair<type> const hashes[] = {
        {djb2_hash("good"),    type::m_good}
      , {djb2_hash("neutral"), type::m_neutral}
      , {djb2_hash("bad"),     type::m_bad}
      , {djb2_hash("mixed"),   type::m_mixed}
    };

    return find_mapping(hashes, djb2_hash(s.c_str()));
}

/**
 * string -> effect_rating
 */
template <>
std::pair<effect_rating, bool> from_string<effect_rating>(std::string const& s) {
    using type = effect_rating;
    
    static hash_pair<type> const hashes[] = {
        {djb2_hash("good"),    type::e_good}
      , {djb2_hash("neutral"), type::e_neutral}
      , {djb2_hash("bad"),     type::e_bad}
      , {djb2_hash("mixed"),   type::e_mixed}
    };

    return find_mapping(hashes, djb2_hash(s.c_str()));
}

void weed_msg(player *p) {
    int howhigh = p->get_effect_dur("weed_high");
    int smarts = p->get_int();
    if(howhigh > 125 && one_in(7)) {
        int msg = rng(0,5);
        switch(msg) {
        case 0: // Freakazoid
            p->add_msg_if_player(_("The scariest thing in the world would be... if all the air in the world turned to WOOD!"));
            return;
        case 1: // Simpsons
            p->add_msg_if_player(_("Could Jesus microwave a burrito so hot, that he himself couldn't eat it?"));
            p->hunger += 2;
            return;
        case 2:
            if(smarts > 8) { // Timothy Leary
                p->add_msg_if_player(_("Science is all metaphor."));
            } else if(smarts < 3){ // It's Always Sunny in Phildelphia
                p->add_msg_if_player(_("Science is a liar sometimes."));
            } else { // Durr
                p->add_msg_if_player(_("Science is... wait, what was I talking about again?"));
            }
            return;
        case 3: // Dazed and Confused
            p->add_msg_if_player(_("Behind every good man there is a woman, and that woman was Martha Washington, man."));
            if(one_in(2)) {
                p->add_msg_if_player(_("Every day, George would come home, and she would have a big fat bowl waiting for him when he came in the door, man."));
                if(one_in(2)) {
                    p->add_msg_if_player(_("She was a hip, hip, hip lady, man."));
                }
            }
            return;
        case 4:
            if(p->has_amount("money_bundle", 1)) { // Half Baked
                p->add_msg_if_player(_("You ever see the back of a twenty dollar bill... on weed?"));
                if(one_in(2)) {
                    p->add_msg_if_player(_("Oh, there's some crazy shit, man. There's a dude in the bushes. Has he got a gun? I dunno!"));
                    if(one_in(3)) {
                        p->add_msg_if_player(_("RED TEAM GO, RED TEAM GO!"));
                    }
                }
            } else if(p->has_amount("holybook_bible", 1)) {
                p->add_msg_if_player(_("You have a sudden urge to flip your bible open to Genesis 1:29..."));
            } else { // Big Lebowski
                p->add_msg_if_player(_("That rug really tied the room together..."));
            }
            return;
        case 5:
            p->add_msg_if_player(_("I used to do drugs...  I still do, but I used to, too."));
        default:
            return;
        }
    } else if(howhigh > 100 && one_in(5)) {
        int msg = rng(0, 5);
        switch(msg) {
        case 0: // Bob Marley
            p->add_msg_if_player(_("The herb reveals you to yourself."));
            return;
        case 1: // Freakazoid
            p->add_msg_if_player(_("Okay, like, the scariest thing in the world would be... if like you went to grab something and it wasn't there!"));
            return;
        case 2: // Simpsons
            p->add_msg_if_player(_("They call them fingers, but I never see them fing."));
            if(smarts > 2 && one_in(2)) {
                p->add_msg_if_player(_("... oh, there they go."));
            }
            return;
        case 3: // Bill Hicks
            p->add_msg_if_player(_("You suddenly realize that all matter is merely energy condensed to a slow vibration, and we are all one consciousness experiencing itself subjectively."));
            return;
        case 4: // Steve Martin
            p->add_msg_if_player(_("I usually only smoke in the late evening."));
            if(one_in(4)) {
                p->add_msg_if_player(_("Oh, occasionally the early evening, but usually the late evening, or the mid-evening."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Just the early evening, mid-evening and late evening."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Occasionally, early afternoon, early mid-afternoon, or perhaps the late mid-afternoon."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Oh, sometimes the early-mid-late-early-morning."));
            }
            if(smarts > 2) {
                p->add_msg_if_player(_("...But never at dusk."));
            }
            return;
        case 5:
        default:
            return;
        }
    } else if(howhigh > 50 && one_in(3)) {
        int msg = rng(0, 5);
        switch(msg) {
        case 0: // Cheech and Chong
            p->add_msg_if_player(_("Dave's not here, man."));
            return;
        case 1: // Real Life
            p->add_msg_if_player(_("Man, a cheeseburger sounds SO awesome right now."));
            p->hunger += 4;
            if(p->has_trait("VEGETARIAN")) {
               p->add_msg_if_player(_("Eh... maybe not."));
            } else if(p->has_trait("LACTOSE")) {
                p->add_msg_if_player(_("I guess, maybe, without the cheese... yeah."));
            }
            return;
        case 2: // Dazed and Confused
            p->add_msg_if_player( _("Walkin' down the hall, by myself, smokin' a j with fifty elves."));
            return;
        case 3: // Half Baked
            p->add_msg_if_player(_("That weed was the shiz-nittlebam snip-snap-sack."));
            return;
        case 4:
            weed_msg(p); // re-roll
        case 5:
        default:
            return;
        }
    }
}

static void extract_effect(
    JsonObject &j
  , std::unordered_map<std::tuple<std::string, bool, std::string, std::string>, double> &data
  , std::string const& mod_type //json field
  , std::string const& data_key //base_mods scaling_mods
  , std::string const& type_key //STR etc
  , std::string const& arg_key  //arg
) {
    if (!j.has_member(mod_type)) {
        return; //TODO: log an error?
    }

    auto jsarr = j.get_array(mod_type);
    auto const n = jsarr.size();

    //value
    auto const v0 = jsarr.get_float(0);
    
    //reduced value, otherwise value
    auto const v1 = (n >= 2) ? jsarr.get_float(1) : v0;

    // insert non zero values
    auto const insert = [&](double const value, bool const reduced) {
        if (value) {
            data[std::make_tuple(data_key, reduced, type_key, arg_key)] = value;
        }
    };

    insert(v0, false);
    insert(v1, true);
}

bool effect_type::load_mod_data(JsonObject &jsobj, std::string const& member) {
    if (jsobj.has_object(member)) {
        return false;
    }

    JsonObject j = jsobj.get_object(member);

    using cs_ref = std::string const&;
    auto const do_extract = [&](cs_ref field, cs_ref type, cs_ref arg) {
        extract_effect(j, mod_data, field, member, type, arg);
    };

    // Stats first
    do_extract("str_mod",   eff_type_str,   eff_arg_min);
    do_extract("dex_mod",   eff_type_dex,   eff_arg_min);
    do_extract("per_mod",   eff_type_per,   eff_arg_min);
    do_extract("int_mod",   eff_type_int,   eff_arg_min);
    do_extract("speed_mod", eff_type_speed, eff_arg_min);

    // Then pain
    do_extract("pain_amount",     eff_type_pain, eff_arg_amount);
    do_extract("pain_min",        eff_type_pain, eff_arg_min);
    do_extract("pain_max",        eff_type_pain, eff_arg_max);
    do_extract("pain_max_val",    eff_type_pain, eff_arg_max_val);
    do_extract("pain_chance",     eff_type_pain, eff_arg_chance_top);
    do_extract("pain_chance_bot", eff_type_pain, eff_arg_chance_bot);
    do_extract("pain_tick",       eff_type_pain, eff_arg_tick);

    // Then hurt
    do_extract("hurt_amount",     eff_type_hurt, eff_arg_amount);
    do_extract("hurt_min",        eff_type_hurt, eff_arg_min);
    do_extract("hurt_max",        eff_type_hurt, eff_arg_max);
    do_extract("hurt_max_val",    eff_type_hurt, eff_arg_max_val);
    do_extract("hurt_chance",     eff_type_hurt, eff_arg_chance_top);
    do_extract("hurt_chance_bot", eff_type_hurt, eff_arg_chance_bot);
    do_extract("hurt_tick",       eff_type_hurt, eff_arg_tick);

    // Then sleep
    do_extract("sleep_amount",     eff_type_sleep, eff_arg_amount);
    do_extract("sleep_min",        eff_type_sleep, eff_arg_min);
    do_extract("sleep_max",        eff_type_sleep, eff_arg_max);
    do_extract("sleep_chance",     eff_type_sleep, eff_arg_chance_top);
    do_extract("sleep_chance_bot", eff_type_sleep, eff_arg_chance_bot);
    do_extract("sleep_tick",       eff_type_sleep, eff_arg_tick);

    // Then pkill
    do_extract("pkill_amount",     eff_type_pkill, eff_arg_amount);
    do_extract("pkill_min",        eff_type_pkill, eff_arg_min);
    do_extract("pkill_max",        eff_type_pkill, eff_arg_max);
    do_extract("pkill_max_val",    eff_type_pkill, eff_arg_max_val);
    do_extract("pkill_chance",     eff_type_pkill, eff_arg_chance_top);
    do_extract("pkill_chance_bot", eff_type_pkill, eff_arg_chance_bot);
    do_extract("pkill_tick",       eff_type_pkill, eff_arg_tick);
        
    // Then stim
    do_extract("stim_amount",     eff_type_stim, eff_arg_amount);
    do_extract("stim_min",        eff_type_stim, eff_arg_min);
    do_extract("stim_max",        eff_type_stim, eff_arg_max);
    do_extract("stim_max_val",    eff_type_stim, eff_arg_min_val);
    do_extract("stim_max_val",    eff_type_stim, eff_arg_max_val);
    do_extract("stim_chance",     eff_type_stim, eff_arg_chance_top);
    do_extract("stim_chance_bot", eff_type_stim, eff_arg_chance_bot);
    do_extract("stim_tick",       eff_type_stim, eff_arg_tick);
        
    // Then health
    do_extract("health_amount",     eff_type_health, eff_arg_amount);
    do_extract("health_min",        eff_type_health, eff_arg_min);
    do_extract("health_max",        eff_type_health, eff_arg_max);
    do_extract("health_max_val",    eff_type_health, eff_arg_min_val);
    do_extract("health_max_val",    eff_type_health, eff_arg_max_val);
    do_extract("health_chance",     eff_type_health, eff_arg_chance_top);
    do_extract("health_chance_bot", eff_type_health, eff_arg_chance_bot);
    do_extract("health_tick",       eff_type_health, eff_arg_tick);

    // Then health mod
    do_extract("h_mod_amount",     eff_type_h_mod, eff_arg_amount);
    do_extract("h_mod_min",        eff_type_h_mod, eff_arg_min);
    do_extract("h_mod_max",        eff_type_h_mod, eff_arg_max);
    do_extract("h_mod_max_val",    eff_type_h_mod, eff_arg_min_val);
    do_extract("h_mod_max_val",    eff_type_h_mod, eff_arg_max_val);
    do_extract("h_mod_chance",     eff_type_h_mod, eff_arg_chance_top);
    do_extract("h_mod_chance_bot", eff_type_h_mod, eff_arg_chance_bot);
    do_extract("h_mod_tick",       eff_type_h_mod, eff_arg_tick);

    // Then radiation
    do_extract("rad_amount",     eff_type_rad, eff_arg_amount);
    do_extract("rad_min",        eff_type_rad, eff_arg_min);
    do_extract("rad_max",        eff_type_rad, eff_arg_max);
    do_extract("rad_max_val",    eff_type_rad, eff_arg_max_val);
    do_extract("rad_chance",     eff_type_rad, eff_arg_chance_top);
    do_extract("rad_chance_bot", eff_type_rad, eff_arg_chance_bot);
    do_extract("rad_tick",       eff_type_rad, eff_arg_tick);
        
    // Then hunger
    do_extract("hunger_amount",     eff_type_hunger, eff_arg_amount);
    do_extract("hunger_min",        eff_type_hunger, eff_arg_min);
    do_extract("hunger_max",        eff_type_hunger, eff_arg_max);
    do_extract("hunger_max_val",    eff_type_hunger, eff_arg_min_val);
    do_extract("hunger_max_val",    eff_type_hunger, eff_arg_max_val);
    do_extract("hunger_chance",     eff_type_hunger, eff_arg_chance_top);
    do_extract("hunger_chance_bot", eff_type_hunger, eff_arg_chance_bot);
    do_extract("hunger_tick",       eff_type_hunger, eff_arg_tick);

    // Then thirst
    do_extract("thirst_amount",     eff_type_thirst, eff_arg_amount);
    do_extract("thirst_min",        eff_type_thirst, eff_arg_min);
    do_extract("thirst_max",        eff_type_thirst, eff_arg_max);
    do_extract("thirst_max_val",    eff_type_thirst, eff_arg_min_val);
    do_extract("thirst_max_val",    eff_type_thirst, eff_arg_max_val);
    do_extract("thirst_chance",     eff_type_thirst, eff_arg_chance_top);
    do_extract("thirst_chance_bot", eff_type_thirst, eff_arg_chance_bot);
    do_extract("thirst_tick",       eff_type_thirst, eff_arg_tick);
        
    // Then fatigue
    do_extract("fatigue_amount",     eff_type_fatigue, eff_arg_amount);
    do_extract("fatigue_min",        eff_type_fatigue, eff_arg_min);
    do_extract("fatigue_max",        eff_type_fatigue, eff_arg_max);
    do_extract("fatigue_max_val",    eff_type_fatigue, eff_arg_min_val);
    do_extract("fatigue_max_val",    eff_type_fatigue, eff_arg_max_val);
    do_extract("fatigue_chance",     eff_type_fatigue, eff_arg_chance_top);
    do_extract("fatigue_chance_bot", eff_type_fatigue, eff_arg_chance_bot);
    do_extract("fatigue_tick",       eff_type_fatigue, eff_arg_tick);
        
    // Then coughing
    do_extract("cough_chance",     eff_type_cough, eff_arg_chance_top);
    do_extract("cough_chance_bot", eff_type_cough, eff_arg_chance_bot);
    do_extract("cough_tick",       eff_type_cough, eff_arg_tick);
        
    // Then vomiting
    do_extract("vomit_chance",     eff_type_vomit, eff_arg_chance_top);
    do_extract("vomit_chance_bot", eff_type_vomit, eff_arg_chance_bot);
    do_extract("vomit_tick",       eff_type_vomit, eff_arg_tick);
        
    return true;
}

effect_type::effect_type() {}
effect_type::effect_type(const effect_type &) {}

effect_rating effect_type::get_rating() const
{
    return rating;
}

bool effect_type::use_name_ints() const
{
    return static_cast<size_t>(max_intensity) <= name.size();
}

bool effect_type::use_desc_ints(bool reduced) const
{
    auto const n = static_cast<size_t>(max_intensity);
    return n <= (reduced ? reduced_desc.size() : desc.size());
}

game_message_type effect_type::gain_game_message_type() const
{
    switch(rating) {
    case e_good:
        return m_good;
    case e_bad:
        return m_bad;
    case e_neutral:
        return m_neutral;
    case e_mixed:
        return m_mixed;
    default:
        return m_neutral;  // Should never happen
    }
}

game_message_type effect_type::lose_game_message_type() const
{
    switch(rating) {
    case e_good:
        return m_bad;
    case e_bad:
        return m_good;
    case e_neutral:
        return m_neutral;
    case e_mixed:
        return m_mixed;
    default:
        return m_neutral;  // Should never happen
    }
}

std::string effect_type::get_apply_message() const
{
    return apply_message;
}

std::string effect_type::get_apply_memorial_log() const
{
    return apply_memorial_log;
}

std::string effect_type::get_remove_message() const
{
    return remove_message;
}

std::string effect_type::get_remove_memorial_log() const
{
    return remove_memorial_log;
}

bool effect_type::get_main_parts() const
{
    return main_parts_only;
}

bool effect_type::load_miss_msgs(JsonObject &jo, std::string const& member)
{
    if (!jo.has_array(member)) {
        return false;
    }

    for (auto outer = jo.get_array(member); outer.has_more(); ) {
        JsonArray inner = outer.next_array();
        miss_msgs.push_back(std::make_pair(inner.get_string(0), inner.get_int(1)));
    }

    return true;
}

bool effect_type::load_decay_msgs(JsonObject &jo, std::string const& member)
{
    if (!jo.has_array(member)) {
        return false;
    }

    for (auto outer = jo.get_array(member); outer.has_more(); ) {
        JsonArray inner = outer.next_array();

        decay_msgs.push_back(std::make_pair(
            inner.get_string(0)                         //message
          , from_string(inner.get_string(1), m_neutral) //rate
        ));
    }

    return true;
}

std::string effect::disp_name() const
{
    // End result should look like "name (l. arm)" or "name [intensity] (l. arm)"
    std::stringstream ret;

    auto const& type = *eff_type;

    auto const i = type.use_name_ints() ? intensity - 1 : 0;

    ret << _(type.name[i].c_str());

    if (i == 0 && intensity > 1) {
        ret << " [" << intensity << "]";
    }

    if (bp != num_bp) {
        ret << " (" << body_part_name(bp) << ")";
    }

    return ret.str();
}

// Used in disp_desc()
struct desc_freq {
    double chance;
    int val;
    std::string pos_string;
    std::string neg_string;
    
    desc_freq(double c, int v, std::string pos, std::string neg) : chance(c), val(v), pos_string(pos), neg_string(neg) {};
};

std::string effect::disp_desc(bool const reduced) const
{
    std::stringstream ret;

    auto const write_stat = [&](std::string const& stat, char const* fmt) {
        auto const value = get_avg_mod(stat, reduced);
        
        if (value != 0) {
            ret << string_format(_(fmt), value);
        }
    };

    auto const write_stats = [&] {
        write_stat(eff_type_str,   "Strength %+d;  ");
        write_stat(eff_type_dex,   "Dexterity %+d;  ");
        write_stat(eff_type_per,   "Perception %+d;  ");
        write_stat(eff_type_int,   "Intelligence %+d;  ");
        write_stat(eff_type_speed, "Speed %+d;  ");

        // Newline if necessary
        if (ret.rdbuf()->in_avail()) {
            ret << "\n";
        }
    };

    write_stats();
    
    // Then print pain/damage/coughing/vomiting, we don't display pkill, health, or radiation
    std::vector<desc_freq> values;

    // Add various desc_freq structs to values. If more effects wish to be placed in the descriptions this is the
    // place to add them.
    
    auto const add_value = [&](std::string const& stat, char const* desc) {
        auto const value   = get_avg_mod(stat, reduced);
        auto const percent = get_percentage(stat, value, reduced);
        auto const str     = _(desc);

        values.push_back(desc_freq(percent, value, str, str));
    };

    add_value(eff_type_pain,    "pain");
    add_value(eff_type_hurt,    "damage");
    add_value(eff_type_thirst,  "thirst");
    add_value(eff_type_hunger,  "hunger");
    add_value(eff_type_fatigue, "fatigue");
    add_value(eff_type_cough,   "coughing");
    add_value(eff_type_vomit,   "vomiting");
    add_value(eff_type_sleep,   "blackouts");

    using container_t = std::vector<std::reference_wrapper<std::string const>>;

    container_t constant;
    container_t frequent;
    container_t uncommon;
    container_t rare;

    for (auto const& i : values) {
        if (i.val == 0) {
            continue;
        }

        auto const& desc = (i.val > 0) ? i.pos_string : i.neg_string;

        constexpr auto p_constant = 50.0;// +50% chance, every other step
        constexpr auto p_frequent = 1.0; // +1% chance, every 100 steps
        constexpr auto p_uncommon = 0.4; // +.4% chance, every 250 steps
        constexpr auto p_rare     = 0.0; // <.4% chance

        if (i.chance >= p_constant) {
            constant.push_back(std::cref(desc));
        } else if (i.chance >= p_frequent) {
            frequent.push_back(std::cref(desc));
        } else if (i.chance >= p_uncommon) {
            uncommon.push_back(std::cref(desc));
        } else if (i.chance > p_rare) {
            rare.push_back(std::cref(desc));
        }
    }

    auto const write_values = [&](container_t const& c, char const* desc) {
        if (c.empty()) {
            return false;
        }

        ret << _(desc);
        
        std::copy(
            std::begin(c), std::end(c)
          , std::ostream_iterator<std::string const&> {ret, ", "}
        );

        return true;
    };

    if (write_values(constant, "Const: ")) {
        ret << " ";
    }

    if (write_values(frequent, "Freq: ")) {
        ret << " ";
    }

    if (write_values(uncommon, "Unfreq: ")) {
        ret << " ";
    }

    if (write_values(rare, "Rare: ")) {
        ret << "\n";
    }
    
    // Then print the effect description
    if (use_part_descs()) {
        //~ Used for effect descriptions, "Your (body_part_name) (effect description)"
        ret << string_format(_("Your %s "), body_part_name(bp).c_str());
    }

    auto const& type = *eff_type;
    auto const  i = type.use_desc_ints(reduced) ? intensity - 1 : 0;
    auto const& s = (reduced ? type.reduced_desc : type.desc)[i];

    ret << _(s.c_str());

    return ret.str();
}

void effect::decay(
    std::vector<std::string> &rem_ids
  , std::vector<body_part> &rem_bps
  , unsigned int const turn
  , bool const player
) {
    // Decay duration if not permanent
    if (!is_permanent()) {
        // Add to removal list if duration is <= 0
        if (--duration <= 0) {
            rem_ids.push_back(get_id());
            rem_bps.push_back(bp);
        }
        
        add_msg( m_debug, "ID: %s, Duration %d", get_id().c_str(), duration );
    }

    // Fix bad intensities
    // Bound intensity to [1, max_intensity] ... not sure if this can happen
    set_intensity(intensity);

    // Store current intensity for comparison later
    auto const old_int = intensity;   

    auto const dtick = eff_type->int_decay_tick;
    // Decay intensity if necessary
    if ((intensity > 1) && (dtick > 0) && (turn % dtick) == 0) {
        mod_intensity(dtick);
    }

    // Force intensity if it is duration based
    if (eff_type->int_dur_factor != 0) {
        //will get clamped to[1, max_intensity]
        set_intensity(duration / eff_type->int_dur_factor);
    }
    
    // Display decay message if available
    auto const msg_size = static_cast<int>(eff_type->decay_msgs.size());
    auto const i = (intensity - 1); // -1 because intensity = 1 is the first message

    if (player && (old_int > intensity) && (i < msg_size)) {
        auto const& msg = eff_type->decay_msgs[i];
        add_msg(msg.second, msg.first.c_str());
    }
}

bool effect::use_part_descs() const
{
    return eff_type->part_descs;
}

int effect::get_duration() const
{
    return duration;
}

int effect::get_max_duration() const
{
    return eff_type->max_duration;
}

void effect::set_duration(int const dur)
{
    // Cap to max_duration if it exists
    auto const max = (eff_type->max_duration > 0)
      ? eff_type->max_duration
      : std::numeric_limits<int>::max();

    duration = clamp(dur, 0, max);
}

void effect::mod_duration(int dur)
{
    set_duration(duration + dur);
}

void effect::mult_duration(double const dur)
{
    set_duration(round_to<int>(duration * dur));
}

body_part effect::get_bp() const
{
    return bp;
}

void effect::set_bp(body_part part)
{
    bp = part;
}

bool effect::is_permanent() const
{
    return permanent;
}

void effect::pause_effect()
{
    permanent = true;
}

void effect::unpause_effect()
{
    permanent = false;
}

int effect::get_intensity() const
{
    return intensity;
}

int effect::get_max_intensity() const
{
    return eff_type->max_intensity;
}

void effect::set_intensity(int nintensity)
{
    if (intensity < 0) {
        //not sure this is needed anymore
        add_msg( m_debug, "Bad intensity, ID: %s", get_id().c_str() );
    }

    intensity = clamp(nintensity, 1, eff_type->max_intensity);
}

void effect::mod_intensity(int nintensity)
{
    set_intensity(intensity + nintensity);
}

std::string effect::get_resist_trait() const
{
    return eff_type->resist_trait;
}

std::string effect::get_resist_effect() const
{
    return eff_type->resist_effect;
}

std::string effect::get_removes_effect() const
{
    return eff_type->removes_effect;
}

//------------------------------------------------------------------------------
//! return the value in mod_data at key amended with mods and arg, otherwise 0.0.
//------------------------------------------------------------------------------
template <typename Container, typename Key = typename Container::key_type>
double get_mod_data_value(
    Container const& mod_data
  , Key& key
  , std::string const& mods
  , std::string const& arg
) {
    using std::end;

    std::get<0>(key) = mods;
    std::get<3>(key) = arg;

    auto const where  = mod_data.find(key);
    auto const found  = where != end(mod_data);
    auto const result = found ? where->second : 0.0;

    return result;
}

//------------------------------------------------------------------------------
//! just a simple struct holding "base_mods" and "scaling_mods"
//------------------------------------------------------------------------------
struct mod_data_t {
    double base;
    double scaling;

    double get_value(int const intensity) const noexcept {
        return base + scaling * (intensity - 1);
    }
};

//------------------------------------------------------------------------------
//! return the "base_mods" and "scaling_mods" pair for a key containing field
//------------------------------------------------------------------------------
template <typename Container, typename Key = typename Container::key_type>
mod_data_t get_mod_data(
    Container const&   mod_data
  , Key&               key
  , std::string const& field
) {
    return {
        get_mod_data_value(mod_data, key, eff_mods_base,    field)
      , get_mod_data_value(mod_data, key, eff_mode_scaling, field)
    };
}

int effect::get_mod(std::string const& arg, bool const reduced) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    auto const& mod_data = eff_type->mod_data;

    auto const min = get_mod_data(mod_data, key, eff_arg_min).get_value(intensity);
    auto const max = get_mod_data(mod_data, key, eff_arg_max).get_value(intensity);

    auto const imin = round_to<long>(min);
    auto const imax = round_to<long>(max);
    
    // Return a random value between [min, max]
    // Else return the minimum value
    auto const result = (imax != 0) ? rng(imin, imax) : imin;

    return static_cast<int>(result);
}

int effect::get_avg_mod(std::string const& arg, bool const reduced) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    auto const& mod_data = eff_type->mod_data;

    auto const min = get_mod_data(mod_data, key, eff_arg_min).get_value(intensity);
    auto const max = get_mod_data(mod_data, key, eff_arg_max).get_value(intensity);

    auto const imin = round_to<int>(min);
    auto const imax = round_to<int>(max);

    // Return an average of min and max
    // Else return the minimum value
    return (imax != 0)
      ? round_to<int>((min + max) / 2.0)
      : imin;
}

int effect::get_amount(std::string const& arg, bool const reduced) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    return round_to<int>(
        get_mod_data(eff_type->mod_data, key, eff_arg_amount).get_value(intensity)
    );
}

int effect::get_min_val(std::string const& arg, bool const reduced) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    return round_to<int>(
        get_mod_data(eff_type->mod_data, key, eff_arg_min_value).get_value(intensity)
    );
}

int effect::get_max_val(std::string const& arg, bool const reduced) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    return round_to<int>(
        get_mod_data(eff_type->mod_data, key, eff_arg_max_value).get_value(intensity)
    );
}

bool effect::get_sizing(std::string const& arg) const
{
    if (arg == eff_type_pain) {
        return eff_type->pain_sizing;
    } else if (arg == eff_type_hurt) {
        return eff_type->hurt_sizing;
    }

    return false;
}

bool is_valueless_effect(int const value, mod_data_t const& top) {
    // Check chances if value is 0 (so we can check valueless effects like vomiting)
    // Else a nonzero value overrides a 0 chance for default purposes

    // If both top values <= 0 then it should never trigger
    // It will also never trigger if top_base + top_scale <= 0

    if (value != 0) {
        return false;
    }

    if (top.base <= 0.0 && top.scaling <= 0.0) {
        return false;
    }

    if (top.base + top.scaling <= 0.0) {
        return false;
    }

    return true;
}

double effect::get_percentage(std::string const& arg, int const val, bool const reduced) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    auto &mod_data = eff_type->mod_data;

    auto const top  = get_mod_data(mod_data, key, eff_arg_chance_top);
    auto const bot  = get_mod_data(mod_data, key, eff_arg_chance_bot);
    auto const tick = get_mod_data(mod_data, key, eff_arg_tick);

    if (val == 0 && !is_valueless_effect(val, top)) {
        return 0.0;
    }
    
    auto const num = 100.0 * top.get_value(intensity);
    auto const den = bot.get_value(intensity);
    
    // Tick is the exception where tick = 0 means tick = 1
    // Divide by ticks between rolls
    auto t = tick.get_value(intensity);
    t = (t > 1.0) ? (1.0 / t) : 1.0;

    // If both bot values are zero the formula is one_in(top), else the formula is x_in_y(top, bot)
    if (den != 0.0) {
        return t * 100.0 * num / den;
    } else if (num != 0.0) {
        return t * 100.0 / num;
    }

    return 0.0;
}

bool effect::activated(unsigned int const turn, std::string const& arg, int const val, bool const reduced, double const mod) const
{
    effect_type::key_type key {"", reduced, arg, ""};

    auto &mod_data = eff_type->mod_data;

    auto const top  = get_mod_data(mod_data, key, eff_arg_chance_top);
    auto const bot  = get_mod_data(mod_data, key, eff_arg_chance_bot);
    auto const tick = get_mod_data(mod_data, key, eff_arg_tick);

    if (val == 0 && !is_valueless_effect(val, top)) {
        return 0.0;
    }
    
    auto const num = mod * top.get_value(intensity);
    auto const den = bot.get_value(intensity);
    
    // Tick is the exception where tick = 0 means tick = 1
    // Divide by ticks between rolls
    auto const t_sum = round_to<int>(tick.get_value(intensity));
    auto const t = (t_sum == 0) ? int {1} : t_sum;

    // Check if tick allows for triggering. If both bot values are zero the formula is 
    // x_in_y(1, top) i.e. one_in(top), else the formula is x_in_y(top, bot),
    // mod multiplies the overall percentage chances
    
    if (turn % t != 0) {
        return false;
    }

   return (den != 0) ? x_in_y(num, den) : x_in_y(mod, num);
}

double effect::get_addict_mod(std::string const& arg, int const addict_level) const
{
    // TODO: convert this to JSON id's and values once we have JSON'ed addictions
    if (arg != eff_type_pkill || eff_type->pkill_addict_reduces) {
        return 1.0;
    }

    return 1.0 / std::max(addict_level * 2.0, 1.0);
}

bool effect::get_harmful_cough() const
{
    return eff_type->harmful_cough;
}

int effect::get_dur_add_perc() const
{
    return eff_type->dur_add_perc;
}

int effect::get_int_add_val() const
{
    return eff_type->int_add_val;
}

std::vector<std::pair<std::string, int>> effect::get_miss_msgs() const
{
    return eff_type->miss_msgs;
}

std::string effect::get_speed_name() const
{
    // uses the speed_mod_name if one exists, else defaults to the first entry in "name".
    auto const& type = *eff_type;

    return type.speed_mod_name.empty()
      ? type.name[0]
      : type.speed_mod_name;
}

effect_type *effect::get_effect_type() const
{
    return eff_type;
}

void load_effect_type(JsonObject &jo)
{
    effect_type new_etype;
    new_etype.id = jo.get_string("id");

    auto const load_vector = [&](std::vector<std::string>& out, std::string const& field) {
        if(!jo.has_member(field)) {
            out.emplace_back();
        }
    
        for (auto jsarr = jo.get_array(field); jsarr.has_more(); ) {
            out.emplace_back(_(jsarr.next_string().c_str()));
        }
    };

    load_vector(new_etype.name, "name");

    new_etype.speed_mod_name = jo.get_string("speed_name", "");

    load_vector(new_etype.desc, "desc");

    if (jo.has_member("reduced_desc")) {
        load_vector(new_etype.reduced_desc, "reduced_desc");
    } else {
        load_vector(new_etype.reduced_desc, "desc");
    }

    new_etype.part_descs = jo.get_bool("part_descs", false);

    auto const rating_fallback = effect_rating::e_neutral;

    new_etype.rating = jo.has_member("rating")
      ? from_string(jo.get_string("rating"), rating_fallback)
      : rating_fallback;

    new_etype.apply_message = jo.get_string("apply_message", "");
    new_etype.remove_message = jo.get_string("remove_message", "");
    new_etype.apply_memorial_log = jo.get_string("apply_memorial_log", "");
    new_etype.remove_memorial_log = jo.get_string("remove_memorial_log", "");
    
    new_etype.resist_trait = jo.get_string("resist_trait", "");
    new_etype.resist_effect = jo.get_string("resist_effect", "");
    new_etype.removes_effect = jo.get_string("removes_effect", "");
    
    new_etype.max_intensity = jo.get_int("max_intensity", 1);
    new_etype.max_duration = jo.get_int("max_duration", 0);
    new_etype.dur_add_perc = jo.get_int("dur_add_perc", 100);
    new_etype.int_add_val = jo.get_int("int_add_val", 0);
    new_etype.int_decay_step = jo.get_int("int_decay_step", -1);
    new_etype.int_decay_tick = jo.get_int("int_decay_tick", 0);
    new_etype.int_dur_factor = jo.get_int("int_dur_factor", 0);
    
    new_etype.load_miss_msgs(jo, "miss_messages");
    new_etype.load_decay_msgs(jo, "decay_messages");
    
    new_etype.main_parts_only = jo.get_bool("main_parts_only", false);
    new_etype.pkill_addict_reduces = jo.get_bool("pkill_addict_reduces", false);
    
    new_etype.pain_sizing = jo.get_bool("pain_sizing", false);
    new_etype.hurt_sizing = jo.get_bool("hurt_sizing", false);
    new_etype.harmful_cough = jo.get_bool("harmful_cough", false);
    
    new_etype.load_mod_data(jo, "base_mods");
    new_etype.load_mod_data(jo, "scaling_mods");

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}

void effect::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("eff_type", eff_type != nullptr ? eff_type->id : "");
    json.member("duration", duration);
    json.member("bp", (int)bp);
    json.member("permanent", permanent);
    json.member("intensity", intensity);
    json.end_object();
}
void effect::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();
    eff_type = &effect_types[jo.get_string("eff_type")];
    duration = jo.get_int("duration");
    bp = (body_part)jo.get_int("bp");
    permanent = jo.get_bool("permanent");
    intensity = jo.get_int("intensity");
}
