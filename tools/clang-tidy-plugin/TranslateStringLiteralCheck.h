#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_TRANSLATESTRINGLITERALCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_TRANSLATESTRINGLITERALCHECK_H

#include <string>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"
#include "ClangTidyCheck.h"

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class TranslateStringLiteralCheck : public ClangTidyCheck
{
    private:
        static std::string pruneFormatStrings( const std::string &str );
        static std::string pruneTags( const std::string &str );
        static std::string removeSubstrings( const std::string &str,
                                             const std::unordered_set<std::string> &substrings );
        static std::string extractText( const std::string &str );
        static bool isUnit( const std::string &str );
        static bool containsTranslatableText( const std::string &str );
    public:
        TranslateStringLiteralCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_TRANSLATESTRINGLITERALCHECK_H
