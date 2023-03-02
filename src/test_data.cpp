#include "test_data.h"

#include "flexbuffer_json.h"

std::set<itype_id> test_data::known_bad;
std::map<vproto_id, std::vector<double>> test_data::drag_data;
std::map<vproto_id, efficiency_data> test_data::eff_data;
std::map<itype_id, double> test_data::expected_dps;

void efficiency_data::deserialize( const JsonObject &jo )
{
    jo.read( "forward", forward );
    jo.read( "reverse", reverse );
}

void test_data::load( const JsonObject &jo )
{
    // It's probably not necessary, but these are set up to
    // extend existing data instead of overwrite it.
    if( jo.has_array( "known_bad" ) ) {
        std::set<itype_id> new_known_bad;
        jo.read( "known_bad", new_known_bad );
        known_bad.insert( new_known_bad.begin(), new_known_bad.end() );
    }

    if( jo.has_object( "drag_data" ) ) {
        std::map<vproto_id, std::vector<double>> new_drag_data;
        jo.read( "drag_data", new_drag_data );
        drag_data.insert( new_drag_data.begin(), new_drag_data.end() );
    }

    if( jo.has_object( "efficiency_data" ) ) {
        std::map<vproto_id, efficiency_data> new_efficiency_data;
        jo.read( "efficiency_data", new_efficiency_data );
        eff_data.insert( new_efficiency_data.begin(), new_efficiency_data.end() );
    }

    if( jo.has_object( "expected_dps" ) ) {
        std::map<itype_id, double> new_expected_dps;
        jo.read( "expected_dps", new_expected_dps );
        expected_dps.insert( new_expected_dps.begin(), new_expected_dps.end() );
    }
}
