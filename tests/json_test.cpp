#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "cached_options.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cata_catch.h"
#include "colony.h"
#include "damage.h"
#include "debug.h"
#include "enum_bitset.h"
#include "item.h"
#include "json.h"
#include "json_loader.h"
#include "magic.h"
#include "mutation.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"

static const damage_type_id damage_pure( "pure" );

static const field_type_str_id field_test_field( "test_field" );

static const flag_id json_flag_DIRTY( "DIRTY" );

static const itype_id itype_test_rag( "test_rag" );

static const mtype_id foo( "foo" );
static const mtype_id mon_test( "mon_test" );

static const requirement_id requirement_data_test_components( "test_components" );

static const skill_id skill_not_spellcraft( "not_spellcraft" );

static const spell_id spell_test_fake_spell( "test_fake_spell" );
static const spell_id spell_test_spell_json( "test_spell_json" );

static const trait_id trait_test_trait( "test_trait" );

template<typename T>
void test_serialization( const T &val, const std::string &s )
{
    CAPTURE( val );
    {
        INFO( "test_serialization" );
        std::ostringstream os;
        JsonOut jsout( os );
        jsout.write( val );
        CHECK( os.str() == s );
    }
    {
        INFO( "test_deserialization" );
        JsonValue jsin = json_loader::from_string( s );
        T read_val;
        CHECK( jsin.read( read_val ) );
        CHECK( val == read_val );
    }
}

TEST_CASE( "avoid_serializing_default_values", "[json]" )
{
    std::ostringstream os;
    JsonOut jsout( os );
    const std::string foo = "foo";
    const std::string bar = "bar";
    jsout.member( foo, foo, foo );
    jsout.member( bar, foo, bar );
    REQUIRE( os.str() == "\"bar\":\"foo\"" );
}

TEST_CASE( "spell_type_handles_all_members", "[json]" )
{
    const spell_type &test_spell = spell_test_spell_json.obj();

    SECTION( "spell_type loads proper values" ) {
        fake_spell fake_additional_effect;
        fake_additional_effect.id = spell_test_fake_spell;
        const std::vector<fake_spell> test_fake_spell_vec{ fake_additional_effect };
        const std::map<std::string, int> test_learn_spell{ { fake_additional_effect.id.c_str(), 1 } };
        const std::set<mtype_id> test_fake_mon{ mon_test };

        CHECK( test_spell.id == spell_test_spell_json );
        CHECK( test_spell.name == to_translation( "test spell" ) );
        CHECK( test_spell.description ==
               to_translation( "a spell to make sure the json deserialization and serialization is working properly" ) );
        CHECK( test_spell.effect_name == "attack" );
        CHECK( test_spell.spell_area == spell_shape::blast );
        CHECK( test_spell.valid_targets.test( spell_target::none ) );
        CHECK( test_spell.effect_str == "string" );
        CHECK( test_spell.skill == skill_not_spellcraft );
        CHECK( test_spell.spell_components == requirement_data_test_components );
        CHECK( test_spell.message == to_translation( "test message" ) );
        CHECK( test_spell.sound_description == to_translation( "test_description" ) );
        CHECK( test_spell.sound_type == sounds::sound_t::weather );
        CHECK( test_spell.sound_ambient == true );
        CHECK( test_spell.sound_id == "test_sound" );
        CHECK( test_spell.sound_variant == "not_default" );
        CHECK( test_spell.targeted_monster_ids == test_fake_mon );
        CHECK( test_spell.additional_spells == test_fake_spell_vec );
        CHECK( test_spell.affected_bps.test( body_part_head ) );
        CHECK( test_spell.spell_tags.test( spell_flag::CONCENTRATE ) );
        CHECK( test_spell.field );
        CHECK( test_spell.field->id() == field_test_field );
        CHECK( test_spell.field_chance.min.dbl_val.value() == 2 );
        CHECK( test_spell.max_field_intensity.min.dbl_val.value() == 2 );
        CHECK( test_spell.min_field_intensity.min.dbl_val.value() == 2 );
        CHECK( test_spell.field_intensity_increment.min.dbl_val.value() == 1 );
        CHECK( test_spell.field_intensity_variance.min.dbl_val.value() == 1 );
        CHECK( test_spell.min_damage.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_damage.min.dbl_val.value() == 1 );
        CHECK( test_spell.damage_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.min_range.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_range.min.dbl_val.value() == 1 );
        CHECK( test_spell.range_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.min_aoe.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_aoe.min.dbl_val.value() == 1 );
        CHECK( test_spell.aoe_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.min_dot.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_dot.min.dbl_val.value() == 1 );
        CHECK( test_spell.dot_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.min_duration.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_duration.min.dbl_val.value() == 1 );
        CHECK( test_spell.duration_increment.min.dbl_val.value() == 1 );
        CHECK( test_spell.min_pierce.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_pierce.min.dbl_val.value() == 1 );
        CHECK( test_spell.pierce_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.base_energy_cost.min.dbl_val.value() == 1 );
        CHECK( test_spell.final_energy_cost.min.dbl_val.value() == 2 );
        CHECK( test_spell.energy_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.spell_class == trait_test_trait );
        CHECK( test_spell.energy_source == magic_energy_type::mana );
        CHECK( test_spell.dmg_type == damage_pure );
        CHECK( test_spell.difficulty.min.dbl_val.value() == 1 );
        CHECK( test_spell.max_level.min.dbl_val.value() == 1 );
        CHECK( test_spell.base_casting_time.min.dbl_val.value() == 1 );
        CHECK( test_spell.final_casting_time.min.dbl_val.value() == 2 );
        CHECK( test_spell.casting_time_increment.min.dbl_val.value() == 1.0f );
        CHECK( test_spell.learn_spells == test_learn_spell );
    }
}

