#include "NoLongCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace cata {

void NoLongCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(valueDecl(hasType(asString("long"))).bind("decl"), this);
  Finder->addMatcher(valueDecl(hasType(asString("unsigned long"))).bind("decl"), this);
}

void NoLongCheck::check(const MatchFinder::MatchResult &Result) {
  const ValueDecl *MatchedDecl = Result.Nodes.getNodeAs<ValueDecl>("decl");
  if( !MatchedDecl || !MatchedDecl->getLocation().isValid() ) {
      return;
  }
  QualType Type = MatchedDecl->getType().getUnqualifiedType();
  if( Type.getAsString() == "long" ) {
    diag(MatchedDecl->getLocation(), "Variable %0 declared as long.  Prefer int or int64_t.")
         << MatchedDecl;
  } else {
    diag(MatchedDecl->getLocation(), "Variable %0 declared as unsigned long.  "
         "Prefer unsigned int or uint64_t.") << MatchedDecl;
  }
}

} // namespace cata
} // namespace tidy
} // namespace clang
