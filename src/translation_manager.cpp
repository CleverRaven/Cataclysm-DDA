#include "translation_manager_impl.h"

TranslationManager &TranslationManager::GetInstance()
{
    static TranslationManager singleton;
    return singleton;
}

TranslationManager::TranslationManager()
{
    impl = std::make_unique<TranslationManagerImpl>();
}

std::unordered_set<std::string> TranslationManager::GetAvailableLanguages()
{
    return impl->GetAvailableLanguages();
}

void TranslationManager::SetLanguage( const std::string &language_code )
{
    impl->SetLanguage( language_code );
}

std::string TranslationManager::GetCurrentLanguage() const
{
    return impl->GetCurrentLanguage();
}

void TranslationManager::LoadDocuments( const std::vector<std::string> &files )
{
    impl->LoadDocuments( files );
}

const char *TranslationManager::Translate( const std::string &message ) const
{
    return impl->Translate( message );
}

const char *TranslationManager::Translate( const char *message ) const
{
    return impl->Translate( message );
}

const char *TranslationManager::TranslatePlural( const char *singular, const char *plural,
        std::size_t n ) const
{
    return impl->TranslatePlural( singular, plural, n );
}

const char *TranslationManager::TranslateWithContext( const char *context,
        const char *message ) const
{
    return impl->TranslateWithContext( context, message );
}

const char *TranslationManager::TranslatePluralWithContext( const char *context,
        const char *singular,
        const char *plural, std::size_t n ) const
{
    return impl->TranslatePluralWithContext( context, singular, plural, n );
}
