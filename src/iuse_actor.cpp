#include "iuse_actor.h"
#include "action.h"
#include "assign.h"
#include "item.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "monster.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "translations.h"
#include "morale_types.h"
#include "messages.h"
#include "material.h"
#include "event.h"
#include "crafting.h"
#include "ui.h"
#include "itype.h"
#include "vehicle.h"
#include "mtype.h"
#include "mapdata.h"
#include "field.h"
#include "weather.h"
#include "pldata.h"
#include "requirements.h"
#include "recipe_dictionary.h"
#include "player.h"
#include "generic_factory.h"
#include "map_iterator.h"
#include "cata_utility.h"
#include "string_input_popup.h"
#include "options.h"

#include <sstream>
#include <algorithm>
#include <numeric>

const skill_id skill_mechanics( "mechanics" );
const skill_id skill_survival( "survival" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_fabrication( "fabrication" );

const species_id ZOMBIE( "ZOMBIE" );

const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_music( "music" );
const efftype_id effect_playing_instrument( "playing_instrument" );
const efftype_id effect_recover( "recover" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_asthma( "asthma" );

static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PRED1( "PRED1" );
static const trait_id trait_PRED2( "PRED2" );
static const trait_id trait_PRED3( "PRED3" );
static const trait_id trait_PRED4( "PRED4" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );

iuse_transform::~iuse_transform()
{
}

iuse_actor *iuse_transform::clone() const
{
    return new iuse_transform(*this);
}

void iuse_transform::load( JsonObject &obj )
{
    target = obj.get_string( "target" ); // required

    obj.read( "msg", msg_transform );
    obj.read( "container", container );
    obj.read( "target_charges", ammo_qty );
    obj.read( "target_ammo", ammo_type );

    obj.read( "countdown", countdown );

    if( !ammo_type.empty() && !container.empty() ) {
        obj.throw_error( "Transform actor specified both ammo type and container type", "target_ammo" );
    }

    obj.read( "active", active );

    obj.read( "moves", moves );
    if( moves < 0 ) {
        obj.throw_error( "transform actor specified negative moves", "moves" );
    }

    obj.read( "need_fire", need_fire );
    need_fire = std::max( need_fire, 0L );
    need_charges_msg = obj.has_string( "need_charges_msg" ) ? _( obj.get_string( "need_charges_msg" ).c_str() ) : _( "The %s is empty!" );

    obj.read( "need_charges", need_charges );
    need_charges = std::max( need_charges, 0L );
    need_fire_msg = obj.has_string( "need_fire_msg" ) ? _( obj.get_string( "need_fire_msg" ).c_str() ) : _( "You need a source of fire!" );

    obj.read( "menu_text", menu_text );
    if( !menu_text.empty() ) {
        menu_text = _( menu_text.c_str() );
    }
}

long iuse_transform::use(player *p, item *it, bool t, const tripoint &pos ) const
{
    if( t ) {
        return 0; // invoked from active item processing, do nothing.
    }

    bool possess = p && p->has_item( *it );

    if( need_charges && it->ammo_remaining() < need_charges ) {
        if( possess ) {
            p->add_msg_if_player( m_info, need_charges_msg.c_str(), it->tname().c_str() );
        }
        return 0;
    }

    if( need_fire && possess ) {
        if( !p->use_charges_if_avail( "fire", need_fire ) ) {
            p->add_msg_if_player( m_info, need_fire_msg.c_str(), it->tname().c_str() );
            return 0;
        }
        if( p->is_underwater() ) {
            p->add_msg_if_player( m_info, _( "You can't do that while underwater" ) );
            return 0;
        }
    }

    if( p ) {
        if( p->sees( pos ) && !msg_transform.empty() ) {
            p->add_msg_if_player( m_neutral, _( msg_transform.c_str() ), it->tname().c_str() );
        }
        if( possess ) {
            p->moves -= moves;
        }
    }

    item *obj;
    if( container.empty() ) {
        obj = &it->convert( target );
        if( ammo_qty >= 0 ) {
            if( !ammo_type.empty() ) {
                obj->ammo_set( ammo_type, ammo_qty );
            } else if( obj->ammo_current() != "null" ) {
                obj->ammo_set( obj->ammo_current(), ammo_qty );
            } else {
                obj->set_countdown( ammo_qty );
            }
        }
    } else {
        it->convert( container );
        obj = &it->emplace_back( target, calendar::turn, std::max( ammo_qty, 1l ) );
    }

    obj->item_counter = countdown > 0 ? countdown : obj->type->countdown_interval;
    obj->active = active || obj->item_counter;

    return 0;
}

std::string iuse_transform::get_name() const
{
    if( !menu_text.empty() ) {
        return menu_text;
    }
    return iuse_actor::get_name();
}

void iuse_transform::finalize( const itype_id & )
{
    if( !item::type_is_defined( target ) ) {
        debugmsg( "Invalid transform target: %s", target.c_str() );
    }

    if( !container.empty() ) {
        if( !item::type_is_defined( container ) ) {
            debugmsg( "Invalid transform container: %s", container.c_str() );
        }

        item dummy( target );
        if( ammo_qty > 1 && !dummy.count_by_charges() ) {
            debugmsg( "Transform target with container must be an item with charges, got non-charged: %s", target.c_str() );
        }
    }
}

void iuse_transform::info( const item &it, std::vector<iteminfo> &dump ) const
{
    const item dummy( target, calendar::turn, std::max( ammo_qty, 1l ) );
    dump.emplace_back( "TOOL", string_format( _( "<bold>Turns into</bold>: %s" ),
                                              dummy.tname().c_str() ) );
    if( countdown > 0 ) {
        dump.emplace_back( "TOOL", _( "Countdown: " ), "", countdown );
    }

    const auto *explosion_use = dummy.get_use( "explosion" );
    if( explosion_use != nullptr ) {
        explosion_use->get_actor_ptr()->info( it, dump );
    }
}

countdown_actor::~countdown_actor() = default;

iuse_actor *countdown_actor::clone() const
{
    return new countdown_actor( *this );
}

void countdown_actor::load( JsonObject &obj )
{
    obj.read( "name", name );
    obj.read( "interval", interval );
    obj.read( "message", message );
}

long countdown_actor::use( player *p, item *it, bool t, const tripoint &pos ) const
{
    if( t ) {
        return 0;
    }

    if( it->active ) {
        return 0;
    }

    if( p ) {
        if( p->sees( pos ) && !message.empty() ) {
            p->add_msg_if_player( m_neutral, _( message.c_str() ), it->tname().c_str() );
        }
    }

    it->item_counter = interval > 0 ? interval : it->type->countdown_interval;
    it->active = true;
    return 0;
}

bool countdown_actor::can_use( const player *, const item *it, bool, const tripoint & ) const
{
    return !it->active;
}

std::string countdown_actor::get_name() const
{
    if( !name.empty() ) {
        return name;
    }
    return iuse_actor::get_name();
}

void countdown_actor::info( const item &it, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "TOOL", _( "Countdown: " ), "",
                       interval > 0 ? interval : it.type->countdown_interval );
    const auto countdown_actor = it.type->countdown_action.get_actor_ptr();
    if( countdown_actor != nullptr ) {
        countdown_actor->info( it, dump );
    }
}

explosion_iuse::~explosion_iuse()
{
}

iuse_actor *explosion_iuse::clone() const
{
    return new explosion_iuse(*this);
}

// defined in iuse.cpp
extern std::vector<tripoint> points_for_gas_cloud(const tripoint &center, int radius);

void explosion_iuse::load( JsonObject &obj )
{
    if( obj.has_object( "explosion" ) ) {
        auto expl = obj.get_object( "explosion" );
        explosion = load_explosion_data( expl );
    }

    obj.read( "draw_explosion_radius", draw_explosion_radius );
    if( obj.has_member( "draw_explosion_color" ) ) {
        draw_explosion_color = color_from_string( obj.get_string( "draw_explosion_color" ) );
    }
    obj.read( "do_flashbang", do_flashbang );
    obj.read( "flashbang_player_immune", flashbang_player_immune );
    obj.read( "fields_radius", fields_radius );
    if( obj.has_member( "fields_type" ) || fields_radius > 0 ) {
        fields_type = field_from_ident( obj.get_string( "fields_type" ) );
    }
    obj.read( "fields_min_density", fields_min_density );
    obj.read( "fields_max_density", fields_max_density );
    obj.read( "emp_blast_radius", emp_blast_radius );
    obj.read( "scrambler_blast_radius", scrambler_blast_radius );
    obj.read( "sound_volume", sound_volume );
    obj.read( "sound_msg", sound_msg );
    obj.read( "no_deactivate_msg", no_deactivate_msg );
}

long explosion_iuse::use(player *p, item *it, bool t, const tripoint &pos) const
{
    if (t) {
        if (sound_volume >= 0) {
            sounds::sound(pos, sound_volume, sound_msg.empty() ? "" : _(sound_msg.c_str()) );
        }
        return 0;
    }
    if( it->charges > 0 ) {
        if (no_deactivate_msg.empty()) {
            p->add_msg_if_player(m_warning,
                                 _("You've already set the %s's timer you might want to get away from it."), it->tname().c_str());
        } else {
            p->add_msg_if_player(m_info, _( no_deactivate_msg.c_str() ), it->tname().c_str());
        }
        return 0;
    }

    if( explosion.power >= 0.0f ) {
        g->explosion( pos, explosion );
    }

    if (draw_explosion_radius >= 0) {
        g->draw_explosion( pos, draw_explosion_radius, draw_explosion_color);
    }
    if (do_flashbang) {
        g->flashbang( pos, flashbang_player_immune);
    }
    if (fields_radius >= 0 && fields_type != fd_null) {
        std::vector<tripoint> gas_sources = points_for_gas_cloud(pos, fields_radius);
        for( auto &gas_source : gas_sources ) {
            const int dens = rng(fields_min_density, fields_max_density);
            g->m.add_field( gas_source, fields_type, dens, 1 );
        }
    }
    if (scrambler_blast_radius >= 0) {
        for (int x = pos.x - scrambler_blast_radius; x <= pos.x + scrambler_blast_radius; x++) {
            for (int y = pos.y - scrambler_blast_radius; y <= pos.y + scrambler_blast_radius; y++) {
                g->scrambler_blast( tripoint( x, y, pos.z ) );
            }
        }
    }
    if (emp_blast_radius >= 0) {
        for (int x = pos.x - emp_blast_radius; x <= pos.x + emp_blast_radius; x++) {
            for (int y = pos.y - emp_blast_radius; y <= pos.y + emp_blast_radius; y++) {
                g->emp_blast( tripoint( x, y, pos.z ) );
            }
        }
    }
    return 1;
}

void explosion_iuse::info( const item &, std::vector<iteminfo> &dump ) const
{
    if( explosion.power <= 0 ) {
        // @todo List other effects, like EMP and clouds
        return;
    }

    dump.emplace_back( "TOOL", _( "Power at <bold>epicenter</bold>: " ), "", explosion.power );
    const auto &sd = explosion.shrapnel;
    if( sd.count > 0 ) {
        dump.emplace_back( "TOOL", _( "Shrapnel <bold>count</bold>: " ), "", sd.count );
        dump.emplace_back( "TOOL", _( "Shrapnel <bold>mass</bold>: " ), "", sd.mass );
    }
}



unfold_vehicle_iuse::~unfold_vehicle_iuse()
{
}

iuse_actor *unfold_vehicle_iuse::clone() const
{
    return new unfold_vehicle_iuse(*this);
}

void unfold_vehicle_iuse::load( JsonObject &obj )
{
    vehicle_id = vproto_id( obj.get_string( "vehicle_name" ) );
    obj.read( "unfold_msg", unfold_msg );
    obj.read( "moves", moves );
    obj.read( "tools_needed", tools_needed );
}

long unfold_vehicle_iuse::use(player *p, item *it, bool /*t*/, const tripoint &/*pos*/) const
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }

    for (auto tool = tools_needed.cbegin(); tool != tools_needed.cend(); ++tool) {
        // Amount == -1 means need one, but don't consume it.
        if (!p->has_amount(tool->first, 1)) {
            p->add_msg_if_player(_("You need %s to do it!"),
                                 item::nname( tool->first ).c_str());
            return 0;
        }
    }

    vehicle *veh = g->m.add_vehicle(vehicle_id, p->posx(), p->posy(), 0, 0, 0, false);
    if (veh == NULL) {
        p->add_msg_if_player(m_info, _("There's no room to unfold the %s."), it->tname().c_str());
        return 0;
    }

    // Mark the vehicle as foldable.
    veh->tags.insert("convertible");
    // Store the id of the item the vehicle is made of.
    veh->tags.insert(std::string("convertible:") + it->typeId());
    if( !unfold_msg.empty() ) {
        p->add_msg_if_player( _( unfold_msg.c_str() ), it->tname().c_str());
    }
    p->moves -= moves;
    // Restore HP of parts if we stashed them previously.
    if( it->has_var( "folding_bicycle_parts" ) ) {
        // Brand new, no HP stored
        return 1;
    }
    std::istringstream veh_data;
    const auto data = it->get_var( "folding_bicycle_parts" );
    veh_data.str(data);
    if (!data.empty() && data[0] >= '0' && data[0] <= '9') {
        // starts with a digit -> old format
        for( auto &elem : veh->parts ) {
            int tmp;
            veh_data >> tmp;
            veh->set_hp( elem, tmp );
        }
    } else {
        try {
            JsonIn json(veh_data);
            // Load parts into a temporary vector to not override
            // cached values (like precalc, passenger_id, ...)
            std::vector<vehicle_part> parts;
            json.read(parts);
            for(size_t i = 0; i < parts.size() && i < veh->parts.size(); i++) {
                const vehicle_part &src = parts[i];
                vehicle_part &dst = veh->parts[i];
                // and now only copy values, that are
                // expected to be consistent.
                veh->set_hp( dst, src.hp() );
                dst.blood = src.blood;
                // door state/amount of fuel/direction of headlight
                dst.ammo_set( src.ammo_current(), src.ammo_remaining() );
                dst.flags = src.flags;
            }
        } catch( const JsonError &e ) {
            debugmsg("Error restoring vehicle: %s", e.c_str());
        }
    }
    return 1;
}