TEST_CASE( "serialize_colony", "[json]" )
{
    cata::colony<std::string> c = { "foo", "bar" };
    test_serialization( c, R"(["foo","bar"])" );
}

TEST_CASE( "serialize_map", "[json]" )
{
    std::map<std::string, std::string> s_map = { { "foo", "foo_val" }, { "bar", "bar_val" } };
    test_serialization( s_map, R"({"bar":"bar_val","foo":"foo_val"})" );
    std::map<mtype_id, std::string> string_id_map = { { foo, "foo_val" } };
    test_serialization( string_id_map, R"({"foo":"foo_val"})" );
}

TEST_CASE( "serialize_pair", "[json]" )
{
    std::pair<std::string, int> p = { "foo", 42 };
    test_serialization( p, R"(["foo",42])" );
}

TEST_CASE( "serialize_sequences", "[json]" )
{
    std::vector<std::string> v = { "foo", "bar" };
    test_serialization( v, R"(["foo","bar"])" );
    std::array<std::string, 2> a = {{ "foo", "bar" }};
    test_serialization( a, R"(["foo","bar"])" );
    std::list<std::string> l = { "foo", "bar" };
    test_serialization( l, R"(["foo","bar"])" );
}

TEST_CASE( "serialize_set", "[json]" )
{
    std::set<std::string> s_set = { "foo", "bar" };
    test_serialization( s_set, R"(["bar","foo"])" );
    std::set<mtype_id> string_id_set = { foo };
    test_serialization( string_id_set, R"(["foo"])" );
}

template<typename Matcher>
static void test_translation_text_style_check( Matcher &&matcher, const std::string &json )
{
    JsonValue jsin = json_loader::from_string( json );
    translation trans;
    const std::string dmsg = capture_debugmsg_during( [&]() {
        jsin.read( trans );
    } );
    CHECK_THAT( dmsg, matcher );
}

