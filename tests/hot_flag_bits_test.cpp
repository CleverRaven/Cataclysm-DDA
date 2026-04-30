#include <cstdint>
#include <sstream>
#include <string>

#include "cata_catch.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "json.h"
#include "json_loader.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_attachable_ear_muffs( "attachable_ear_muffs" );
static const itype_id itype_helmet_army( "helmet_army" );

static constexpr uint64_t bit_fit = static_cast<uint64_t>( hot_flag_bit::FIT );
static constexpr uint64_t bit_dirty = static_cast<uint64_t>( hot_flag_bit::DIRTY );
static constexpr uint64_t bit_varsize = static_cast<uint64_t>( hot_flag_bit::VARSIZE );
static constexpr uint64_t bit_diamond = static_cast<uint64_t>( hot_flag_bit::DIAMOND );

TEST_CASE( "hot_flag_bits_set_unset_toggle_own_bits", "[item][flag]" )
{
    item it( itype_helmet_army );
    REQUIRE( ( it.own_hot_flags() & bit_fit ) == 0 );

    it.set_flag( flag_FIT );
    CHECK( ( it.own_hot_flags() & bit_fit ) == bit_fit );
    CHECK( ( it.combined_hot_flags() & bit_fit ) == bit_fit );

    it.unset_flag( flag_FIT );
    CHECK( ( it.own_hot_flags() & bit_fit ) == 0 );
}

TEST_CASE( "hot_flag_bits_unset_flags_clears_all_own_bits", "[item][flag]" )
{
    item it( itype_helmet_army );
    it.set_flag( flag_FIT );
    it.set_flag( flag_DIRTY );
    REQUIRE( ( it.own_hot_flags() & ( bit_fit | bit_dirty ) ) == ( bit_fit | bit_dirty ) );

    it.unset_flags();
    CHECK( it.own_hot_flags() == 0 );
}

TEST_CASE( "hot_flag_bits_type_bits_reflect_itype_json_flags", "[item][flag]" )
{
    item it( itype_helmet_army );
    // helmet_army declares VARSIZE in JSON.
    CHECK( ( it.combined_hot_flags() & bit_varsize ) == bit_varsize );
    CHECK( ( it.own_hot_flags() & bit_varsize ) == 0 );
}

TEST_CASE( "hot_flag_bits_set_flag_with_non_hot_flag_does_not_set_bits", "[item][flag]" )
{
    item it( itype_helmet_army );
    const uint64_t before = it.own_hot_flags();
    it.set_flag( flag_ALARMCLOCK );
    CHECK( it.own_hot_flags() == before );
}

TEST_CASE( "hot_flag_bits_convert_recomputes_inherited_bits", "[item][flag]" )
{
    item it( itype_helmet_army );
    it.convert( itype_helmet_army, nullptr );
    CHECK( ( it.combined_hot_flags() & bit_varsize ) == bit_varsize );
}

TEST_CASE( "hot_flag_bits_save_load_restores_own_bits", "[item][flag]" )
{
    item original( itype_helmet_army );
    original.set_flag( flag_FIT );
    REQUIRE( ( original.own_hot_flags() & bit_fit ) == bit_fit );

    std::ostringstream os;
    JsonOut jsout( os );
    original.serialize( jsout );

    item loaded;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv;
    loaded.deserialize( jo );

    CHECK( loaded.has_flag( flag_FIT ) );
    CHECK( ( loaded.own_hot_flags() & bit_fit ) == bit_fit );
    CHECK( ( loaded.combined_hot_flags() & bit_varsize ) == bit_varsize );
}

// Build a helmet_army with an inherits_flags pocket holding an item that carries
// flag_FIT, so the parent's hot_flags_inherited gains the FIT bit.
static item make_parent_with_inherited_fit()
{
    item parent( itype_helmet_army );
    item child( itype_attachable_ear_muffs );
    child.set_flag( flag_FIT );
    parent.put_in( child, pocket_type::CONTAINER );
    return parent;
}

TEST_CASE( "hot_flag_bits_inherited_bits_set_when_contents_change", "[item][flag]" )
{
    item parent = make_parent_with_inherited_fit();
    CHECK( ( parent.combined_hot_flags() & bit_fit ) == bit_fit );
    CHECK( ( parent.own_hot_flags() & bit_fit ) == 0 );
}

TEST_CASE( "hot_flag_bits_convert_preserves_inherited_bits_via_rebuild", "[item][flag]" )
{
    item parent = make_parent_with_inherited_fit();
    REQUIRE( ( parent.combined_hot_flags() & bit_fit ) == bit_fit );
    parent.convert( itype_helmet_army, nullptr );
    CHECK( ( parent.combined_hot_flags() & bit_fit ) == bit_fit );
}

TEST_CASE( "hot_flag_bits_save_load_restores_inherited_bits", "[item][flag]" )
{
    item parent = make_parent_with_inherited_fit();
    REQUIRE( ( parent.combined_hot_flags() & bit_fit ) == bit_fit );

    std::ostringstream os;
    JsonOut jsout( os );
    parent.serialize( jsout );

    item loaded;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv;
    loaded.deserialize( jo );

    CHECK( loaded.has_flag( flag_FIT ) );
    CHECK( ( loaded.combined_hot_flags() & bit_fit ) == bit_fit );
    CHECK( ( loaded.own_hot_flags() & bit_fit ) == 0 );
}

TEST_CASE( "hot_flag_bits_combined_view_agrees_with_public_has_flag", "[item][flag]" )
{
    item it( itype_helmet_army );
    it.set_flag( flag_FIT );
    CHECK( it.has_flag( flag_FIT ) == ( ( it.combined_hot_flags() & bit_fit ) != 0 ) );
    CHECK( it.has_flag( flag_DIAMOND ) == ( ( it.combined_hot_flags() & bit_diamond ) != 0 ) );
    CHECK( it.has_flag( flag_VARSIZE ) == ( ( it.combined_hot_flags() & bit_varsize ) != 0 ) );
    CHECK( it.has_flag( flag_DIRTY ) == ( ( it.combined_hot_flags() & bit_dirty ) != 0 ) );
}
