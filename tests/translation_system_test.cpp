#include <cstring>
#include "cata_catch.h"
#include "filesystem.h"
#include "string_formatter.h"
#include "translation_document.h"
#include "translation_manager_impl.h"

#if defined(LOCALIZE)

static void LoadMODocument( const char *path )
{
    volatile TranslationDocument document( path );
}

TEST_CASE( "TranslationDocument loads valid MO", "[translations]" )
{
    const char *path = "./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo";
    CAPTURE( path );
    REQUIRE( file_exist( fs::u8path( path ) ) );
    REQUIRE_NOTHROW( LoadMODocument( path ) );
}

TEST_CASE( "TranslationDocument rejects invalid MO", "[translations]" )
{
    const char *path = "./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/INVALID_RAND.mo";
    CAPTURE( path );
    REQUIRE( file_exist( fs::u8path( path ) ) );
    REQUIRE_THROWS_AS( LoadMODocument( path ), InvalidTranslationDocumentException );
}

TEST_CASE( "TranslationDocument loads all core MO", "[translations]" )
{
    const std::unordered_set<std::string> languages =
        TranslationManager::GetInstance().GetAvailableLanguages();
    for( const std::string &lang : languages ) {
        const std::string path = string_format( "./lang/mo/%s/LC_MESSAGES/cataclysm-dda.mo", lang );
        CAPTURE( path );
        REQUIRE( file_exist( path ) );
        REQUIRE_NOTHROW( LoadMODocument( path.c_str() ) );
    }
}

TEST_CASE( "No string buffer overlap in TranslationDocument", "[translations]" )
{
    const std::unordered_set<std::string> languages =
        TranslationManager::GetInstance().GetAvailableLanguages();
    for( const std::string &lang : languages ) {
        const std::string path = string_format( "./lang/mo/%s/LC_MESSAGES/cataclysm-dda.mo", lang );
        CAPTURE( path );
        REQUIRE( file_exist( path ) );
        TranslationDocument document( path );
        // The following code walks through every string contained in the MO document
        // So AddressSanitizer can also detect memory access violation if there is any
        const std::size_t n = document.Count();
        const char *last_ending = nullptr;
        for( std::size_t i = 0; i < n; i++ ) {
            const char *str = document.GetOriginalString( i );
            CHECK( last_ending < str );
            last_ending = str + std::strlen( str );
        }
        last_ending = nullptr;
        for( std::size_t i = 0; i < n; i++ ) {
            const char *str = document.GetTranslatedString( i );
            CHECK( last_ending < str );
            last_ending = str + std::strlen( str );
        }
    }
}

TEST_CASE( "TranslationDocument loading benchmark", "[.][benchmark][translations]" )
{
    BENCHMARK( "Load Russian" ) {
        return TranslationDocument( "./lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo" );
    };
}

TEST_CASE( "TranslationManager loading benchmark", "[.][benchmark][translations]" )
{
    BENCHMARK( "Load Russian" ) {
        TranslationManager manager;
        manager.LoadDocuments( std::vector<std::string> {"./lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo"} );
        return manager.Translate( "battery" );
    };
}

TEST_CASE( "TranslationManager translate benchmark", "[.][benchmark][translations]" )
{
    TranslationManager manager;

    // Russian
    REQUIRE( file_exist( fs::u8path( "./lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo" ) ) );
    manager.LoadDocuments( std::vector<std::string> {"./lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo"} );
    REQUIRE( strcmp( manager.Translate( "battery" ), "battery" ) != 0 );
    BENCHMARK( "Russian" ) {
        return manager.Translate( "battery" );
    };

    // Chinese
    REQUIRE( file_exist( fs::u8path( "./lang/mo/zh_CN/LC_MESSAGES/cataclysm-dda.mo" ) ) );
    manager.LoadDocuments( std::vector<std::string> {"./lang/mo/zh_CN/LC_MESSAGES/cataclysm-dda.mo"} );
    REQUIRE( strcmp( manager.Translate( "battery" ), "battery" ) != 0 );
    BENCHMARK( "Chinese" ) {
        return manager.Translate( "battery" );
    };

    // English
    manager.LoadDocuments( std::vector<std::string>() );
    REQUIRE( strcmp( manager.Translate( "battery" ), "battery" ) == 0 );
    BENCHMARK( "English" ) {
        return manager.Translate( "battery" );
    };
}

