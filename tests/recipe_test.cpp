#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "build_reqs.h"
#include "calendar.h"
#include "cata_catch.h"
#include "json.h"
#include "mapgendata.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

static std::string reqs_to_json_string( const requirement_data &reqs )
{
    std::ostringstream os;
    JsonOut jsout( os );
    reqs.dump( jsout );
    return os.str();
}

static const build_reqs &get_default_build_reqs( const recipe_id rec )
{
    const std::unordered_map<mapgen_arguments, build_reqs> &all_reqs =
        rec->blueprint_build_reqs().reqs_by_parameters;

    REQUIRE( all_reqs.size() == 1 );
    auto it = all_reqs.find( {} );
    REQUIRE( it != all_reqs.end() );
    return it->second;
}

TEST_CASE( "blueprint_autocalc_only", "[recipe][blueprint]" )
{
    recipe_id stove_1( "test_base_stove_1" );
    recipe_id stove_2( "test_base_stove_2" );
    recipe_id stove_3( "test_base_stove_3" );

    const build_reqs &stove_1_reqs = get_default_build_reqs( stove_1 );
    const build_reqs &stove_2_reqs = get_default_build_reqs( stove_2 );
    const build_reqs &stove_3_reqs = get_default_build_reqs( stove_3 );

    CHECK( stove_1_reqs.time == to_moves<int>( 1_hours ) );
    CHECK( stove_1_reqs.time == stove_2_reqs.time );
    CHECK( stove_3_reqs.time == 0 );

    CHECK( stove_1->required_all_skills_string( stove_1_reqs.skills ) ==
           "<color_white>fabrication (5)</color> and <color_white>mechanics (3)</color>" );
    CHECK( stove_1->required_all_skills_string( stove_1_reqs.skills ) ==
           stove_2->required_all_skills_string( stove_2_reqs.skills ) );
    CHECK( stove_3->required_all_skills_string( stove_3_reqs.skills ) ==
           "<color_cyan>none</color>" );

    CHECK( reqs_to_json_string( stove_1_reqs.consolidated_reqs ) ==
           R"({"tools":[],"qualities":[[{"id":"SAW_M"}]],)"
           R"("components":[[["metal_tank",1]],[["pipe",1]]]})" );
    CHECK( stove_1_reqs.consolidated_reqs.has_same_requirements_as(
               stove_2_reqs.consolidated_reqs ) );
    CHECK( reqs_to_json_string( stove_3_reqs.consolidated_reqs ) ==
           R"({"tools":[],"qualities":[],"components":[]})" );
}

TEST_CASE( "blueprint_autocalc_with_components", "[recipe][blueprint]" )
{
    recipe_id stove_1( "test_base_stove_plus_pipe_1" );
    recipe_id stove_2( "test_base_stove_plus_pipe_2" );
    recipe_id stove_3( "test_base_stove_plus_pipe_3" );
    recipe_id stove_4( "test_base_stove_plus_pipe_4" );

    const build_reqs &stove_1_reqs = get_default_build_reqs( stove_1 );
    const build_reqs &stove_2_reqs = get_default_build_reqs( stove_2 );
    const build_reqs &stove_3_reqs = get_default_build_reqs( stove_3 );
    const build_reqs &stove_4_reqs = get_default_build_reqs( stove_4 );

    CHECK( stove_1_reqs.time == to_moves<int>( 1_hours + 20_minutes ) );
    CHECK( stove_1_reqs.time == stove_2_reqs.time );
    CHECK( stove_3_reqs.time == to_moves<int>( 20_minutes ) );
    CHECK( stove_4_reqs.time == stove_3_reqs.time );

    CHECK( stove_1->required_all_skills_string( stove_1_reqs.skills ) ==
           "<color_white>fabrication (5)</color> and <color_white>mechanics (3)</color>" );
    CHECK( stove_1->required_all_skills_string( stove_1_reqs.skills ) ==
           stove_2->required_all_skills_string( stove_2_reqs.skills ) );
    CHECK( stove_3->required_all_skills_string( stove_3_reqs.skills ) ==
           "<color_cyan>none</color>" );
    CHECK( stove_3->required_all_skills_string( stove_3_reqs.skills ) ==
           stove_4->required_all_skills_string( stove_4_reqs.skills ) );

    CHECK( reqs_to_json_string( stove_1_reqs.consolidated_reqs ) ==
           R"({"tools":[],"qualities":[[{"id":"CUT","level":2}],[{"id":"SAW_M"}]],)"
           R"("components":[[["metal_tank",1]],[["pipe",3]]]})" );
    CAPTURE( reqs_to_json_string( stove_2->simple_requirements() ) );
    CHECK( stove_1_reqs.consolidated_reqs.has_same_requirements_as(
               stove_2_reqs.consolidated_reqs ) );
    CHECK( reqs_to_json_string( stove_3_reqs.consolidated_reqs ) ==
           R"({"tools":[],"qualities":[[{"id":"CUT","level":2}]],"components":[[["pipe",2],["scrap",1]]]})" );
    CHECK( stove_3_reqs.consolidated_reqs.has_same_requirements_as(
               stove_4_reqs.consolidated_reqs ) );
}
