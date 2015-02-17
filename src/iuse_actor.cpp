#include "iuse_actor.h"
#include "item.h"
#include "game.h"
#include "monster.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "monstergenerator.h"
#include <sstream>
#include <algorithm>

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
}

long iuse_transform::use(player *p, item *it, bool t, point /*pos*/) const
{
    if( t ) {
        // Invoked from active item processing, do nothing.
        return 0;
    }
    if (need_fire > 0 && p != NULL && p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater"));
        return 0;
    }
    if (need_charges > 0 && it->charges < need_charges) {
        if (!need_charges_msg.empty()) {
            p->add_msg_if_player(m_info, _( need_charges_msg.c_str() ), it->tname().c_str());
        }
        return 0;
    }
    if (p != NULL && need_fire > 0 && !p->use_charges_if_avail("fire", need_fire)) {
        if (!need_fire_msg.empty()) {
            p->add_msg_if_player(m_info, _( need_fire_msg.c_str() ), it->tname().c_str());
        }
        return 0;
    }
    // load this from the original item, not the transformed one.
    const long charges_to_use = it->type->charges_to_use();
    if( !msg_transform.empty() ) {
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
        it->make(container_id);
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
    p->moves -= moves;
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

long auto_iuse_transform::use(player *p, item *it, bool t, point pos) const
{
    if (t) {
        if (!when_underwater.empty() && p != NULL && p->is_underwater()) {
            // Dirty hack to display the "when unterwater" message instead of the normal message
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
        p->add_msg_if_player(m_info, _( non_interactive_msg.c_str() ), it->tname().c_str());
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
extern std::vector<point> points_for_gas_cloud(const point &center, int radius);

void explosion_iuse::load( JsonObject &obj )
{
    obj.read( "explosion_power", explosion_power );
    obj.read( "explosion_shrapnel", explosion_shrapnel );
    obj.read( "explosion_fire", explosion_fire );
    obj.read( "explosion_blast", explosion_blast );
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

long explosion_iuse::use(player *p, item *it, bool t, point pos) const
{
    if (t) {
        if (sound_volume >= 0) {
            sounds::sound(pos.x, pos.y, sound_volume, sound_msg);
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
        g->explosion(pos.x, pos.y, explosion_power, explosion_shrapnel, explosion_fire, explosion_blast);
    }
    if (draw_explosion_radius >= 0) {
        g->draw_explosion(pos.x, pos.y, draw_explosion_radius, draw_explosion_color);
    }
    if (do_flashbang) {
        g->flashbang(pos.x, pos.y, flashbang_player_immune);
    }
    if (fields_radius >= 0 && fields_type != fd_null) {
        std::vector<point> gas_sources = points_for_gas_cloud(pos, fields_radius);
        for( auto &gas_source : gas_sources ) {
            const point &p = gas_source;
            const int dens = rng(fields_min_density, fields_max_density);
            g->m.add_field(p.x, p.y, fields_type, dens);
        }
    }
    if (scrambler_blast_radius >= 0) {
        for (int x = pos.x - scrambler_blast_radius; x <= pos.x + scrambler_blast_radius; x++) {
            for (int y = pos.y - scrambler_blast_radius; y <= pos.y + scrambler_blast_radius; y++) {
                g->scrambler_blast(x, y);
            }
        }
    }
    if (emp_blast_radius >= 0) {
        for (int x = pos.x - emp_blast_radius; x <= pos.x + emp_blast_radius; x++) {
            for (int y = pos.y - emp_blast_radius; y <= pos.y + emp_blast_radius; y++) {
                g->emp_blast(x, y);
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
    obj.read( "vehicle_name", vehicle_name );
    obj.read( "unfold_msg", unfold_msg );
    obj.read( "moves", moves );
    obj.read( "tools_needed", tools_needed );
}

long unfold_vehicle_iuse::use(player *p, item *it, bool /*t*/, point /*pos*/) const
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

    vehicle *veh = g->m.add_vehicle(vehicle_name, p->posx(), p->posy(), 0, 0, 0, false);
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
        } catch(std::string e) {
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

void consume_drug_iuse::load( JsonObject &obj )
{
    obj.read( "activation_message", activation_message );
    obj.read( "charges_needed", charges_needed );
    obj.read( "tools_needed", tools_needed );

    if( obj.has_array( "effects" ) ) {
        JsonArray jsarr = obj.get_array( "effects" );
        while( jsarr.has_more() ) {
            JsonObject e = jsarr.next_object();
            effect_data new_eff( e.get_string( "id", "null" ), e.get_int( "duration", 0 ),
                                 body_parts[e.get_string( "bp", "NUM_BP" )], e.get_bool( "permanent", false ) );
            effects.push_back( new_eff );
        }
    }
    obj.read( "stat_adjustments", stat_adjustments );
    obj.read( "fields_produced", fields_produced );
}

long consume_drug_iuse::use(player *p, item *it, bool, point) const
{
    // Check prerequisites first.
    for( auto tool = tools_needed.cbegin(); tool != tools_needed.cend(); ++tool ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p->has_amount( tool->first, 1 ) ) {
            p->add_msg_if_player( _("You need %s to consume %s!"),
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
            p->add_msg_if_player( _("You need %s to consume %s!"),
                                  item::nname( consumable->first ).c_str(),
                                  it->type_name( 1 ).c_str() );
            return -1;
        }
    }
    // Apply the various effects.
    for( auto eff : effects ) {
        if (eff.id == "null") {
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
            g->m.add_field(p->posx() + int(rng(-2, 2)), p->posy() + int(rng(-2, 2)), fid, field->second);
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

long delayed_transform_iuse::use( player *p, item *it, bool t, point pos ) const
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
    mtype_id = obj.get_string( "monster_id" );
    obj.read( "friendly_msg", friendly_msg );
    obj.read( "hostile_msg", hostile_msg );
    obj.read( "difficulty", difficulty );
    obj.read( "moves", moves );
    obj.read( "place_randomly", place_randomly );
    obj.read( "skill1", skill1 );
    obj.read( "skill2", skill2 );
}

long place_monster_iuse::use( player *p, item *it, bool, point ) const
{
    monster newmon( GetMType( mtype_id ) );
    point target;
    if( place_randomly ) {
        std::vector<point> valid;
        for( int x = p->posx() - 1; x <= p->posx() + 1; x++ ) {
            for( int y = p->posy() - 1; y <= p->posy() + 1; y++ ) {
                if( g->is_empty( x, y ) ) {
                    valid.push_back( point( x, y ) );
                }
            }
        }
        if( valid.empty() ) { // No valid points!
            p->add_msg_if_player( m_info, _( "There is no adjacent square to release the %s in!" ),
                                  newmon.name().c_str() );
            return 0;
        }
        target = valid[rng( 0, valid.size() - 1 )];
    } else {
        const std::string query = string_format( _( "Place the %s where?" ), newmon.name().c_str() );
        if( !choose_adjacent( query, target.x, target.y ) ) {
            return 0;
        }
        if( !g->is_empty( target.x, target.y ) ) {
            p->add_msg_if_player( m_info, _( "You cannot place a %s there." ), newmon.name().c_str() );
            return 0;
        }
    }
    p->moves -= moves;
    newmon.spawn( target.x, target.y );
    for( auto & amdef : newmon.ammo ) {
        item ammo_item( amdef.first, 0 );
        const int available = p->charges_of( amdef.first );
        if( available == 0 ) {
            amdef.second = 0;
            p->add_msg_if_player( m_info,
                                  _( "If you had standard factory-built %s bullets, you could load the %s." ),
                                  ammo_item.type_name( 2 ).c_str(), newmon.name().c_str() );
            continue;
        }
        // Don't load more than the default from the the monster definition.
        ammo_item.charges = std::min( available, amdef.second );
        p->use_charges( amdef.first, ammo_item.charges );
        //~ First %s is the ammo item (with plural form and count included), second is the monster name
        p->add_msg_if_player( ngettext( "You load %d x %s round into the %s.",
                                        "You load %d x %s rounds into the %s.", ammo_item.charges ),
                              ammo_item.charges, ammo_item.type_name( ammo_item.charges ).c_str(),
                              newmon.name().c_str() );
        amdef.second = ammo_item.charges;
    }
    newmon.init_from_item( *it );
    if( rng( 0, p->int_cur / 2 ) + p->skillLevel( skill1 ) / 2 + p->skillLevel( skill2 ) <
        rng( 0, difficulty ) ) {
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
    if( newmon.type->id == "mon_laserturret" && !g->is_in_sunlight( newmon.posx(), newmon.posy() ) ) {
        p->add_msg_if_player( _( "A flashing LED on the laser turret appears to indicate low light." ) );
    }
    g->add_zombie( newmon );
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

long ups_based_armor_actor::use( player *p, item *it, bool t, point ) const
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

long pick_lock_actor::use( player *p, item *it, bool, point ) const
{
    if( p == nullptr || p->is_npc() ) {
        return 0;
    }
    int dirx, diry;
    if( !choose_adjacent( _( "Use your pick lock where?" ), dirx, diry ) ) {
        return 0;
    }
    if( dirx == p->posx() && diry == p->posy() ) {
        p->add_msg_if_player( m_info, _( "You pick your nose and your sinuses swing open." ) );
        return 0;
    }
    const ter_id type = g->m.ter( dirx, diry );
    const int npcdex = g->npc_at( dirx, diry );
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

    p->practice( "mechanics", 1 );
    p->moves -= std::min( 0, ( 1000 - ( pick_quality * 100 ) ) - ( p->dex_cur + p->skillLevel( "mechanics" ) ) * 5 );
    int pick_roll = ( dice( 2, p->skillLevel( "mechanics" ) ) + dice( 2, p->dex_cur ) - it->damage / 2 ) * pick_quality;
    int door_roll = dice( 4, 30 );
    if( pick_roll >= door_roll ) {
        p->practice( "mechanics", 1 );
        p->add_msg_if_player( m_good, "%s", open_message.c_str() );
        g->m.ter_set( dirx, diry, new_type );
    } else if( door_roll > ( 1.5 * pick_roll ) && it->damage < 100 ) {
        it->damage++;
        if( it->damage >= 5 ) {
            p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it, and you destroy your tool." ) );
        } else {
            p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it, and you damage your tool." ) );
        }
    } else {
        p->add_msg_if_player( m_bad, _( "The lock stumps your efforts to pick it." ) );
    }
    if( type == t_door_locked_alarm && ( door_roll + dice( 1, 30 ) ) > pick_roll &&
        it->damage < 100 ) {
        sounds::sound( p->posx(), p->posy(), 40, _( "An alarm sounds!" ) );
        if( !g->event_queued( EVENT_WANTED ) ) {
            g->add_event( EVENT_WANTED, int( calendar::turn ) + 300, 0, g->get_abs_levx(), g->get_abs_levy() );
        }
    }
    // Special handling, normally the item isn't used up, but it is if broken.
    if( it->damage >= 5 ) {
        return 1;
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

void reveal_map_actor::reveal_targets( const std::string &target, int reveal_distance ) const
{
    const auto places = overmap_buffer.find_all( g->om_global_location(), target, radius, false );
    for( auto & place : places ) {
        overmap_buffer.reveal( place, reveal_distance, g->levz );
    }
}

long reveal_map_actor::use( player *p, item *it, bool, point ) const
{
    if( it->already_used_by_player( *p ) ) {
        p->add_msg_if_player( _( "There isn't anything new on the %s." ), it->tname().c_str() );
        return 0;
    } else if( g->levz < 0 ) {
        p->add_msg_if_player( _( "You should read your %s when you get to the surface." ),
                              it->tname().c_str() );
        return 0;
    }
    for( auto & omt : omt_types ) {
        reveal_targets( omt, 0 );
    }
    if( !message.empty() ) {
        p->add_msg_if_player( m_good, "%s", _( message.c_str() ) );
    }
    it->mark_as_used_by_player( *p );
    return 0;
}

firestarter_actor::~firestarter_actor() {}

void firestarter_actor::load( JsonObject &obj )
{
    moves_cost = obj.get_int( "moves_cost", 0 );
}

iuse_actor *firestarter_actor::clone() const
{
    return new firestarter_actor( *this );
}

bool firestarter_actor::prep_firestarter_use( const player *p, const item *it, point &pos )
{
    if( (it->charges == 0) && (!it->has_flag("LENS"))){ // lenses do not need charges
        return false;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if( !choose_adjacent(_("Light where?"), pos.x, pos.y) ) {
        return false;
    }
    if( pos.x == p->posx() && pos.y == p->posy() ) {
        p->add_msg_if_player(m_info, _("You would set yourself on fire."));
        p->add_msg_if_player(_("But you're already smokin' hot."));
        return false;
    }
    if( g->m.get_field( pos, fd_fire ) ) {
        // check if there's already a fire
        p->add_msg_if_player(m_info, _("There is already a fire."));
        return false;
    }
    if( g->m.flammable_items_at(pos.x, pos.y) ||
        g->m.has_flag("FLAMMABLE", pos.x, pos.y) || g->m.has_flag("FLAMMABLE_ASH", pos.x, pos.y) ||
        g->m.get_field_strength( pos, fd_web ) > 0 ) {
        return true;
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to light there."));
        return false;
    }
}

void firestarter_actor::resolve_firestarter_use( const player *p, const item *, const point &pos )
{
    if( g->m.add_field( pos, fd_fire, 1, 100 ) ) {
        p->add_msg_if_player(_("You successfully light a fire."));
    }
}

// TODO: Move prep_firestarter_use here
long firestarter_actor::use( player *p, item *it, bool t, point pos ) const
{
    if (it->has_flag("REFILLABLE_LIGHTER")) {
        if( p->is_underwater() ) {
            p->add_msg_if_player(_("The lighter is extinguished."));
            it->make("ref_lighter");
            it->active = false;
            return 0;
        }
        if (t) {
            if (it->charges < it->type->charges_to_use()) {
                p->add_msg_if_player(_("The lighter burns out."));
                it->make("ref_lighter");
                it->active = false;
            }
        } else if (it->charges <= 0) {
            p->add_msg_if_player(_("The %s winks out."), it->tname().c_str());
        } else { // Turning it off
            int choice = menu(true, _("refillable lighter (lit)"), _("extinguish"),
                              _("light something"), _("cancel"), NULL);
            switch (choice) {
                case 1: {
                    p->add_msg_if_player(_("You extinguish the lighter."));
                    it->make("ref_lighter");
                    it->active = false;
                    return 0;
                }
                break;
                case 2:
                    if( prep_firestarter_use(p, it, pos) ) {
                        p->moves -= moves_cost;
                        resolve_firestarter_use(p, it, pos);
                        return it->type->charges_to_use();
                    }
            }
        }
        return it->type->charges_to_use();
    } else { // common ligher or matches
        if( prep_firestarter_use(p, it, pos) ) {
            p->moves -= moves_cost;
            resolve_firestarter_use( p, it, pos );
            return it->type->charges_to_use();
        }
    }
    return 0;
}

bool firestarter_actor::can_use( const player* p, const item*, bool, const point& ) const
{
    if( p->is_underwater() ) {
        return false;
    }

    return true;
}

extended_firestarter_actor::~extended_firestarter_actor() {}

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
    float moves_modifier = 1 / ( p->get_skill_level("survival") * 0.33 + 0.33 );
    if( moves_modifier < 1 ) {
        moves_modifier = 1;
    }
    return int(moves_base * moves_modifier);
}

long extended_firestarter_actor::use( player *p, item *it, bool, point pos ) const
{
    if( need_sunlight ) {
        // Needs the correct weather, light and to be outside.
        if( (g->weather == WEATHER_CLEAR || g->weather == WEATHER_SUNNY) &&
            g->natural_light_level() >= 60 && !g->m.has_flag("INDOORS", pos.x, pos.y) ) {
            if( prep_firestarter_use(p, it, pos ) ) {
                // turns needed for activity.
                const int turns = calculate_time_for_lens_fire( p, g->natural_light_level() );
                if( turns/1000 > 1 ) {
                    // If it takes less than a minute, no need to inform the player about time.
                    p->add_msg_if_player(m_info, _("If the current weather holds, it will take around %d minutes to light a fire."), turns / 1000);
                }
                p->assign_activity( ACT_START_FIRE, turns, -1, p->get_item_position(it), it->tname() );
                // Keep natural_light_level for comparing throughout the activity.
                p->activity.values.push_back( g->natural_light_level() );
                p->activity.placement = pos;
                p->practice("survival", 5);
            }
        } else {
            p->add_msg_if_player(_("You need direct sunlight to light a fire with this."));
        }
    } else {
        if( prep_firestarter_use(p, it, pos) ) {
            float skillLevel = float(p->get_skill_level("survival"));
            // success chance is 100% but time spent is min 5 minutes at skill == 5 and
            // it increases for lower skill levels.
            // max time is 1 hour for 0 survival
            const float moves_base = 5 * 1000;
            if( skillLevel < 1 ) {
                // avoid dividing by zero. scaled so that skill level 0 means 60 minutes work
                skillLevel = 0.536;
            }
            // At survival=5 modifier=1, at survival=1 modifier=~6.
            float moves_modifier = std::pow( 5 / skillLevel, 1.113 );
            if (moves_modifier < 1) {
                moves_modifier = 1; // activity time improvement is capped at skillevel 5
            }
            const int turns = int( moves_base * moves_modifier );
            p->add_msg_if_player(m_info, _("At your skill level, it will take around %d minutes to light a fire."), turns / 1000);
            p->assign_activity(ACT_START_FIRE, turns, -1, p->get_item_position(it), it->tname());
            p->activity.placement = pos;
            p->practice("survival", 10);
            it->charges -= it->type->charges_to_use() * round(moves_modifier);
            return 0;
        }
    }
    return 0;
}

bool extended_firestarter_actor::can_use( const player* p, const item* it, bool t, const point& pos ) const
{
    if( !firestarter_actor::can_use( p, it, t, pos ) ) {
        return false;
    }

    if( need_sunlight ) {
        return ( g->weather == WEATHER_CLEAR || g->weather == WEATHER_SUNNY ) &&
                 g->natural_light_level() >= 60 && !g->m.has_flag( "INDOORS", pos.x, pos.y );
    }

    return true;
}

salvage_actor::~salvage_actor() {}

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

long salvage_actor::use( player *p, item *it, bool t, point ) const
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
    if (it->is_container() && !it->contents.empty()) {
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
    int entropy_threshold = std::max(5, 10 - p->skillLevel("fabrication"));
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
    const Skill* isFab = Skill::skill("fabrication");
    p->practice(isFab, rng(0, 5), 1);

    // Higher fabrication, less chance of entropy, but still a chance.
    if( rng(1, 10) <= entropy_threshold ) {
        count -= 1;
    }
    // Fail dex roll, potentially lose more parts.
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
            add_msg( m_good, ngettext("Salvaged %1$i %2$s.", "Salvaged %1$i %2$ss.", amount),
                     amount, result.display_name().c_str() );
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
        material_whitelist.emplace_back("wood");
        material_whitelist.emplace_back("plastic");
        material_whitelist.emplace_back("glass");
        material_whitelist.emplace_back("chitin");
        material_whitelist.emplace_back("iron");
        material_whitelist.emplace_back("steel");
        material_whitelist.emplace_back("silver");
    }

    if( !on_items && on_terrain ) {
        debugmsg( "Tried to create an useless inscribe_actor" );
        on_items = true;
    }
}

iuse_actor *inscribe_actor::clone() const
{
    return new inscribe_actor( *this );
}

bool inscribe_actor::item_inscription( item *cut, std::string verb, std::string gerund ) const
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

    const bool hasnote = cut->has_var( "item_note" );
    std::string message = "";
    std::string messageprefix = string_format(hasnote ? _("(To delete, input one '.')\n") : "") +
                                string_format(_("%1$s on the %2$s is: "),
                                        gerund.c_str(), cut->type_name().c_str());
    message = string_input_popup(string_format(_("%s what?"), verb.c_str()), 64,
                                 (hasnote ? cut->get_var( "item_note" ) : message),
                                 messageprefix, "inscribe_item", 128);

    if( !message.empty() ) {
        if( hasnote && message == "." ) {
            cut->erase_var( "item_note" );
            cut->erase_var( "item_note_type" );
            cut->erase_var( "item_note_typez" );
        } else {
            cut->set_var( "item_note", message );
            cut->set_var( "item_note_type", gerund );
        }
    }

    return true;
}


long inscribe_actor::use( player *p, item *it, bool t, point ) const
{
    if( t ) {
        return 0;
    }

    int choice = INT_MAX;
    if( on_terrain && on_items ) {
        uimenu imenu;
        imenu.text = _("Write on what?");
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
        std::string message = string_input_popup( _("Write what?"), 0, "", "", "graffiti");
        if( message.empty() ) {
            return 0;
        } else {
            g->m.set_graffiti( p->posx(), p->posy(), message );
            add_msg( _("You write a message on the ground.") );
            p->moves -= 2 * message.length();
        }
        
        return it->type->charges_to_use();
    }

    int pos = g->inv( _("Inscribe which item?") );
    item *cut = &( p->i_at(pos) );
    // inscribe_item returns false if the action fails or is canceled somehow.
    if( item_inscription( cut, verb, gerund ) ) {
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

extern hp_part use_healing_item(player *p, item *it, int normal_power, 
                                       int head_power, int torso_power, int bleed,
                                       int bite, int infect, bool force);

bool cauterize_actor::cauterize_effect( player *p, item *it, bool force ) const
{
    hp_part hpart = use_healing_item( p, it, -2, -2, -2, 100, 50, 0, force );
    if( hpart != num_hp_parts ) {
        p->add_msg_if_player(m_neutral, _("You cauterize yourself."));
        if (!(p->has_trait("NOPAIN"))) {
            p->mod_pain(15);
            p->add_msg_if_player(m_bad, _("It hurts like hell!"));
        } else {
            p->add_msg_if_player(m_neutral, _("It itches a little."));
        }
        const body_part bp = player::hp_to_bp( hpart );
        if (p->has_effect("bite", bp)) {
            p->add_effect("bite", 2600, bp, true);
        }
        return true;
    }

    return 0;
}

long cauterize_actor::use( player *p, item *it, bool t, point ) const
{
    if( t ) {
        return 0;
    }

    bool has_disease = p->has_effect("bite") || p->has_effect("bleed");
    bool did_cauterize = false;
    if( flame && !p->has_charges("fire", 4) ) {
        p->add_msg_if_player( m_info, _("You need a source of flame (4 charges worth) before you can cauterize yourself.") );
        return 0;
    } else if( !flame && it->charges >= 0 && it->type->charges_to_use() < it->charges ) {
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

void enzlave_actor::load( JsonObject & )
{
}

iuse_actor *enzlave_actor::clone() const
{
    return new enzlave_actor( *this );
}

long enzlave_actor::use( player *p, item *it, bool t, point ) const
{
    if( t ) {
        return 0;
    }

    auto items = g->m.i_at( p->posx(), p->posy() );
    std::vector<const item *> corpses;

    const int cancel = 0;

    for( auto &it : items ) {
        const auto mt = it.get_mtype();
        if( it.is_corpse() && mt->in_species("ZOMBIE") && mt->mat == "flesh" &&
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
    if( p->morale_level() <= (15 * (tolerance_level - p->skillLevel("survival") )) - 150 ) {
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

        int moraleMalus = -50 * (5.0 / (float) p->skillLevel("survival"));
        int maxMalus = -250 * (5.0 / (float)p->skillLevel("survival"));
        int duration = 300 * (5.0 / (float)p->skillLevel("survival"));
        int decayDelay = 30 * (5.0 / (float)p->skillLevel("survival"));

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
    int skills = p->skillLevel("survival") + p->skillLevel("firstaid") + (p->dex_cur / 2);
    skills *= 2;

    int success = rng(0, skills) - rng(0, difficulty);

    const int moves = difficulty * 1200 / p->skillLevel("firstaid");

    p->assign_activity(ACT_MAKE_ZLAVE, moves);
    p->activity.values.push_back(success);
    p->activity.str_values.push_back(corpses[selected_corpse]->display_name());
    return it->type->charges_to_use();
}

bool enzlave_actor::can_use( const player *p, const item*, bool, const point& ) const
{
    return p->get_skill_level( "survival" ) > 1 && p->get_skill_level( "firstaid" ) > 1;
}

void fireweapon_off_actor::load( JsonObject &obj )
{
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

long fireweapon_off_actor::use( player *p, item *it, bool t, point ) const
{
    if( t ) {
        return 0;
    }

    p->moves -= moves;
    if( rng( 0, 10 ) - it->damage > success_chance && 
          it->charges > 0 && !p->is_underwater() ) {
        if( noise > 0 ) {
            sounds::sound( p->posx(), p->posy(), noise, _(success_message.c_str()) );
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

bool fireweapon_off_actor::can_use( const player *p, const item *it, bool, const point& ) const
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
}

iuse_actor *fireweapon_on_actor::clone() const
{
    return new fireweapon_on_actor( *this );
}

long fireweapon_on_actor::use( player *p, item *it, bool t, point ) const
{
    bool extinguish = true;
    if( !t ) {
        p->add_msg_if_player( _(voluntary_extinguish_message.c_str()) );
    } else if( it->charges < it->type->charges_to_use() ) {
        p->add_msg_if_player(m_bad, _(charges_extinguish_message.c_str()) );
    } else if (p->is_underwater()) {
        p->add_msg_if_player( _(water_extinguish_message.c_str()) );
    } else {
        extinguish = false;
    }
    
    if( extinguish ) {
        it->active = false;
        it_tool *tool = dynamic_cast<it_tool *>( it->type );
        if( tool == nullptr ) {
            debugmsg( "Non-tool has fireweapon_on actor" );
            it->make( "none" );
        }

        it->make( tool->revert_to );
    } else if( one_in( noise_chance ) ) {
        if( noise > 0 ) {
            sounds::sound( p->posx(), p->posy(), noise, _(noise_message.c_str()) );
        } else {
            p->add_msg_if_player( _(noise_message.c_str()) );
        }
    }

    return it->type->charges_to_use();
}
