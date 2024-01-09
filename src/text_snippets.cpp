#include "text_snippets.h"

#include <algorithm>
#include <cstddef>
#include <new>
#include <random>
#include <utility>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "path_info.h"
#include "rng.h"

snippet_library SNIPPET;

void snippet_library::load_snippet( const JsonObject &jsobj )
{
    if( hash_to_id_migration.has_value() ) {
        debugmsg( "snippet_library::load_snippet called after snippet_library::migrate_hash_to_id." );
    }
    hash_to_id_migration = std::nullopt;
    const std::string category = jsobj.get_string( "category" );
    if( jsobj.has_array( "text" ) ) {
        add_snippets_from_json( category, jsobj.get_array( "text" ) );
    } else {
        add_snippet_from_json( category, jsobj );
    }
}

void snippet_library::add_snippets_from_json( const std::string &category, const JsonArray &jarr )
{
    if( hash_to_id_migration.has_value() ) {
        debugmsg( "snippet_library::add_snippets_from_json called after snippet_library::migrate_hash_to_id." );
    }
    hash_to_id_migration = std::nullopt;
    for( const JsonValue entry : jarr ) {
        if( entry.test_string() ) {
            translation text;
            if( !entry.read( text ) ) {
                entry.throw_error( "Error reading snippet from JSON array" );
            }
            std::vector<weighted_translation> &no_id = snippets_by_category[category].no_id;
            const uint64_t weight_acc = no_id.empty() ? 1 : no_id.back().weight_acc + 1;
            no_id.emplace_back( weighted_translation{ weight_acc, text } );
        } else {
            JsonObject jo = entry.get_object();
            add_snippet_from_json( category, jo );
        }
    }
}

void snippet_library::add_snippet_from_json( const std::string &category, const JsonObject &jo )
{
    if( hash_to_id_migration.has_value() ) {
        debugmsg( "snippet_library::add_snippet_from_json called after snippet_library::migrate_hash_to_id." );
    }
    hash_to_id_migration = std::nullopt;
    uint64_t weight = 1;
    translation text;
    mandatory( jo, false, "text", text );
    optional( jo, false, "weight", weight, 1 );
    if( jo.has_member( "id" ) ) {
        snippet_id id;
        jo.read( "id", id );
        if( id.is_null() ) {
            jo.throw_error_at( "id", "Null snippet id specified" );
        }
        if( snippets_by_id.find( id ) != snippets_by_id.end() ) {
            jo.throw_error_at( "id", "Duplicate snippet id" );
        }
        std::vector<weighted_id> &ids = snippets_by_category[category].ids;
        const uint64_t weight_acc = ids.empty() ? weight : ids.back().weight_acc + weight;
        ids.emplace_back( weighted_id{ weight_acc, id } );
        snippets_by_id[id] = text;
        if( jo.has_member( "effect_on_examine" ) ) {
            EOC_by_id[id] = talk_effect_t( jo, "effect_on_examine" );
        }
        translation name;
        optional( jo, false, "name", name );
        name_by_id[id] = name;
    } else {
        std::vector<weighted_translation> &no_id = snippets_by_category[category].no_id;
        const uint64_t weight_acc = no_id.empty() ? weight : no_id.back().weight_acc + weight;
        no_id.emplace_back( weighted_translation{ weight_acc, text } );
    }
}

