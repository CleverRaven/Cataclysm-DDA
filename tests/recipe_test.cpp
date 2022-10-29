#include <sstream>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "json.h"
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

TEST_CASE( "blueprint_autocalc_only", "[recipe][blueprint]" )
{
    Character &you = get_player_character();

    recipe_id stove_1( "test_base_stove_1" );
    recipe_id stove_2( "test_base_stove_2" );
    recipe_id stove_3( "test_base_stove_3" );

    CHECK( stove_1->time_to_craft( you ) == 1_hours );
    CHECK( stove_1->time_to_craft( you ) == stove_2->time_to_craft( you ) );
    CHECK( stove_3->time_to_craft( you ) == 0_hours );

    CHECK( stove_1->required_all_skills_string() ==
           "<color_white>fabrication (5)</color> and <color_white>mechanics (3)</color>" );
    CHECK( stove_1->required_all_skills_string() == stove_2->required_all_skills_string() );
    CHECK( stove_3->required_all_skills_string() == "<color_cyan>none</color>" );

    CHECK( reqs_to_json_string( stove_1->simple_requirements() ) ==
           R"({"tools":[],"qualities":[[{"id":"SAW_M"}]],)"
           R"("components":[[["metal_tank",1]],[["pipe",1]]]})" );
    CHECK( stove_1->simple_requirements().has_same_requirements_as(
               stove_2->simple_requirements() ) );
    CHECK( reqs_to_json_string( stove_3->simple_requirements() ) ==
           R"({"tools":[],"qualities":[],"components":[]})" );
}

TEST_CASE( "blueprint_autocalc_with_components", "[recipe][blueprint]" )
{
    Character &you = get_player_character();

    recipe_id stove_1( "test_base_stove_plus_pipe_1" );
    recipe_id stove_2( "test_base_stove_plus_pipe_2" );
    recipe_id stove_3( "test_base_stove_plus_pipe_3" );

    CHECK( stove_1->time_to_craft( you ) == 1_hours + 20_minutes );
    CHECK( stove_1->time_to_craft( you ) == stove_2->time_to_craft( you ) );
    CHECK( stove_3->time_to_craft( you ) == 20_minutes );

    CHECK( stove_1->required_all_skills_string() ==
           "<color_white>fabrication (5)</color> and <color_white>mechanics (3)</color>" );
    CHECK( stove_1->required_all_skills_string() == stove_2->required_all_skills_string() );
    CHECK( stove_3->required_all_skills_string() == "<color_cyan>none</color>" );

    CHECK( reqs_to_json_string( stove_1->simple_requirements() ) ==
           R"({"tools":[],"qualities":[[{"id":"CUT","level":2}],[{"id":"SAW_M"}]],)"
           R"("components":[[["pipe",2],["scrap",1]],[["metal_tank",1]],[["pipe",1]]]})" );
    CAPTURE( reqs_to_json_string( stove_2->simple_requirements() ) );
    CHECK( stove_1->simple_requirements().has_same_requirements_as(
               stove_2->simple_requirements() ) );
    CHECK( reqs_to_json_string( stove_3->simple_requirements() ) ==
           R"({"tools":[],"qualities":[[{"id":"CUT","level":2}]],"components":[[["pipe",2],["scrap",1]]]})" );
}
