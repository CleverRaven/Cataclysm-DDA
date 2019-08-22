#include "NoLongCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/STLExtras.h>
#include <string>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

namespace clang
{
class MacroArgs;
class MacroDefinition;
}  // namespace clang

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

class NoLongMacrosCallbacks : public PPCallbacks
{
    public:
        NoLongMacrosCallbacks( NoLongCheck *Check ) :
            Check( Check ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs * ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();
            if( MacroName == "LONG_MIN" || MacroName == "LONG_MAX" || MacroName == "ULONG_MAX" ) {
                Check->diag( Range.getBegin(), "Use of long-specific macro %0" ) << MacroName;
            }
        }
    private:
        NoLongCheck *Check;
};

void NoLongCheck::registerPPCallbacks( CompilerInstance &Compiler )
{
    Compiler.getPreprocessor().addPPCallbacks(
        llvm::make_unique<NoLongMacrosCallbacks>( this ) );
}

void NoLongCheck::registerMatchers( MatchFinder *Finder )
{
    using TypeMatcher = clang::ast_matchers::internal::Matcher<QualType>;
    const TypeMatcher isIntegerOrRef =
        qualType( anyOf( isInteger(), references( isInteger() ) ),
                  unless( autoType() ), unless( references( autoType() ) ) );
    Finder->addMatcher( valueDecl( hasType( isIntegerOrRef ) ).bind( "decl" ), this );
    Finder->addMatcher( functionDecl( returns( isIntegerOrRef ) ).bind( "return" ), this );
    Finder->addMatcher( cxxStaticCastExpr( hasDestinationType( isIntegerOrRef ) ).bind( "cast" ),
                        this );
}

static std::string AlternativesFor( QualType Type )
{
    Type = Type.getNonReferenceType();
    Type = Type.getLocalUnqualifiedType();
    std::string name = Type.getAsString();
    if( name == "long" ) {
        return "Prefer int or int64_t to long";
    } else if( name == "unsigned long" ) {
        return "Prefer unsigned int, size_t, or uint64_t to unsigned long";
    } else {
        return {};
    }
}

static void CheckDecl( NoLongCheck &Check, const MatchFinder::MatchResult &Result )
{
    const ValueDecl *MatchedDecl = Result.Nodes.getNodeAs<ValueDecl>( "decl" );
    if( !MatchedDecl || !MatchedDecl->getLocation().isValid() ) {
        return;
    }
    QualType Type = MatchedDecl->getType();
    std::string alternatives = AlternativesFor( Type );
    if( alternatives.empty() ) {
        return;
    }
    if( MatchedDecl->getName().startswith( "__" ) ) {
        // Can happen for e.g. compiler-generated code inside an implicitly
        // generated function
        return;
    }
    Decl::Kind contextKind = MatchedDecl->getDeclContext()->getDeclKind();
    if( contextKind == Decl::Function || contextKind == Decl::CXXMethod ||
        contextKind == Decl::CXXConstructor || contextKind == Decl::CXXConversion ||
        contextKind == Decl::CXXDestructor || contextKind == Decl::CXXDeductionGuide ) {
        TemplateSpecializationKind tsk =
            static_cast<const FunctionDecl *>(
                MatchedDecl->getDeclContext() )->getTemplateSpecializationKind();
        if( tsk == TSK_ImplicitInstantiation ) {
            // This happens for e.g. a parameter 'T a' to an instantiated
            // template function where T is long.  We don't want to report such
            // cases.
            return;
        }
    }
    Check.diag(
        MatchedDecl->getLocation(), "Variable %0 declared as %1.  %2." ) <<
                MatchedDecl << Type << alternatives;
}

static void CheckReturn( NoLongCheck &Check, const MatchFinder::MatchResult &Result )
{
    const FunctionDecl *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>( "return" );
    if( !MatchedDecl || !MatchedDecl->getLocation().isValid() ) {
        return;
    }
    QualType Type = MatchedDecl->getDeclaredReturnType();
    std::string alternatives = AlternativesFor( Type );
    if( alternatives.empty() ) {
        return;
    }
    if( MatchedDecl->isTemplateInstantiation() ) {
        return;
    }

    Decl::Kind contextKind = MatchedDecl->getDeclContext()->getDeclKind();
    if( contextKind == Decl::ClassTemplateSpecialization ) {
        TemplateSpecializationKind tsk =
            static_cast<const CXXRecordDecl *>(
                MatchedDecl->getDeclContext() )->getTemplateSpecializationKind();
        if( tsk == TSK_ImplicitInstantiation ) {
            // This happens for e.g. a parameter 'T a' to an instantiated
            // template function where T is long.  We don't want to report such
            // cases.
            return;
        }
    }
    Check.diag(
        MatchedDecl->getLocation(), "Function %0 declared as returning %1.  %2." ) <<
                MatchedDecl << Type << alternatives;
}

static void CheckCast( NoLongCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CXXStaticCastExpr *MatchedDecl = Result.Nodes.getNodeAs<CXXStaticCastExpr>( "cast" );
    if( !MatchedDecl ) {
        return;
    }
    QualType Type = MatchedDecl->getType();
    std::string alternatives = AlternativesFor( Type );
    if( alternatives.empty() ) {
        return;
    }
    SourceLocation location = MatchedDecl->getTypeInfoAsWritten()->getTypeLoc().getBeginLoc();
    Check.diag( location, "Static cast to %0.  %1." ) << Type << alternatives;
}

void NoLongCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
    CheckReturn( *this, Result );
    CheckCast( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