consume_drug_iuse::~consume_drug_iuse() {}

iuse_actor *consume_drug_iuse::clone() const
{
    return new consume_drug_iuse(*this);
}

static effect_data load_effect_data( JsonObject &e )
{
    return effect_data( efftype_id( e.get_string( "id" ) ), e.get_int( "duration", 0 ),
        get_body_part_token( e.get_string( "bp", "NUM_BP" ) ), e.get_bool( "permanent", false ) );
}

void consume_drug_iuse::load( JsonObject &obj )
{
    obj.read( "activation_message", activation_message );
    obj.read( "charges_needed", charges_needed );
    obj.read( "tools_needed", tools_needed );

    if( obj.has_array( "effects" ) ) {
        JsonArray jsarr = obj.get_array( "effects" );
        while( jsarr.has_more() ) {
            JsonObject e = jsarr.next_object();
            effects.push_back( load_effect_data( e ) );
        }
    }
    obj.read( "stat_adjustments", stat_adjustments );
    obj.read( "fields_produced", fields_produced );
    obj.read( "moves", moves);

    auto arr = obj.get_array( "vitamins" );
    while( arr.has_more() ) {
        auto vit = arr.next_array();
        auto lo = vit.get_int ( 1 );
        auto hi = vit.size() >= 3 ? vit.get_int( 2 ) : lo;
        vitamins.emplace( vitamin_id( vit.get_string( 0 ) ), std::make_pair( lo, hi ) );
    }
}

void consume_drug_iuse::info( const item&, std::vector<iteminfo>& dump ) const
{
    const std::string vits = enumerate_as_string( vitamins.begin(), vitamins.end(),
    []( const decltype( vitamins )::value_type &v ) {
        const int rate = g->u.vitamin_rate( v.first );
        if( rate <= 0 ) {
            return std::string();
        }
        const int lo = int( v.second.first  / ( DAYS( 1 ) / float( rate ) ) * 100 );
        const int hi = int( v.second.second / ( DAYS( 1 ) / float( rate ) ) * 100 );

        return string_format( lo == hi ? "%s (%i%%)" : "%s (%i-%i%%)", v.first.obj().name().c_str(), lo, hi );
    } );

    if( !vits.empty() ) {
        dump.emplace_back( "TOOL", _( "Vitamins (RDA): " ), vits.c_str() );
    }

    if( tools_needed.count( "syringe" ) ) {
        dump.emplace_back( "TOOL", _( "You need a <info>syringe</info> to inject this drug" ) );
    }
}

long consume_drug_iuse::use(player *p, item *it, bool, const tripoint& ) const
{
    // Check prerequisites first.
    for( auto tool = tools_needed.cbegin(); tool != tools_needed.cend(); ++tool ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p->has_amount( tool->first, 1 ) ) {
            p->add_msg_player_or_say( _("You need %1$s to consume %2$s!"),
                _("I need a %1$s to consume %2$s!"),
                item::nname( tool->first ).c_str(),
                it->type_name( 1 ).c_str() );
            return -1;
        }
    }
    for( auto consumable = charges_needed.cbegin(); consumable != charges_needed.cend();
         ++consumable ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p->has_charges( consumable->first, (consumable->second == -1) ?
                             1 : consumable->second ) ) {
            p->add_msg_player_or_say( _("You need %1$s to consume %2$s!"),
                _("I need a %1$s to consume %2$s!"),
                item::nname( consumable->first ).c_str(),
                it->type_name( 1 ).c_str() );
            return -1;
        }
    }
    // Apply the various effects.
    for( auto eff : effects ) {
        int dur = eff.duration;
        if (p->has_trait( trait_TOLERANCE )) {
            dur *= .8;
        } else if (p->has_trait( trait_LIGHTWEIGHT )) {
            dur *= 1.2;
        }
        p->add_effect( eff.id, dur, eff.bp, eff.permanent );
    }
    for( auto stat = stat_adjustments.cbegin(); stat != stat_adjustments.cend(); ++stat ) {
        p->mod_stat( stat->first, stat->second );
    }
    for( auto field = fields_produced.cbegin(); field != fields_produced.cend(); ++field ) {
        const field_id fid = field_from_ident( field->first );
        for(int i = 0; i < 3; i++) {
            g->m.add_field({p->posx() + int(rng(-2, 2)), p->posy() + int(rng(-2, 2)), p->posz()}, fid, field->second, 0);
        }
    }

    // for vitamins that accumulate (max > 0) multivitamins risk causing hypervitaminosis
    for( const auto& v : vitamins ) {
        // players with mutations that remove the requirement for a vitamin cannot suffer accmulation of it
        p->vitamin_mod( v.first, rng( v.second.first, v.second.second ), p->vitamin_rate( v.first ) > 0 ? false : true );
    }

    // Output message.
    p->add_msg_if_player( string_format( _( activation_message.c_str() ), it->type_name( 1 ).c_str() ).c_str() );
    // Consume charges.
    for( auto consumable = charges_needed.cbegin(); consumable != charges_needed.cend();
         ++consumable ) {
        if( consumable->second != -1 ) {
            p->use_charges( consumable->first, consumable->second );
        }
    }
    p->moves -= moves;
    return it->type->charges_to_use();
}

delayed_transform_iuse::~delayed_transform_iuse() {}

iuse_actor *delayed_transform_iuse::clone() const
{
    return new delayed_transform_iuse(*this);
}

void delayed_transform_iuse::load( JsonObject &obj )
{
    iuse_transform::load( obj );
    not_ready_msg = obj.get_string( "not_ready_msg" );
    transform_age = obj.get_int( "transform_age" );
}

int delayed_transform_iuse::time_to_do( const item &it ) const
{
    return it.bday + transform_age - calendar::turn.get_turn();
}

long delayed_transform_iuse::use( player *p, item *it, bool t, const tripoint &pos ) const
{
    if( time_to_do( *it ) > 0 ) {
        p->add_msg_if_player( m_info, _( not_ready_msg.c_str() ) );
        return 0;
    }
    return iuse_transform::use( p, it, t, pos );
}

place_monster_iuse::~place_monster_iuse() {}

iuse_actor *place_monster_iuse::clone() const
{
    return new place_monster_iuse(*this);
}

void place_monster_iuse::load( JsonObject &obj )
{
    mtypeid = mtype_id( obj.get_string( "monster_id" ) );
    obj.read( "friendly_msg", friendly_msg );
    obj.read( "hostile_msg", hostile_msg );
    obj.read( "difficulty", difficulty );
    obj.read( "moves", moves );
    obj.read( "place_randomly", place_randomly );
    skill1 = skill_id( obj.get_string( "skill1", skill1.str() ) );
    skill2 = skill_id( obj.get_string( "skill2", skill2.str() ) );
}

long place_monster_iuse::use( player *p, item *it, bool, const tripoint &pos ) const
{
    monster newmon( mtypeid );
    tripoint target;
    if( place_randomly ) {
        std::vector<tripoint> valid;
        for( int x = p->posx() - 1; x <= p->posx() + 1; x++ ) {
            for( int y = p->posy() - 1; y <= p->posy() + 1; y++ ) {
                tripoint dest( x, y, pos.z );
                if( g->is_empty( dest ) ) {
                    valid.push_back( dest );
                }
            }
        }
        if( valid.empty() ) { // No valid points!
            p->add_msg_if_player( m_info, _( "There is no adjacent square to release the %s in!" ),
                                  newmon.name().c_str() );
            return 0;
        }
        target = random_entry( valid );
    } else {
        const std::string query = string_format( _( "Place the %s where?" ), newmon.name().c_str() );
        if( !choose_adjacent( query, target ) ) {
            return 0;
        }
        if( !g->is_empty( target ) ) {
            p->add_msg_if_player( m_info, _( "You cannot place a %s there." ), newmon.name().c_str() );
            return 0;
        }
    }
    p->moves -= moves;
    newmon.spawn( target );
    if (!newmon.has_flag(MF_INTERIOR_AMMO)) {
        for( auto & amdef : newmon.ammo ) {
            item ammo_item( amdef.first, 0 );
            const int available = p->charges_of( amdef.first );
            if( available == 0 ) {
                amdef.second = 0;
                p->add_msg_if_player( m_info,
                                      _( "If you had standard factory-built %1$s bullets, you could load the %2$s." ),
                                      ammo_item.type_name( 2 ).c_str(), newmon.name().c_str() );
                continue;
            }
            // Don't load more than the default from the monster definition.
            ammo_item.charges = std::min( available, amdef.second );
            p->use_charges( amdef.first, ammo_item.charges );
            //~ First %s is the ammo item (with plural form and count included), second is the monster name
            p->add_msg_if_player( ngettext( "You load %1$d x %2$s round into the %3$s.",
                                            "You load %1$d x %2$s rounds into the %3$s.", ammo_item.charges ),
                                  ammo_item.charges, ammo_item.type_name( ammo_item.charges ).c_str(),
                                  newmon.name().c_str() );
            amdef.second = ammo_item.charges;
        }
    }
    newmon.init_from_item( *it );
    int skill_offset = 0;
    if( skill1 ) {
        skill_offset += p->get_skill_level( skill1 ) / 2;
    }
    if( skill2 ) {
        skill_offset += p->get_skill_level( skill2 );
    }
    /** @EFFECT_INT increases chance of a placed turret being friendly */
    if( rng( 0, p->int_cur / 2 ) + skill_offset < rng( 0, difficulty ) ) {
        if( hostile_msg.empty() ) {
            p->add_msg_if_player( m_bad, _( "The %s scans you and makes angry beeping noises!" ),
                                  newmon.name().c_str() );
        } else {
            p->add_msg_if_player( m_bad, "%s", _( hostile_msg.c_str() ) );
        }
    } else {
        if( friendly_msg.empty() ) {
            p->add_msg_if_player( m_warning, _( "The %s emits an IFF beep as it scans you." ),
                                  newmon.name().c_str() );
        } else {
            p->add_msg_if_player( m_warning, "%s", _( friendly_msg.c_str() ) );
        }
        newmon.friendly = -1;
    }
    // TODO: add a flag instead of monster id or something?
    if( newmon.type->id == mtype_id( "mon_laserturret" ) && !g->is_in_sunlight( newmon.pos() ) ) {
        p->add_msg_if_player( _( "A flashing LED on the laser turret appears to indicate low light." ) );
    }
    g->add_zombie( newmon, true );
    return 1;
}

ups_based_armor_actor::~ups_based_armor_actor() {}

iuse_actor *ups_based_armor_actor::clone() const
{
    return new ups_based_armor_actor(*this);
}

void ups_based_armor_actor::load( JsonObject &obj )
{
    obj.read( "activate_msg", activate_msg );
    obj.read( "deactive_msg", deactive_msg );
    obj.read( "out_of_power_msg", out_of_power_msg );
}

bool has_powersource(const item &i, const player &p) {
    if( i.is_power_armor() && p.can_interface_armor() && p.power_level > 0 ) {
        return true;
    }
    return p.has_charges( "UPS", 1 );
}

long ups_based_armor_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( p == nullptr ) {
        return 0;
    }
    if( t ) {
        // Normal, continuous usage, do nothing. The item is *not* charge-based.
        return 0;
    }
    if( p->get_item_position( it ) >= -1 ) {
        p->add_msg_if_player( m_info, _( "You should wear the %s before activating it." ), it->tname().c_str() );
        return 0;
    }
    if( !it->active && !has_powersource( *it, *p ) ) {
        p->add_msg_if_player( m_info, _( "You need some source of power for your %s (a simple UPS will do)." ), it->tname().c_str() );
        if( it->is_power_armor() ) {
            p->add_msg_if_player( m_info, _( "There is also a certain bionic that helps with this kind of armor." ) );
        }
        return 0;
    }
    it->active = !it->active;
    if( it->active ) {
        if( activate_msg.empty() ) {
            p->add_msg_if_player( m_info, _( "You activate your %s." ), it->tname().c_str() );
        } else {
            p->add_msg_if_player( m_info, _( activate_msg.c_str() ) , it->tname().c_str() );
        }
    } else {
        if( deactive_msg.empty() ) {
            p->add_msg_if_player( m_info, _( "You deactivate your %s." ), it->tname().c_str() );
        } else {
            p->add_msg_if_player( m_info, _( deactive_msg.c_str() ) , it->tname().c_str() );
        }
    }
    return 0;
}


pick_lock_actor::~pick_lock_actor() {}

iuse_actor *pick_lock_actor::clone() const
{
    return new pick_lock_actor(*this);
}

void pick_lock_actor::load( JsonObject &obj )
{
    pick_quality = obj.get_int( "pick_quality" );
}

