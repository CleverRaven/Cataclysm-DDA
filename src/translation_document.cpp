#if defined(LOCALIZE)

#include "debug.h"
#include "filesystem.h"
#include "string_formatter.h"
#include "translation_document.h"
#include "translation_plural_evaluator.h"

InvalidTranslationDocumentException::InvalidTranslationDocumentException(
    const std::string &path,
    const std::string &message )
{
    this->error_message = string_format( "Invalid MO document %1$s: %2$s", path, message );
}

const char *InvalidTranslationDocumentException::what() const noexcept
{
    return error_message.c_str();
}

std::uint8_t TranslationDocument::GetByte( const std::size_t byteIndex ) const
{
    return static_cast<std::uint8_t>( data.at( byteIndex ) );
}

std::uint32_t TranslationDocument::GetUint32BE( const std::size_t byteIndex ) const
{
    std::uint32_t a = GetByte( byteIndex );
    std::uint32_t b = GetByte( byteIndex + 1 );
    std::uint32_t c = GetByte( byteIndex + 2 );
    std::uint32_t d = GetByte( byteIndex + 3 );
    return ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
}

std::uint32_t TranslationDocument::GetUint32LE( const std::size_t byteIndex ) const
{
    std::uint32_t a = GetByte( byteIndex + 3 );
    std::uint32_t b = GetByte( byteIndex + 2 );
    std::uint32_t c = GetByte( byteIndex + 1 );
    std::uint32_t d = GetByte( byteIndex );
    return ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
}

std::uint32_t TranslationDocument::GetUint32( const std::size_t byteIndex ) const
{
    return ( this->*GetUint32FPtr )( byteIndex );
}

const char *TranslationDocument::GetString( const std::size_t byteIndex ) const
{
    return data.c_str() + byteIndex;
}

std::size_t TranslationDocument::EvaluatePluralForm( std::size_t n ) const
{
    if( plural_rules ) {
        return plural_rules->Evaluate( n );
    } else {
        return 0;
    }
}