TEST_CASE( "TranslationManager translates message", "[translations]" )
{
    std::vector<std::string> files{"./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo"};
    TranslationManager manager;
    manager.LoadDocuments( files );
    std::string translated = manager.Translate( "battery" );
    CHECK( translated == "батарейка" );
}

TEST_CASE( "TranslationManager returns untranslated message as is", "[translations]" )
{
    std::vector<std::string> files{"./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo"};
    TranslationManager manager;
    manager.LoadDocuments( files );
    const char *message = "__UnTrAnSlAtEd!!!__#";
    std::string translated = manager.Translate( message );
    CHECK( translated == message );
}

TEST_CASE( "TranslationManager returns empty string as is", "[translations]" )
{
    std::vector<std::string> files{"./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo"};
    TranslationManager manager;
    manager.LoadDocuments( files );
    std::string translated = manager.Translate( "" );
    CHECK( translated.empty() );
}

TEST_CASE( "TranslationManager translates message with context", "[translations]" )
{
    std::vector<std::string> files{"./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo"};
    TranslationManager manager;
    manager.LoadDocuments( files );
    std::string translated_weapon_pike = manager.TranslateWithContext( "weapon", "pike" );
    CHECK( translated_weapon_pike == "пика" );
    std::string translated_fish_pike = manager.TranslateWithContext( "fish", "pike" );
    CHECK( translated_fish_pike == "щука" );
    std::string untranslated_unknown_pike = manager.TranslateWithContext( "!@#$", "pike" );
    CHECK( untranslated_unknown_pike == "pike" );
}

static void CheckPluralEvaluation( const TranslationPluralRulesEvaluator &evaluator,
                                   const std::function<std::size_t( std::size_t )> &ground_truth )
{
    for( std::size_t n = 0; n < 200; n++ ) {
        CHECK( evaluator.Evaluate( n ) == ground_truth( n ) );
    }
}

TEST_CASE( "TranslationPluralRulesEvaluator", "[translations]" )
{
    SECTION( "Arabic" ) {
        auto arabic_ground_truth = []( std::size_t n ) {
            return n == 0 ? 0 : n == 1 ? 1 : n == 2 ? 2 : n % 100 >= 3 && n % 100 <= 10 ? 3 : n % 100 >= 11 &&
                   n % 100 <= 99 ? 4 : 5;
        };
        const std::string arabic_rules =
            // NOLINTNEXTLINE(cata-text-style)
            "Plural-Forms: nplurals=6; plural=n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5;";
        TranslationPluralRulesEvaluator arabic_evaluator( arabic_rules );
        CheckPluralEvaluation( arabic_evaluator, arabic_ground_truth );
    }
    SECTION( "CJK" ) {
        auto cjk_ground_truth = []( std::size_t ) {
            return 0;
        };
        // NOLINTNEXTLINE(cata-text-style)
        const std::string cjk_rules = "Plural-Forms: nplurals=1; plural=0;";
        TranslationPluralRulesEvaluator cjk_evaluator( cjk_rules );
        CheckPluralEvaluation( cjk_evaluator, cjk_ground_truth );
    }
    SECTION( "Spanish" ) {
        auto spanish_ground_truth = []( std::size_t n ) {
            return n != 1;
        };
        // NOLINTNEXTLINE(cata-text-style)
        const std::string spanish_rules = "Plural-Forms: nplurals=2; plural=(n != 1);";
        TranslationPluralRulesEvaluator spanish_evaluator( spanish_rules );
        CheckPluralEvaluation( spanish_evaluator, spanish_ground_truth );
    }
    SECTION( "Russian" ) {
        auto russian_ground_truth = []( std::size_t n ) {
            return n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 12 ||
                    n % 100 > 14 ) ? 1 : n % 10 == 0 || ( n % 10 >= 5 && n % 10 <= 9 ) || ( n % 100 >= 11 &&
                            n % 100 <= 14 ) ? 2 : 3;
        };
        const std::string russian_rules =
            // NOLINTNEXTLINE(cata-text-style)
            "Plural-Forms: nplurals=4; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<12 || n%100>14) ? 1 : n%10==0 || (n%10>=5 && n%10<=9) || (n%100>=11 && n%100<=14)? 2 : 3);";
        TranslationPluralRulesEvaluator russian_evaluator( russian_rules );
        CheckPluralEvaluation( russian_evaluator, russian_ground_truth );
    }
}