long pick_lock_actor::use( player *p, item *it, bool, const tripoint& ) const
{
    if( p == nullptr || p->is_npc() ) {
        return 0;
    }
    tripoint dirp;
    if( !choose_adjacent( _( "Use your lockpick where?" ), dirp ) ) {
        return 0;
    }
    if( dirp == p->pos() ) {
        p->add_msg_if_player( m_info, _( "You pick your nose and your sinuses swing open." ) );
        return 0;
    }
    const ter_id type = g->m.ter( dirp );
    const int npcdex = g->npc_at( dirp );
    if( npcdex != -1 ) {
        p->add_msg_if_player( m_info,
                              _( "You can pick your friends, and you can\npick your nose, but you can't pick\nyour friend's nose" ) );
        return 0;
    }

    ter_id new_type;
    std::string open_message;
    if( type == t_chaingate_l ) {
        new_type = t_chaingate_c;
        open_message = _( "With a satisfying click, the chain-link gate opens." );
    } else if( type == t_door_locked || type == t_door_locked_alarm ||
               type == t_door_locked_interior ) {
        new_type = t_door_c;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    } else if( type == t_door_locked_peep ) {
        new_type = t_door_c_peep;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    } else if( type == t_door_metal_pickable ) {
        new_type = t_door_metal_c;
        open_message = _( "With a satisfying click, the lock on the door opens." );
    } else if( type == t_door_bar_locked ) {
        new_type = t_door_bar_o;
        //Bar doors auto-open (and lock if closed again) so show a different message)
        open_message = _( "The door swings open..." );
    } else if( type == t_door_c ) {
        add_msg( m_info, _( "That door isn't locked." ) );
        return 0;
    } else {
        add_msg( m_info, _( "That cannot be picked." ) );
        return 0;
    }

    p->practice( skill_mechanics, 1 );
    /** @EFFECT_DEX speeds up door lock picking */

    /** @EFFECT_MECHANICS speeds up door lock picking */
    p->moves -= std::max(0, ( 1000 - ( pick_quality * 100 ) ) - ( p->dex_cur + p->get_skill_level( skill_mechanics ) ) * 5);
    /** @EFFECT_DEX improves chances of successfully picking door lock, reduces chances of bad outcomes */

    bool destroy = false;

    /** @EFFECT_MECHANICS improves chances of successfully picking door lock, reduces chances of bad outcomes */
    int pick_roll = ( dice( 2, p->get_skill_level( skill_mechanics ) ) + dice( 2, p->dex_cur ) - it->damage() / 2 ) * pick_quality;
    int door_roll = dice( 4, 30 );
    if( pick_roll >= door_roll ) {
        p->practice( skill_mechanics, 1 );
        p->add_msg_if_player( m_good, "%s", open_message.c_str() );
        g->m.ter_set( dirp, new_type );
    } else if( door_roll > ( 1.5 * pick_roll ) ) {
        if( it->inc_damage() ) {
            p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it, and you destroy your tool." ) );
            destroy = true;
        } else {
            p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it, and you damage your tool." ) );
        }
    } else {
        p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it." ) );
    }
    if( type == t_door_locked_alarm && ( door_roll + dice( 1, 30 ) ) > pick_roll ) {
        sounds::sound( p->pos(), 40, _( "an alarm sound!" ) );
        if( !g->event_queued( EVENT_WANTED ) ) {
            g->add_event( EVENT_WANTED, int( calendar::turn ) + 300, 0, p->global_sm_location() );
        }
    }
    if( destroy ) {
        p->i_rem( it );
        return 0;
    }
    return it->type->charges_to_use();
}


reveal_map_actor::~reveal_map_actor() {}

iuse_actor *reveal_map_actor::clone() const
{
    return new reveal_map_actor(*this);
}

void reveal_map_actor::load( JsonObject &obj )
{
    radius = obj.get_int( "radius" );
    message = obj.get_string( "message" );
    JsonArray jarr = obj.get_array( "terrain" );
    while( jarr.has_more() ) {
        omt_types.push_back( jarr.next_string() );
    }
}

void reveal_map_actor::reveal_targets( tripoint const & center, const std::string &target, int reveal_distance ) const
{
    const auto places = overmap_buffer.find_all( center, target, radius, false );
    for( auto & place : places ) {
        overmap_buffer.reveal( place, reveal_distance );
    }
}

long reveal_map_actor::use( player *p, item *it, bool, const tripoint& ) const
{
    if( it->already_used_by_player( *p ) ) {
        p->add_msg_if_player( _( "There isn't anything new on the %s." ), it->tname().c_str() );
        return 0;
    } else if( g->get_levz() < 0 ) {
        p->add_msg_if_player( _( "You should read your %s when you get to the surface." ),
                              it->tname().c_str() );
        return 0;
    }
    const auto center = p->global_omt_location();
    for( auto & omt : omt_types ) {
        reveal_targets( center, omt, 0 );
    }
    if( !message.empty() ) {
        p->add_msg_if_player( m_good, "%s", _( message.c_str() ) );
    }
    it->mark_as_used_by_player( *p );
    return 0;
}

void firestarter_actor::load( JsonObject &obj )
{
    moves_cost_fast = obj.get_int( "moves", moves_cost_fast );
    moves_cost_slow = obj.get_int( "moves_slow", moves_cost_fast * 10 );
    need_sunlight = obj.get_bool( "need_sunlight", false );
}

iuse_actor *firestarter_actor::clone() const
{
    return new firestarter_actor( *this );
}

bool firestarter_actor::prep_firestarter_use( const player *p, const item *it, tripoint &pos )
{
    if( it->ammo_remaining() < it->ammo_required() ) {
        p->add_msg_if_player( m_info, _("This tool doesn't have enough charges.") );
        return false;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if( pos == p->pos() && !choose_adjacent( _("Light where?"), pos ) ) {
        g->refresh_all();
        return false;
    }
    if( pos == p->pos() ) {
        p->add_msg_if_player(m_info, _("You would set yourself on fire."));
        p->add_msg_if_player(_("But you're already smokin' hot."));
        return false;
    }
    if( g->m.get_field( pos, fd_fire ) ) {
        // check if there's already a fire
        p->add_msg_if_player(m_info, _("There is already a fire."));
        return false;
    }
    if( g->m.flammable_items_at( pos ) ||
        g->m.has_flag( "FLAMMABLE", pos ) || g->m.has_flag( "FLAMMABLE_ASH", pos ) ||
        g->m.get_field_strength( pos, fd_web ) > 0 ) {
        // Check for a brazier.
        bool has_unactivated_brazier = false;
        for( const auto &i : g->m.i_at( pos ) ) {
            if( i.typeId() == "brazier" ) {
                has_unactivated_brazier = true;
            }
        }
        if( has_unactivated_brazier &&
            !query_yn(_("There's a brazier there but you haven't set it up to contain the fire. Continue?")) ) {
            return false;
        }
        return true;
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to light there."));
        return false;
    }
}

void firestarter_actor::resolve_firestarter_use( const player *p, const item *, const tripoint &pos )
{
    if( g->m.add_field( pos, fd_fire, 1, 100 ) ) {
        p->add_msg_if_player(_("You successfully light a fire."));
    }
}

bool firestarter_actor::can_use( const player* p, const item*, bool, const tripoint& ) const
{
    if( p->is_underwater() ) {
        return false;
    }

    if( light_mod( p->pos() ) <= 0.0f ) {
        return false;
    }

    return true;
}

float firestarter_actor::light_mod( const tripoint &pos ) const
{
    if( !need_sunlight ) {
        return 1.0f;
    }

    const float light_level = g->natural_light_level( pos.z );
    if( (g->weather == WEATHER_CLEAR || g->weather == WEATHER_SUNNY) &&
        light_level >= 60.0f && !g->m.has_flag( TFLAG_INDOORS, pos ) ) {
        return std::pow( light_level / 80.0f, 8 );
    }

    return 0.0f;
}

int firestarter_actor::moves_cost_by_fuel( const tripoint &pos ) const
{
    if( g->m.flammable_items_at( pos, 100 ) ) {
        return moves_cost_fast;
    }

    if( g->m.flammable_items_at( pos, 10 ) ) {
        return ( moves_cost_slow + moves_cost_fast ) / 2;
    }

    return moves_cost_slow;
}

long firestarter_actor::use( player *p, item *it, bool t, const tripoint &spos ) const
{
    if( t ) {
        return 0;
    }

    tripoint pos = spos;
    float light = light_mod( pos );
    if( light <= 0.0f ) {
        p->add_msg_if_player( _("You need direct sunlight to light a fire with this.") );
        return 0;
    }

    if( !prep_firestarter_use( p, it, pos ) ) {
        return 0;
    }

    double skill_level = p->get_skill_level( skill_survival );
    /** @EFFECT_SURVIVAL speeds up fire starting */
    float moves_modifier = std::pow( 0.8, std::min( 5.0, skill_level ) );
    const int moves_base = moves_cost_by_fuel( pos );
    const int min_moves = std::min<int>( moves_base, sqrt( 1 + moves_base / MOVES( 1 ) ) * MOVES( 1 ) );
    const int moves = std::max<int>( min_moves, moves_base * moves_modifier ) / light;
    if( moves > MOVES( MINUTES( 1 ) ) ) {
        // If more than 1 minute, inform the player
        static const std::string sun_msg =
            _("If the current weather holds, it will take around %d minutes to light a fire.");
        static const std::string normal_msg =
            _("At your skill level, it will take around %d minutes to light a fire.");
        p->add_msg_if_player( m_info, ( need_sunlight ? sun_msg : normal_msg ).c_str(),
            moves / MOVES( MINUTES( 1 ) ) );
    } else if( moves < MOVES( 2 ) ) {
        // If less than 2 turns, don't start a long action
        resolve_firestarter_use( p, it, pos );
        p->mod_moves( -moves );
        return it->type->charges_to_use();
    }
    p->assign_activity( activity_id( "ACT_START_FIRE" ), moves, -1, p->get_item_position( it ), it->tname() );
    p->activity.values.push_back( g->natural_light_level( pos.z ) );
    p->activity.placement = pos;
    p->practice( skill_survival, moves_modifier + moves_cost_fast / 100 + 2, 5 );
    return it->type->charges_to_use();
}

void salvage_actor::load( JsonObject &obj )
{
    assign( obj, "cost", cost );
    assign( obj, "moves_per_part", moves_per_part );

    if( obj.has_array( "material_whitelist" ) ) {
        material_whitelist.clear();
        assign( obj, "material_whitelist", material_whitelist );
    }
}

iuse_actor *salvage_actor::clone() const
{
    return new salvage_actor( *this );
}

long salvage_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( t ) {
        return 0;
    }

    int inventory_index = g->inv_for_filter( _("Cut up what?"), [ this ]( const item &it ) {
        return valid_to_cut_up( &it );
    } );

    item *cut = &( p->i_at( inventory_index ) );
    if( !try_to_cut_up(p, cut) ) {
        // Messages should have already been displayed.
        return 0;
    }

    return cut_up( p, it, cut );
}

static const units::volume minimal_volume_to_cut = 250_ml;

bool salvage_actor::valid_to_cut_up(const item *it) const
{
    if( it->is_null() ) {
        return false;
    }
    // There must be some historical significance to these items.
    if (!it->is_salvageable()) {
        return false;
    }
    if( !it->only_made_of( material_whitelist ) ) {
        return false;
    }
    if( !it->contents.empty() ) {
        return false;
    }
    if (it->volume() < minimal_volume_to_cut) {
        return false;
    }

    return true;
}

// *it here is the item that is a candidate for being chopped up.
// This is the former valid_to_cut_up with all the messages and queries
bool salvage_actor::try_to_cut_up( player *p, item *it ) const
{
    int pos = p->get_item_position(it);

    if( it->is_null() ) {
        add_msg(m_info, _("You do not have that item."));
        return false;
    }
    // There must be some historical significance to these items.
    if( !it->is_salvageable() ) {
        add_msg(m_info, _("Can't salvage anything from %s."), it->tname().c_str());
        if( p->rate_action_disassemble( *it ) != HINT_CANT ) {
            add_msg(m_info, _("Try disassembling the %s instead."), it->tname().c_str());
        }
        return false;
    }

    if( !it->only_made_of( material_whitelist ) ) {
        add_msg(m_info, _("The %s is made of material that cannot be cut up."), it->tname().c_str());
        return false;
    }
    if( !it->contents.empty() ) {
        add_msg(m_info, _("Please empty the %s before cutting it up."), it->tname().c_str());
        return false;
    }
    if( it->volume() < minimal_volume_to_cut ) {
        add_msg(m_info, _("The %s is too small to salvage material from."), it->tname().c_str());
        return false;
    }
    // Softer warnings at the end so we don't ask permission and then tell them no.
    if( it == &p->weapon ) {
        if(!query_yn(_("You are wielding that, are you sure?"))) {
            return false;
        }
    } else if( pos == INT_MIN ) {
        // Not in inventory
        return true;
    } else if( pos < -1 ) {
        if(!query_yn(_("You're wearing that, are you sure?"))) {
            return false;
        }
    }

    return true;
}

