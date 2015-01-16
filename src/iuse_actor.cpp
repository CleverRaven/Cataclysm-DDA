#include "iuse_actor.h"
#include "item.h"
#include "game.h"
#include "monster.h"
#include "overmapbuffer.h"
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
            g->sound(pos.x, pos.y, sound_volume, sound_msg);
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

    vehicle *veh = g->m.add_vehicle(vehicle_name, p->posx, p->posy, 0, 0, 0, false);
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
            g->m.add_field(p->posx + int(rng(-2, 2)), p->posy + int(rng(-2, 2)), fid, field->second);
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
        for( int x = p->posx - 1; x <= p->posx + 1; x++ ) {
            for( int y = p->posy - 1; y <= p->posy + 1; y++ ) {
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
    const int damfac = 5 - std::max<int>( 0, it->damage ); // 5 (no damage) ... 1 (max damage)
    // One hp at least, everything else would be unfair (happens only to monster with *very* low hp),
    newmon.hp = std::max( 1, newmon.hp * damfac / 5 );
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
    if( dirx == p->posx && diry == p->posy ) {
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
        g->sound( p->posx, p->posy, 40, _( "An alarm sounds!" ) );
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
