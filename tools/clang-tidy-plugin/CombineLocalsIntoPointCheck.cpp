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

namespace clang::tidy::cata
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
            if( ND->getIdentifier() && ND->getName() == Name ) {
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

    const Decl *NextDecl = XDecl->getNextDeclInContext();
    const VarDecl *YDecl = dyn_cast_or_null<VarDecl>( NextDecl );
    if( !YDecl ) {
        return;
    }
    NextDecl = YDecl->getNextDeclInContext();
    const VarDecl *ZDecl = dyn_cast_or_null<VarDecl>( NextDecl );

    // Avoid altering bool variables
    if( StringRef( XDecl->getType().getAsString() ).ends_with( "ool" ) ) {
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

    // Only one modification per function
    const FunctionDecl *Func = getContainingFunction( Result, XDecl );

    if( !Func ) {
        return;
    }

    auto FuncInsert = Check.alteredFunctions.emplace(
                          Func, FunctionReplacementData{ XDecl, YDecl, ZDecl, {}, {} } );
    if( !FuncInsert.second ) {
        return;
    }
    FunctionReplacementData &FuncReplacement = FuncInsert.first->second;

    if( ZDecl ) {
        const Stmt *ZGrandparentStmt = GetGrandparent( ZDecl );

        if( XGrandparentStmt != ZGrandparentStmt ) {
            ZDecl = nullptr;
        }
    }

    const Expr *XInit = XDecl->getAnyInitializer();
    const Expr *YInit = YDecl->getAnyInitializer();
    const Expr *ZInit = ZDecl ? ZDecl->getAnyInitializer() : nullptr;

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
    FuncReplacement.Fixits.push_back( FixItHint::CreateReplacement( RangeToReplace, Replacement ) );
}

static void CheckDeclRef( CombineLocalsIntoPointCheck &Check,
                          const MatchFinder::MatchResult &Result )
{
    const DeclRefExpr *DeclRef = Result.Nodes.getNodeAs<DeclRefExpr>( "declref" );
    const VarDecl *Decl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    if( !DeclRef || !Decl ) {
        return;
    }

    auto NewName = Check.usageReplacements.find( Decl );
    if( NewName == Check.usageReplacements.end() ) {
        return;
    }

    if( getText( Result, DeclRef ) != Decl->getName() ) {
        return;
    }

    auto Func = Check.alteredFunctions.find( getContainingFunction( Result, Decl ) );
    if( Func == Check.alteredFunctions.end() ) {
        Check.diag( DeclRef->getBeginLoc(), "Internal check error finding replacement data for ref",
                    DiagnosticIDs::Error );
        return;
    }
    FunctionReplacementData &Replacement = Func->second;
    Replacement.RefReplacements.push_back(
        DeclRefReplacementData{ DeclRef, Decl, NewName->second } );
    Replacement.Fixits.push_back(
        FixItHint::CreateReplacement( DeclRef->getSourceRange(), NewName->second ) );
}

void CombineLocalsIntoPointCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
    CheckDeclRef( *this, Result );
}

void CombineLocalsIntoPointCheck::onEndOfTranslationUnit()
{
    for( const std::pair<const FunctionDecl *const, FunctionReplacementData> &p :
         alteredFunctions ) {
        const FunctionReplacementData &data = p.second;
        std::string Message = data.ZDecl ?
                              "Variables %0, %1, and %2 could be combined into a single 'tripoint' variable." :
                              "Variables %0 and %1 could be combined into a single 'point' variable.";
        {
            DiagnosticBuilder Diag = diag( data.XDecl->getBeginLoc(), Message )
                                     << data.XDecl << data.YDecl << data.ZDecl;
            for( const FixItHint &FixIt : data.Fixits ) {
                Diag << FixIt;
            }
        }
        diag( data.YDecl->getLocation(), "%0 variable", DiagnosticIDs::Note ) << data.YDecl;
        if( data.ZDecl ) {
            diag( data.ZDecl->getLocation(), "%0 variable", DiagnosticIDs::Note )
                    << data.ZDecl;
        }
        for( const DeclRefReplacementData &RefReplacment : data.RefReplacements ) {
            diag( RefReplacment.DeclRef->getBeginLoc(), "Update %0 to '%1'.",
                  DiagnosticIDs::Note )
                    << RefReplacment.Decl << RefReplacment.NewName;
        }
    }
}

} // namespace clang::tidy::cata
