#pragma once
#ifndef CATA_SRC_TRANSLATION_MANAGER_H
#define CATA_SRC_TRANSLATION_MANAGER_H

#if defined(LOCALIZE)

#include <memory>
#include <unordered_set>

class TranslationManagerInterface
{
    public:
        virtual ~TranslationManagerInterface() = default;
        virtual std::unordered_set<std::string> GetAvailableLanguages() = 0;
        virtual void SetLanguage( const std::string &language_code ) = 0;
        virtual std::string GetCurrentLanguage() const = 0;
        virtual void LoadDocuments( const std::vector<std::string> &files ) = 0;

        virtual const char *Translate( const std::string &message ) const = 0;
        virtual const char *Translate( const char *message ) const = 0;
        virtual const char *TranslatePlural( const char *singular, const char *plural,
                                             std::size_t n ) const = 0;
        virtual const char *TranslateWithContext( const char *context, const char *message ) const = 0;
        virtual const char *TranslatePluralWithContext( const char *context, const char *singular,
                const char *plural, std::size_t n ) const = 0;
};

class TranslationManager
{
    private:
        std::unique_ptr<TranslationManagerInterface> impl;
    public:
        TranslationManager();
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

#endif

#endif // CATA_SRC_TRANSLATION_MANAGER_H
