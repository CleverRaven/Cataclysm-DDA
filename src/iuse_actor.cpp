#include "iuse_actor.h"
#include "action.h"
#include "item.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "monster.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "translations.h"
#include "morale.h"
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

#include <sstream>
#include <algorithm>

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

iuse_transform::~iuse_transform()
{
}

iuse_actor *iuse_transform::clone() const
{
    return new iuse_transform(*this);
}

void iuse_transform::load( JsonObject &obj )
{
    // Mandatory:
    target_id = obj.get_string( "target" );
    // Optional (default is good enough):
    obj.read( "msg", msg_transform );
    obj.read( "target_charges", target_charges );
    obj.read( "container", container_id );
    obj.read( "active", active );
    obj.read( "need_fire", need_fire );
    obj.read( "need_fire_msg", need_fire_msg );
    obj.read( "need_charges", need_charges );
    obj.read( "need_charges_msg", need_charges_msg );
    obj.read( "moves", moves );
    obj.read( "menu_option_text", menu_option_text );
}

long iuse_transform::use(player *p, item *it, bool t, const tripoint &pos ) const
{
    if( t ) {
        // Invoked from active item processing, do nothing.
        return 0;
    }
    // We can't just check for p != nullptr, because item::process sets p = g->u
    // Not needed here (player always has the item), but is in auto_transform
    const bool player_has_item = p != nullptr && p->has_item( it );
    if( player_has_item && p->is_underwater() ) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater"));
        return 0;
    }
    if( player_has_item && need_charges > 0 && it->charges < need_charges ) {
        if (!need_charges_msg.empty()) {
            p->add_msg_if_player(m_info, _( need_charges_msg.c_str() ), it->tname().c_str());
        }
        return 0;
    }
    if( player_has_item && need_fire > 0 && !p->use_charges_if_avail("fire", need_fire) ) {
        if (!need_fire_msg.empty()) {
            p->add_msg_if_player(m_info, _( need_fire_msg.c_str() ), it->tname().c_str());
        }
        return 0;
    }
    // load this from the original item, not the transformed one.
    const long charges_to_use = it->type->charges_to_use();
    if( p != nullptr && !msg_transform.empty() && p->sees( pos ) ) {
        p->add_msg_if_player(m_neutral, _( msg_transform.c_str() ), it->tname().c_str());
    }
    item *target;
    if (container_id.empty()) {
        // No container, assume simple type transformation like foo_off -> foo_on
        it->make(target_id);
        target = it;
    } else {
        // Transform into something in a container, assume the content is
        // "created" right now and give the content the current time as birthday
        it->make(container_id, true);
        it->contents.push_back(item(target_id, calendar::turn));
        target = &it->contents.back();
    }
    target->active = active;
    if (target_charges > -2) {
        // -1 is for items that can not have any charges at all.
        target->charges = target_charges;
    } else if( charges_to_use > 0 && target->charges >= 0 ) {
    // Makes no sense to set the charges via this iuse and than remove some of them, you can combine
    // both into the target_charges value.
    // Also if the target does not use charges (item::charges == -1), don't change them at all.
    // This allows simple transformations like "folded" <=> "unfolded", and "retracted" <=> "extended"
    // Active item handling has gotten complicated, so having this consume charges itself
    // instead of passing it off to the caller.
        target->charges -= std::min(charges_to_use, target->charges);
    }

    if( player_has_item ) {
        p->moves -= moves;
    }
    return 0;
}



auto_iuse_transform::~auto_iuse_transform()
{
}

iuse_actor *auto_iuse_transform::clone() const
{
    return new auto_iuse_transform(*this);
}

void auto_iuse_transform::load( JsonObject &obj )
{
    iuse_transform::load( obj );
    obj.read( "when_underwater", when_underwater );
    obj.read( "non_interactive_msg", non_interactive_msg );
}