void snippet_library::reload_names( const cata_path &path )
{
    read_from_file_json( path, [this]( const JsonArray & names_json ) {
        struct usage_info {
            bool gendered;
            category_snippets *cat;
            category_snippets *male_cat;
            category_snippets *female_cat;
        };

        const std::unordered_map<std::string, usage_info> usages = {
            { "backer", { true, nullptr, &snippets_by_category["<male_backer_name>"], &snippets_by_category["<female_backer_name>"] } },
            { "given", { true, nullptr, &snippets_by_category["<male_given_name>"], &snippets_by_category["<female_given_name>"] } },
            { "family", { false, &snippets_by_category["<family_name>"], nullptr, nullptr } },
            { "nick", { false, &snippets_by_category["<nick_name>"], nullptr, nullptr } },
            { "city", { false, &snippets_by_category["<city_name>"], nullptr, nullptr } },
            { "world", { false, &snippets_by_category["<world_name>"], nullptr, nullptr } },
        };

        // First clear previously loaded names
        for( const std::pair<const std::string, usage_info> &kv : usages ) {
            const usage_info &use = kv.second;
            if( use.gendered ) {
                use.male_cat->no_id.clear();
                use.female_cat->no_id.clear();
            } else {
                use.cat->no_id.clear();
            }
        }

        for( JsonObject jo : names_json ) {
            const std::string usage = jo.get_string( "usage" );
            const auto it = usages.find( usage );
            if( it == usages.end() ) {
                debugmsg( "Unknown name usage %s.", usage );
                continue;
            }
            const usage_info &info = it->second;
            std::vector<category_snippets *> cats;
            if( info.gendered ) {
                const std::string gender = jo.get_string( "gender" );
                if( gender == "unisex" ) {
                    cats.emplace_back( info.male_cat );
                    cats.emplace_back( info.female_cat );
                } else if( gender == "male" ) {
                    cats.emplace_back( info.male_cat );
                } else if( gender == "female" ) {
                    cats.emplace_back( info.female_cat );
                } else {
                    debugmsg( "Unknown name gender %s.", gender );
                    continue;
                }
            } else {
                cats.emplace_back( info.cat );
            }
            if( jo.has_array( "name" ) ) {
                for( const std::string n : jo.get_array( "name" ) ) {
                    for( category_snippets *const cat : cats ) {
                        const uint64_t weight_acc = cat->no_id.empty() ? 1 : cat->no_id.back().weight_acc + 1;
                        cat->no_id.emplace_back( weighted_translation{ weight_acc, no_translation( n ) } );
                    }
                }
            } else {
                const std::string n = jo.get_string( "name" );
                for( category_snippets *const cat : cats ) {
                    const uint64_t weight_acc = cat->no_id.empty() ? 1 : cat->no_id.back().weight_acc + 1;
                    cat->no_id.emplace_back( weighted_translation{ weight_acc, no_translation( n ) } );
                }
            }
        }

        // Add fallback in case no name was loaded
        for( const std::pair<const std::string, usage_info> &kv : usages ) {
            const usage_info &use = kv.second;
            if( use.gendered ) {
                if( use.male_cat->no_id.empty() ) {
                    use.male_cat->no_id.emplace_back( weighted_translation{ 1, to_translation( "Tom" ) } );
                }
                if( use.female_cat->no_id.empty() ) {
                    use.female_cat->no_id.emplace_back( weighted_translation{ 1, to_translation( "Tom" ) } );
                }
            } else {
                if( use.cat->no_id.empty() ) {
                    use.cat->no_id.emplace_back( weighted_translation{ 1, to_translation( "Tom" ) } );
                }
            }
        }
    } );
}

void snippet_library::clear_snippets()
{
    hash_to_id_migration = std::nullopt;
    snippets_by_category.clear();
    snippets_by_id.clear();
    // Needed by world creation etc
    reload_names( PATH_INFO::names() );
}

bool snippet_library::has_category( const std::string &category ) const
{
    return snippets_by_category.find( category ) != snippets_by_category.end();
}

