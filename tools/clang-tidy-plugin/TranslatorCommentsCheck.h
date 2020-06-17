#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_TRANSLATORCOMMENTSCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_TRANSLATORCOMMENTSCHECK_H

#include <ClangTidy.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <set>

namespace clang
{
class CompilerInstance;

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class TranslatorCommentsCheck : public ClangTidyCheck
{
    public:
        TranslatorCommentsCheck( StringRef Name, ClangTidyContext *Context );

        void registerPPCallbacks( CompilerInstance &Compiler ) override;
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;

        std::set<SourceLocation> MarkedStrings;
        bool MatchingStarted;
    private:
        class TranslatorCommentsHandler;

        std::unique_ptr<TranslatorCommentsHandler> Handler;
        class TranslationMacroCallback;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_TRANSLATORCOMMENTSCHECK_H