template<typename Matcher>
static void test_pl_translation_text_style_check( Matcher &&matcher, const std::string &json )
{
    JsonValue jsin = json_loader::from_string( json );
    translation trans( translation::plural_tag {} );
    const std::string dmsg = capture_debugmsg_during( [&]() {
        jsin.read( trans );
    } );
    CHECK_THAT( dmsg, matcher );
}

TEST_CASE( "translation_text_style_check", "[json][translation]" )
{
    // this test case is mainly for checking the format of debug messages.
    // the text style check itself is tested in the lit test of clang-tidy.
    restore_on_out_of_scope<error_log_format_t> restore_error_log_format( error_log_format );
    restore_on_out_of_scope<check_plural_t> restore_check_plural( check_plural );
    restore_on_out_of_scope<json_error_output_colors_t> error_colors( json_error_output_colors );
    error_log_format = error_log_format_t::human_readable;
    check_plural = check_plural_t::certain;
    json_error_output_colors = json_error_output_colors_t::no_colors;

    // NOLINTBEGIN(cata-text-style)
    // string, ascii
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:5: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"("foo. bar.")" "\n"
            R"(   ▲▲▲)" "\n" ),
        R"("foo. bar.")" );
    // string, unicode
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:8: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"("…foo. bar.")" "\n"
            R"(    ▲▲▲)" "\n" ),
        R"("…foo. bar.")" );
    // string, escape sequence
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:11: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"("\u2026foo. bar.")" "\n"
            R"(         ▲▲▲)" "\n" ),
        R"("\u2026foo. bar.")" );
    // object, ascii
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:13: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"({"str": "foo. bar."})" "\n"
            R"(           ▲▲▲)" "\n" ),
        R"({"str": "foo. bar."})" );
    // object, unicode
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:16: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"({"str": "…foo. bar."})" "\n"
            R"(            ▲▲▲)" "\n" ),
        R"({"str": "…foo. bar."})" );
    // object, escape sequence
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:19: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"({"str": "\u2026foo. bar."})" "\n"
            R"(                 ▲▲▲)" "\n" ),
        R"({"str": "\u2026foo. bar."})" );

    // test unexpected plural forms
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:11: str_sp not supported here)" "\n"
            R"()" "\n"
            R"({"str_sp": "foo"})" "\n"
            R"(         ▲▲▲)" "\n" ),
        R"({"str_sp": "foo"})" );
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:25: str_pl not supported here)" "\n"
            R"()" "\n"
            R"({"str": "foo", "str_pl": "foo"})" "\n"
            R"(                       ▲▲▲)" "\n" ),
        R"({"str": "foo", "str_pl": "foo"})" );

    // test plural forms
    test_translation_text_style_check(
        Catch::Equals( "" ),
        R"("box")" );
    test_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str": "box"})" );

    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"("bar")" );
    test_pl_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:EOF: Cannot autogenerate plural )"
            R"(form.  Please specify the plural form explicitly using 'str' and )"
            R"('str_pl', or 'str_sp' if the singular and plural forms are the same.)" ),
        R"("box")" );

    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str": "bar"})" );
    test_pl_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:8: Cannot autogenerate plural )"
            R"(form.  Please specify the plural form explicitly using 'str' and )"
            R"('str_pl', or 'str_sp' if the singular and plural forms are the same.)"
            "\n"
            R"()" "\n"
            R"({"str": "box"})" "\n"
            R"(      ▲▲▲)" "\n" ),
        R"({"str": "box"})" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str_sp": "bar"})" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str_sp": "box"})" );

    test_pl_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:25: "str_pl" is not necessary here since the plural form can be automatically generated.)"
            "\n"
            R"()" "\n"
            R"({"str": "bar", "str_pl": "bars"})" "\n"
            R"(                       ▲▲▲)" "\n" ),
        R"({"str": "bar", "str_pl": "bars"})" );
    test_pl_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:25: Please use "str_sp" instead of "str" and "str_pl" for text with identical singular and plural forms)"
            "\n"
            R"()" "\n"
            R"({"str": "bar", "str_pl": "bar"})" "\n"
            R"(                       ▲▲▲)" "\n" ),
        R"({"str": "bar", "str_pl": "bar"})" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str": "box", "str_pl": "boxs"})" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str": "box", "str_pl": "boxes"})" );

    // ensure nolint member suppresses text style check
    test_translation_text_style_check(
        Catch::Equals( "" ),
        R"~({"str": "foo. bar", "//NOLINT(cata-text-style)": "blah"})~" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"~({"str": "box", "//NOLINT(cata-text-style)": "blah"})~" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"~({"str": "bar", "str_pl": "bars", "//NOLINT(cata-text-style)": "blah"})~" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"~({"str": "bar", "str_pl": "bar", "//NOLINT(cata-text-style)": "blah"})~" );

    {
        restore_on_out_of_scope<check_plural_t> restore_check_plural_2( check_plural );
        check_plural = check_plural_t::none;
        test_pl_translation_text_style_check(
            Catch::Equals( "" ),
            R"("box")" );
        test_pl_translation_text_style_check(
            Catch::Equals( "" ),
            R"({"str": "box"})" );
        test_pl_translation_text_style_check(
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:25: "str_pl" is not necessary here )"
                R"(since the plural form can be automatically generated.)" "\n"
                R"()" "\n"
                R"({"str": "bar", "str_pl": "bars"})" "\n"
                R"(                       ▲▲▲)" "\n" ),
            R"({"str": "bar", "str_pl": "bars"})" );
        test_pl_translation_text_style_check(
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:25: Please use "str_sp" instead of "str" )"
                R"(and "str_pl" for text with identical singular and plural forms)" "\n"
                R"()" "\n"
                R"({"str": "bar", "str_pl": "bar"})" "\n"
                R"(                       ▲▲▲)" "\n" ),
            R"({"str": "bar", "str_pl": "bar"})" );
        test_translation_text_style_check(
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:11: str_sp not supported here)" "\n"
                R"()" "\n"
                R"({"str_sp": "foo"})" "\n"
                R"(         ▲▲▲)" "\n" ),
            R"({"str_sp": "foo"})" );
        test_translation_text_style_check(
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:25: str_pl not supported here)" "\n"
                R"()" "\n"
                R"({"str": "foo", "str_pl": "foo"})" "\n"
                R"(                       ▲▲▲)" "\n" ),
            R"({"str": "foo", "str_pl": "foo"})" );
        test_translation_text_style_check(
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:5: insufficient spaces at this location.  2 required, but only 1 found.)"
                "\n"
                R"(    Suggested fix: insert " ")" "\n"
                R"(    At the following position (marked with caret))" "\n"
                R"()" "\n"
                R"("foo. bar.")" "\n"
                R"(   ▲▲▲)" "\n" ),
            R"("foo. bar.")" );
    }

    // ensure sentence text style check is disabled when plural form is enabled
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"("foo. bar")" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str": "foo. bar"})" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str": "foo. bar", "str_pl": "foo. baz"})" );
    test_pl_translation_text_style_check(
        Catch::Equals( "" ),
        R"({"str_sp": "foo. bar"})" );
    // NOLINTEND(cata-text-style)
}

