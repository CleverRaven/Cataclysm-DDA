#include <array>
#include <vector>

#include "debug.h"
#include "output.h"
#include "translation_gendered.h"
#include "translations.h"

static bool sanity_checked_genders = false;

void reset_sanity_check_genders()
{
    sanity_checked_genders = false;
}

static void sanity_check_genders( const std::vector<std::string> &language_genders )
{
    if( sanity_checked_genders ) {
        return;
    }
    sanity_checked_genders = true;

    constexpr std::array<const char *, 3> all_genders = {{"f", "m", "n"}};

    for( const std::string &gender : language_genders ) {
        if( find( all_genders.begin(), all_genders.end(), gender ) == all_genders.end() ) {
            debugmsg( "Unexpected gender '%s' in grammatical gender list for "
                      "this language", gender );
        }
    }
}

std::string gettext_gendered( const GenderMap &genders, const std::string &msg )
{
    //~ Space-separated list of grammatical genders. Default should be first.
    //~ Use short names and try to be consistent between languages as far as
    //~ possible.  Current choices are m (male), f (female), n (neuter).
    //~ As appropriate we might add e.g. a (animate) or c (common).
    //~ New genders must be added to all_genders in lang/extract_json_strings.py
    //~ and src/translations.cpp.
    //~ The primary purpose of this is for NPC dialogue which might depend on
    //~ gender.  Only add genders to the extent needed by such translations.
    //~ They are likely only needed if they affect the first and second
    //~ person.  For example, one gender suffices for English even though
    //~ third person pronouns differ.
    std::string language_genders_s = pgettext( "grammatical gender list", "n" );
    std::vector<std::string> language_genders = string_split( language_genders_s, ' ' );

    sanity_check_genders( language_genders );

    if( language_genders.empty() ) {
        language_genders.emplace_back( "n" );
    }

    std::vector<std::string> chosen_genders;
    for( const auto &subject_genders : genders ) {
        // default if no match
        std::string chosen_gender = language_genders[0];
        for( const std::string &gender : subject_genders.second ) {
            if( std::find( language_genders.begin(), language_genders.end(), gender ) !=
                language_genders.end() ) {
                chosen_gender = gender;
                break;
            }
        }
        chosen_genders.push_back( subject_genders.first + ":" + chosen_gender );
    }
    std::string context = join( chosen_genders, " " );
    return pgettext( context.c_str(), msg.c_str() );
}