TranslationDocument::TranslationDocument( const std::string &path )
{
    this->path = path;
    cata::ifstream fin( fs::u8path( path ), std::ios::in | std::ios::binary );
    if( !fin ) {
        throw InvalidTranslationDocumentException( path, "unable to read the file" );
    }
    const std::uintmax_t file_size = fs::file_size( path );
    constexpr std::size_t max_file_size = 50 * 1024 * 1024;
    if( file_size < 20 ) {
        throw InvalidTranslationDocumentException( path, "file too small" );
    }
    if( file_size > max_file_size ) {
        throw InvalidTranslationDocumentException( path, "file too large" );
    }
    data = std::string( ( std::istreambuf_iterator<char>( fin ) ), std::istreambuf_iterator<char>() );
    if( data.size() != file_size ) {
        throw InvalidTranslationDocumentException( path, "did not read the entire file" );
    }
    if( GetByte( 0 ) == 0x95U &&
        GetByte( 1 ) == 0x04U &&
        GetByte( 2 ) == 0x12U &&
        GetByte( 3 ) == 0xdeU ) {
        endianness = Endianness::BigEndian;
        GetUint32FPtr = &TranslationDocument::GetUint32BE;
    } else if( GetByte( 0 ) == 0xdeU &&
               GetByte( 1 ) == 0x12U &&
               GetByte( 2 ) == 0x04U &&
               GetByte( 3 ) == 0x95U ) {
        endianness = Endianness::LittleEndian;
        GetUint32FPtr = &TranslationDocument::GetUint32LE;
    } else {
        throw InvalidTranslationDocumentException( path, "does not contain MO magic number" );
    }
    const std::uint32_t version = GetUint32( 4 );
    constexpr std::uint32_t supported_version = 0;
    if( version != supported_version ) {
        throw InvalidTranslationDocumentException( path,
                string_format( "unsupported MO format revision %zu, expect %zu", version, supported_version ) );
    }
    number_of_strings = GetUint32( 8 );
    original_strings_table_offset = GetUint32( 12 );
    translated_strings_table_offset = GetUint32( 16 );
    if( original_strings_table_offset + 8ULL * number_of_strings > data.size() ) {
        throw InvalidTranslationDocumentException( path,
                string_format( "original strings table offset %zu with %zu entries exceeds buffer size %zu",
                               original_strings_table_offset, number_of_strings, data.size() ) );
    }
    if( translated_strings_table_offset + 8ULL * number_of_strings > data.size() ) {
        throw InvalidTranslationDocumentException( path,
                string_format( "translated strings table offset %zu with %zu entries exceeds buffer size %zu",
                               translated_strings_table_offset, number_of_strings, data.size() ) );
    }
    original_offsets.reserve( number_of_strings );
    translated_offsets.reserve( number_of_strings );
    for( std::size_t i = 0; i < number_of_strings; i++ ) {
        std::size_t length = GetUint32( original_strings_table_offset + 8 * i );
        std::size_t offset = GetUint32( original_strings_table_offset + 8 * i + 4 );
        if( offset >= data.size() || length >= data.size() || offset + length >= data.size() ) {
            throw InvalidTranslationDocumentException( path,
                    string_format( "original string %zu offset %zu with length %zu exceeds buffer size %zu",
                                   i, offset, length, data.size() ) );
        }
        if( data[offset + length] != '\0' ) {
            throw InvalidTranslationDocumentException( path,
                    string_format( "original string %zu offset %zu with length %zu not terminated by '\\0'",
                                   i, offset, length ) );
        }
        original_offsets.emplace_back( offset );
    }
    for( std::size_t i = 0; i < number_of_strings; i++ ) {
        std::vector<std::size_t> offsets;
        std::size_t length = GetUint32( translated_strings_table_offset + 8 * i );
        std::size_t offset = GetUint32( translated_strings_table_offset + 8 * i + 4 );
        if( offset >= data.size() || length >= data.size() || offset + length >= data.size() ) {
            throw InvalidTranslationDocumentException( path,
                    string_format( "translated string %zu offset %zu with length %zu exceeds buffer size %zu",
                                   i, offset, length, data.size() ) );
        }
        if( data[offset + length] != '\0' ) {
            throw InvalidTranslationDocumentException( path,
                    string_format( "translated string %zu offset %zu with length %zu not terminated by '\\0'",
                                   i, offset, length ) );
        }
        offsets.emplace_back( offset );
        for( std::size_t idx = offset; idx + 1 < offset + length; idx++ ) {
            if( data[idx] == '\0' ) {
                offsets.emplace_back( idx + 1 );
            }
        }
        translated_offsets.emplace_back( offsets );
    }
    const std::string metadata( GetTranslatedString( 0 ) );
    const std::string plural_rules_header( "Plural-Forms:" );
    std::size_t plural_rules_header_pos = metadata.find( plural_rules_header );
    if( plural_rules_header_pos != std::string::npos ) {
        plural_rules_header_pos += plural_rules_header.length();
        std::size_t new_line_pos = metadata.find( '\n', plural_rules_header_pos );
        std::string plural_rules_str;
        if( new_line_pos != std::string::npos ) {
            plural_rules_str = metadata.substr( plural_rules_header_pos,
                                                new_line_pos - plural_rules_header_pos );
        } else {
            plural_rules_str = metadata.substr( plural_rules_header_pos );
        }
        try {
            plural_rules = std::make_unique<TranslationPluralRulesEvaluator>( plural_rules_str );
        } catch( TranslationPluralRulesEvaluator::HeaderError &e ) {
            DebugLog( D_ERROR, DC_ALL ) << "Error reading plural forms rules header of " << path << " : " <<
                                        e.what();
            throw InvalidTranslationDocumentException( path, string_format( "Plural forms HeaderError: %s",
                    e.what() ) );
        } catch( TranslationPluralRulesEvaluator::LexicalError &e ) {
            DebugLog( D_ERROR, DC_ALL ) << "Invalid plural forms rules expression in " << path << " : " <<
                                        e.what();
            throw InvalidTranslationDocumentException( path, string_format( "Plural forms LexicalError: %s",
                    e.what() ) );
        } catch( TranslationPluralRulesEvaluator::ParseError &e ) {
            DebugLog( D_ERROR, DC_ALL ) << "Error parsing plural forms rules expression in " << path << " : "
                                        << e.what();
            throw InvalidTranslationDocumentException( path, string_format( "Plural forms ParseError: %s",
                    e.what() ) );
        }
    }
}

std::size_t TranslationDocument::Count() const
{
    return number_of_strings;
}

const char *TranslationDocument::GetOriginalString( const std::size_t index ) const
{
    return GetString( original_offsets[index] );
}

const char *TranslationDocument::GetTranslatedString( const std::size_t index ) const
{
    return GetString( translated_offsets[index][0] );
}

const char *TranslationDocument::GetTranslatedStringPlural( const std::size_t index,
        std::size_t n ) const
{
    std::size_t plural_form = EvaluatePluralForm( n );
    if( plural_form >= translated_offsets[index].size() ) {
        DebugLog( D_ERROR, DC_ALL ) << "Plural forms expression evaluated out-of-bound at string entry " <<
                                    index << " with n=" << n;
        return GetString( translated_offsets[index][0] );
    }
    return GetString( translated_offsets[index][plural_form] );
}

#endif // defined(LOCALIZE)