TEST_CASE( "translation_text_style_check_error_recovery", "[json][translation]" )
{
    restore_on_out_of_scope<error_log_format_t> restore_error_log_format( error_log_format );
    restore_on_out_of_scope<json_error_output_colors_t> error_colors( json_error_output_colors );
    error_log_format = error_log_format_t::human_readable;
    json_error_output_colors = json_error_output_colors_t::no_colors;

    // NOLINTBEGIN(cata-text-style)
    SECTION( "string" ) {
        const std::string json =
            R"([)" "\n"
            R"(  "foo. bar.",)" "\n"
            R"(  "foobar")" "\n"
            R"(])" "\n";
        JsonArray ja = json_loader::from_string( json );
        translation trans;
        const std::string dmsg = capture_debugmsg_during( [&]() {
            ja.read_next( trans );
        } );
        // check that the correct debug message is shown
        CHECK_THAT(
            dmsg,
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:2:7: insufficient spaces at this location.  2 required, but only 1 found.)"
                "\n"
                R"(    Suggested fix: insert " ")" "\n"
                R"(    At the following position (marked with caret))" "\n"
                R"()" "\n"
                R"([)" "\n"
                R"(  "foo. bar.",)" "\n"
                R"(     ▲▲▲)" "\n"
                R"(  "foobar")" "\n"
                R"(])" "\n\n" ) );
    }

    SECTION( "object" ) {
        const std::string json =
            R"([)" "\n"
            R"(  { "str": "foo. bar." },)" "\n"
            R"(  "foobar")" "\n"
            R"(])" "\n";
        JsonArray ja = json_loader::from_string( json );
        JsonValue jv = ja.next_value();
        translation trans;
        const std::string dmsg = capture_debugmsg_during( [&]() {
            jv.read( trans );
        } );
        // check that the correct debug message is shown
        CHECK_THAT(
            dmsg,
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:2:16: insufficient spaces at this location.  2 required, but only 1 found.)"
                "\n"
                R"(    Suggested fix: insert " ")" "\n"
                R"(    At the following position (marked with caret))" "\n"
                R"()" "\n"
                R"([)" "\n"
                R"(  { "str": "foo. bar." },)" "\n"
                R"(              ▲▲▲)" "\n"
                R"(  "foobar")" "\n"
                R"(])" "\n\n" ) );
    }
    // NOLINTEND(cata-text-style)
}

