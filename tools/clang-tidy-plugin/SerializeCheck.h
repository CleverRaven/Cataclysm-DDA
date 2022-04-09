#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_SERIALIZECHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_SERIALIZECHECK_H

#include <unordered_map>

#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"
#include "ClangTidyCheck.h"
#include "Utils.h"

namespace clang
{
class CompilerInstance;

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class SerializeCheck : public ClangTidyCheck
{
    public:
        SerializeCheck( StringRef Name, ClangTidyContext *Context );

        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;
    private:
        const SourceManager *sm_ = nullptr;
        std::unordered_map<const CXXMethodDecl *, std::vector<const FieldDecl *>> mentioned_decls_;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_SERIALIZECHECK_H