// function returns charges from *it during the cutting process of the *cut.
// *it cuts
// *cut gets cut
int salvage_actor::cut_up(player *p, item *it, item *cut) const
{
    bool filthy = cut->is_filthy();
    int pos = p->get_item_position(cut);
    // total number of raw components == total volume of item.
    // This can go awry if there is a volume / recipe mismatch.
    int count = cut->volume() / minimal_volume_to_cut;
    // Chance of us losing a material component to entropy.
    /** @EFFECT_FABRICATION reduces chance of losing components when cutting items up */
    int entropy_threshold = std::max(5, 10 - p->get_skill_level( skill_fabrication ) );
    // What material components can we get back?
    std::vector<material_id> cut_material_components = cut->made_of();
    // What materials do we salvage (ids and counts).
    std::map<std::string, int> materials_salvaged;

    // Final just in case check (that perhaps was not done elsewhere);
    if( cut == it ) {
        add_msg( m_info, _("You can not cut the %s with itself."), it->tname().c_str() );
        return 0;
    }
    if( !cut->contents.empty() ) {
        // Should have been ensured by try_to_cut_up
        debugmsg( "tried to cut a non-empty item %s", cut->tname().c_str() );
        return 0;
    }

    // Time based on number of components.
    p->moves -= moves_per_part * count;
    // Not much practice, and you won't get very far ripping things up.
    p->practice( skill_fabrication, rng(0, 5), 1 );

    // Higher fabrication, less chance of entropy, but still a chance.
    if( rng(1, 10) <= entropy_threshold ) {
        count -= 1;
    }
    // Fail dex roll, potentially lose more parts.
    /** @EFFECT_DEX randomly reduces component loss when cutting items up */
    if (dice(3, 4) > p->dex_cur) {
        count -= rng(0, 2);
    }
    // If more than 1 material component can still be be salvaged,
    // chance of losing more components if the item is damaged.
    // If the item being cut is not damaged, no additional losses will be incurred.
    if (count > 0 && cut->damage() > 0) {
        float component_success_chance = std::min( std::pow( 0.8, cut->damage() ), 1.0 );
        for(int i = count; i > 0; i--) {
            if(component_success_chance < rng_float(0,1)) {
                count--;
            }
        }
    }

    // Decided to split components evenly. Since salvage will likely change
    // soon after I write this, I'll go with the one that is cleaner.
    for (auto material : cut_material_components) {
        const material_type &mt = material.obj();
        materials_salvaged[mt.salvaged_into()] = std::max( 0, count / ( int )cut_material_components.size() );
    }

    add_msg(m_info, _("You try to salvage materials from the %s."), cut->tname().c_str());

    // Clean up before removing the item.
    remove_ammo(cut, *p);
    // Original item has been consumed.
    if( pos != INT_MIN ) {
        p->i_rem(pos);
    } else {
        g->m.i_rem( p->posx(), p->posy(), cut );
    }

    for( auto salvaged : materials_salvaged ) {
        std::string mat_name = salvaged.first;
        int amount = salvaged.second;
        item result( mat_name, int(calendar::turn) );
        if (amount > 0) {
            add_msg( m_good, ngettext("Salvaged %1$i %2$s.", "Salvaged %1$i %2$s.", amount),
                     amount, result.display_name( amount ).c_str() );
            if( filthy ) {
                result.item_tags.insert( "FILTHY" );
            }
            if( pos != INT_MIN ) {
                p->i_add_or_drop(result, amount);
            } else {
                for( int i = 0; i < amount; i++ ) {
                    g->m.spawn_an_item( p->posx(), p->posy(), result, amount, 0 );
                }
            }
        } else {
            add_msg( m_bad, _("Could not salvage a %s."), result.display_name().c_str() );
        }
    }
    // No matter what, cutting has been done by the time we get here.
    return cost >= 0 ? cost : it->ammo_required();
}

void inscribe_actor::load( JsonObject &obj )
{
    assign( obj, "cost", cost );
    assign( obj, "on_items", on_items );
    assign( obj, "on_terrain", on_terrain );
    assign( obj, "material_restricted", material_restricted );

    if( obj.has_array( "material_whitelist" ) ) {
        material_whitelist.clear();
        assign( obj, "material_whitelist", material_whitelist );
    }

    assign( obj, "verb", verb );
    assign( obj, "gerund", gerund );

    if( !on_items && !on_terrain ) {
        obj.throw_error( "Tried to create an useless inscribe_actor, at least on of \"on_items\" or \"on_terrain\" should be true" );
    }
}

iuse_actor *inscribe_actor::clone() const
{
    return new inscribe_actor( *this );
}

bool inscribe_actor::item_inscription( item *cut ) const
{
    if( !cut->made_of(SOLID) ) {
        add_msg( m_info, _("You can't inscribe an item that isn't solid!") );
        return false;
    }

    if( material_restricted && !cut->made_of_any( material_whitelist ) ) {
        std::string lower_verb = verb;
        std::transform(lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower);
        add_msg(m_info, _("You can't %1$s %2$s because of the material it is made of."),
                lower_verb.c_str(), cut->display_name().c_str());
        return false;
    }

    enum inscription_type {
        INSCRIPTION_LABEL,
        INSCRIPTION_NOTE,
        INSCRIPTION_CANCEL
    };

    uimenu menu;
    menu.text = string_format(_("%s meaning?"), _( verb.c_str() ) );
    menu.addentry(INSCRIPTION_LABEL, true, -1, _("It's a label"));
    menu.addentry(INSCRIPTION_NOTE, true, -1, _("It's a note"));
    menu.addentry(INSCRIPTION_CANCEL, true, 'q', _("Cancel"));
    menu.query();

    std::string carving, carving_type;
    switch ( menu.ret )
    {
    case INSCRIPTION_LABEL:
        carving = "item_label";
        carving_type = "item_label_type";
        break;
    case INSCRIPTION_NOTE:
        carving = "item_note";
        carving_type = "item_note_type";
        break;
    case INSCRIPTION_CANCEL:
        return false;
    }

    const bool hasnote = cut->has_var( carving );
    std::string messageprefix = string_format(hasnote ? _("(To delete, input one '.')\n") : "") +
                                string_format(_("%1$s on the %2$s is: "),
                                        _( gerund.c_str() ), cut->type_name().c_str());

    string_input_popup popup;
    popup.title( string_format( _( "%s what?" ), _( verb.c_str() ) ) )
         .width( 64 )
         .text( hasnote ? cut->get_var( carving ) : "" )
         .description( messageprefix )
         .identifier( "inscribe_item" )
         .max_length( 128 )
         .query();
    if( popup.canceled() ) {
        return false;
    }
    const std::string message = popup.text();
    if( hasnote && message == "." ) {
        cut->erase_var( carving );
        cut->erase_var( carving_type );
    } else {
        cut->set_var( carving, message );
        cut->set_var( carving_type, _( gerund.c_str() ) );
    }

    return true;
}


long inscribe_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( t ) {
        return 0;
    }

    int choice = INT_MAX;
    if( on_terrain && on_items ) {
        uimenu imenu;
        imenu.text = string_format( _("%s on what?"), _( verb.c_str() ) );
        imenu.addentry( 0, true, MENU_AUTOASSIGN, _("The ground") );
        imenu.addentry( 1, true, MENU_AUTOASSIGN, _("An item") );
        imenu.addentry( 2, true, MENU_AUTOASSIGN, _("Cancel") );
        imenu.query();
        choice = imenu.ret;
    } else if( on_terrain ) {
        choice = 0;
    } else {
        choice = 1;
    }

    if( choice < 0 || choice > 2 ) {
        return 0;
    }

    if( choice == 0 ) {
        return iuse::handle_ground_graffiti( p, it, string_format( _("%s what?"), _( verb.c_str() ) ) );
    }

    int pos = g->inv_for_all( _( "Inscribe which item?" ) );
    item *cut = &( p->i_at(pos) );
    // inscribe_item returns false if the action fails or is canceled somehow.

    if( item_inscription( cut ) ) {
        return cost >= 0 ? cost : it->ammo_required();
    }

    return 0;
}

void cauterize_actor::load( JsonObject &obj )
{
    assign( obj, "cost", cost );
    assign( obj, "flame", flame );
}

iuse_actor *cauterize_actor::clone() const
{
    return new cauterize_actor( *this );
}

static heal_actor prepare_dummy()
{
    heal_actor dummy;
    dummy.limb_power = -2;
    dummy.head_power = -2;
    dummy.torso_power = -2;
    dummy.bleed = 1.0f;
    dummy.bite = 0.5f;
    dummy.move_cost = 100;
    return dummy;
}

bool cauterize_actor::cauterize_effect( player *p, item *it, bool force )
{
    // TODO: Make this less hacky
    static const heal_actor dummy = prepare_dummy();
    hp_part hpart = dummy.use_healing_item( *p, *p, *it, force );
    if( hpart != num_hp_parts ) {
        p->add_msg_if_player(m_neutral, _("You cauterize yourself."));
        if (!(p->has_trait( trait_NOPAIN ))) {
            p->mod_pain(15);
            p->add_msg_if_player(m_bad, _("It hurts like hell!"));
        } else {
            p->add_msg_if_player(m_neutral, _("It itches a little."));
        }
        const body_part bp = player::hp_to_bp( hpart );
        if (p->has_effect( effect_bite, bp)) {
            p->add_effect( effect_bite, 2600, bp, true);
        }

        p->moves = 0;
        return true;
    }

    return 0;
}

long cauterize_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( t ) {
        return 0;
    }

    bool has_disease = p->has_effect( effect_bite ) || p->has_effect( effect_bleed );
    bool did_cauterize = false;
    if( flame && !p->has_charges("fire", 4) ) {
        p->add_msg_if_player( m_info, _("You need a source of flame (4 charges worth) before you can cauterize yourself.") );
        return 0;
    } else if( !flame && !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _("You need at least %d charges to cauterize wounds."), it->ammo_required() );
        return 0;
    } else if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _("You can't cauterize anything underwater.") );
        return 0;
    } else if( has_disease ) {
        did_cauterize = cauterize_effect( p, it, !has_disease );
    } else {
        if( ( p->has_trait( trait_MASOCHIST ) || p->has_trait( trait_MASOCHIST_MED ) || p->has_trait( trait_CENOBITE ) ) &&
            query_yn(_("Cauterize yourself for fun?"))) {
            did_cauterize = cauterize_effect( p, it, true );
        } else {
            p->add_msg_if_player( m_info, _("You are not bleeding or bitten, there is no need to cauterize yourself.") );
        }
    }

    if( !did_cauterize ) {
        return 0;
    }

    if( flame ) {
        p->use_charges("fire", 4);
        return 0;

    } else {
        return cost >= 0 ? cost : it->ammo_required();
    }
}

bool cauterize_actor::can_use( const player *p, const item *it, bool, const tripoint& ) const
{
    if( flame && !p->has_charges( "fire", 4 ) ) {
        return false;
    } else if( !flame && it->type->charges_to_use() > it->charges ) {
        return false;
    } else if( p->is_underwater() ) {
        return false;
    } else if( p->has_effect( effect_bite ) || p->has_effect( effect_bleed ) ) {
        return true;
    } else if( p->has_trait( trait_MASOCHIST ) || p->has_trait( trait_MASOCHIST_MED ) || p->has_trait( trait_CENOBITE ) ) {
        return true;
    }

    return false;
}

void enzlave_actor::load( JsonObject &obj )
{
    assign( obj, "cost", cost );
}

iuse_actor *enzlave_actor::clone() const
{
    return new enzlave_actor( *this );
}

long enzlave_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( t ) {
        return 0;
    }

    auto items = g->m.i_at( p->posx(), p->posy() );
    std::vector<const item *> corpses;

    const int cancel = 0;

    for( auto &it : items ) {
        const auto mt = it.get_mtype();
        if( it.is_corpse() && mt->in_species( ZOMBIE ) && mt->made_of( material_id( "flesh" ) ) &&
            mt->sym == "Z" && it.active && !it.has_var( "zlave" ) ) {
            corpses.push_back( &it );
        }
    }

    if( corpses.empty() ) {
        p->add_msg_if_player(_("No suitable corpses"));
        return 0;
    }

    int tolerance_level = 9;
    if( p->has_trait( trait_PSYCHOPATH ) || p->has_trait( trait_SAPIOVORE ) ) {
        tolerance_level = 0;
    } else if( p->has_trait( trait_PRED4 ) ) {
        tolerance_level = 5;
    } else if( p->has_trait( trait_PRED3 ) ) {
        tolerance_level = 7;
    }

    // Survival skill increases your willingness to get things done,
    // but it doesn't make you feel any less bad about it.
    /** @EFFECT_SURVIVAL increases tolerance for enzlavement */
    if( p->get_morale_level() <= ( 15 * ( tolerance_level - p->get_skill_level( skill_survival ) ) ) - 150 ) {
        add_msg(m_neutral, _("The prospect of cutting up the copse and letting it rise again as a slave is too much for you to deal with right now."));
        return 0;
    }

    uimenu amenu;

    amenu.selected = 0;
    amenu.text = _("Selectively butcher the downed zombie into a zombie slave?");
    amenu.addentry(cancel, true, 'q', _("Cancel"));
    for (size_t i = 0; i < corpses.size(); i++) {
        amenu.addentry(i + 1, true, -1, corpses[i]->display_name().c_str());
    }

    amenu.query();

    if (cancel == amenu.ret) {
        p->add_msg_if_player(_("Make love, not zlave."));
        return 0;
    }

    if( tolerance_level == 0 ) {
        // You just don't care, no message.
    } else if( tolerance_level <= 5 ) {
        add_msg(m_neutral, _("Well, it's more constructive than just chopping 'em into gooey meat..."));
    } else {
        add_msg(m_bad, _("You feel horrible for mutilating and enslaving someone's corpse."));

        /** @EFFECT_SURVIVAL decreases moral penalty and duration for enzlavement */
        int moraleMalus = -50 * (5.0 / (float) p->get_skill_level( skill_survival ));
        int maxMalus = -250 * (5.0 / (float)p->get_skill_level( skill_survival ));
        int duration = 300 * (5.0 / (float)p->get_skill_level( skill_survival ));
        int decayDelay = 30 * (5.0 / (float)p->get_skill_level( skill_survival ));

        if (p->has_trait( trait_PACIFIST )) {
            moraleMalus *= 5;
            maxMalus *= 3;
        } else if (p->has_trait( trait_PRED1 )) {
            moraleMalus /= 4;
        } else if (p->has_trait( trait_PRED2 )) {
            moraleMalus /= 5;
        }

        p->add_morale(MORALE_MUTILATE_CORPSE, moraleMalus, maxMalus, duration, decayDelay);
    }

    const int selected_corpse = amenu.ret - 1;

    const item *body = corpses[selected_corpse];
    const mtype *mt = body->get_mtype();

    // HP range for zombies is roughly 36 to 120, with the really big ones having 180 and 480 hp.
    // Speed range is 20 - 120 (for humanoids, dogs get way faster)
    // This gives us a difficulty ranging rougly from 10 - 40, with up to +25 for corpse damage.
    // An average zombie with an undamaged corpse is 0 + 8 + 14 = 22.
    int difficulty = ( body->damage() * 5 ) + ( mt->hp / 10 ) + ( mt->speed / 5 );
    // 0 - 30
    /** @EFFECT_DEX increases chance of success for enzlavement */

    /** @EFFECT_SURVIVAL increases chance of success for enzlavement */

    /** @EFFECT_FIRSTAID increases chance of success for enzlavement */
    int skills = p->get_skill_level( skill_survival ) + p->get_skill_level( skill_firstaid ) + (p->dex_cur / 2);
    skills *= 2;

    int success = rng(0, skills) - rng(0, difficulty);

    /** @EFFECT_FIRSTAID speeds up enzlavement */
    const int moves = difficulty * 1200 / p->get_skill_level( skill_firstaid );

    p->assign_activity( activity_id( "ACT_MAKE_ZLAVE" ), moves);
    p->activity.values.push_back(success);
    p->activity.str_values.push_back(corpses[selected_corpse]->display_name());

    return cost >= 0 ? cost : it->ammo_required();
}

