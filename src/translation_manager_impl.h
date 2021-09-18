#pragma once
#ifndef CATA_SRC_TRANSLATION_MANAGER_IMPL_H
#define CATA_SRC_TRANSLATION_MANAGER_IMPL_H

#include <unordered_map>

#include "translation_document.h"
#include "translation_manager.h"

class TranslationManagerImpl : public TranslationManagerInterface
{
    private:
        std::vector<TranslationDocument> documents;
        std::unordered_map<std::string, std::pair<std::size_t, std::size_t>> string_to_document_no;

        std::unordered_map<std::string, std::vector<std::string>> mo_files;
        static std::string LanguageCodeOfPath( const std::string &path );
        void ScanTranslationDocuments();
        std::string ConstructContextualQuery( const char *context, const char *message ) const;
        void Reset();
        std::string current_language_code;
    public:
        TranslationManagerImpl();
        std::unordered_set<std::string> GetAvailableLanguages() override;
        void SetLanguage( const std::string &language_code ) override;
        std::string GetCurrentLanguage() const override;
        void LoadDocuments( const std::vector<std::string> &files ) override;

        const char *Translate( const std::string &message ) const override;
        const char *Translate( const char *message ) const override;
        const char *TranslatePlural( const char *singular, const char *plural,
                                     std::size_t n ) const override;
        const char *TranslateWithContext( const char *context, const char *message ) const override;
        const char *TranslatePluralWithContext( const char *context, const char *singular,
                                                const char *plural, std::size_t n ) const override;
};

#endif // CATA_SRC_TRANSLATION_MANAGER_IMPL_H
