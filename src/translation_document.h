#pragma once
#ifndef CATA_SRC_TRANSLATION_DOCUMENT_H
#define CATA_SRC_TRANSLATION_DOCUMENT_H

#if defined(LOCALIZE)

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "mmap_file.h"
#include "translation_plural_evaluator.h"

class InvalidTranslationDocumentException : public std::exception
{
    private:
        std::string error_message;
    public:
        explicit InvalidTranslationDocumentException( const std::string &path, const std::string &message );
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
        std::shared_ptr<mmap_file> mmap_message_object;
        std::size_t number_of_strings; // N
        std::size_t original_strings_table_offset; // O
        std::size_t translated_strings_table_offset; // T
        // std::size_t hash_table_size; // S
        // std::size_t hash_table_offset; // H
        const char *data;
        std::size_t data_size;
        Endianness endianness;
        std::vector<std::size_t> original_offsets;
        std::vector<std::vector<std::size_t>> translated_offsets;
        std::unique_ptr<TranslationPluralRulesEvaluator> plural_rules;

        std::uint8_t GetByte( std::size_t byteIndex ) const;
        std::uint32_t GetUint32BE( std::size_t byteIndex ) const;
        std::uint32_t GetUint32LE( std::size_t byteIndex ) const;
        std::uint32_t ( TranslationDocument::*GetUint32FPtr )( const std::size_t ) const;
        std::uint32_t GetUint32( std::size_t byteIndex ) const;
        const char *GetString( std::size_t byteIndex ) const;
        std::size_t EvaluatePluralForm( std::size_t n ) const;
    public:
        TranslationDocument() = delete;
        explicit TranslationDocument( const std::string &path );

        std::size_t Count() const;
        const char *GetOriginalString( std::size_t index ) const;
        const char *GetTranslatedString( std::size_t index ) const;
        const char *GetTranslatedStringPlural( std::size_t index, std::size_t n ) const;
};

#endif // defined(LOCALIZE)

#endif // CATA_SRC_TRANSLATION_DOCUMENT_H