bool enzlave_actor::can_use( const player *p, const item*, bool, const tripoint& ) const
{
    /** @EFFECT_SURVIVAL >1 allows enzlavement */

    /** @EFFECT_FIRSTAID >1 allows enzlavement */
    return p->get_skill_level( skill_survival ) > 1 && p->get_skill_level( skill_firstaid ) > 1;
}

void fireweapon_off_actor::load( JsonObject &obj )
{
    target_id           = obj.get_string( "target_id" );
    success_message     = obj.get_string( "success_message" );
    lacks_fuel_message  = obj.get_string( "lacks_fuel_message" );
    failure_message     = obj.get_string( "failure_message", "" );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
    success_chance      = obj.get_int( "success_chance", INT_MIN );
}

iuse_actor *fireweapon_off_actor::clone() const
{
    return new fireweapon_off_actor( *this );
}

long fireweapon_off_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( t ) {
        return 0;
    }

    if( it->charges <= 0 ) {
        p->add_msg_if_player( _(lacks_fuel_message.c_str()) );
        return 0;
    }

    p->moves -= moves;
    if( rng( 0, 10 ) - it->damage() > success_chance && !p->is_underwater() ) {
        if( noise > 0 ) {
            sounds::sound( p->pos(), noise, _(success_message.c_str()) );
        } else {
            p->add_msg_if_player( _(success_message.c_str()) );
        }

        it->convert( target_id );
        it->active = true;
    } else if( !failure_message.empty() ) {
        p->add_msg_if_player( m_bad, _(failure_message.c_str()) );
    }

    return it->type->charges_to_use();
}

bool fireweapon_off_actor::can_use( const player *p, const item *it, bool, const tripoint& ) const
{
    return it->charges > it->type->charges_to_use() && !p->is_underwater();
}

void fireweapon_on_actor::load( JsonObject &obj )
{
    noise_message                   = obj.get_string( "noise_message" );
    voluntary_extinguish_message    = obj.get_string( "voluntary_extinguish_message" );
    charges_extinguish_message      = obj.get_string( "charges_extinguish_message" );
    water_extinguish_message        = obj.get_string( "water_extinguish_message" );
    noise                           = obj.get_int( "noise", 0 );
    noise_chance                    = obj.get_int( "noise_chance", 1 );
    auto_extinguish_chance          = obj.get_int( "auto_extinguish_chance", 0 );
    if( auto_extinguish_chance > 0 ) {
        auto_extinguish_message         = obj.get_string( "auto_extinguish_message" );
    }
}

iuse_actor *fireweapon_on_actor::clone() const
{
    return new fireweapon_on_actor( *this );
}

long fireweapon_on_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    bool extinguish = true;
    if( it->charges == 0 ) {
        p->add_msg_if_player( m_bad, _(charges_extinguish_message.c_str()) );
    } else if( p->is_underwater() ) {
        p->add_msg_if_player( m_bad, _(water_extinguish_message.c_str()) );
    } else if( auto_extinguish_chance > 0 && one_in( auto_extinguish_chance ) ) {
        p->add_msg_if_player( m_bad, _(auto_extinguish_message.c_str()) );
    } else if( !t ) {
        p->add_msg_if_player( _(voluntary_extinguish_message.c_str()) );
    } else {
        extinguish = false;
    }

    if( extinguish ) {
        it->deactivate( p, false );

    } else if( one_in( noise_chance ) ) {
        if( noise > 0 ) {
            sounds::sound( p->pos(), noise, _(noise_message.c_str()) );
        } else {
            p->add_msg_if_player( _(noise_message.c_str()) );
        }
    }

    return it->type->charges_to_use();
}

void manualnoise_actor::load( JsonObject &obj )
{
    no_charges_message  = obj.get_string( "no_charges_message" );
    use_message         = obj.get_string( "use_message" );
    noise_message       = obj.get_string( "noise_message", "" );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
}

iuse_actor *manualnoise_actor::clone() const
{
    return new manualnoise_actor( *this );
}

long manualnoise_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( t ) {
        return 0;
    }
    if( it->type->charges_to_use() != 0 && it->charges < it->type->charges_to_use() ) {
        p->add_msg_if_player( _(no_charges_message.c_str()) );
        return 0;
    }
    {
        p->moves -= moves;
        if( noise > 0 ) {
            sounds::sound( p->pos(), noise, noise_message.empty() ? "" : _(noise_message.c_str()) );
        }
        p->add_msg_if_player( _(use_message.c_str()) );
    }
    return it->type->charges_to_use();
}

bool manualnoise_actor::can_use( const player*, const item *it, bool, const tripoint& ) const
{
    return it->type->charges_to_use() == 0 || it->charges >= it->type->charges_to_use();
}

iuse_actor *musical_instrument_actor::clone() const
{
    return new musical_instrument_actor(*this);
}

void musical_instrument_actor::load( JsonObject &obj )
{
    speed_penalty = obj.get_int( "speed_penalty", 10 );
    volume = obj.get_int( "volume" );
    fun = obj.get_int( "fun" );
    fun_bonus = obj.get_int( "fun_bonus", 0 );
    description_frequency = obj.get_int( "description_frequency" );
    descriptions = obj.get_string_array( "descriptions" );
}

long musical_instrument_actor::use( player *p, item *it, bool t, const tripoint& ) const
{
    if( p == nullptr ) {
        // No haunted pianos here!
        it->active = false;
        return 0;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_bad, _("You can't play music underwater") );
        it->active = false;
        return 0;
    }

    // Stop playing a wind instrument when winded or even eventually become winded while playing it?
    // It's impossible to distinguish instruments for now anyways.
    if( p->has_effect( effect_sleep ) || p->has_effect( effect_stunned ) || p->has_effect( effect_asthma ) ) {
        p->add_msg_if_player( m_bad, _("You stop playing your %s"), it->display_name().c_str() );
        it->active = false;
        return 0;
    }

    if( !t && it->active ) {
        p->add_msg_if_player( _("You stop playing your %s"), it->display_name().c_str() );
        it->active = false;
        return 0;
    }

    // Check for worn or wielded - no "floating"/bionic instruments for now
    // TODO: Distinguish instruments played with hands and with mouth, consider encumbrance
    const int inv_pos = p->get_item_position( it );
    if( inv_pos >= 0 || inv_pos == INT_MIN ) {
        p->add_msg_if_player( m_bad, _("You need to hold or wear %s to play it"), it->display_name().c_str() );
        it->active = false;
        return 0;
    }

    // At speed this low you can't coordinate your actions well enough to play the instrument
    if( p->get_speed() <= 25 + speed_penalty ) {
        p->add_msg_if_player( m_bad, _("You feel too weak to play your %s"), it->display_name().c_str() );
        it->active = false;
        return 0;
    }

    // We can play the music now
    if( !it->active ) {
        p->add_msg_if_player( m_good, _("You start playing your %s"), it->display_name().c_str() );
        it->active = true;
    }

    if( p->get_effect_int( effect_playing_instrument ) <= speed_penalty ) {
        // Only re-apply the effect if it wouldn't lower the intensity
        p->add_effect( effect_playing_instrument, 2, num_bp, false, speed_penalty );
    }

    std::string desc = "";
    /** @EFFECT_PER increases morale bonus when playing an instrument */
    const int morale_effect = fun + fun_bonus * p->per_cur;
    if( morale_effect >= 0 && calendar::turn.once_every( description_frequency ) ) {
        if( !descriptions.empty() ) {
            desc = _( random_entry( descriptions ).c_str() );
        }
    } else if( morale_effect < 0 && int(calendar::turn) % 10 ) {
        // No musical skills = possible morale penalty
        desc = _("You produce an annoying sound");
    }

    sounds::ambient_sound( p->pos(), volume, desc );

    if( !p->has_effect( effect_music ) && p->can_hear( p->pos(), volume ) ) {
        p->add_effect( effect_music, 1 );
        const int sign = morale_effect > 0 ? 1 : -1;
        p->add_morale( MORALE_MUSIC, sign, morale_effect, 5, 2 );
    }

    return 0;
}

bool musical_instrument_actor::can_use( const player *p, const item*, bool, const tripoint& ) const
{
    // TODO (maybe): Mouth encumbrance? Smoke? Lack of arms? Hand encumbrance?
    if( p->is_underwater() ) {
        return false;
    }

    return true;
}

iuse_actor *holster_actor::clone() const
{
    return new holster_actor( *this );
}

void holster_actor::load( JsonObject &obj )
{
    holster_prompt = obj.get_string( "holster_prompt", "" );
    holster_msg = obj.get_string( "holster_msg", "" );
    assign( obj, "max_volume", max_volume );
    if( !assign( obj, "min_volume", min_volume ) ) {
        min_volume = max_volume / 3;
    }

    max_weight = obj.get_int( "max_weight", max_weight );
    multi      = obj.get_int( "multi",      multi );
    draw_cost  = obj.get_int( "draw_cost",  draw_cost );

    auto tmp = obj.get_string_array( "skills" );
    std::transform( tmp.begin(), tmp.end(), std::back_inserter( skills ),
    []( const std::string & elem ) {
        return skill_id( elem );
    } );

    flags = obj.get_string_array( "flags" );
}

bool holster_actor::can_holster( const item& obj ) const {
    if( obj.volume() > max_volume || obj.volume() < min_volume ) {
        return false;
    }
    if( max_weight > 0 && obj.weight() > max_weight ) {
        return false;
    }
    return std::any_of( flags.begin(), flags.end(), [&](const std::string& f) { return obj.has_flag(f); } ) ||
           std::find( skills.begin(), skills.end(), obj.gun_skill() ) != skills.end();
}

bool holster_actor::store( player &p, item& holster, item& obj ) const
{
    if( obj.is_null() || holster.is_null() ) {
        debugmsg( "Null item was passed to holster_actor" );
        return false;
    }

    // if selected item is unsuitable inform the player why not
    if( obj.volume() > max_volume ) {
        p.add_msg_if_player( m_info, _( "Your %1$s is too big to fit in your %2$s" ),
                             obj.tname().c_str(), holster.tname().c_str() );
        return false;
    }

    if( obj.volume() < min_volume ) {
        p.add_msg_if_player( m_info, _( "Your %1$s is too small to fit in your %2$s" ),
                              obj.tname().c_str(), holster.tname().c_str() );
        return false;
    }

    if( max_weight > 0 && obj.weight() > max_weight ) {
        p.add_msg_if_player( m_info, _( "Your %1$s is too heavy to fit in your %2$s" ),
                             obj.tname().c_str(), holster.tname().c_str() );
        return false;
    }

    if( std::none_of( flags.begin(), flags.end(), [&]( const std::string & f ) { return obj.has_flag( f ); } ) &&
        std::find( skills.begin(), skills.end(), obj.gun_skill() ) == skills.end() )
    {
       p.add_msg_if_player( m_info, _( "You can't put your %1$s in your %2$s" ),
                             obj.tname().c_str(), holster.tname().c_str() );
        return false;
    }


    p.add_msg_if_player( holster_msg.empty() ? _( "You holster your %s" ) : _( holster_msg.c_str() ),
                         obj.tname().c_str(), holster.tname().c_str() );

    // holsters ignore penalty effects (eg. GRABBED) when determining number of moves to consume
    p.store( holster, obj, draw_cost, false );
    return true;
}