std::optional<translation> snippet_library::get_snippet_by_id( const snippet_id &id ) const
{
    const auto it = snippets_by_id.find( id );
    if( it == snippets_by_id.end() ) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<talk_effect_t> snippet_library::get_EOC_by_id( const snippet_id &id ) const
{
    const auto it = EOC_by_id.find( id );
    if( it == EOC_by_id.end() ) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<translation> snippet_library::get_name_by_id( const snippet_id &id ) const
{
    const auto it = name_by_id.find( id );
    if( it == name_by_id.end() ) {
        return std::nullopt;
    }
    return it->second;
}

const translation &snippet_library::get_snippet_ref_by_id( const snippet_id &id ) const
{
    const auto it = snippets_by_id.find( id );
    if( it == snippets_by_id.end() ) {
        static const translation empty_translation;
        return empty_translation;
    }
    return it->second;
}

bool snippet_library::has_snippet_with_id( const snippet_id &id ) const
{
    return snippets_by_id.find( id ) != snippets_by_id.end();
}

std::string snippet_library::expand( const std::string &str ) const
{
    size_t tag_begin = str.find( '<' );
    if( tag_begin == std::string::npos ) {
        return str;
    }
    size_t tag_end = str.find( '>', tag_begin + 1 );
    if( tag_end == std::string::npos ) {
        return str;
    }

    std::string symbol = str.substr( tag_begin, tag_end - tag_begin + 1 );
    std::optional<translation> replacement = random_from_category( symbol );
    if( !replacement.has_value() ) {
        return str.substr( 0, tag_end + 1 )
               + expand( str.substr( tag_end + 1 ) );
    }
    return str.substr( 0, tag_begin )
           + expand( replacement.value().translated() )
           + expand( str.substr( tag_end + 1 ) );
}

snippet_id snippet_library::random_id_from_category( const std::string &cat ) const
{
    const auto it = snippets_by_category.find( cat );
    if( it == snippets_by_category.end() ) {
        return snippet_id::NULL_ID();
    }
    if( !it->second.no_id.empty() ) {
        debugmsg( "ids are required, but not specified for some snippets in category %s", cat );
    }
    const uint64_t weight_sum =
        it->second.ids.empty() ? 0 : it->second.ids.back().weight_acc;
    if( weight_sum == 0 ) {
        return snippet_id::NULL_ID();
    }
    // uniform_int_distribution always returns zero when the random engine is
    // cata_default_random_engine aka std::minstd_rand0 and the seed is small,
    // so std::mt19937 is used instead. This engine is deterministically seeded,
    // so acceptable.
    // NOLINTNEXTLINE(cata-determinism)
    std::mt19937 generator( rng_bits() );
    std::uniform_int_distribution<uint64_t> dis( 0, weight_sum - 1 );
    const auto sit = std::upper_bound(
                         it->second.ids.begin(), it->second.ids.end(), dis( generator ),
    []( const uint64_t loc, const weighted_id & wid ) {
        return loc < wid.weight_acc;
    } );
    return sit->value;
}

std::optional<translation> snippet_library::random_from_category( const std::string &cat ) const
{
    return random_from_category( cat, rng_bits() );
}

std::optional<translation> snippet_library::random_from_category( const std::string &cat,
        unsigned int seed ) const
{
    const auto it = snippets_by_category.find( cat );
    if( it == snippets_by_category.end() ) {
        return std::nullopt;
    }
    const uint64_t ids_weighted_sum =
        it->second.ids.empty() ? 0 : it->second.ids.back().weight_acc;
    const uint64_t weight_sum = ids_weighted_sum +
                                ( it->second.no_id.empty() ? 0 : it->second.no_id.back().weight_acc );
    if( weight_sum == 0 ) {
        return std::nullopt;
    }
    // uniform_int_distribution always returns zero when the random engine is
    // cata_default_random_engine aka std::minstd_rand0 and the seed is small,
    // so std::mt19937 is used instead. This engine is deterministically seeded,
    // so acceptable.
    // NOLINTNEXTLINE(cata-determinism)
    std::mt19937 generator( seed );
    std::uniform_int_distribution<uint64_t> dis( 0, weight_sum - 1 );
    const uint64_t loc = dis( generator );
    if( loc < ids_weighted_sum ) {
        const auto sit = std::upper_bound(
                             it->second.ids.begin(), it->second.ids.end(), loc,
        []( const uint64_t loc, const weighted_id & wid ) {
            return loc < wid.weight_acc;
        } );
        return get_snippet_by_id( sit->value );
    } else {
        const auto sit = std::upper_bound(
                             it->second.no_id.begin(), it->second.no_id.end(), loc - ids_weighted_sum,
        []( const uint64_t loc, const weighted_translation & wt ) {
            return loc < wt.weight_acc;
        } );
        return sit->value;
    }
}

snippet_id snippet_library::migrate_hash_to_id( const int old_hash )
{
    if( !hash_to_id_migration.has_value() ) {
        hash_to_id_migration.emplace();
        for( const auto &id_and_text : snippets_by_id ) {
            std::optional<int> hash = id_and_text.second.legacy_hash();
            if( hash ) {
                hash_to_id_migration->emplace( hash.value(), id_and_text.first );
            }
        }
    }
    const auto it = hash_to_id_migration->find( old_hash );
    if( it == hash_to_id_migration->end() ) {
        return snippet_id::NULL_ID();
    }
    return it->second;
}

std::vector<std::pair<snippet_id, std::string>> snippet_library::get_snippets_by_category(
            const std::string &cat, bool add_null_id )
{
    std::vector<std::pair<snippet_id, std::string>> ret;
    if( !has_category( cat ) ) {
        return ret;
    }
    std::unordered_map<std::string, category_snippets>::iterator snipp_cat = snippets_by_category.find(
                cat );
    if( snipp_cat != snippets_by_category.end() ) {
        category_snippets snipps = snipp_cat->second;
        if( add_null_id && !snipps.ids.empty() ) {
            ret.emplace_back( snippet_id::NULL_ID(), "" );
        }
        for( weighted_id id : snipps.ids ) {
            std::string desc = get_snippet_ref_by_id( id.value ).translated();
            ret.emplace_back( id.value, desc );
        }
    }
    return ret;
}

template<> const translation &snippet_id::obj() const
{
    return SNIPPET.get_snippet_ref_by_id( *this );
}

template<> bool snippet_id::is_valid() const
{
    return SNIPPET.has_snippet_with_id( *this );
}
