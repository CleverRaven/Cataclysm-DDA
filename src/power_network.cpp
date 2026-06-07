#include "power_network.h"

#include <utility>

#include "flexbuffer_json.h"
#include "json.h"
#include "units.h"
#include "vehicle.h"

void power_network_node::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "pos", position );
    json.member( "solar", rated_solar );
    json.member( "wind", rated_wind );
    json.member( "water", rated_water );
    json.member( "consumption", rated_consumption );
    json.member( "battery_cap", battery_capacity_kJ );
    json.end_object();
}

void power_network_node::deserialize( const JsonObject &data )
{
    data.read( "pos", position );
    data.read( "solar", rated_solar );
    data.read( "wind", rated_wind );
    data.read( "water", rated_water );
    data.read( "consumption", rated_consumption );
    data.read( "battery_cap", battery_capacity_kJ );
}

void power_network::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "last_resolved", last_resolved );
    json.member( "root_pos", root_position );
    json.member( "nodes", nodes );
    json.end_object();
}

void power_network::deserialize( const JsonObject &data )
{
    data.read( "id", id );
    data.read( "last_resolved", last_resolved );
    data.read( "root_pos", root_position );
    data.read( "nodes", nodes );
}

void power_network_manager::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "next_id", next_id_ );
    json.member( "networks" );
    json.start_array();
    for( const auto &[net_id, net] : networks_ ) {
        net.serialize( json );
    }
    json.end_array();
    json.end_object();
}

void power_network_manager::deserialize( const JsonObject &data )
{
    clear();
    data.read( "next_id", next_id_ );
    for( JsonObject net_json : data.get_array( "networks" ) ) {
        power_network net;
        net.deserialize( net_json );
        networks_[net.id] = std::move( net );
    }
}

void power_network_manager::clear()
{
    networks_.clear();
    next_id_ = 1;
}

static power_network_node make_node_from_vehicle( const vehicle &veh )
{
    power_network_node node;
    node.position = veh.pos_abs();
    node.rated_solar = veh.rated_solar_epower();
    node.rated_wind = veh.rated_wind_epower();
    node.rated_water = veh.rated_water_epower();
    node.rated_consumption = veh.total_accessory_epower();
    auto [remaining, capacity] = veh.battery_power_level();
    node.battery_capacity_kJ = capacity;

    return node;
}

void power_network_manager::begin_rebuild()
{
    // Build position-to-old-network-id lookup from current networks
    old_pos_to_id_.clear();
    for( const auto &[net_id, net] : networks_ ) {
        for( const power_network_node &nd : net.nodes ) {
            old_pos_to_id_[nd.position] = net_id;
        }
    }
    claimed_ids_.clear();
    pending_networks_.clear();
    rebuild_next_id_ = next_id_;
}

void power_network_manager::add_grid( vehicle *root,
                                      const std::map<vehicle *, float> &grid )
{
    power_network net;
    net.root_position = root->pos_abs();
    net.last_resolved = calendar::turn;

    // Count overlaps with old networks to find best match for id stability
    std::unordered_map<int, int> overlap_counts;
    for( const auto &[grid_veh, loss] : grid ) {
        net.nodes.push_back( make_node_from_vehicle( *grid_veh ) );

        auto it = old_pos_to_id_.find( grid_veh->pos_abs() );
        if( it != old_pos_to_id_.end() ) {
            overlap_counts[it->second]++;
        }
    }

    // Find the old id with the most overlap that hasn't been claimed yet
    int best_old_id = 0;
    int best_count = 0;
    for( const auto &[prev_id, count] : overlap_counts ) {
        if( count > best_count && !claimed_ids_.count( prev_id ) ) {
            best_count = count;
            best_old_id = prev_id;
        }
    }

    if( best_old_id != 0 ) {
        net.id = best_old_id;
        claimed_ids_.insert( best_old_id );
    } else {
        net.id = rebuild_next_id_++;
    }

    pending_networks_[net.id] = std::move( net );
}

void power_network_manager::finish_rebuild()
{
    networks_ = std::move( pending_networks_ );
    next_id_ = rebuild_next_id_;

    // Clean up temporary state
    old_pos_to_id_.clear();
    claimed_ids_.clear();
    pending_networks_.clear();
}

const power_network *power_network_manager::find_network_at(
    const tripoint_abs_ms &pos ) const
{
    for( const auto &[net_id, net] : networks_ ) {
        for( const power_network_node &node : net.nodes ) {
            if( node.position == pos ) {
                return &net;
            }
        }
    }
    return nullptr;
}