long holster_actor::use( player *p, item *it, bool, const tripoint & ) const
{
    std::string prompt = holster_prompt.empty() ? _( "Holster item" ) : _( holster_prompt.c_str() );

    if( &p->weapon == it ) {
        p->add_msg_if_player( _( "You need to unwield your %s before using it." ), it->tname().c_str() );
        return 0;
    }

    int pos = 0;
    std::vector<std::string> opts;

    if( ( int ) it->contents.size() < multi ) {
        opts.push_back( prompt );
        pos = -1;
    }

    std::transform( it->contents.begin(), it->contents.end(), std::back_inserter( opts ),
    []( const item & elem ) {
        return string_format( _( "Draw %s" ), elem.display_name().c_str() );
    } );

    if( opts.size() > 1 ) {
        pos += uimenu( false, string_format( _( "Use %s" ), it->tname().c_str() ).c_str(), opts ) - 1;
    }

    if( pos >= 0 ) {
        // worn holsters ignore penalty effects (eg. GRABBED) when determining number of moves to consume
        if( p->is_worn( *it ) ) {
            p->wield_contents( *it, pos, draw_cost, false );
        } else {
            p->wield_contents( *it, pos );
        }

    } else {
        item &obj = p->i_at( g->inv_for_filter( prompt, [&](const item& e) { return can_holster(e); } ) );
        if( obj.is_null() ) {
            p->add_msg_if_player( _( "Never mind." ) );
            return 0;
        }

        store( *p, *it, obj );
    }

    return 0;
}

void holster_actor::info( const item&, std::vector<iteminfo>& dump ) const
{
    dump.emplace_back( "TOOL", _( "Can contain items up to " ), string_format( "<num> %s", volume_units_abbr() ),
                       convert_volume( max_volume.value() ), false, "", max_weight <= 0 );

    if( max_weight > 0 ) {
        dump.emplace_back( "TOOL", "holster_kg", string_format( _( " or <num> %s" ), weight_units() ),
                           convert_weight( max_weight ), false, "", true, false, false );
    }
}

iuse_actor *bandolier_actor::clone() const
{
    return new bandolier_actor( *this );
}

void bandolier_actor::load( JsonObject &obj )
{
    capacity = obj.get_int( "capacity", capacity );
    ammo.clear();
    for( auto &e : obj.get_tags( "ammo" ) ) {
        ammo.insert( ammotype( e ) );
    }

    draw_cost = obj.get_int( "draw_cost", draw_cost );
}

void bandolier_actor::info( const item&, std::vector<iteminfo>& dump ) const
{
    if( !ammo.empty() ) {
        auto str = std::accumulate( std::next( ammo.begin() ), ammo.end(),
                                    string_format( "<stat>%s</stat>", _( ammo_name( *ammo.begin() ).c_str() ) ),
                                    [&]( const std::string& lhs, const ammotype& rhs ) {
                return lhs + string_format( ", <stat>%s</stat>", _( ammo_name( rhs ).c_str() ) );
        } );

        dump.emplace_back( "TOOL", string_format(
            ngettext( "Can be activated to store a single round of ",
                      "Can be activated to store up to <stat>%i</stat> rounds of ", capacity ),
                      capacity ),
            str );
    }
}

bool bandolier_actor::can_store( const item &bandolier, const item &obj ) const
{
    if( !obj.is_ammo() ) {
        return false;
    }
    if( !bandolier.contents.empty() && ( bandolier.contents.front().typeId() != obj.typeId() ||
                                         bandolier.contents.front().charges >= capacity ) ) {
        return false;
    }

    return std::any_of( obj.type->ammo->type.begin(), obj.type->ammo->type.end(),
                        [&]( const ammotype &e ) { return ammo.count( e ); } );
}

bool bandolier_actor::reload( player &p, item &obj ) const
{
    if( !obj.is_bandolier() ) {
        debugmsg( "Invalid item passed to bandolier_actor" );
        return false;
    }

    // find all nearby compatible ammo (matching type currently contained if appropriate)
    auto found = p.nearby( [&]( const item *e, const item *parent ) {
        return parent != &obj && can_store( obj, *e );
    } );

    if( found.empty() ) {
        p.add_msg_if_player( m_bad, _( "No matching ammo for the %1$s" ), obj.type_name().c_str() );
        return false;
    }

    // convert these into reload options and display the selection prompt
    std::vector<item::reload_option> opts;
    std::transform( std::make_move_iterator( found.begin() ), std::make_move_iterator( found.end() ),
                    std::back_inserter( opts ), [&]( item_location &&e ) {
        return item::reload_option( &p, &obj, &obj, std::move( e ) );
    } );

    item::reload_option sel = p.select_ammo( obj, opts );
    if( !sel ) {
        return false; // cancelled menu
    }

    p.mod_moves( -sel.moves() );

    // add or stack the ammo dependent upon existing contents
    if( obj.contents.empty() ) {
        item put = sel.ammo->split( sel.qty() );
        if( !put.is_null() ) {
            obj.put_in( put );
        } else {
            obj.put_in( *sel.ammo );
            sel.ammo.remove_item();
        }
    } else {
        obj.contents.front().charges += sel.qty();
        if( sel.ammo->charges > sel.qty() ) {
            sel.ammo->charges -= sel.qty();
        } else {
            sel.ammo.remove_item();
        }
    }

    p.add_msg_if_player( _( "You store the %1$s in your %2$s" ),
                         obj.contents.front().tname( sel.qty() ).c_str(),
                         obj.type_name().c_str() );

    return true;
}


long bandolier_actor::use( player *p, item *it, bool, const tripoint & ) const
{
    if( &p->weapon == it ) {
        p->add_msg_if_player( _( "You need to unwield your %s before using it." ),
                              it->type_name().c_str() );
        return 0;
    }

    uimenu menu;
    menu.text = _( "Store ammo" );
    menu.return_invalid = true;

    std::vector<std::function<void()>> actions;

    menu.addentry( -1, it->contents.empty() || it->contents.front().charges < capacity,
                   'r', string_format( _( "Store ammo in %s" ), it->type_name().c_str() ) );

    actions.emplace_back( [&] { reload( *p, *it ); } );

    menu.addentry( -1, !it->contents.empty(), 'u', string_format( _( "Unload %s" ),
                   it->type_name().c_str() ) );

    actions.emplace_back( [&] {
        if( p->i_add_or_drop( it->contents.front() ) ) {
            it->contents.erase( it->contents.begin() );
        } else {
            p->add_msg_if_player( _( "Never mind." ) );
        }
    } );

    menu.query();
    if( menu.ret >= 0 ) {
        actions[ menu.ret ]();
    }

    return 0;
}

iuse_actor *ammobelt_actor::clone() const
{
    return new ammobelt_actor( *this );
}

void ammobelt_actor::load( JsonObject &obj )
{
    belt = obj.get_string( "belt" );
}

void ammobelt_actor::info( const item&, std::vector<iteminfo>& dump ) const
{
    std::string name = item::find_type( belt )->nname( 1 );
    dump.emplace_back( "AMMO", string_format( _( "Can be used to assemble: %s" ), name.c_str() ) );
}

long ammobelt_actor::use( player *p, item *, bool, const tripoint& ) const
{
    if( !p ) {
        return 0;
    }

    item mag( belt );
    mag.ammo_unset();

    if( p->rate_action_reload( mag ) != HINT_GOOD ) {
        p->add_msg_if_player( _( "Insufficient %s to assemble %s" ),
                              ammo_name( mag.ammo_type() ).c_str(), mag.tname().c_str() );
        return 0;
    }

    item::reload_option opt = p->select_ammo( mag, true );
    if( opt ) {
        p->assign_activity( activity_id( "ACT_RELOAD" ), opt.moves(), opt.qty() );
        p->activity.targets.emplace_back( *p, &p->i_add( mag ) );
        p->activity.targets.push_back( std::move( opt.ammo ) );
    }

    return 0;
}

void repair_item_actor::load( JsonObject &obj )
{
    // Mandatory:
    JsonArray jarr = obj.get_array( "materials" );
    while( jarr.has_more() ) {
        materials.emplace( jarr.next_string() );
    }

    // TODO: Make skill non-mandatory while still erroring on invalid skill
    const std::string skill_string = obj.get_string( "skill" );
    used_skill = skill_id( skill_string );
    if( !used_skill.is_valid() ) {
        obj.throw_error( "Invalid skill", "skill" );
    }

    cost_scaling = obj.get_float( "cost_scaling" );

    // Optional
    tool_quality = obj.get_int( "tool_quality", 0 );
    move_cost    = obj.get_int( "move_cost", 500 );
    trains_skill_to = obj.get_int( "trains_skill_to", 5 ) - 1;
}

bool could_repair( const player &p, const item &it, bool print_msg )
{
    if( p.is_underwater() ) {
        if( print_msg ) {
            p.add_msg_if_player(m_info, _("You can't do that while underwater."));
        }
        return false;
    }
    if( p.fine_detail_vision_mod() > 4 ) {
        if( print_msg ) {
            p.add_msg_if_player(m_info, _("You can't see to do that!"));
        }
        return false;
    }
    if( !it.ammo_sufficient() ) {
        if( print_msg ) {
            p.add_msg_if_player( m_info, _("Your tool does not have enough charges to do that.") );
        }
        return false;
    }

    return true;
}

long repair_item_actor::use( player *p, item *it, bool, const tripoint & ) const
{
    if( !could_repair( *p, *it, true ) ) {
        return 0;
    }
    const int pos = g->inv_for_filter( _( "Repair what?" ), [this, it]( const item &itm ) {
        return itm.made_of_any( materials ) && !itm.count_by_charges() && !itm.is_firearm() && &itm != it;
    }, string_format( _( "You have no items that could be repaired with a %s." ), it->type_name( 1 ).c_str() ) );

    if( pos == INT_MIN ) {
        p->add_msg_if_player( m_info, _( "Never mind." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_REPAIR_ITEM" ), 0, p->get_item_position( it ), pos );
    // We also need to store the repair actor subtype in the activity
    p->activity.str_values.push_back( type );
    // All repairs are done in the activity, including charge cost
    return 0;
}

iuse_actor *repair_item_actor::clone() const
{
    return new repair_item_actor( *this );
}

bool repair_item_actor::handle_components( player &pl, const item &fix,
    bool print_msg, bool just_check ) const
{
    // Entries valid for repaired items
    std::set<material_id> valid_entries;
    for( const auto &mat : materials ) {
        if( fix.made_of( mat ) ) {
            valid_entries.insert( mat );
        }
    }

    std::vector<item_comp> comps;
    if( valid_entries.empty() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("Your %s is not made of any of:"),
                                  fix.tname().c_str());
            for( const auto &mat_name : materials ) {
                const auto &mat = mat_name.obj();
                pl.add_msg_if_player( m_info, _("%s (repaired using %s)"), mat.name().c_str(),
                                      item::nname( mat.repaired_with(), 2 ).c_str() );
            }
        }

        return false;
    }

    const inventory &crafting_inv = pl.crafting_inventory();

    // Repairing or modifying items requires at least 1 repair item,
    //  otherwise number is related to size of item
    // Round up if checking, but roll if actually consuming
    // TODO: should 250_ml be part of the cost_scaling?
    const int items_needed = std::max<int>( 1, just_check ?
        ceil( fix.volume() / 250_ml * cost_scaling ) :
        divide_roll_remainder( fix.volume() / 250_ml * cost_scaling, 1.0f ) );

    // Go through all discovered repair items and see if we have any of them available
    for( const auto &entry : valid_entries ) {
        const auto component_id = entry.obj().repaired_with();
        if( item::count_by_charges( component_id ) ) {
            if( crafting_inv.has_charges( component_id, items_needed ) ) {
                comps.emplace_back( component_id, items_needed );
            }
        } else if( crafting_inv.has_amount( component_id, items_needed ) ) {
            comps.emplace_back( component_id, items_needed );
        }
    }

    if( comps.empty() ) {
        if( print_msg ) {
            for( const auto &entry : valid_entries ) {
                const auto &mat_comp = entry.obj().repaired_with();
                pl.add_msg_if_player( m_info,
                    _("You don't have enough %s to do that. Have: %d, need: %d"),
                    item::nname( mat_comp, 2 ).c_str(),
                    item::find_type( mat_comp )->count_by_charges() ?
                        crafting_inv.amount_of( mat_comp, false ) :
                        crafting_inv.charges_of( mat_comp, items_needed ),
                    items_needed );
            }
        }

        return false;
    }

    if( !just_check ) {
        if( comps.empty() ) {
            // This shouldn't happen - the check in can_repair should prevent it
            // But report it, just in case
            debugmsg( "Attempted repair with no components" );
        }

        pl.consume_items( comps );
    }

    return true;
}


// Returns the level of the lowest level recipe that results in item of `fix`'s type
// If the recipe is not known by the player, +1 to difficulty
// If player doesn't meet the requirements of the recipe, +1 to difficulty
// If the recipe doesn't exist, difficulty is 10
int repair_item_actor::repair_recipe_difficulty( const player &pl,
    const item &fix, bool training ) const
{
    const auto &type = fix.typeId();
    int min = 5;
    for( const auto &e : recipe_dict ) {
        const auto r = e.second;
        if( type != r.result ) {
            continue;
        }

        int cur_difficulty = r.difficulty;
        if( !training && !pl.knows_recipe( &r ) ) {
            cur_difficulty++;
        }

        if( !training && !pl.has_recipe_requirements( r ) ) {
            cur_difficulty++;
        }

        min = std::min( cur_difficulty, min );
    }

    return min;
}

bool repair_item_actor::can_repair( player &pl, const item &tool, const item &fix, bool print_msg ) const
{
    if( !could_repair( pl, tool, print_msg ) ) {
        return false;
    }

    // In some rare cases (indices getting scrambled after inventory overflow)
    //  our `fix` can be a different item.
    if( fix.is_null() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("You do not have that item!") );
        }
        return false;
    }
    if( fix.is_firearm() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("That requires gunsmithing tools.") );
        }
        return false;
    }
    if( fix.count_by_charges() || fix.has_flag( "NO_REPAIR" ) ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("You cannot repair this type of item.") );
        }
        return false;
    }

    if( &fix == &tool || any_of( materials.begin(), materials.end(), [&fix]( const material_id &mat ) {
            return mat.obj().repaired_with() == fix.typeId();
        } ) ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("This can be used to repair other items, not itself.") );
        }
        return false;
    }

    if( !handle_components( pl, fix, print_msg, true ) ) {
        return false;
    }

    if( fix.has_flag("VARSIZE") && !fix.has_flag("FIT") ) {
        return true;
    }

    if( fix.damage() > 0 ) {
        return true;
    }

    if( fix.damage() < 0 ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("Your %s is already enhanced."), fix.tname().c_str() );
        }
        return false;
    }

    if( fix.has_flag("PRIMITIVE_RANGED_WEAPON") ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("You cannot improve your %s any more this way."), fix.tname().c_str());
        }
        return false;
    }

    return true;
}