long auto_iuse_transform::use(player *p, item *it, bool t, const tripoint &pos) const
{
    if (t) {
        if (!when_underwater.empty() && p != NULL && p->is_underwater()) {
            // Dirty hack to display the "when underwater" message instead of the normal message
            std::swap(const_cast<auto_iuse_transform *>(this)->when_underwater,
                      const_cast<auto_iuse_transform *>(this)->msg_transform);
            const long tmp = iuse_transform::use(p, it, t, pos);
            std::swap(const_cast<auto_iuse_transform *>(this)->when_underwater,
                      const_cast<auto_iuse_transform *>(this)->msg_transform);
            return tmp;
        }
        // Normal use, don't need to do anything here.
        return 0;
    }
    if (it->charges > 0 && !non_interactive_msg.empty()) {
        if( p != nullptr ) {
            p->add_msg_if_player(m_info, _( non_interactive_msg.c_str() ), it->tname().c_str());
        }
        // Activated by the player, but not allowed to do so
        return 0;
    }
    return iuse_transform::use(p, it, t, pos);
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
    obj.read( "explosion_power", explosion_power );
    obj.read( "explosion_shrapnel", explosion_shrapnel );
    obj.read( "explosion_distance_factor", explosion_distance_factor );
    obj.read( "explosion_fire", explosion_fire );
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
    if (it->charges > 0) {
        if (no_deactivate_msg.empty()) {
            p->add_msg_if_player(m_warning,
                                 _("You've already set the %s's timer you might want to get away from it."), it->tname().c_str());
        } else {
            p->add_msg_if_player(m_info, _( no_deactivate_msg.c_str() ), it->tname().c_str());
        }
        return 0;
    }
    if (explosion_power >= 0) {
        g->explosion( pos, explosion_power, explosion_distance_factor, explosion_shrapnel, explosion_fire );
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
    veh->tags.insert(std::string("convertible:") + it->type->id);
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
            veh_data >> elem.hp;
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
                dst.hp = src.hp;
                dst.blood = src.blood;
                dst.bigness = src.bigness;
                // door state/amount of fuel/direction of headlight
                dst.amount = src.amount;
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
    return effect_data( efftype_id( e.get_string( "id", "null" ) ), e.get_int( "duration", 0 ),
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
        if( eff.id == efftype_id( "null" ) ) {
            continue;
        }

        int dur = eff.duration;
        if (p->has_trait("TOLERANCE")) {
            dur *= .8;
        } else if (p->has_trait("LIGHTWEIGHT")) {
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
    // Output message.
    p->add_msg_if_player( _(activation_message.c_str()) );
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
        skill_offset += p->skillLevel( skill1 ) / 2;
    }
    if( skill2 ) {
        skill_offset += p->skillLevel( skill2 );
    }
    ///\EFFECT_INT increases chance of a placed turret being friendly
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

bool has_power_armor_interface(const player &p)
{
    return p.has_active_bionic( "bio_power_armor_interface" ) || p.has_active_bionic( "bio_power_armor_interface_mkII" );
}

bool has_powersource(const item &i, const player &p) {
    if( i.is_power_armor() && has_power_armor_interface( p ) && p.power_level > 0 ) {
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
    if( !choose_adjacent( _( "Use your pick lock where?" ), dirp ) ) {
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
    ///\EFFECT_DEX speeds up door lock picking

    ///\EFFECT_MECHANICS speeds up door lock picking
    p->moves -= std::max(0, ( 1000 - ( pick_quality * 100 ) ) - ( p->dex_cur + p->skillLevel( skill_mechanics ) ) * 5);
    ///\EFFECT_DEX improves chances of successfully picking door lock, reduces chances of bad outcomes

    ///\EFFECT_MECHANICS improves chances of successfully picking door lock, reduces chances of bad outcomes
    int pick_roll = ( dice( 2, p->skillLevel( skill_mechanics ) ) + dice( 2, p->dex_cur ) - it->damage / 2 ) * pick_quality;
    int door_roll = dice( 4, 30 );
    if( pick_roll >= door_roll ) {
        p->practice( skill_mechanics, 1 );
        p->add_msg_if_player( m_good, "%s", open_message.c_str() );
        g->m.ter_set( dirp, new_type );
    } else if( door_roll > ( 1.5 * pick_roll ) ) {
        if( it->damage++ >= MAX_ITEM_DAMAGE ) {
            p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it, and you destroy your tool." ) );
        } else {
            p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it, and you damage your tool." ) );
        }
    } else {
        p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it." ) );
    }
    if( type == t_door_locked_alarm && ( door_roll + dice( 1, 30 ) ) > pick_roll ) {
        sounds::sound( p->pos(), 40, _( "An alarm sounds!" ) );
        if( !g->event_queued( EVENT_WANTED ) ) {
            g->add_event( EVENT_WANTED, int( calendar::turn ) + 300, 0, p->global_sm_location() );
        }
    }
    if( it->damage > MAX_ITEM_DAMAGE ) {
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
    moves_cost = obj.get_int( "moves_cost", 0 );
}

iuse_actor *firestarter_actor::clone() const
{
    return new firestarter_actor( *this );
}

bool firestarter_actor::prep_firestarter_use( const player *p, const item *it, tripoint &pos )
{
    if( (it->charges == 0) && (!it->has_flag("LENS"))){ // lenses do not need charges
        return false;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if( !choose_adjacent(_("Light where?"), pos ) ) {
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
            if( i.type->id == "brazier" ) {
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

// TODO: Move prep_firestarter_use here
long firestarter_actor::use( player *p, item *it, bool t, const tripoint &pos ) const
{
    if( t ) {
        return 0;
    }

    tripoint tmp = pos;
    if( prep_firestarter_use(p, it, tmp) ) {
        p->moves -= moves_cost;
        resolve_firestarter_use( p, it, tmp );
        return it->type->charges_to_use();
    }

    return 0;
}

bool firestarter_actor::can_use( const player* p, const item*, bool, const tripoint& ) const
{
    if( p->is_underwater() ) {
        return false;
    }

    return true;
}

void extended_firestarter_actor::load( JsonObject &obj )
{
    need_sunlight = obj.get_bool( "need_sunlight", false );
    moves_cost = obj.get_int( "moves_cost", 0 );
}

iuse_actor *extended_firestarter_actor::clone() const
{
    return new extended_firestarter_actor( *this );
}

int extended_firestarter_actor::calculate_time_for_lens_fire( const player *p, float light_level ) const
{
    // base moves based on sunlight levels... 1 minute when sunny (80 lighting),
    // ~10 minutes when clear (60 lighting)
    float moves_base = std::pow( 80 / light_level, 8 ) * 1000 ;
    // survival 0 takes 3 * moves_base, survival 1 takes 1,5 * moves_base,
    // max moves capped at moves_base
    ///\EFFECT_SURVIVAL speeds up fire starting with lens
    float moves_modifier = 1 / ( p->get_skill_level( skill_survival ) * 0.33 + 0.33 );
    if( moves_modifier < 1 ) {
        moves_modifier = 1;
    }
    return int(moves_base * moves_modifier);
}

long extended_firestarter_actor::use( player *p, item *it, bool, const tripoint &spos ) const
{
    tripoint pos = spos;
    if( need_sunlight ) {
        // Needs the correct weather, light and to be outside.
        if( (g->weather == WEATHER_CLEAR || g->weather == WEATHER_SUNNY) &&
            g->natural_light_level( pos.z ) >= 60 && !g->m.has_flag( TFLAG_INDOORS, pos ) ) {
            if( prep_firestarter_use(p, it, pos ) ) {
                // turns needed for activity.
                const int turns = calculate_time_for_lens_fire( p, g->natural_light_level( pos.z ) );
                if( turns/1000 > 1 ) {
                    // If it takes less than a minute, no need to inform the player about time.
                    p->add_msg_if_player(m_info, _("If the current weather holds, it will take around %d minutes to light a fire."), turns / 1000);
                }
                p->assign_activity( ACT_START_FIRE, turns, -1, p->get_item_position(it), it->tname() );
                // Keep natural_light_level for comparing throughout the activity.
                p->activity.values.push_back( g->natural_light_level( pos.z ) );
                p->activity.placement = pos;
                p->practice( skill_survival, 5 );
            }
        } else {
            p->add_msg_if_player(_("You need direct sunlight to light a fire with this."));
        }
    } else {
        if( prep_firestarter_use(p, it, pos) ) {
            float skillLevel = float(p->get_skill_level( skill_survival ));
            // success chance is 100% but time spent is min 5 minutes at skill == 5 and
            // it increases for lower skill levels.
            // max time is 1 hour for 0 survival
            const float moves_base = 5 * 1000;
            if( skillLevel < 1 ) {
                // avoid dividing by zero. scaled so that skill level 0 means 60 minutes work
                skillLevel = 0.536;
            }
            // At survival=5 modifier=1, at survival=1 modifier=~6.
            ///\EFFECT_SURVIVAL speeds up fire starting
            float moves_modifier = std::pow( 5 / skillLevel, 1.113 );
            if (moves_modifier < 1) {
                moves_modifier = 1; // activity time improvement is capped at skillevel 5
            }
            const int turns = int( moves_base * moves_modifier );
            p->add_msg_if_player(m_info, _("At your skill level, it will take around %d minutes to light a fire."), turns / 1000);
            p->assign_activity(ACT_START_FIRE, turns, -1, p->get_item_position(it), it->tname());
            p->activity.placement = pos;
            p->practice( skill_survival, 10 );
            it->charges -= it->type->charges_to_use() * round(moves_modifier);
            return 0;
        }
    }
    return 0;
}

bool extended_firestarter_actor::can_use( const player* p, const item* it, bool t, const tripoint& pos ) const
{
    if( !firestarter_actor::can_use( p, it, t, pos ) ) {
        return false;
    }

    if( need_sunlight ) {
        return ( g->weather == WEATHER_CLEAR || g->weather == WEATHER_SUNNY ) &&
                 g->natural_light_level( pos.z ) >= 60 && !g->m.has_flag( TFLAG_INDOORS, pos );
    }

    return true;
}

void salvage_actor::load( JsonObject &obj )
{
    moves_per_part = obj.get_int( "moves_per_part", 25 );
    if( obj.has_array( "material_whitelist" ) ) {
        JsonArray jarr = obj.get_array( "material_whitelist" );
        while( jarr.has_more() ) {
            const auto material_id = jarr.next_string();
            material_whitelist.push_back( material_id );
        }
    } else {
        // Default to old salvageable materials
        material_whitelist.push_back("cotton");
        material_whitelist.push_back("leather");
        material_whitelist.push_back("fur");
        material_whitelist.push_back("nomex");
        material_whitelist.push_back("kevlar");
        material_whitelist.push_back("plastic");
        material_whitelist.push_back("wood");
        material_whitelist.push_back("wool");
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

    int inventory_index = g->inv_for_salvage( _("Cut up what?"), *this );
    item *cut = &( p->i_at( inventory_index ) );
    if( !try_to_cut_up(p, cut) ) {
        // Messages should have already been displayed.
        return 0;
    }

    return cut_up( p, it, cut );
}

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
    if (it->volume() == 0) {
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
        if (it->is_disassemblable()) {
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
    if( it->volume() == 0 ) {
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
    int pos = p->get_item_position(cut);
    // total number of raw components == total volume of item.
    // This can go awry if there is a volume / recipe mismatch.
    int count = cut->volume();
    // Chance of us losing a material component to entropy.
    ///\EFFECT_FABRICATION reduces chance of losing components when cutting items up
    int entropy_threshold = std::max(5, 10 - p->skillLevel( skill_fabrication ) );
    // What material components can we get back?
    std::vector<std::string> cut_material_components = cut->made_of();
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
    ///\EFFECT_DEX randomly reduces component loss when cutting items up
    if (dice(3, 4) > p->dex_cur) {
        count -= rng(0, 2);
    }
    // If more than 1 material component can still be be salvaged,
    // chance of losing more components if the item is damaged.
    // If the item being cut is not damaged, no additional losses will be incurred.
    if (count > 0 && cut->damage > 0) {
        float component_success_chance = std::min(std::pow(0.8, cut->damage), 1.0);
        for(int i = count; i > 0; i--) {
            if(component_success_chance < rng_float(0,1)) {
                count--;
            }
        }
    }

    // Decided to split components evenly. Since salvage will likely change
    // soon after I write this, I'll go with the one that is cleaner.
    for (auto material : cut_material_components) {
        material_type * mt = material_type::find_material(material);
        std::string salvaged_id = mt->salvage_id();
        float salvage_multiplier = mt->salvage_multiplier();
        materials_salvaged[salvaged_id] = count * salvage_multiplier / cut_material_components.size();
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
    return it->type->charges_to_use();
}

void inscribe_actor::load( JsonObject &obj )
{
    on_items = obj.get_bool( "on_items", true );
    on_terrain = obj.get_bool( "on_terrain", false );
    material_restricted = obj.get_bool( "material_restricted", true );

    if( obj.has_array( "material_whitelist" ) ) {
        JsonArray jarr = obj.get_array( "material_whitelist" );
        while( jarr.has_more() ) {
            const auto material_id = jarr.next_string();
            material_whitelist.push_back( material_id );
        }
    } else if( material_restricted ) {
        material_whitelist.reserve( 7 );
        // Default to old carveable materials
        material_whitelist.push_back("wood");
        material_whitelist.push_back("plastic");
        material_whitelist.push_back("glass");
        material_whitelist.push_back("chitin");
        material_whitelist.push_back("iron");
        material_whitelist.push_back("steel");
        material_whitelist.push_back("silver");
    }

    verb = _(obj.get_string( "verb", "Carve" ).c_str());
    gerund = _(obj.get_string( "gerund", "Carved" ).c_str());

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
    menu.text = string_format(_("%s meaning?"), verb.c_str());
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
    std::string message = "";
    std::string messageprefix = string_format(hasnote ? _("(To delete, input one '.')\n") : "") +
                                string_format(_("%1$s on the %2$s is: "),
                                        gerund.c_str(), cut->type_name().c_str());
    message = string_input_popup(string_format(_("%s what?"), verb.c_str()), 64,
                                 (hasnote ? cut->get_var( carving ) : message),
                                 messageprefix, "inscribe_item", 128);

    if( !message.empty() )
    {
        if( hasnote && message == "." ) {
            cut->erase_var( carving );
            cut->erase_var( carving_type );
        } else {
            cut->set_var( carving, message );
            cut->set_var( carving_type, gerund );
        }
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
        imenu.text = string_format( _("%s on what?"), verb.c_str() );
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
        return iuse::handle_ground_graffiti( p, it, string_format( _("%s what?"), verb.c_str()) );
    }

    int pos = g->inv( _("Inscribe which item?") );
    item *cut = &( p->i_at(pos) );
    // inscribe_item returns false if the action fails or is canceled somehow.
    if( item_inscription( cut ) ) {
        return it->type->charges_to_use();
    }

    return 0;
}

void cauterize_actor::load( JsonObject &obj )
{
    flame = obj.get_bool( "flame", true );
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
        if (!(p->has_trait("NOPAIN"))) {
            p->mod_pain(15);
            p->add_msg_if_player(m_bad, _("It hurts like hell!"));
        } else {
            p->add_msg_if_player(m_neutral, _("It itches a little."));
        }
        const body_part bp = player::hp_to_bp( hpart );
        if (p->has_effect( effect_bite, bp)) {
            p->add_effect( effect_bite, 2600, bp, true);
        }
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
    } else if( !flame && it->type->charges_to_use() > it->charges ) {
        p->add_msg_if_player( m_info, _("You need at least %d charges to cauterize wounds."), it->type->charges_to_use() );
        return 0;
    } else if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _("You can't cauterize anything underwater.") );
        return 0;
    } else if( has_disease ) {
        did_cauterize = cauterize_effect( p, it, !has_disease );
    } else {
        if( ( p->has_trait("MASOCHIST") || p->has_trait("MASOCHIST_MED") || p->has_trait("CENOBITE") ) &&
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
    }

    return it->type->charges_to_use();
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
    } else if( p->has_trait("MASOCHIST") || p->has_trait("MASOCHIST_MED") || p->has_trait("CENOBITE") ) {
        return true;
    }

    return false;
}

void enzlave_actor::load( JsonObject & )
{
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
        if( it.is_corpse() && mt->in_species( ZOMBIE ) && mt->has_material("flesh") &&
            mt->sym == "Z" && it.active && !it.has_var( "zlave" ) ) {
            corpses.push_back( &it );
        }
    }

    if( corpses.empty() ) {
        p->add_msg_if_player(_("No suitable corpses"));
        return 0;
    }

    int tolerance_level = 9;
    if( p->has_trait("PSYCHOPATH") || p->has_trait("SAPIOVORE") ) {
        tolerance_level = 0;
    } else if( p->has_trait("PRED4") ) {
        tolerance_level = 5;
    } else if( p->has_trait("PRED3") ) {
        tolerance_level = 7;
    }

    // Survival skill increases your willingness to get things done,
    // but it doesn't make you feel any less bad about it.
    ///\EFFECT_SURVIVAL increases tolerance for enzlavement
    if( p->morale_level() <= (15 * (tolerance_level - p->skillLevel( skill_survival ) )) - 150 ) {
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

        ///\EFFECT_SURVIVAL decreases moral penalty and duration for enzlavement
        int moraleMalus = -50 * (5.0 / (float) p->skillLevel( skill_survival ));
        int maxMalus = -250 * (5.0 / (float)p->skillLevel( skill_survival ));
        int duration = 300 * (5.0 / (float)p->skillLevel( skill_survival ));
        int decayDelay = 30 * (5.0 / (float)p->skillLevel( skill_survival ));

        if (p->has_trait("PACIFIST")) {
            moraleMalus *= 5;
            maxMalus *= 3;
        } else if (p->has_trait("PRED1")) {
            moraleMalus /= 4;
        } else if (p->has_trait("PRED2")) {
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
    int difficulty = (body->damage * 5) + (mt->hp / 10) + (mt->speed / 5);
    // 0 - 30
    ///\EFFECT_DEX increases chance of success for enzlavement

    ///\EFFECT_SURVIVAL increases chance of success for enzlavement

    ///\EFFECT_FIRSTAID increases chance of success for enzlavement
    int skills = p->skillLevel( skill_survival ) + p->skillLevel( skill_firstaid ) + (p->dex_cur / 2);
    skills *= 2;

    int success = rng(0, skills) - rng(0, difficulty);

    ///\EFFECT_FIRSTAID speeds up enzlavement
    const int moves = difficulty * 1200 / p->skillLevel( skill_firstaid );

    p->assign_activity(ACT_MAKE_ZLAVE, moves);
    p->activity.values.push_back(success);
    p->activity.str_values.push_back(corpses[selected_corpse]->display_name());
    return it->type->charges_to_use();
}

bool enzlave_actor::can_use( const player *p, const item*, bool, const tripoint& ) const
{
    ///\EFFECT_SURVIVAL >1 allows enzlavement

    ///\EFFECT_FIRSTAID >1 allows enzlavement
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
    if( rng( 0, 10 ) - it->damage > success_chance && !p->is_underwater() ) {
        if( noise > 0 ) {
            sounds::sound( p->pos(), noise, _(success_message.c_str()) );
        } else {
            p->add_msg_if_player( _(success_message.c_str()) );
        }

        it->make( target_id );
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
        it->active = false;
        const auto tool = dynamic_cast<const it_tool *>( it->type );
        if( tool == nullptr ) {
            debugmsg( "Non-tool has fireweapon_on actor" );
            it->make( "none" );
        }

        it->make( tool->revert_to );
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

    if( !t ) {
        // TODO: Make the player stop playing music when paralyzed/choking
        if( it->active || p->has_effect( effect_sleep) ) {
            p->add_msg_if_player( _("You stop playing your %s"), it->display_name().c_str() );
            it->active = false;
            return 0;
        }
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
    ///\EFFECT_PER increases morale bonus when playing an instrument
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
    holster_msg    = obj.get_string( "holster_msg",    "" );

    max_volume = obj.get_int( "max_volume" );
    min_volume = obj.get_int( "min_volume", max_volume / 3 );
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
    p.store( &holster, &obj, draw_cost, false );
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
        // holsters ignore penalty effects (eg. GRABBED) when determining number of moves to consume
        p->wield_contents( it, pos, draw_cost, false );

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

void repair_item_actor::load( JsonObject &obj )
{
    // Mandatory:
    JsonArray jarr = obj.get_array( "materials" );
    while( jarr.has_more() ) {
        materials.push_back( jarr.next_string() );
    }

    // TODO: Make skill non-mandatory while still erroring on invalid skill
    const std::string skill_string = obj.get_string( "skill" );
    used_skill = skill_id( skill_string );
    if( !used_skill.is_valid() ) {
        obj.throw_error( "Invalid skill", "skill" );
    }

    cost_scaling = obj.get_float( "cost_scaling" );

    // Kinda hacky: get subtype of the actor for item action menu
    type = obj.get_string( "item_action_type" );

    // Optional
    tool_quality = obj.get_int( "tool_quality", 0 );
    move_cost    = obj.get_int( "move_cost", 500 );
}

// TODO: This should be a property of material json, not a hardcoded hack
const itype_id &material_component( const std::string &material_id )
{
    static const std::map< std::string, itype_id > material_id_map {
        // Metals (welded)
        { "kevlar", "kevlar_plate" },
        { "plastic", "plastic_chunk" },
        { "iron", "scrap" },
        { "steel", "scrap" },
        { "hardsteel", "scrap" },
        { "aluminum", "material_aluminium_ingot" },
        { "copper", "scrap_copper" },
        // Fabrics (sewn)
        { "cotton", "rag" },
        { "leather", "leather" },
        { "fur", "fur" },
        { "nomex", "nomex" },
        { "wool", "felt_patch" }
    };

    static const itype_id null_material = "";
    const auto iter = material_id_map.find( material_id );
    if( iter != material_id_map.end() ) {
        return iter->second;
    }

    return null_material;
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
    int charges_used = dynamic_cast<const it_tool*>( it.type )->charges_to_use();
    if( it.charges < charges_used ) {
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

    int pos = g->inv_for_filter( _("Repair what?"), [this, it]( const item &itm ) {
        return itm.made_of_any( materials ) && !itm.is_ammo() && !itm.is_firearm() && &itm != it;
    } );

    item &fix = p->i_at( pos );
    if( fix.is_null() ) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }

    p->assign_activity( ACT_REPAIR_ITEM, 0, p->get_item_position( it ), pos );
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
    std::set<std::string> valid_entries;
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
                const auto mat = material_type::find_material( mat_name );
                const auto mat_comp = material_component( mat_name );
                pl.add_msg_if_player( m_info, _("%s (repaired using %s)"),
                                      mat->name().c_str(), item::nname( mat_comp, 2 ).c_str() );
            }
        }

        return false;
    }

    // Repairing apparently doesn't always consume items;
    // maybe it should just consume less or something?
    // Anyway, don't ask for items if we won't need any.
    if( !(fix.damage >= 3 || fix.damage == 0) ) {
        return true;
    }

    const inventory &crafting_inv = pl.crafting_inventory();

    // Repairing or modifying items requires at least 1 repair item,
    //  otherwise number is related to size of item
    const int items_needed = std::max<int>( 1, ceil( fix.volume() * cost_scaling ) );

    // Go through all discovered repair items and see if we have any of them available
    for( const auto &entry : valid_entries ) {
        const auto component_id = material_component( entry );
        if( crafting_inv.has_amount( component_id, items_needed ) ) {
            // We've found enough of a material, add it to list
            comps.push_back( item_comp( component_id, items_needed ) );
        }
    }

    if( comps.empty() ) {
        if( print_msg ) {
            for( const auto &entry : valid_entries ) {
                const auto &mat_comp = material_component( entry );
                pl.add_msg_if_player( m_info,
                    _("You don't have enough %s to do that. Have: %d, need: %d"),
                    item::nname( mat_comp, 2 ).c_str(),
                    crafting_inv.amount_of( mat_comp, false ), items_needed );
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
    if( fix.is_ammo() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("You cannot repair this type of item.") );
        }
        return false;
    }

    if( &fix == &tool || any_of( materials.begin(), materials.end(), [&fix]( const std::string &mat ) {
            return material_component( mat ) == fix.typeId();
        } ) ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("This can be used to repair other items, not itself.") );
        }
        return false;
    }

    if( !handle_components( pl, fix, print_msg, true ) ) {
        return false;
    }

    if( fix.damage == 0 && fix.has_flag("PRIMITIVE_RANGED_WEAPON") ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _("You cannot improve your %s any more this way."), fix.tname().c_str());
        }
        return false;
    }

    if( fix.damage >= 0 || (fix.has_flag("VARSIZE") && !fix.has_flag("FIT")) ) {
        return true;
    }

    if( print_msg ) {
        pl.add_msg_if_player( m_info, _("Your %s is already enhanced."), fix.tname().c_str() );
    }
    return false;
}

repair_item_actor::attempt_hint repair_item_actor::repair( player &pl, item &tool, item &fix ) const
{
    if( !can_repair( pl, tool, fix, true ) ) {
        return AS_CANT;
    }

    pl.practice( used_skill, 8 );
    ///\EFFECT_TAILOR randomly improves clothing repair efforts
    ///\EFFECT_MECHANICS randomly improves metal repair efforts
    // Let's make refitting/reinforcing as hard as recovering an almost-wrecked item
    // TODO: Make difficulty depend on the item type (for example, on recipe's difficulty)
    const int difficulty = fix.damage == 0 ? 4 : fix.damage;
    float repair_chance = (5 + pl.get_skill_level( used_skill ) - difficulty) / 100.0f;
    ///\EFFECT_DEX randomly reduces the chances of damaging an item when repairing
    float damage_chance = (5 - (pl.dex_cur - tool_quality) / 5.0f) / 100.0f;
    float roll_value = rng_float( 0.0, 1.0 );
    enum roll_result {
        SUCCESS,
        FAILURE,
        NEUTRAL
    } roll;

    if( roll_value > 1.0f - damage_chance ) {
        roll = FAILURE;
    } else if( roll_value < repair_chance ) {
        roll = SUCCESS;
    } else {
        roll = NEUTRAL;
    }

    if( fix.damage > 0 ) {
        if( roll == FAILURE ) {
            pl.add_msg_if_player(m_bad, _("You damage your %s further!"), fix.tname().c_str());
            fix.damage++;
            if( fix.damage >= 5 ) {
                pl.add_msg_if_player(m_bad, _("You destroy it!"));
                const int pos = pl.get_item_position( &fix );
                if( pos != INT_MIN ) {
                    pl.i_rem_keep_contents( pos );
                } else {
                    // NOTE: Repairing items outside inventory is NOT yet supported!
                    debugmsg( "Tried to remove an item that doesn't exist" );
                }

                return AS_DESTROYED;
            }

            return AS_FAILURE;
        }

        if( roll == SUCCESS ) {
            pl.add_msg_if_player(m_good, _("You repair your %s!"), fix.tname().c_str());
            handle_components( pl, fix, false, false );
            fix.damage--;
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    if( fix.damage == 0 && fix.has_flag("PRIMITIVE_RANGED_WEAPON") ) {
        pl.add_msg_if_player(m_info, _("You cannot improve your %s any more this way."), fix.tname().c_str());
        return AS_CANT;
    }

    if( fix.damage == 0 || (fix.has_flag("VARSIZE") && !fix.has_flag("FIT")) ) {
        if( roll == FAILURE ) {
            pl.add_msg_if_player(m_bad, _("You damage your %s!"), fix.tname().c_str());
            fix.damage++;
            return AS_FAILURE;
        }

        if( roll == SUCCESS && fix.has_flag("VARSIZE") && !fix.has_flag("FIT") ) {
            pl.add_msg_if_player(m_good, _("You take your %s in, improving the fit."),
                                 fix.tname().c_str());
            fix.item_tags.insert("FIT");
            handle_components( pl, fix, false, false );
            return AS_SUCCESS;
        }

        if( roll == SUCCESS && (fix.has_flag("FIT") || !fix.has_flag("VARSIZE")) ) {
            pl.add_msg_if_player(m_good, _("You make your %s extra sturdy."), fix.tname().c_str());
            fix.damage--;
            handle_components( pl, fix, false, false );
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    pl.add_msg_if_player(m_info, _("Your %s is already enhanced."), fix.tname().c_str());
    return AS_CANT;
}

void heal_actor::load( JsonObject &obj )
{
    // Mandatory
    limb_power = obj.get_int( "limb_power" );
    move_cost = obj.get_int( "move_cost" );

    // Optional
    head_power = obj.get_int( "head_power", 0.8f * limb_power );
    torso_power = obj.get_int( "torso_power", 1.5f * limb_power );

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

    player &patient = get_patient( *p, pos );
    const hp_part hpp = use_healing_item( *p, patient, *it, false );
    if( hpp == num_hp_parts ) {
        return 0;
    }

    int cost = move_cost;
    if( long_action ) {
        // A hack: long action healing on NPCs isn't done yet.
        // So just heal at start and paralyze the player for 5 minutes.
        cost /= (p->skillLevel( skill_firstaid ) + 1);
    }

    if( long_action && &patient == p ) {
        // Assign first aid long action.
        ///\EFFECT_FIRSTAID speeds up firstaid activity
        p->assign_activity( ACT_FIRSTAID, cost, 0, p->get_item_position( it ), it->tname() );
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
        bonus_mult = 0.8f;
    } else if( healed == hp_torso ) {
        heal_base = torso_power;
        bonus_mult = 1.5f;
    } else {
        heal_base = limb_power;
        bonus_mult = 1.0f;
    }

    if( heal_base > 0 ) {
        ///\EFFECT_FIRSTAID increases healing item effects
        float bonus = healer.get_skill_level( skill_firstaid ) * bonus_scaling;
        return heal_base + bonus_mult * bonus;
    }

    return heal_base;
}

long heal_actor::finish_using( player &healer, player &patient, item &it, hp_part healed ) const
{
    healer.practice( skill_firstaid, 8 );
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
            patient.remove_effect( effect_bleed, bp_healed);
            heal_msg( m_good, _("You stop the bleeding."), _("The bleeding is stopped.") );
        } else {
            heal_msg( m_warning, _("You fail to stop the bleeding."), _("The wound still bleeds.") );
        }
    }
    if( patient.has_effect( effect_bite, bp_healed ) ) {
        if( x_in_y( bite, 1.0f ) ) {
            patient.remove_effect( effect_bite, bp_healed);
            heal_msg( m_good, _("You clean the wound."), _("The wound is cleaned.") );
        } else {
            heal_msg( m_warning, _("Your wound still aches."), _("The wound still looks bad.") );
        }
    }
    if( patient.has_effect( effect_infected, bp_healed ) ) {
        if( x_in_y( infect, 1.0f ) ) {
            int infected_dur = patient.get_effect_dur( effect_infected, bp_healed );
            patient.remove_effect( effect_infected, bp_healed);
            patient.add_effect( effect_recover, infected_dur);
            heal_msg( m_good, _("You disinfect the wound."), _("The wound is disinfected.") );
        } else {
            heal_msg( m_warning, _("Your wound still hurts."), _("The wound still looks nasty.") );
        }
    }

    if( long_action ) {
        healer.add_msg_if_player( _("You finish using the %s."), it.tname().c_str() );
    }

    for( auto eff : effects ) {
        if( eff.id == efftype_id( "null" ) ) {
            continue;
        }

        patient.add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
    }

    if( !used_up_item.empty() ) {
        // If the item is a tool, `make` it the new form
        // Otherwise it probably was consumed, so create a new one
        if( it.is_tool() ) {
            it.make( used_up_item );
        } else {
            item used_up( used_up_item, it.bday );
            healer.i_add_or_drop( used_up );
        }
    }

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
        patient.has_trait( "SELFAWARE" ) :
        ///\EFFECT_PER slightly increases precision when using first aid on someone else

        ///\EFFECT_FIRSTAID increases precision when using first aid on someone else
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
        if( healer.activity.type != ACT_FIRSTAID ) {
            const std::string menu_header = it.tname();
            healed = pick_part_to_heal( healer, patient, menu_header,
                                        limb_power, head_bonus, torso_bonus,
                                        bleed, bite, infect, force );
            if( healed == num_hp_parts ) {
                return num_hp_parts; // canceled
            }
        }
        // Brick healing if using a first aid kit for the first time.
        if( long_action && healer.activity.type != ACT_FIRSTAID ) {
            // Cancel and wait for activity completion.
            return healed;
        } else if( healer.activity.type == ACT_FIRSTAID ) {
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
