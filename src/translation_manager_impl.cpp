#if defined(LOCALIZE)

#include <cstring>

#include "cached_options.h"
#include "debug.h"
#include "filesystem.h"
#include "path_info.h"
#include "translations.h"
#include "translation_manager_impl.h"

std::string TranslationManagerImpl::LanguageCodeOfPath( const std::string &path )
{
    const std::size_t end = path.rfind( "/LC_MESSAGES" );
    if( end == std::string::npos ) {
        return std::string();
    }
    const std::size_t begin = path.rfind( '/', end - 1 ) + 1;
    if( begin == std::string::npos ) {
        return std::string();
    }
    return path.substr( begin, end - begin );
}

void TranslationManagerImpl::ScanTranslationDocuments()
{
    DebugLog( D_INFO, DC_ALL ) << "[i18n] Scanning core translations from " << locale_dir();
    DebugLog( D_INFO, DC_ALL ) << "[i18n] Scanning mod translations from " << PATH_INFO::user_moddir();
    std::vector<std::string> core_mo_dirs = get_files_from_path( "LC_MESSAGES", locale_dir(),
                                            true );
    std::vector<std::string> mods_mo_dirs = get_files_from_path( "LC_MESSAGES",
                                            PATH_INFO::user_moddir(),
                                            true );
    std::vector<std::string> mo_dirs;
    mo_dirs.insert( mo_dirs.end(), core_mo_dirs.begin(), core_mo_dirs.end() );
    mo_dirs.insert( mo_dirs.end(), mods_mo_dirs.begin(), mods_mo_dirs.end() );
    for( const auto &dir : mo_dirs ) {
        std::vector<std::string> mo_dir_files = get_files_from_path( ".mo", dir, false, true );
        for( const auto &file : mo_dir_files ) {
            const std::string lang = LanguageCodeOfPath( file );
            if( mo_files.count( lang ) == 0 ) {
                mo_files[lang] = std::vector<std::string>();
            }
            mo_files[lang].emplace_back( file );
        }
    }
}

void TranslationManagerImpl::Reset()
{
    documents.clear();
    string_to_document_no.clear();
}

TranslationManagerImpl::TranslationManagerImpl()
{
    current_language_code = "en";
}

std::unordered_set<std::string> TranslationManagerImpl::GetAvailableLanguages()
{
    if( mo_files.empty() ) {
        ScanTranslationDocuments();
    }
    std::unordered_set<std::string> languages;
    for( const auto &kv : mo_files ) {
        languages.insert( kv.first );
    }
    return languages;
}

void TranslationManagerImpl::SetLanguage( const std::string &language_code )
{
    if( mo_files.empty() ) {
        ScanTranslationDocuments();
    }
    if( language_code == current_language_code ) {
        return;
    }
    current_language_code = language_code;
    if( mo_files.count( current_language_code ) == 0 ) {
        Reset();
        return;
    }
    LoadDocuments( mo_files[current_language_code] );
}

std::string TranslationManagerImpl::GetCurrentLanguage() const
{
    return current_language_code;
}

void TranslationManagerImpl::LoadDocuments( const std::vector<std::string> &files )
{
    Reset();
    for( const std::string &file : files ) {
        try {
            // Skip loading MO files from TEST_DATA mods if not in test mode
            if( not test_mode ) {
                if( file.find( "TEST_DATA" ) != std::string::npos ) {
                    continue;
                }
            }
            if( file_exist( file ) ) {
                documents.emplace_back( file );
            }
        } catch( const InvalidTranslationDocumentException &e ) {
            DebugLog( D_ERROR, DC_ALL ) << e.what();
        }
    }
    for( std::size_t document = 0; document < documents.size(); document++ ) {
        for( std::size_t i = 0; i < documents[document].Count(); i++ ) {
            const char *message = documents[document].GetOriginalString( i );
            if( message[0] != '\0' ) {
                string_to_document_no[message] = std::make_pair( document, i );
            }
        }
    }
}

const char *TranslationManagerImpl::Translate( const std::string &message ) const
{
    auto it = string_to_document_no.find( message );
    if( it == string_to_document_no.end() ) {
        return message.c_str();
    }
    const std::size_t document = it->second.first;
    const std::size_t string_index = it->second.second;
    return documents[document].GetTranslatedString( string_index );
}

const char *TranslationManagerImpl::Translate( const char *message ) const
{
    auto it = string_to_document_no.find( message );
    if( it == string_to_document_no.end() ) {
        return message;
    }
    const std::size_t document = it->second.first;
    const std::size_t string_index = it->second.second;
    return documents[document].GetTranslatedString( string_index );
}

const char *TranslationManagerImpl::TranslatePlural( const char *singular, const char *plural,
        std::size_t n ) const
{
    auto it = string_to_document_no.find( singular );
    if( it == string_to_document_no.end() ) {
        if( n == 1 ) {
            return singular;
        } else {
            return plural;
        }
    }
    const std::size_t document = it->second.first;
    const std::size_t string_index = it->second.second;
    return documents[document].GetTranslatedStringPlural( string_index, n );
}

std::string TranslationManagerImpl::ConstructContextualQuery( const char *context,
        const char *message ) const
{
    std::string query;
    query.reserve( strlen( context ) + 1 + strlen( message ) );
    query.append( context );
    query.append( "\004" );
    query.append( message );
    return query;
}

const char *TranslationManagerImpl::TranslateWithContext( const char *context,
        const char *message ) const
{
    std::string query = ConstructContextualQuery( context, message );
    auto it = string_to_document_no.find( query );
    if( it == string_to_document_no.end() ) {
        return message;
    }
    const std::size_t document = it->second.first;
    const std::size_t string_index = it->second.second;
    return documents[document].GetTranslatedString( string_index );
}

const char *TranslationManagerImpl::TranslatePluralWithContext( const char *context,
        const char *singular,
        const char *plural,
        std::size_t n ) const
{
    std::string query = ConstructContextualQuery( context, singular );
    auto it = string_to_document_no.find( query );
    if( it == string_to_document_no.end() ) {
        if( n == 1 ) {
            return singular;
        } else {
            return plural;
        }
    }
    const std::size_t document = it->second.first;
    const std::size_t string_index = it->second.second;
    return documents[document].GetTranslatedStringPlural( string_index, n );
}

#endif // defined(LOCALIZE)
