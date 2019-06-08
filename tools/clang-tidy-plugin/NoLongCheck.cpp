#include "NoLongCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace cata {

void NoLongCheck::registerMatchers(MatchFinder *Finder) {
    using TypeMatcher = clang::ast_matchers::internal::Matcher<QualType>;
    const TypeMatcher isLong = anyOf(asString("long"), asString("unsigned long"));
    Finder->addMatcher(valueDecl(hasType(isLong)).bind("decl"), this);
    Finder->addMatcher(functionDecl(returns(isLong)).bind("return"), this);
}

static void CheckDecl(NoLongCheck &Check, const MatchFinder::MatchResult &Result) {
    const ValueDecl *MatchedDecl = Result.Nodes.getNodeAs<ValueDecl>("decl");
    if( !MatchedDecl || !MatchedDecl->getLocation().isValid() ) {
        return;
    }
    QualType Type = MatchedDecl->getType().getUnqualifiedType();
    if( Type.getAsString() == "long" ) {
        Check.diag(
            MatchedDecl->getLocation(), "Variable %0 declared as long.  "
            "Prefer int or int64_t.") << MatchedDecl;
    } else {
        Check.diag(
            MatchedDecl->getLocation(), "Variable %0 declared as unsigned long.  "
            "Prefer unsigned int or uint64_t.") << MatchedDecl;
    }
}

static void CheckReturn(NoLongCheck &Check, const MatchFinder::MatchResult &Result) {
    const FunctionDecl *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("return");
    if( !MatchedDecl || !MatchedDecl->getLocation().isValid() ) {
        return;
    }
    QualType Type = MatchedDecl->getReturnType().getUnqualifiedType();
    if( Type.getAsString() == "long" ) {
        Check.diag(
            MatchedDecl->getLocation(), "Function %0 declared as returning long.  "
            "Prefer int or int64_t.") << MatchedDecl;
    } else {
        Check.diag(
            MatchedDecl->getLocation(), "Function %0 declared as returning unsigned long.  "
            "Prefer unsigned int or uint64_t.") << MatchedDecl;
    }
}

void NoLongCheck::check(const MatchFinder::MatchResult &Result) {
    CheckDecl(*this, Result);
    CheckReturn(*this, Result);
}

} // namespace cata
} // namespace tidy
} // namespace clang