std::pair<float, float> repair_item_actor::repair_chance(
    const player &pl, const item &fix, repair_item_actor::repair_type action_type ) const
{
    /** @EFFECT_TAILOR randomly improves clothing repair efforts */
    /** @EFFECT_MECHANICS randomly improves metal repair efforts */
    const int skill = pl.get_skill_level( used_skill );
    const int recipe_difficulty = repair_recipe_difficulty( pl, fix );
    int action_difficulty = 0;
    switch( action_type ) {
        case RT_REPAIR:
            action_difficulty = fix.damage();
            break;
        case RT_REFIT:
            // Let's make refitting as hard as recovering an almost-wrecked item
            action_difficulty = fix.max_damage();
            break;
        case RT_REINFORCE:
            // Reinforcing is at least as hard as refitting
            action_difficulty = std::max( fix.max_damage(), recipe_difficulty );
            break;
        case RT_PRACTICE:
            // Skill gain scales with recipe difficulty, so practice difficulty should too
            action_difficulty = recipe_difficulty;
        default:
            std::make_pair( 0.0f, 0.0f );
    }

    const int difficulty = recipe_difficulty + action_difficulty;
    // Sample numbers:
    // Item   | Damage | Skill | Dex | Success | Failure
    // Hoodie |    2   |   3   |  10 |   6%    |   0%
    // Hazmat |    1   |   10  |  10 |   8%    |   0%
    // Hazmat |    1   |   5   |  20 |   0%    |   2%
    // t-shirt|    4   |   1   |  5  |   2%    |   3%
    // Duster |    2   |   5   |  5  |   10%   |   0%
    // Duster |    2   |   2   |  10 |   4%    |   1%
    // Duster | Refit  |   2   |  10 |   0%    |   N/A
    float success_chance = (10 + 2 * skill - 2 * difficulty + tool_quality / 5.0f) / 100.0f;
    /** @EFFECT_DEX reduces the chances of damaging an item when repairing */
    float damage_chance = (difficulty - skill - (tool_quality + pl.dex_cur) / 5.0f) / 100.0f;

    damage_chance = std::max( 0.0f, std::min( 1.0f, damage_chance ) );
    success_chance = std::max( 0.0f, std::min( 1.0f - damage_chance, success_chance ) );


    return std::make_pair( success_chance, damage_chance );
}

repair_item_actor::repair_type repair_item_actor::default_action( const item &fix, int current_skill_level ) const
{
    if( fix.damage() > 0 ) {
        return RT_REPAIR;
    }

    if( fix.has_flag("VARSIZE") && !fix.has_flag("FIT") ) {
        return RT_REFIT;
    }

    if( fix.damage() == 0 ) {
        return RT_REINFORCE;
    }

    if( current_skill_level <= trains_skill_to ) {
        return RT_PRACTICE;
    }

    return RT_NOTHING;
}

bool damage_item( player &pl, item &fix )
{
    pl.add_msg_if_player(m_bad, _("You damage your %s!"), fix.tname().c_str());
    if( fix.inc_damage() ) {
        pl.add_msg_if_player(m_bad, _("You destroy it!"));
        const int pos = pl.get_item_position( &fix );
        if( pos != INT_MIN ) {
            pl.i_rem_keep_contents( pos );
        } else {
            // NOTE: Repairing items outside inventory is NOT yet supported!
            debugmsg( "Tried to remove an item that doesn't exist" );
        }

        return true;
    }

    return false;
}