TEST_CASE( "TranslationManager translates plural messages", "[translations]" )
{
    SECTION( "English" ) {
        std::vector<std::string> files;
        TranslationManager manager;
        manager.LoadDocuments( files );
        std::string translated_0_battery = manager.TranslatePlural( "battery", "batteries", 0 );
        CHECK( translated_0_battery == "batteries" );
        std::string translated_1_battery = manager.TranslatePlural( "battery", "batteries", 1 );
        CHECK( translated_1_battery == "battery" );
        std::string translated_2_battery = manager.TranslatePlural( "battery", "batteries", 2 );
        CHECK( translated_2_battery == "batteries" );
        std::string translated_3_battery = manager.TranslatePlural( "battery", "batteries", 3 );
        CHECK( translated_3_battery == "batteries" );
        std::string translated_4_battery = manager.TranslatePlural( "battery", "batteries", 4 );
        CHECK( translated_4_battery == "batteries" );
    }

    SECTION( "Russian" ) {
        std::vector<std::string> files{"./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo"};
        TranslationManager manager;
        manager.LoadDocuments( files );
        std::string translated_0_battery = manager.TranslatePlural( "battery", "batteries", 0 );
        CHECK( translated_0_battery == "батареек" );
        std::string translated_1_battery = manager.TranslatePlural( "battery", "batteries", 1 );
        CHECK( translated_1_battery == "батарейка" );
        std::string translated_2_battery = manager.TranslatePlural( "battery", "batteries", 2 );
        CHECK( translated_2_battery == "батарейки" );
        std::string translated_3_battery = manager.TranslatePlural( "battery", "batteries", 3 );
        CHECK( translated_3_battery == "батарейки" );
        std::string translated_4_battery = manager.TranslatePlural( "battery", "batteries", 4 );
        CHECK( translated_4_battery == "батарейки" );
    }
}

TEST_CASE( "TranslationPluralRulesEvaluatorPerformance", "[.][benchmark][translations]" )
{
    TranslationManager manager;
    manager.LoadDocuments( std::vector<std::string> {"./lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo"} );
    REQUIRE( strcmp( manager.TranslatePlural( "battery", "batteries", 1 ),
                     "батарейка" ) == 0 );
    BENCHMARK( "Russian plural rules evaluation" ) {
        return manager.TranslatePlural( "battery", "batteries", 1 );
    };
    manager.LoadDocuments( std::vector<std::string>() );
    REQUIRE( strcmp( manager.TranslatePlural( "battery", "batteries", 1 ), "battery" ) == 0 );
    BENCHMARK( "English plural rules evaluation" ) {
        return manager.TranslatePlural( "battery", "batteries", 1 );
    };
    manager.LoadDocuments( std::vector<std::string> {"./lang/mo/zh_CN/LC_MESSAGES/cataclysm-dda.mo"} );
    REQUIRE( strcmp( manager.TranslatePlural( "battery", "batteries", 1 ), "电池" ) == 0 );
    BENCHMARK( "Chinese plural rules evaluation" ) {
        return manager.TranslatePlural( "battery", "batteries", 1 );
    };
}

#endif
