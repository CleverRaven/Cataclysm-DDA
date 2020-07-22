#include "CombineLocalsIntoPointCheck.h"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <climits>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Casting.h>
#include <string>
#include <unordered_set>

#include "Utils.h"
#include "clang/Basic/OperatorKinds.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void CombineLocalsIntoPointCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl( hasType( isInteger() ), isXParam() ).bind( "xdecl" ),
        this
    );
    Finder->addMatcher(
        declRefExpr(
            hasDeclaration( varDecl( hasType( isInteger() ) ).bind( "decl" ) )
        ).bind( "declref" ),
        this
    );
}

static bool nameExistsInContext( const DeclContext *Context, const std::string &Name )
{
    for( const Decl *D : Context->decls() ) {
        if( const NamedDecl *ND = dyn_cast<NamedDecl>( D ) ) {
            if( ND->getName() == Name ) {
                return true;
            }
        }
    }
    return false;
}

static bool isKeyword( const std::string &S )
{
    static const std::unordered_set<std::string> keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
        "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char",
        "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval",
        "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield",
        "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit",
        "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
        "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator",
        "or", "or_eq", "private", "protected", "public", "reflexpr", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast",
        "struct", "switch", "synchronized", "template", "this", "thread_local", "throw", "true",
        "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq",
    };
    return keywords.count( S );
}

static void CheckDecl( CombineLocalsIntoPointCheck &Check, const MatchFinder::MatchResult &Result )
{
    const VarDecl *XDecl = Result.Nodes.getNodeAs<VarDecl>( "xdecl" );
    if( !XDecl ) {
        return;
    }

    // Only one modification per function
    if( !Check.alteredFunctions.insert( getContainingFunction( Result, XDecl ) ).second ) {
        return;
    }

    const Decl *NextDecl = XDecl->getNextDeclInContext();
    const VarDecl *YDecl = dyn_cast_or_null<VarDecl>( NextDecl );
    if( !YDecl ) {
        return;
    }
    NextDecl = YDecl->getNextDeclInContext();
    const VarDecl *ZDecl = dyn_cast_or_null<VarDecl>( NextDecl );

    // Avoid altering bool variables
    if( StringRef( XDecl->getType().getAsString() ).endswith( "ool" ) ) {
        return;
    }

    if( XDecl->getType().getTypePtr()->isUnsignedIntegerType() ) {
        return;
    }

    NameConvention NameMatcher( XDecl->getName() );

    if( !NameMatcher || NameMatcher.Match( YDecl->getName() ) != NameConvention::YName ) {
        return;
    }

    if( ZDecl && NameMatcher.Match( ZDecl->getName() ) != NameConvention::ZName ) {
        ZDecl = nullptr;
    }

    auto GetGrandparent = [&]( const Decl * D ) -> const Stmt* {
        const Stmt *ParentStmt = getParent<Stmt>( Result, D );
        if( !ParentStmt )
        {
            return nullptr;
        }
        return getParent<Stmt>( Result, ParentStmt );
    };

    // Verify that all the decls are in the same place
    const Stmt *XGrandparentStmt = GetGrandparent( XDecl );
    const Stmt *YGrandparentStmt = GetGrandparent( YDecl );

    if( XGrandparentStmt != YGrandparentStmt ) {
        return;
    }

    if( ZDecl ) {
        const Stmt *ZGrandparentStmt = GetGrandparent( ZDecl );

        if( XGrandparentStmt != ZGrandparentStmt ) {
            ZDecl = nullptr;
        }
    }

    const Expr *XInit = XDecl->getAnyInitializer();
    const Expr *YInit = YDecl->getInit();
    const Expr *ZInit = ZDecl ? ZDecl->getInit() : nullptr;

    if( !XInit || !YInit || ( ZDecl && !ZInit ) ) {
        return;
    }

    std::string NewVarName = NameMatcher.getRoot();

    if( !NewVarName.empty() && NewVarName.front() == '_' ) {
        NewVarName.erase( NewVarName.begin() );
    }

    if( !NewVarName.empty() && NewVarName.back() == '_' ) {
        NewVarName.pop_back();
    }

    if( NewVarName.empty() ) {
        NewVarName = "p";
    }

    if( !isalpha( NewVarName.front() ) ) {
        NewVarName = "p" + NewVarName;
    }

    // Ensure we don't use a keyword
    if( isKeyword( NewVarName ) ) {
        NewVarName += "_";
    }

    // Ensure we don't collide with an existing name
    const DeclContext *Context = XDecl->getDeclContext();
    if( nameExistsInContext( Context, NewVarName ) ) {
        for( int suffix = 2; ; ++suffix ) {
            std::string Candidate = NewVarName + std::to_string( suffix );
            if( !nameExistsInContext( Context, Candidate ) ) {
                NewVarName = Candidate;
                break;
            }
        }
    }

    Check.usageReplacements.emplace( XDecl, NewVarName + ".x" );
    Check.usageReplacements.emplace( YDecl, NewVarName + ".y" );
    if( ZDecl ) {
        Check.usageReplacements.emplace( ZDecl, NewVarName + ".z" );
    }

    // Construct replacement text
    std::string Replacement =
        ( "point " + NewVarName + "( " + getText( Result, XInit ) + ", " +
          getText( Result, YInit ) ).str();
    if( ZDecl ) {
        Replacement = ( "tri" + Replacement + ", " + getText( Result, ZInit ) ).str();
    }
    Replacement += " )";

    if( XDecl->isConstexpr() ) {
        Replacement = "constexpr " + Replacement;
    } else if( XDecl->getType().isConstQualified() ) {
        Replacement = "const " + Replacement;
    }

    if( XDecl->isStaticLocal() ) {
        Replacement = "static " + Replacement;
    }

    SourceLocation EndLoc = ZDecl ? ZDecl->getEndLoc() : YDecl->getEndLoc();
    SourceRange RangeToReplace( XDecl->getBeginLoc(), EndLoc );
    std::string Message = ZDecl ?
                          "Variables %0, %1, and %2 could be combined into a single 'tripoint' variable." :
                          "Variables %0 and %1 could be combined into a single 'point' variable.";
    Check.diag( XDecl->getBeginLoc(), Message )
            << XDecl << YDecl << ZDecl
            << FixItHint::CreateReplacement( RangeToReplace, Replacement );
    Check.diag( YDecl->getLocation(), "%0 variable", DiagnosticIDs::Note ) << YDecl;
    if( ZDecl ) {
        Check.diag( ZDecl->getLocation(), "%0 variable", DiagnosticIDs::Note ) << ZDecl;
    }
}

static void CheckDeclRef( CombineLocalsIntoPointCheck &Check,
                          const MatchFinder::MatchResult &Result )
{
    const DeclRefExpr *DeclRef = Result.Nodes.getNodeAs<DeclRefExpr>( "declref" );
    const VarDecl *Decl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    if( !DeclRef || !Decl ) {
        return;
    }

    auto Replacement = Check.usageReplacements.find( Decl );
    if( Replacement == Check.usageReplacements.end() ) {
        return;
    }

    if( getText( Result, DeclRef ) != Decl->getName() ) {
        return;
    }

    Check.diag( DeclRef->getBeginLoc(), "Update %0 to '%1'.", DiagnosticIDs::Note )
            << Decl << Replacement->second
            << FixItHint::CreateReplacement( DeclRef->getSourceRange(), Replacement->second );
}

void CombineLocalsIntoPointCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
    CheckDeclRef( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
