#pragma once
#ifndef CATA_SRC_TRANSLATION_MANAGER_H
#define CATA_SRC_TRANSLATION_MANAGER_H

#if defined(LOCALIZE)

#include <memory>
#include <unordered_set>
#include <vector>

#include "pimpl.h"

class TranslationManager
{
    private:
        class Impl;
        pimpl<Impl> impl;
    public:
        TranslationManager() = default;
        TranslationManager( const TranslationManager & ) = delete;
        TranslationManager( TranslationManager && ) = delete;
        static TranslationManager &GetInstance();
        std::unordered_set<std::string> GetAvailableLanguages();
        void SetLanguage( const std::string &language_code );
        std::string GetCurrentLanguage() const;
        void LoadDocuments( const std::vector<std::string> &files );

        const char *Translate( const std::string &message ) const;
        const char *Translate( const char *message ) const;
        const char *TranslatePlural( const char *singular, const char *plural, std::size_t n ) const;
        const char *TranslateWithContext( const char *context, const char *message ) const;
        const char *TranslatePluralWithContext( const char *context, const char *singular,
                                                const char *plural, std::size_t n ) const;
};

#endif // defined(LOCALIZE)

#endif // CATA_SRC_TRANSLATION_MANAGER_H
