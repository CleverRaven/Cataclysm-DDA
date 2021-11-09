#pragma once
#ifndef CATA_SRC_IEXAMINE_H
#define CATA_SRC_IEXAMINE_H

#include <iosfwd>
#include <list>
#include <memory>
#include <tuple>
#include <vector>

#include "optional.h"
#include "ret_val.h"
#include "type_id.h"

class item;
class JsonObject;
class Character;
class time_point;
class vpart_reference;
struct itype;
struct tripoint;

using seed_tuple = std::tuple<itype_id, std::string, int>;

struct iexamine_actor {
    const std::string type;

    explicit iexamine_actor( const std::string &type ) : type( type ) {}

    virtual void load( const JsonObject & ) = 0;
    virtual void call( Character &, const tripoint & ) const = 0;
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

bool try_start_hacking( Character &you, const tripoint &examp, bool interactive );

void egg_sack_generic( Character &you, const tripoint &examp, const mtype_id &montype,
                       bool interactive );

void none( Character &you, const tripoint &examp, bool interactive );

bool always_false( const tripoint &examp );
bool false_and_debugmsg( const tripoint &examp );
bool always_true( const tripoint &examp );
bool harvestable_now( const tripoint &examp );

void gaspump( Character &you, const tripoint &examp, bool interactive );
void atm( Character &you, const tripoint &examp, bool interactive );
void vending( Character &you, const tripoint &examp, bool interactive );
void toilet( Character &, const tripoint &examp, bool interactive );
void elevator( Character &you, const tripoint &examp, bool interactive );
void nanofab( Character &you, const tripoint &examp, bool interactive );
void controls_gate( Character &you, const tripoint &examp, bool interactive );
void cardreader_robofac( Character &you, const tripoint &examp, bool interactive );
void cardreader_foodplace( Character &you, const tripoint &examp, bool interactive );
void intercom( Character &you, const tripoint &examp, bool interactive );
void cvdmachine( Character &you, const tripoint &examp, bool interactive );
void change_appearance( Character &you, const tripoint &examp, bool interactive );
void rubble( Character &you, const tripoint &examp, bool interactive );
void chainfence( Character &you, const tripoint &examp, bool interactive );
void bars( Character &you, const tripoint &examp, bool interactive );
void deployed_furniture( Character &you, const tripoint &pos, bool interactive );
void portable_structure( Character &you, const tripoint &examp, bool interactive );
void pit( Character &you, const tripoint &examp, bool interactive );
void pit_covered( Character &you, const tripoint &examp, bool interactive );
void slot_machine( Character &you, const tripoint &examp, bool interactive );
void safe( Character &you, const tripoint &examp, bool interactive );
void gunsafe_el( Character &you, const tripoint &examp, bool interactive );
void harvest_furn_nectar( Character &you, const tripoint &examp, bool interactive );
void harvest_furn( Character &you, const tripoint &examp, bool interactive );
void harvest_ter_nectar( Character &you, const tripoint &examp, bool interactive );
void harvest_ter( Character &you, const tripoint &examp, bool interactive );
void harvested_plant( Character &you, const tripoint &examp, bool interactive );
void locked_object( Character &you, const tripoint &examp, bool interactive );
void locked_object_pickable( Character &you, const tripoint &examp, bool interactive );
void bulletin_board( Character &you, const tripoint &examp, bool interactive );
void pedestal_wyrm( Character &you, const tripoint &examp, bool interactive );
void pedestal_temple( Character &you, const tripoint &examp, bool interactive );
void door_peephole( Character &you, const tripoint &examp, bool interactive );
void fswitch( Character &you, const tripoint &examp, bool interactive );
void flower_poppy( Character &you, const tripoint &examp, bool interactive );
void flower_cactus( Character &you, const tripoint &examp, bool interactive );
void flower_dahlia( Character &you, const tripoint &examp, bool interactive );
void flower_marloss( Character &you, const tripoint &examp, bool interactive );
void egg_sackbw( Character &you, const tripoint &examp, bool interactive );
void egg_sackcs( Character &you, const tripoint &examp, bool interactive );
void egg_sackws( Character &you, const tripoint &examp, bool interactive );
void fungus( Character &you, const tripoint &examp, bool interactive );
void dirtmound( Character &you, const tripoint &examp, bool interactive );
void aggie_plant( Character &you, const tripoint &examp, bool interactive );
void tree_hickory( Character &you, const tripoint &examp, bool interactive );
void tree_maple( Character &you, const tripoint &examp, bool interactive );
void tree_maple_tapped( Character &you, const tripoint &examp, bool interactive );
void shrub_marloss( Character &you, const tripoint &examp, bool interactive );
void tree_marloss( Character &you, const tripoint &examp, bool interactive );
void shrub_wildveggies( Character &you, const tripoint &examp, bool interactive );
void water_source( Character &, const tripoint &examp, bool interactive );
void clean_water_source( Character &, const tripoint &examp, bool interactive );
void kiln_empty( Character &you, const tripoint &examp, bool interactive );
void kiln_full( Character &you, const tripoint &examp, bool interactive );
void arcfurnace_empty( Character &you, const tripoint &examp, bool interactive );
void arcfurnace_full( Character &you, const tripoint &examp, bool interactive );
void autoclave_empty( Character &you, const tripoint &examp, bool interactive );
void autoclave_full( Character &, const tripoint &examp, bool interactive );
void fireplace( Character &you, const tripoint &examp, bool interactive );
void fvat_empty( Character &you, const tripoint &examp, bool interactive );
void fvat_full( Character &you, const tripoint &examp, bool interactive );
void keg( Character &you, const tripoint &examp, bool interactive );
void reload_furniture( Character &you, const tripoint &examp, bool interactive );
void curtains( Character &you, const tripoint &examp, bool interactive );
void sign( Character &you, const tripoint &examp, bool interactive );
void pay_gas( Character &you, const tripoint &examp, bool interactive );
void ledge( Character &you, const tripoint &examp, bool interactive );
void autodoc( Character &you, const tripoint &examp, bool interactive );
void attunement_altar( Character &you, const tripoint &examp, bool interactive );
void translocator( Character &you, const tripoint &examp, bool interactive );
void on_smoke_out( const tripoint &examp,
                   const time_point &start_time ); //activates end of smoking effects
void mill_finalize( Character &, const tripoint &examp, const time_point &start_time );
void quern_examine( Character &you, const tripoint &examp, bool interactive );
void smoker_options( Character &you, const tripoint &examp, bool interactive );
void open_safe( Character &you, const tripoint &examp, bool interactive );
void workbench( Character &you, const tripoint &examp, bool interactive );
void workbench_internal( Character &you, const tripoint &examp,
                         const cata::optional<vpart_reference> &part );
void workout( Character &you, const tripoint &examp, bool interactive );
void invalid( Character &you, const tripoint &examp, bool interactive );

bool pour_into_keg( const tripoint &pos, item &liquid );
cata::optional<tripoint> getGasPumpByNumber( const tripoint &p, int number );
bool toPumpFuel( const tripoint &src, const tripoint &dst, int units );
cata::optional<tripoint> getNearFilledGasTank( const tripoint &center, int &fuel_units,
        fuel_station_fuel_type &fuel_type );

bool has_keg( const tripoint &pos );

std::list<item> get_harvest_items( const itype &type, int plant_count,
                                   int seed_count, bool byproducts );

// Planting functions
std::vector<seed_tuple> get_seed_entries( const std::vector<item *> &seed_inv );
int query_seed( const std::vector<seed_tuple> &seed_entries );
void plant_seed( Character &you, const tripoint &examp, const itype_id &seed_id );
void harvest_plant_ex( Character &you, const tripoint &examp, bool interactive );
void harvest_plant( Character &you, const tripoint &examp, bool from_activity, bool interactive );
void fertilize_plant( Character &you, const tripoint &tile, const itype_id &fertilizer );
itype_id choose_fertilizer( Character &you, const std::string &pname, bool ask_player,
                            bool rand_fert = false );
ret_val<bool> can_fertilize( Character &you, const tripoint &tile, const itype_id &fertilizer );

// Skill training common functions
void practice_survival_while_foraging( Character *you );

} // namespace iexamine

namespace iexamine_helper
{
bool drink_nectar( Character &you );
void handle_harvest( Character &you, const std::string &itemid, bool force_drop );
} // namespace iexamine_helper

using iexamine_examine_function = void ( * )( Character &, const tripoint &, bool );
using iexamine_can_examine_function = bool ( * )( const tripoint & );
struct iexamine_functions {
    iexamine_can_examine_function can_examine;
    iexamine_examine_function examine;
};

iexamine_functions iexamine_functions_from_string( const std::string &function_name );

#endif // CATA_SRC_IEXAMINE_H