TEST_CASE( "report_unvisited_members", "[json]" )
{
    restore_on_out_of_scope<error_log_format_t> restore_error_log_format( error_log_format );
    restore_on_out_of_scope<json_error_output_colors_t> error_colors( json_error_output_colors );
    error_log_format = error_log_format_t::human_readable;
    json_error_output_colors = json_error_output_colors_t::no_colors;

    // NOLINTBEGIN(cata-text-style)
    SECTION( "unvisited members" ) {
        const std::string json = R"({"foo": "foo", "bar": "bar"})";
        const std::string dmsg = capture_debugmsg_during( [&]() {
            JsonObject jo = json_loader::from_string( json );
            jo.get_string( "foo" );
        } );
        CHECK_THAT(
            dmsg,
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:22: Invalid or misplaced field name "bar" in JSON data)"
                "\n\n"
                R"({"foo": "foo", "bar": "bar"})" "\n"
                R"(                    ▲▲▲)" "\n" ) );
    }

    SECTION( "comments" ) {
        const std::string json = R"({"foo": "foo", "//": "foobar", "//bar": "bar"})";
        const std::string dmsg = capture_debugmsg_during( [&]() {
            JsonObject jo = json_loader::from_string( json );
            jo.get_string( "foo" );
        } );
        CHECK_THAT( dmsg, Catch::Equals( "" ) );
    }

    SECTION( "misplaced translator comments" ) {
        const std::string json = R"({"foo": "foo", "//~": "bar"})";
        const std::string dmsg = capture_debugmsg_during( [&]() {
            JsonObject jo = json_loader::from_string( json );
            jo.get_string( "foo" );
        } );
        CHECK_THAT(
            dmsg,
            Catch::Equals(
                R"((json-error))" "\n"
                R"(Json error: <unknown source file>:1:22: "//~" should be within a text object and contain comments for translators.)"
                "\n\n"
                R"({"foo": "foo", "//~": "bar"})" "\n"
                R"(                    ▲▲▲)" "\n" ) );
    }

    SECTION( "valid translator comments" ) {
        const std::string json = R"({"str": "foo", "//~": "bar"})";
        const std::string dmsg = capture_debugmsg_during( [&]() {
            JsonValue jv = json_loader::from_string( json );
            translation msg;
            jv.read( msg );
        } );
        CHECK_THAT( dmsg, Catch::Equals( "" ) );
    }
    // NOLINTEND(cata-text-style)
}

