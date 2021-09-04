#pragma once
#ifndef CATA_SRC_TRANSLATION_DOCUMENT_H
#define CATA_SRC_TRANSLATION_DOCUMENT_H

#if defined(LOCALIZE)

#include <string>
#include <vector>

class InvalidTranslationDocumentException : public std::exception
{
    private:
        std::string path;
        std::string error_message;
    public:
        explicit InvalidTranslationDocumentException( const std::string &path );
        const char *GetPath() const noexcept;
        const char *what() const noexcept override;
};

enum class Endianness {
    BigEndian,
    LittleEndian
};

/**
 * Represents a GNU gettext Message Object (.mo) document
 * Format specification: https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html
 */
class TranslationDocument
{
    private:
        std::string path;
        std::size_t number_of_strings; // N
        std::size_t original_strings_table_offset; // O
        std::size_t translated_strings_table_offset; // T
        // std::size_t hash_table_size; // S
        // std::size_t hash_table_offset; // H
        std::string data;
        Endianness endianness;
        std::vector<std::size_t> original_offsets;
        std::vector<std::vector<std::size_t>> translated_offsets;

        std::uint8_t GetByte( const std::size_t byteIndex ) const;
        std::uint32_t GetUint32BE( const std::size_t byteIndex ) const;
        std::uint32_t GetUint32LE( const std::size_t byteIndex ) const;
        std::uint32_t ( TranslationDocument::*GetUint32FPtr )( const std::size_t ) const;
        std::uint32_t GetUint32( const std::size_t byteIndex ) const;
        const char *GetString( const std::size_t byteIndex ) const;
        std::size_t EvaluatePluralForm( std::size_t n ) const;
    public:
        TranslationDocument() = delete;
        explicit TranslationDocument( const std::string &path );

        std::size_t Count() const;
        const char *GetOriginalString( const std::size_t index ) const;
        const char *GetTranslatedString( const std::size_t index ) const;
        const char *GetTranslatedStringPlural( const std::size_t index, std::size_t n ) const;
};

#endif

#endif // CATA_SRC_TRANSLATION_DOCUMENT_H