repair_item_actor::attempt_hint repair_item_actor::repair( player &pl, item &tool, item &fix ) const
{
    if( !can_repair( pl, tool, fix, true ) ) {
        return AS_CANT;
    }

    const int current_skill_level = pl.get_skill_level( used_skill );
    const auto action = default_action( fix, current_skill_level );
    const auto chance = repair_chance( pl, fix, action );
    const int practice_amount = repair_recipe_difficulty( pl, fix, true );
    float roll_value = rng_float( 0.0, 1.0 );
    enum roll_result {
        SUCCESS,
        FAILURE,
        NEUTRAL
    } roll;

    if( roll_value > 1.0f - chance.second ) {
        roll = FAILURE;
    } else if( roll_value < chance.first ) {
        roll = SUCCESS;
    } else {
        roll = NEUTRAL;
    }

    if( action == RT_NOTHING ) {
        pl.add_msg_if_player( m_bad, _("You won't learn anything more by doing that.") );
        return AS_CANT;
    }

    // If not for this if, it would spam a lot
    if( current_skill_level <= trains_skill_to ) {
        pl.practice( used_skill, practice_amount / 2 + 1, trains_skill_to );
    }

    if( roll == FAILURE ) {
        return damage_item( pl, fix ) ? AS_DESTROYED : AS_FAILURE;
    }

    if( action == RT_PRACTICE ) {
        return AS_RETRY;
    }

    if( action == RT_REPAIR ) {
        if( roll == SUCCESS ) {
            pl.add_msg_if_player(m_good, _("You repair your %s!"), fix.tname().c_str());
            handle_components( pl, fix, false, false );
            fix.mod_damage( -1 );
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    if( action == RT_REFIT ) {
        if( roll == SUCCESS ) {
            pl.add_msg_if_player(m_good, _("You take your %s in, improving the fit."),
                                 fix.tname().c_str());
            fix.item_tags.insert("FIT");
            handle_components( pl, fix, false, false );
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    if( action == RT_REINFORCE ) {
        if( fix.has_flag("PRIMITIVE_RANGED_WEAPON") ) {
            pl.add_msg_if_player( m_info, _("You cannot improve your %s any more this way."), fix.tname().c_str() );
            return AS_CANT;
        }

        if( roll == SUCCESS ) {
            pl.add_msg_if_player(m_good, _("You make your %s extra sturdy."), fix.tname().c_str());
            fix.mod_damage( -1 );
            handle_components( pl, fix, false, false );
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    pl.add_msg_if_player( m_info, _("Your %s is already enhanced."), fix.tname().c_str() );
    return AS_CANT;
}

const std::string &repair_item_actor::action_description( repair_item_actor::repair_type rt )
{
    static const std::array<std::string, NUM_REPAIR_TYPES> arr = {{
        _("Nothing"),
        _("Repairing"),
        _("Refiting"),
        _("Reinforcing"),
        _("Practicing")
    }};

    return arr[rt];
}

std::string repair_item_actor::get_name() const
{
    const std::string mats = enumerate_as_string( materials.begin(), materials.end(),
    []( const material_id &mid ) {
        return mid->name();
    } );
    return string_format( _( "Repair %s" ), mats.c_str() );
}

void heal_actor::load( JsonObject &obj )
{
    // Mandatory
    move_cost = obj.get_int( "move_cost" );
    limb_power = obj.get_float( "limb_power" );

    // Optional
    head_power = obj.get_float( "head_power", 0.8f * limb_power );
    torso_power = obj.get_float( "torso_power", 1.5f * limb_power );

    limb_scaling = obj.get_float( "limb_scaling", 0.25f * limb_power );
    float scaling_ratio = limb_scaling / limb_power;
    head_scaling = obj.get_float( "head_scaling", scaling_ratio * head_power );
    torso_scaling = obj.get_float( "torso_scaling", scaling_ratio * torso_power );

    bleed = obj.get_float( "bleed", 0.0f );
    bite = obj.get_float( "bite", 0.0f );
    infect = obj.get_float( "infect", 0.0f );

    long_action = obj.get_bool( "long_action", false );

    if( obj.has_array( "effects" ) ) {
        JsonArray jsarr = obj.get_array( "effects" );
        while( jsarr.has_more() ) {
            JsonObject e = jsarr.next_object();
            effects.push_back( load_effect_data( e ) );
        }
    }

    used_up_item = obj.get_string( "used_up_item", used_up_item );
}

player &get_patient( player &healer, const tripoint &pos )
{
    if( healer.pos() == pos ) {
        return healer;
    }

    if( g->u.pos() == pos ) {
        return g->u;
    }

    const int npc_index = g->npc_at( pos );
    if( npc_index == -1 ) {
        // Default to heal self on failure not to break old functionality
        add_msg( m_debug, "No heal target at position %d,%d,%d", pos.x, pos.y, pos.z );
        return healer;
    }

    return (player&)(*g->active_npc[npc_index]);
}

long heal_actor::use( player *p, item *it, bool, const tripoint &pos ) const
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _("You can't do that while underwater.") );
        return 0;
    }
    
    if( get_option<bool>( "FILTHY_WOUNDS" ) && it->is_filthy() ) {
        p->add_msg_if_player( m_info, _( "You can't use filthy items for healing." ) );
        return 0;
    }

    player &patient = get_patient( *p, pos );
    const hp_part hpp = use_healing_item( *p, patient, *it, false );
    if( hpp == num_hp_parts ) {
        return 0;
    }

    int cost = move_cost;
    if( long_action ) {
        // A hack: long action healing on NPCs isn't done yet.
        // So just heal at start and paralyze the player for 5 minutes.
        cost /= std::min( 10, p->get_skill_level( skill_firstaid ) + 1 );
    }

    // NPCs can use first aid now, but they can't perform long actions
    if( long_action && &patient == p && !p->is_npc() ) {
        // Assign first aid long action.
        /** @EFFECT_FIRSTAID speeds up firstaid activity */
        p->assign_activity( activity_id( "ACT_FIRSTAID" ), cost, 0, p->get_item_position( it ), it->tname() );
        p->activity.values.push_back( hpp );
        p->moves = 0;
        return 0;
    }

    p->moves -= cost;
    p->add_msg_if_player(m_good, _("You use your %s."), it->tname().c_str());
    return it->type->charges_to_use();
}

iuse_actor *heal_actor::clone() const
{
    return new heal_actor( *this );
}

int heal_actor::get_heal_value( const player &healer, hp_part healed ) const
{
    int heal_base;
    float bonus_mult;
    if( healed == hp_head ) {
        heal_base = head_power;
        bonus_mult = head_scaling;
    } else if( healed == hp_torso ) {
        heal_base = torso_power;
        bonus_mult = torso_scaling;
    } else {
        heal_base = limb_power;
        bonus_mult = limb_scaling;
    }

    if( heal_base > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        return heal_base + bonus_mult * healer.get_skill_level( skill_firstaid );
    }

    return heal_base;
}

long heal_actor::finish_using( player &healer, player &patient, item &it, hp_part healed ) const
{
    float practice_amount = std::max( 9.0f, limb_power * 3.0f );
    const int dam = get_heal_value( healer, healed );

    if( (patient.hp_cur[healed] >= 1) && (dam > 0)) { // Prevent first-aid from mending limbs
        patient.heal(healed, dam);
    } else if ((patient.hp_cur[healed] >= 1) && (dam < 0)) {
        const body_part bp = player::hp_to_bp( healed );
        patient.apply_damage( nullptr, bp, -dam ); //hurt takes + damage
    }

    const body_part bp_healed = player::hp_to_bp( healed );

    const bool u_see = healer.is_player() || patient.is_player() ||
        g->u.sees( healer ) || g->u.sees( patient );
    const bool player_healing_player = healer.is_player() && patient.is_player();
    // Need a helper here - messages are from healer's point of view
    // but it would be cool if NPCs could use this function too
    const auto heal_msg = [&]( game_message_type msg_type,
        const char *player_player_msg, const char *other_msg ) {
        if( !u_see ) {
            return;
        }

        if( player_healing_player ) {
            add_msg( msg_type, player_player_msg );
        } else {
            add_msg( msg_type, other_msg );
        }
    };

    if( patient.has_effect( effect_bleed, bp_healed ) ) {
        if( x_in_y( bleed, 1.0f ) ) {
            patient.remove_effect( effect_bleed, bp_healed );
            heal_msg( m_good, _("You stop the bleeding."), _("The bleeding is stopped.") );
        } else {
            heal_msg( m_warning, _("You fail to stop the bleeding."), _("The wound still bleeds.") );
        }

        practice_amount += bleed * 3.0f;
    }
    if( patient.has_effect( effect_bite, bp_healed ) ) {
        if( x_in_y( bite, 1.0f ) ) {
            patient.remove_effect( effect_bite, bp_healed);
            heal_msg( m_good, _("You clean the wound."), _("The wound is cleaned.") );
        } else {
            heal_msg( m_warning, _("Your wound still aches."), _("The wound still looks bad.") );
        }

        practice_amount += bite * 3.0f;
    }
    if( patient.has_effect( effect_infected, bp_healed ) ) {
        if( x_in_y( infect, 1.0f ) ) {
            int infected_dur = patient.get_effect_dur( effect_infected, bp_healed );
            patient.remove_effect( effect_infected, bp_healed );
            patient.add_effect( effect_recover, infected_dur );
            heal_msg( m_good, _("You disinfect the wound."), _("The wound is disinfected.") );
        } else {
            heal_msg( m_warning, _("Your wound still hurts."), _("The wound still looks nasty.") );
        }

        practice_amount += infect * 10.0f;
    }

    if( long_action ) {
        healer.add_msg_if_player( _("You finish using the %s."), it.tname().c_str() );
    }

    for( auto eff : effects ) {
        patient.add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
    }

    if( !used_up_item.empty() ) {
        // If the item is a tool, `make` it the new form
        // Otherwise it probably was consumed, so create a new one
        if( it.is_tool() ) {
            it.convert( used_up_item );
        } else {
            item used_up( used_up_item, it.bday );
            healer.i_add_or_drop( used_up );
        }
    }

    healer.practice( skill_firstaid, (int)practice_amount );
    return it.type->charges_to_use();
}

hp_part pick_part_to_heal(
    const player &healer, const player &patient,
    const std::string &menu_header,
    int limb_power, int head_bonus, int torso_bonus,
    float bleed_chance, float bite_chance, float infect_chance,
    bool force )
{
    const bool bleed = bleed_chance > 0.0f;
    const bool bite = bite_chance > 0.0f;
    const bool infect = infect_chance > 0.0f;
    const bool precise = &healer == &patient ?
        patient.has_trait( trait_SELFAWARE ) :
        /** @EFFECT_PER slightly increases precision when using first aid on someone else */

        /** @EFFECT_FIRSTAID increases precision when using first aid on someone else */
        (healer.get_skill_level( skill_firstaid ) * 4 + healer.per_cur >= 20);
    while( true ) {
        hp_part healed_part = patient.body_window( menu_header, force, precise,
                                                   limb_power, head_bonus, torso_bonus,
                                                   bleed, bite, infect );
        if( healed_part == num_hp_parts ) {
            return num_hp_parts;
        }

        body_part bp = player::hp_to_bp( healed_part );
        if( ( infect && patient.has_effect( effect_infected, bp ) ) ||
            ( bite && patient.has_effect( effect_bite, bp ) ) ||
            ( bleed && patient.has_effect( effect_bleed, bp ) ) ) {
            return healed_part;
        }

        if( patient.hp_cur[healed_part] == 0 ) {
            if( healed_part == hp_arm_l || healed_part == hp_arm_r ) {
                add_msg( m_info, _("That arm is broken.  It needs surgical attention or a splint.") );
            } else if( healed_part == hp_leg_l || healed_part == hp_leg_r ) {
                add_msg( m_info, _("That leg is broken.  It needs surgical attention or a splint.") );
            } else {
                add_msg( m_info, "That body part is bugged.  It needs developer's attention." );
            }

            continue;
        }

        if( force || patient.hp_cur[healed_part] < patient.hp_max[healed_part] ) {
            return healed_part;
        }
    }

    // Won't happen?
    return num_hp_parts;
}

hp_part heal_actor::use_healing_item( player &healer, player &patient, item &it, bool force ) const
{
    hp_part healed = num_hp_parts;
    const int head_bonus = get_heal_value( healer, hp_head );
    const int limb_power = get_heal_value( healer, hp_arm_l );
    const int torso_bonus = get_heal_value( healer, hp_torso );

    if( healer.is_npc() ) {
        // NPCs heal whichever has sustained the most damage
        int highest_damage = 0;
        for (int i = 0; i < num_hp_parts; i++) {
            int damage = patient.hp_max[i] - patient.hp_cur[i];
            if (i == hp_head) {
                damage *= 1.5;
            }
            if (i == hp_torso) {
                damage *= 1.2;
            }
            // Consider states too
            // Weights are arbitrary, may need balancing
            const body_part i_bp = player::hp_to_bp( hp_part( i ) );
            damage += bleed * patient.get_effect_dur( effect_bleed, i_bp ) / 50;
            damage += bite * patient.get_effect_dur( effect_bite, i_bp ) / 100;
            damage += infect * patient.get_effect_dur( effect_infected, i_bp ) / 100;
            if (damage > highest_damage) {
                highest_damage = damage;
                healed = hp_part(i);
            }
        }
    } else if( patient.is_player() ) {
        // Player healing self - let player select
        if( healer.activity.id() != activity_id( "ACT_FIRSTAID" ) ) {
            const std::string menu_header = it.tname();
            healed = pick_part_to_heal( healer, patient, menu_header,
                                        limb_power, head_bonus, torso_bonus,
                                        bleed, bite, infect, force );
            if( healed == num_hp_parts ) {
                return num_hp_parts; // canceled
            }
        }
        // Brick healing if using a first aid kit for the first time.
        if( long_action && healer.activity.id() != activity_id( "ACT_FIRSTAID" ) ) {
            // Cancel and wait for activity completion.
            return healed;
        } else if( healer.activity.id() == activity_id( "ACT_FIRSTAID" ) ) {
            // Completed activity, extract body part from it.
            healed = (hp_part)healer.activity.values[0];
        }
    } else {
        // Player healing NPC
        // TODO: Remove this hack, allow using activities on NPCs
        const std::string menu_header = it.tname();
        healed = pick_part_to_heal( healer, patient, menu_header,
                                    limb_power, head_bonus, torso_bonus,
                                    bleed, bite, infect, force );
    }

    if( healed != num_hp_parts ) {
        finish_using( healer, patient, it, healed );
    }

    return healed;
}

void heal_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    if( head_power > 0 || torso_power > 0 || limb_power > 0 ) {
        dump.emplace_back( "TOOL", _( "<bold>Base healing:</bold> " ), "", -999, true, "", true );
        dump.emplace_back( "TOOL", _( "Head: " ), "", head_power, true, "", false );
        dump.emplace_back( "TOOL", _( "  Torso: " ), "", torso_power, true, "", false );
        dump.emplace_back( "TOOL", _( "  Limbs: " ), "", limb_power, true, "", true );
        if( g != nullptr ) {
            dump.emplace_back( "TOOL", _( "<bold>Actual healing:</bold> " ), "", -999, true, "", true );
            dump.emplace_back( "TOOL", _( "Head: " ), "", get_heal_value( g->u, hp_head ), true, "", false );
            dump.emplace_back( "TOOL", _( "  Torso: " ), "", get_heal_value( g->u, hp_torso ), true, "", false );
            dump.emplace_back( "TOOL", _( "  Limbs: " ), "", get_heal_value( g->u, hp_arm_l ), true, "", true );
        }
    }

    if( bleed > 0.0f || bite > 0.0f || infect > 0.0f ) {
        dump.emplace_back( "TOOL", _( "<bold>Chance to heal (percent):</bold> " ), "", -999, true, "", true );
        if( bleed > 0.0f ) {
            dump.emplace_back( "TOOL", _( "<bold>Bleeding</bold>:" ), "", (int)(bleed * 100), true, "", true );
        }
        if( bite > 0.0f ) {
            dump.emplace_back( "TOOL", _( "<bold>Bite</bold>:" ), "", (int)(bite * 100), true, "", true );
        }
        if( infect > 0.0f ) {
            dump.emplace_back( "TOOL", _( "<bold>Infection</bold>:" ), "", (int)(infect * 100), true, "", true );
        }
    }

    dump.emplace_back( "TOOL", _( "<bold>Moves to use</bold>:" ), "", move_cost );
}



void place_trap_actor::data::load( JsonObject obj )
{
    assign( obj, "trap", trap );
    assign( obj, "done_message", done_message );
    assign( obj, "practice", practice );
    assign( obj, "moves", moves );
}

void place_trap_actor::load( JsonObject &obj )
{
    assign( obj, "allow_underwater", allow_underwater );
    assign( obj, "allow_under_player", allow_under_player );
    assign( obj, "needs_solid_neighbor", needs_solid_neighbor );
    assign( obj, "needs_neighbor_terrain", needs_neighbor_terrain );
    assign( obj, "bury_question", bury_question );
    if( !bury_question.empty() ) {
        buried_data.load( obj.get_object( "bury" ) );
    }
    unburied_data.load( obj );
    assign( obj, "outer_layer_trap", outer_layer_trap );
}

iuse_actor *place_trap_actor::clone() const
{
    return new place_trap_actor( *this );
}

bool is_solid_neighbor( const tripoint &pos, const int offset_x, const int offset_y )
{
    const tripoint a = pos + tripoint( offset_x, offset_y, 0 );
    const tripoint b = pos - tripoint( offset_x, offset_y, 0 );
    return g->m.move_cost( a ) != 2 && g->m.move_cost( b ) != 2;
}

bool has_neighbor( const tripoint &pos, const ter_id &terrain_id )
{
    for( const tripoint &t : g->m.points_in_radius( pos, 1, 0 ) ) {
        if( g->m.ter( t ) == terrain_id ) {
            return true;
        }
    }
    return false;
}

bool place_trap_actor::is_allowed( player &p, const tripoint &pos, const std::string &name ) const
{
    if( !allow_under_player && pos == p.pos() ) {
        p.add_msg_if_player( m_info, _( "Yeah. Place the %s at your feet. Real damn smart move." ), name.c_str() );
        return false;
    }
    if( g->m.move_cost( pos ) != 2 ) {
        p.add_msg_if_player( m_info, _( "You can't place a %s there." ), name.c_str() );
        return false;
    }
    if( needs_solid_neighbor ) {
        if( !is_solid_neighbor( pos, 1, 0 ) && !is_solid_neighbor( pos, 0, 1 ) &&
            !is_solid_neighbor( pos, 1, 1 ) && !is_solid_neighbor( pos, 1, -1) ) {
            p.add_msg_if_player( m_info, _( "You must place the %s between two solid tiles." ), name.c_str() );
            return false;
        }
    }
    if( needs_neighbor_terrain && !has_neighbor( pos, needs_neighbor_terrain ) ) {
        p.add_msg_if_player( m_info, _( "The %s needs a %s adjacent to it." ), name.c_str(), needs_neighbor_terrain.obj().name.c_str() );
        return false;
    }
    const trap &existing_trap = g->m.tr_at( pos );
    if( !existing_trap.is_null() ) {
        if( existing_trap.can_see( pos, p ) ) {
            p.add_msg_if_player( m_info, _( "You can't place a %s there. It contains a trap already." ), name.c_str() );
        } else {
            p.add_msg_if_player( m_bad, _( "You trigger a %s!" ), existing_trap.name.c_str() );
            existing_trap.trigger( pos, &p );
        }
        return false;
    }
    return true;
}

void place_and_add_as_known( player &p, const tripoint &pos, const trap_str_id &id )
{
    g->m.trap_set( pos, id );
    const trap &tr = g->m.tr_at( pos );
    if( !tr.can_see( pos, p ) ) {
        p.add_known_trap( pos, tr );
    }
}

long place_trap_actor::use( player * const p, item * const it, bool, const tripoint & ) const
{
    const bool is_3x3_trap = !outer_layer_trap.is_null();
    const bool could_bury = !bury_question.empty();
    if( !allow_underwater && p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    tripoint pos = p->pos();
    if( !choose_adjacent( string_format( _( "Place %s where?" ), it->tname().c_str() ), pos ) ) {
        return 0;
    }
    if( !is_allowed( *p, pos, it->tname() ) ) {
        return 0;
    }
    if( is_3x3_trap ) {
        pos.x = ( pos.x - p->posx() ) * 2 + p->posx(); //math correction for blade trap
        pos.y = ( pos.y - p->posy() ) * 2 + p->posy();
        for( const tripoint &t : g->m.points_in_radius( pos, 1, 0 ) ) {
            if( !is_allowed( *p, t, it->tname() ) ) {
                p->add_msg_if_player( m_info, _( "That trap needs a 3x3 space to be clear, centered two tiles from you." ) );
                return false;
            }
        }
    }

    const bool has_shovel = p->has_quality( quality_id( "DIG" ), 3 );
    const bool is_diggable = g->m.has_flag( "DIGGABLE", pos );
    bool bury = false;
    if( could_bury && has_shovel && is_diggable ) {
        bury = query_yn( _( bury_question.c_str() ) );
    }
    const auto &data = bury ? buried_data : unburied_data;

    p->add_msg_if_player( m_info, _( data.done_message.c_str() ), g->m.name( pos ).c_str() );
    p->practice( skill_id( "traps" ), data.practice );
    p->mod_moves( -data.moves );

    place_and_add_as_known( *p, pos, data.trap );
    if( is_3x3_trap ) {
        for( const tripoint &t : g->m.points_in_radius( pos, 1, 0 ) ) {
            if( t != pos ) {
                place_and_add_as_known( *p, t, outer_layer_trap );
            }
        }
    }
    return 1;
}

void emit_actor::load( JsonObject &obj )
{
    assign( obj, "emits", emits );
    assign( obj, "scale_qty", scale_qty );
}

long emit_actor::use( player*, item *it, bool, const tripoint &pos ) const
{
    const float scaling = scale_qty ? it->charges : 1;
    for( const auto &e : emits ) {
        g->m.emit_field( pos, e, scaling );
    }

    return 1;
}

iuse_actor *emit_actor::clone() const
{
    return new emit_actor( *this );
}

void emit_actor::finalize( const itype_id &my_item_type )
{
    /*
    // @todo This must be called after all finalization
    for( const auto& e : emits ) {
        if( !e.is_valid() ) {
            debugmsg( "Item %s has unknown emit source %s", my_item_type.c_str(), e.c_str() );
        }
    }
    */

    if( scale_qty && !item::count_by_charges( my_item_type ) ) {
        debugmsg( "Item %s has emit_actor with scale_qty, but is not counted by charges", my_item_type.c_str() );
        scale_qty = false;
    }
}