TEST_CASE( "correct_cursor_position_for_unicode_json_error", "[json]" )
{
    restore_on_out_of_scope<error_log_format_t> restore_error_log_format( error_log_format );
    restore_on_out_of_scope<json_error_output_colors_t> error_colors( json_error_output_colors );
    error_log_format = error_log_format_t::human_readable;
    json_error_output_colors = json_error_output_colors_t::no_colors;

    // NOLINTBEGIN(cata-text-style)
    // check long unicode strings point at the correct column
    const std::string json =
        R"({ "两两两两两两两两两两两两两两两两两两两两两两两两": 两 })";
    try {
        JsonArray ja = json_loader::from_string( json );
        JsonValue jv = ja.next_value();
    } catch( JsonError &e ) {
        // check that the correct debug message is shown
        const std::string e_what = e.what();
        const std::string e_expected =
            R"(Json error: <unknown source file>:1:79: illegal character: code: -28)"
            "\n\n"
            R"({ "两两两两两两两两两两两两两两两两两两两两两两两两": 两 })"
            "\n"
            R"(                                                     ▲▲▲)"
            "\n";
        CHECK_THAT( e_what, Catch::Equals( e_expected ) );
        SUCCEED();
        return;
    }
    FAIL();
    // NOLINTEND(cata-text-style)
}

static void test_get_string( const std::string &str, const std::string &json )
{
    CAPTURE( json );
    JsonValue jsin = json_loader::from_string( json );
    CHECK( jsin.get_string() == str );
}

template<typename Matcher>
static void test_get_string_throws_matches( Matcher &&matcher, const std::string &json )
{
    CAPTURE( json );
    CHECK_THROWS_MATCHES( ( [&] {
        JsonValue jsin = json_loader::from_string( json );
        jsin.get_string();
    } )(), JsonError, matcher );
}

template<typename Matcher>
static void test_string_error_throws_matches( Matcher &&matcher, const std::string &json,
        const int offset )
{
    CAPTURE( json );
    CAPTURE( offset );
    JsonValue jsin = json_loader::from_string( json );
    CHECK_THROWS_MATCHES( jsin.string_error( offset, "<message>" ), JsonError, matcher );
}

