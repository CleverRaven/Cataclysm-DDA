#pragma once
#ifndef CATA_SRC_IEXAMINE_H
#define CATA_SRC_IEXAMINE_H

#include <algorithm>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "coords_fwd.h"
#include "ret_val.h"
#include "type_id.h"

class Character;
class JsonObject;
class item;
class time_point;
class vpart_reference;
struct itype;

using seed_tuple = std::tuple<itype_id, std::string, int>;

struct iexamine_actor {
    const std::string type;

    explicit iexamine_actor( const std::string &type ) : type( type ) {}

    virtual void load( const JsonObject &, const std::string & ) = 0;
    virtual void call( Character &, const tripoint_bub_ms & ) const = 0;
    virtual void finalize() const = 0;

    virtual std::unique_ptr<iexamine_actor> clone() const = 0;

    virtual ~iexamine_actor() = default;
};

enum fuel_station_fuel_type {
    FUEL_TYPE_NONE,
    FUEL_TYPE_GASOLINE,
    FUEL_TYPE_DIESEL
};

namespace iexamine
{

bool can_hack( Character &you );

bool try_start_hacking( Character &you, const tripoint_bub_ms &examp );

void egg_sack_generic( Character &you, const tripoint_bub_ms &examp, const mtype_id &montype );

void none( Character &you, const tripoint_bub_ms &examp );

bool always_false( const tripoint_bub_ms &examp );
bool false_and_debugmsg( const tripoint_bub_ms &examp );
bool always_true( const tripoint_bub_ms &examp );
bool harvestable_now( const tripoint_bub_ms &examp );

void gaspump( Character &you, const tripoint_bub_ms &examp );
void atm( Character &you, const tripoint_bub_ms &examp );
void vending( Character &you, const tripoint_bub_ms &examp );
void elevator( Character &you, const tripoint_bub_ms &examp );
void genemill( Character &you, const tripoint_bub_ms &examp );
void nanofab( Character &you, const tripoint_bub_ms &examp );
void controls_gate( Character &you, const tripoint_bub_ms &examp );
void cardreader( Character &you, const tripoint_bub_ms &examp );
void cardreader_robofac( Character &you, const tripoint_bub_ms &examp );
void cardreader_foodplace( Character &you, const tripoint_bub_ms &examp );
void intercom( Character &you, const tripoint_bub_ms &examp );
void cvdmachine( Character &you, const tripoint_bub_ms &examp );
void change_appearance( Character &you, const tripoint_bub_ms &examp );
void rubble( Character &you, const tripoint_bub_ms &examp );
void chainfence( Character &you, const tripoint_bub_ms &examp );
void bars( Character &you, const tripoint_bub_ms &examp );
void deployed_furniture( Character &you, const tripoint_bub_ms &pos );
void portable_structure( Character &you, const tripoint_bub_ms &examp );
void pit( Character &you, const tripoint_bub_ms &examp );
void pit_covered( Character &you, const tripoint_bub_ms &examp );
void safe( Character &you, const tripoint_bub_ms &examp );
void gunsafe_el( Character &you, const tripoint_bub_ms &examp );
void harvest_furn_nectar( Character &you, const tripoint_bub_ms &examp );
void harvest_furn( Character &you, const tripoint_bub_ms &examp );
void harvest_ter_nectar( Character &you, const tripoint_bub_ms &examp );
void harvest_ter( Character &you, const tripoint_bub_ms &examp );
void harvested_plant( Character &you, const tripoint_bub_ms &examp );
void locked_object( Character &you, const tripoint_bub_ms &examp );
void locked_object_pickable( Character &you, const tripoint_bub_ms &examp );
void bulletin_board( Character &you, const tripoint_bub_ms &examp );
void pedestal_wyrm( Character &you, const tripoint_bub_ms &examp );
void pedestal_temple( Character &you, const tripoint_bub_ms &examp );
void door_peephole( Character &you, const tripoint_bub_ms &examp );
void fswitch( Character &you, const tripoint_bub_ms &examp );
void flower_tulip( Character &you, const tripoint_bub_ms &examp );
void flower_spurge( Character &you, const tripoint_bub_ms &examp );
void flower_poppy( Character &you, const tripoint_bub_ms &examp );
void flower_cactus( Character &you, const tripoint_bub_ms &examp );
void flower_bluebell( Character &you, const tripoint_bub_ms &examp );
void flower_dahlia( Character &you, const tripoint_bub_ms &examp );
void flower_marloss( Character &you, const tripoint_bub_ms &examp );
void fungus( Character &you, const tripoint_bub_ms &examp );
void dirtmound( Character &you, const tripoint_bub_ms &examp );
void aggie_plant( Character &you, const tripoint_bub_ms &examp );
void tree_hickory( Character &you, const tripoint_bub_ms &examp );
void tree_maple( Character &you, const tripoint_bub_ms &examp );
void tree_maple_tapped( Character &you, const tripoint_bub_ms &examp );
void shrub_marloss( Character &you, const tripoint_bub_ms &examp );
void tree_marloss( Character &you, const tripoint_bub_ms &examp );
void shrub_wildveggies( Character &you, const tripoint_bub_ms &examp );
void part_con( Character &you, const tripoint_bub_ms &examp );
void water_source( Character &, const tripoint_bub_ms &examp );
void finite_water_source( Character &, const tripoint_bub_ms &examp );
void kiln_empty( Character &you, const tripoint_bub_ms &examp );
bool kiln_prep( Character &you, const tripoint_bub_ms &examp );
bool kiln_fire( Character &you, const tripoint_bub_ms &examp );
void kiln_full( Character &you, const tripoint_bub_ms &examp );
void stook_empty( Character &, const tripoint_bub_ms &examp );
void stook_full( Character &, const tripoint_bub_ms &examp );
void arcfurnace_empty( Character &you, const tripoint_bub_ms &examp );
void arcfurnace_full( Character &you, const tripoint_bub_ms &examp );
void autoclave_empty( Character &you, const tripoint_bub_ms &examp );
void autoclave_full( Character &, const tripoint_bub_ms &examp );
void fireplace( Character &you, const tripoint_bub_ms &examp );
void fvat_empty( Character &you, const tripoint_bub_ms &examp );
void fvat_full( Character &you, const tripoint_bub_ms &examp );
void compost_empty( Character &you, const tripoint_bub_ms &examp );
void compost_full( Character &you, const tripoint_bub_ms &examp );
void keg( Character &you, const tripoint_bub_ms &examp );
void reload_furniture( Character &you, const tripoint_bub_ms &examp );
void curtains( Character &you, const tripoint_bub_ms &examp );
void sign( Character &you, const tripoint_bub_ms &examp );
void pay_gas( Character &you, const tripoint_bub_ms &examp );
void ledge( Character &you, const tripoint_bub_ms &examp );
void autodoc( Character &you, const tripoint_bub_ms &examp );
void attunement_altar( Character &you, const tripoint_bub_ms &examp );
void translocator( Character &you, const tripoint_bub_ms &examp );
void on_smoke_out( const tripoint_bub_ms &examp,
                   const time_point &start_time ); //activates end of smoking effects
void mill_finalize( Character &, const tripoint_bub_ms &examp );
void quern_examine( Character &you, const tripoint_bub_ms &examp );
void smoker_options( Character &you, const tripoint_bub_ms &examp );
bool smoker_prep( Character &you, const tripoint_bub_ms &examp );
bool smoker_fire( Character &you, const tripoint_bub_ms &examp );
void open_safe( Character &you, const tripoint_bub_ms &examp );
void workbench( Character &you, const tripoint_bub_ms &examp );
void workbench_internal( Character &you, const tripoint_bub_ms &examp,
                         const std::optional<vpart_reference> &part );
void workout( Character &you, const tripoint_bub_ms &examp );
void invalid( Character &you, const tripoint_bub_ms &examp );

bool pour_into_keg( const tripoint_bub_ms &pos, item &liquid, bool silent );
std::optional<tripoint_bub_ms> getGasPumpByNumber( const tripoint_bub_ms &p, int number );
bool toPumpFuel( const tripoint_bub_ms &src, const tripoint_bub_ms &dst, int units );
std::optional<tripoint_bub_ms> getNearFilledGasTank( const tripoint_bub_ms &center, int &fuel_units,
        fuel_station_fuel_type &fuel_type );

bool has_keg( const tripoint_bub_ms &pos );

std::list<item> get_harvest_items( const itype &type, int plant_count,
                                   int seed_count, bool byproducts );

// Planting functions
std::vector<seed_tuple> get_seed_entries( const std::vector<item *> &seed_inv );
int query_seed( const std::vector<seed_tuple> &seed_entries );
void plant_seed( Character &you, const tripoint_bub_ms &examp, const itype_id &seed_id );
void clear_overgrown( Character &you, const tripoint_bub_ms &examp );
void harvest_plant_ex( Character &you, const tripoint_bub_ms &examp );
void harvest_plant( Character &you, const tripoint_bub_ms &examp, bool from_activity );
void fertilize_plant( Character &you, const tripoint_bub_ms &tile, const itype_id &fertilizer );
itype_id choose_fertilizer( Character &you, const std::string &pname, bool ask_player );
ret_val<void> can_fertilize( Character &you, const tripoint_bub_ms &tile,
                             const itype_id &fertilizer );

// Skill training common functions
void practice_survival_while_foraging( Character &who );

} // namespace iexamine

namespace iexamine_helper
{
bool drink_nectar( Character &you );
void handle_harvest( Character &you, const itype_id &itemid, bool force_drop );
} // namespace iexamine_helper

using iexamine_examine_function = void ( * )( Character &, const tripoint_bub_ms & );
using iexamine_can_examine_function = bool ( * )( const tripoint_bub_ms & );
struct iexamine_functions {
    iexamine_can_examine_function can_examine;
    iexamine_examine_function examine;
};

iexamine_functions iexamine_functions_from_string( const std::string &function_name );

#endif // CATA_SRC_IEXAMINE_H
