#include "cata_catch.h"
#include "filesystem.h"
#include "string_formatter.h"
#include "translation_document.h"
#include "translation_manager.h"

#if defined(LOCALIZE)

static void LoadMODocument( const char *path )
{
    volatile TranslationDocument document( path );
}

TEST_CASE( "TranslationDocument loads valid MO", "[translations]" )
{
    const char *path = "./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo";
    REQUIRE( file_exist( path ) );
    REQUIRE_NOTHROW( LoadMODocument( path ) );
}

TEST_CASE( "TranslationDocument rejects invalid MO", "[translations]" )
{
    const char *path = "./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/INVALID_RAND.mo";
    REQUIRE( file_exist( path ) );
    REQUIRE_THROWS_AS( LoadMODocument( path ), InvalidTranslationDocumentException );
}

TEST_CASE( "TranslationDocument loads all core MO", "[translations]" )
{
    const std::unordered_set<std::string> languages =
        TranslationManager::GetInstance().GetAvailableLanguages();
    for( const std::string &lang : languages ) {
        const std::string path = string_format( "./lang/mo/%s/LC_MESSAGES/cataclysm-dda.mo", lang );
        REQUIRE( file_exist( path ) );
        REQUIRE_NOTHROW( LoadMODocument( path.c_str() ) );
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
        return manager;
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

#endif