TEST_CASE( "jsonin_get_string", "[json]" )
{
    restore_on_out_of_scope<error_log_format_t> restore_error_log_format( error_log_format );
    restore_on_out_of_scope<json_error_output_colors_t> error_colors( json_error_output_colors );
    error_log_format = error_log_format_t::human_readable;
    json_error_output_colors = json_error_output_colors_t::no_colors;

    // NOLINTBEGIN(cata-text-style)
    // read plain text
    test_get_string( "foo", R"("foo")" );
    // ignore starting spaces
    test_get_string( "bar", R"(  "bar")" );
    // read unicode characters
    test_get_string( "……", R"("……")" );
    test_get_string( "……", "\"\u2026\u2026\"" );
    test_get_string( "\xe2\x80\xa6", R"("…")" );
    test_get_string( "\u00A0", R"("\u00A0")" );
    test_get_string( "\u00A0", R"("\u00a0")" );
    // read escaped unicode
    test_get_string( "…", R"("\u2026")" );
    // read utf8 sequence
    test_get_string( "…", "\"\xe2\x80\xa6\"" );
    // read newline
    test_get_string( "a\nb\nc", R"("a\nb\nc")" );
    // read slash
    test_get_string( "foo\\bar", R"("foo\\bar")" );
    // read escaped characters
    test_get_string( "\"\\/\b\f\n\r\t\u2581", R"("\"\\\/\b\f\n\r\t\u2581")" );

    // empty json
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: input file is empty" ),
        std::string() );
    // no starting quote
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: cannot parse value starting with: abc" ),
        R"(abc)" );
    // no ending quote
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: illegal character in string constant" ),
        R"(")" );
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: illegal character in string constant" ),
        R"("foo)" );
    // incomplete escape sequence and no ending quote
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: unknown escape code in string constant" ),
        R"("\)" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:3: escape code must be followed by 4 hex digits)" "\n"
            R"()" "\n"
            R"("\u12)" "\n"
            R"( ▲▲▲)" "\n" ),
        R"("\u12)" );
    // incorrect escape sequence
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:2: unknown escape code in string constant)" "\n"
            R"()" "\n"
            R"("\.")" "\n"
            R"(▲▲▲)" "\n" ),
        R"("\.")" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:3: escape code must be followed by 4 hex digits)" "\n"
            R"()" "\n"
            R"("\uDEFG")" "\n"
            R"( ▲▲▲)" "\n" ),
        R"("\uDEFG")" );
    // not a valid utf8 sequence
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: illegal UTF-8 sequence" ),
        "\"\x80\"" );
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: illegal UTF-8 sequence" ),
        "\"\xFC\x80\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:EOF: illegal UTF-8 sequence)" ),
        "\"\xFD\x80\x80\x80\x80\x80\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:EOF: illegal UTF-8 sequence)" ),
        "\"\xFC\x80\x80\x80\x80\xC0\"" );
    // end of line
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:2: illegal character in string constant)" "\n"
            R"()" "\n"
            R"("a)" "\n"
            R"(▲▲▲)" "\n\"\n" ),
        "\"a\n\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:2: illegal character in string constant)" "\n"
            R"()" "\n"
            R"("b)" "\r\"\n"
            R"(▲▲▲)" "\n" ),
        "\"b\r\"" );

    // test throwing error after the given number of unicode characters
    // ascii
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:1: <message>)" "\n"
            R"()" "\n"
            R"("foobar")" "\n"
            R"(▲▲▲)" "\n" ),
        R"("foobar")", 0 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:4: <message>)" "\n"
            R"()" "\n"
            R"("foobar")" "\n"
            R"(  ▲▲▲)" "\n" ),
        R"("foobar")", 3 );
    // unicode
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:4: <message>)" "\n"
            R"()" "\n"
            R"("foo…bar1")" "\n"
            R"(  ▲▲▲)" "\n" ),
        R"("foo…bar1")", 3 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: <message>)" "\n"
            R"()" "\n"
            R"("foo…bar2")" "\n"
            R"(   ▲▲▲)" "\n" ),
        R"("foo…bar2")", 4 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:8: <message>)" "\n"
            R"()" "\n"
            R"("foo…bar3")" "\n"
            R"(    ▲▲▲)" "\n" ),
        R"("foo…bar3")", 5 );
    // escape sequence
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:11: <message>)" "\n"
            R"()" "\n"
            R"("foo\u2026bar")" "\n"
            R"(         ▲▲▲)" "\n" ),
        R"("foo\u2026bar")", 5 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: <message>)" "\n"
            R"()" "\n"
            R"("foo\nbar")" "\n"
            R"(     ▲▲▲)" "\n" ),
        R"("foo\nbar")", 5 );
    // NOLINTEND(cata-text-style)
}

