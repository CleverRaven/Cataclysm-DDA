#pragma once
#ifndef CATA_SRC_MATERIAL_H
#define CATA_SRC_MATERIAL_H

#include <cstddef>
#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "fire.h"
#include "translations.h"
#include "type_id.h"

class material_type;

enum class damage_type : int;
class JsonObject;

using mat_burn_products = std::vector<std::pair<itype_id, float>>;
using material_list = std::vector<material_type>;
using material_id_list = std::vector<material_id>;

// values for how breathable a material is
enum class breathability_rating : int {
    IMPERMEABLE = 0,
    POOR,
    AVERAGE,
    GOOD,
    MOISTURE_WICKING,
    SECOND_SKIN,
    last
};

template<>
struct enum_traits<breathability_rating> {
    static constexpr breathability_rating last = breathability_rating::last;
};

struct fuel_explosion_data {
    int explosion_chance_hot = 0;
    int explosion_chance_cold = 0;
    float explosion_factor = 0.0f;
    bool fiery_explosion = false;
    float fuel_size_factor = 0.0f;

    bool is_empty() const;

    bool was_loaded = false;
    void load( const JsonObject &jsobj );
    void deserialize( const JsonObject &jo );
};

struct fuel_data {
    public:
        /** Energy of the fuel per litre */
        units::energy energy = 0_J;
        fuel_explosion_data explosion_data;
        std::string pump_terrain = "t_null";
        bool is_perpetual_fuel = false;

        bool was_loaded = false;
        void load( const JsonObject &jsobj );
        void deserialize( const JsonObject &jo );
};

class material_type
{
    public:
        material_id id;
        std::vector<std::pair<material_id, mod_id>> src;
        bool was_loaded = false;

    private:
        translation _name;
        std::optional<itype_id> _salvaged_into; // this material turns into this item when salvaged
        itype_id _repaired_with = itype_id( "null" ); // this material can be repaired with this item
        std::map<damage_type, float> _resistances;   // negative integers means susceptibility
        int _chip_resist = 0;                         // Resistance to physical damage of the item itself
        float _density = 1;                             // relative to "powder", which is 1
        // ability of a fabric to allow moisture vapor to be transmitted through the material
        breathability_rating _breathability = breathability_rating::IMPERMEABLE;
        // How resistant this material is to wind as a percentage - 0 to 100
        std::optional<int> _wind_resist;
        float _specific_heat_liquid = 4.186f;
        float _specific_heat_solid = 2.108f;
        float _latent_heat = 334.0f;
        float _freeze_point = 0; // Celsius
        bool _edible = false;
        bool _rotting = false;
        bool _soft = false;
        bool _uncomfortable = false;
        bool _conductive = false; // If this material conducts electricity

        // the thickness that sheets of this material come in, anything that uses it should be a multiple of this
        float _sheet_thickness = 0.0f;

        translation _bash_dmg_verb;
        translation _cut_dmg_verb;
        std::vector<translation> _dmg_adj;

        std::map<vitamin_id, double> _vitamins;

        std::vector<mat_burn_data> _burn_data;

        fuel_data fuel;

        //Burn products defined in JSON as "burn_products": [ [ "X", float efficiency ], [ "Y", float efficiency ] ]
        mat_burn_products _burn_products;

    public:
        material_type();

        void load( const JsonObject &jsobj, const std::string &src );
        void check() const;

        material_id ident() const;
        std::string name() const;
        /**
         * @returns An empty optional if this material can not be
         * salvaged into any items (e.g. for powder, liquids).
         * Or a valid id of the item type that this can be salvaged
         * into (e.g. clothes made of material leather can be salvaged
         * into leather patches).
         */
        std::optional<itype_id> salvaged_into() const;
        itype_id repaired_with() const;
        float resist( damage_type dmg_type ) const;
        std::string bash_dmg_verb() const;
        std::string cut_dmg_verb() const;
        std::string dmg_adj( int damage ) const;
        int chip_resist() const;
        float specific_heat_liquid() const;
        float specific_heat_solid() const;
        float latent_heat() const;
        float freeze_point() const;
        float density() const;

        bool is_conductive() const;

        bool is_valid_thickness( float thickness ) const;
        float thickness_multiple() const;

        // converts from the breathability enum to a fixed integer value from 0-100
        static int breathability_to_rating( breathability_rating breathability );
        int breathability() const;
        std::optional<int> wind_resist() const;
        bool edible() const;
        bool rotting() const;
        bool soft() const;
        bool uncomfortable() const;

        double vitamin( const vitamin_id &id ) const {
            const auto iter = _vitamins.find( id );
            return iter != _vitamins.end() ? iter->second : 0;
        }

        fuel_data get_fuel_data() const;

        const mat_burn_data &burn_data( size_t intensity ) const;
        const mat_burn_products &burn_products() const;
};

namespace materials
{

void load( const JsonObject &jo, const std::string &src );
void check();
void reset();

material_list get_all();
std::set<material_id> get_rotting();

} // namespace materials

#endif // CATA_SRC_MATERIAL_H