TEST_CASE( "item_colony_ser_deser", "[json][item]" )
{
    // calculates the number of substring (needle) occurrences withing the target string (haystack)
    // doesn't include overlaps
    const auto count_occurences = []( const std::string_view haystack, const std::string_view needle ) {
        int occurrences = 0;
        std::string::size_type pos = 0;
        while( ( pos = haystack.find( needle, pos ) ) != std::string::npos ) {
            occurrences++;
            pos += needle.length();
        }
        return occurrences;
    };

    // checks if two colonies are equal using `same_for_rle` for item comparison
    const auto is_same = []( const cata::colony<item> &a, const cata::colony<item> &b ) {
        return std::equal( a.begin(), a.end(), b.begin(),
        []( const item & a, const item & b ) {
            return a.same_for_rle( b );
        } );
    };

    SECTION( "identical items are collapsed" ) {
        cata::colony<item> col;
        for( int i = 0; i < 10; ++i ) {
            // currently tools cannot be stackable
            col.insert( item( itype_test_rag ) );
        }
        REQUIRE( col.size() == 10 );
        REQUIRE( col.begin()->same_for_rle( *std::next( col.begin() ) ) );

        std::string json;
        std::ostringstream os;
        JsonOut jsout( os );
        jsout.write( col );
        json = os.str();
        CAPTURE( json );
        {
            INFO( "should be compressed into the single item" );
            CHECK( count_occurences( json, "\"typeid\":\"test_rag\"" ) == 1 );
        }
        {
            INFO( "should contain the number of items" );
            CHECK( json.find( "10" ) != std::string::npos );
        }
        JsonValue jsin = json_loader::from_string( json );
        cata::colony<item> read_val;
        {
            INFO( "should be read successfully" );
            CHECK( jsin.read( read_val ) );
        }
        {
            INFO( "should be identical to the original " );
            CHECK( is_same( col, read_val ) );
        }
    }

    SECTION( "different items are saved individually" ) {
        cata::colony<item> col;
        col.insert( item( itype_test_rag ) );
        col.insert( item( itype_test_rag ) );
        ( *col.rbegin() ).set_flag( json_flag_DIRTY );

        REQUIRE( col.size() == 2 );
        REQUIRE( !col.begin()->same_for_rle( *col.rbegin() ) );
        REQUIRE( ( *col.rbegin() ).same_for_rle( *col.rbegin() ) );

        std::string json;
        std::ostringstream os;
        JsonOut jsout( os );
        jsout.write( col );
        json = os.str();
        CAPTURE( json );
        {
            INFO( "should not be compressed" );
            CHECK( count_occurences( json, "\"typeid\":\"test_rag" ) == 2 );
        }
        JsonValue jsin = json_loader::from_string( json );
        cata::colony<item> read_val;
        {
            INFO( "should be read successfully" );
            CHECK( jsin.read( read_val ) );
        }
        {
            INFO( "should be identical to the original " );
            CHECK( is_same( col, read_val ) );
        }
    }

    SECTION( "incorrect items in json are skipped" ) {
        // first item is an array without the run length defined (illegal)
        const char *json =
            R"([[{"typeid":"test_rag","item_vars":{"magazine_converted":"1"}}],)" "\n"
            R"(    {"typeid":"test_rag","item_vars":{"magazine_converted":"1"}}])";
        JsonValue jsin = json_loader::from_string( json );
        cata::colony<item> read_val;
        {
            INFO( "should be read successfully" );
            CHECK( jsin.read( read_val ) );
        }
        {
            INFO( "one item was skipped" );
            CHECK( read_val.size() == 1 );
        }
        {
            INFO( "item type was read correctly" );
            CHECK( read_val.begin()->typeId() == itype_test_rag );
        }
    }
}

TEST_CASE( "serialize_optional", "[json]" )
{
    SECTION( "simple_empty_optional" ) {
        std::optional<int> o;
        test_serialization( o, "null" );
    }
    SECTION( "optional_of_int" ) {
        std::optional<int> o( 7 );
        test_serialization( o, "7" );
    }
    SECTION( "vector_of_empty_optional" ) {
        std::vector<std::optional<int>> v( 3 );
        test_serialization( v, "[null,null,null]" );
    }
    SECTION( "vector_of_optional_of_int" ) {
        std::vector<std::optional<int>> v{ { 1 }, { 2 }, { 3 } };
        test_serialization( v, "[1,2,3]" );
    }
}
