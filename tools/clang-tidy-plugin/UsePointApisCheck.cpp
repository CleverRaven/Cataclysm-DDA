#include "UsePointApisCheck.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

#include "Utils.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void UsePointApisCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            forEachArgumentWithParam(
                expr().bind( "xarg" ),
                parmVarDecl(
                    anyOf( hasType( asString( "int" ) ), hasType( asString( "const int" ) ) ),
                    isXParam()
                ).bind( "xparam" )
            ),
            callee( functionDecl().bind( "callee" ) )
        ).bind( "call" ),
        this
    );
    Finder->addMatcher(
        cxxConstructExpr(
            forEachArgumentWithParam(
                expr().bind( "xarg" ),
                parmVarDecl(
                    anyOf( hasType( asString( "int" ) ), hasType( asString( "const int" ) ) ),
                    isXParam()
                ).bind( "xparam" )
            ),
            hasDeclaration( cxxMethodDecl( unless( ofClass( isPointType() ) ) ).bind( "callee" ) )
        ).bind( "constructorCall" ),
        this
    );
}

static bool doFunctionsMatch( const FunctionDecl *Callee, const FunctionDecl *OtherCallee,
                              unsigned int NumCoordParams, unsigned int SkipArgs,
                              unsigned int MinArg, bool IsTripoint )
{
    const unsigned int ExpectedNumParams = Callee->getNumParams() - ( NumCoordParams - 1 );

    if( OtherCallee->getNumParams() != ExpectedNumParams ) {
        return false;
    }
    // Check that arguments match up as expected
    unsigned int CalleeParamI = 0;
    unsigned int OtherCalleeParamI = 0;

    for( ; CalleeParamI < Callee->getNumParams(); ++CalleeParamI, ++OtherCalleeParamI ) {
        const ParmVarDecl *CalleeParam = Callee->getParamDecl( CalleeParamI );
        const ParmVarDecl *OtherCalleeParam =
            OtherCallee->getParamDecl( OtherCalleeParamI );

        if( CalleeParamI == MinArg - SkipArgs ) {
            std::string ShortTypeName = IsTripoint ? "tripoint" : "point";
            std::string ExpectedTypeName = "const struct " + ShortTypeName + " &";
            if( OtherCalleeParam->getType().getAsString() != ExpectedTypeName ) {
                return false;
            }
            CalleeParamI += NumCoordParams - 1;
        } else {
            // Compare the types as strings because if e.g. the two overloads
            // are function templates then the tmplate parameters will be
            // different types.
            if( CalleeParam->getType().getLocalUnqualifiedType().getAsString() !=
                OtherCalleeParam->getType().getLocalUnqualifiedType().getAsString() ) {
                return false;
            }
        }
    }

    return true;
}

static void CheckCall( UsePointApisCheck &Check, const MatchFinder::MatchResult &Result )
{
    const ParmVarDecl *XParam = Result.Nodes.getNodeAs<ParmVarDecl>( "xparam" );
    const Expr *XArg = Result.Nodes.getNodeAs<Expr>( "xarg" );
    const CallExpr *Call = Result.Nodes.getNodeAs<CallExpr>( "call" );
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCall" );
    const FunctionDecl *Callee = Result.Nodes.getNodeAs<FunctionDecl>( "callee" );
    if( !XParam || !XArg || !( Call || ConstructorCall ) || !Callee ) {
        return;
    }

    const Expr *YArg = nullptr;
    const Expr *ZArg = nullptr;
    unsigned int MinArg = UINT_MAX;
    unsigned int MaxArg = 0;

    unsigned int NumCallArgs = Call ? Call->getNumArgs() : ConstructorCall->getNumArgs();
    SourceLocation CallBeginLoc = Call ? Call->getBeginLoc() : ConstructorCall->getBeginLoc();
    auto GetCallArg = [&]( unsigned int Arg ) {
        return Call ? Call->getArg( Arg ) : ConstructorCall->getArg( Arg );
    };

    // For operator() calls there is an extra 'this' argument that doesn't
    // correspond to any parameter, so we need to skip over it.
    unsigned int SkipArgs = 0;
    if( Callee->getOverloadedOperator() == OO_Call ) {
        SkipArgs = 1;
    }

    if( NumCallArgs - SkipArgs > Callee->getNumParams() ) {
        Check.diag(
            CallBeginLoc,
            "Internal check error: call has more arguments (%0) than function has parameters (%1)"
        ) << Call->getNumArgs() << Callee->getNumParams();
        Check.diag( Callee->getLocation(), "called function %0", DiagnosticIDs::Note ) << Callee;
        return;
    }

    NameConvention NameMatcher( XParam->getName() );

    if( !NameMatcher ) {
        return;
    }

    for( unsigned int i = SkipArgs; i < NumCallArgs; ++i ) {
        const ParmVarDecl *Param = Callee->getParamDecl( i - SkipArgs );
        bool Matched = true;
        switch( NameMatcher.Match( Param->getName() ) ) {
            case NameConvention::XName:
                break;
            case NameConvention::YName:
                YArg = GetCallArg( i );
                break;
            case NameConvention::ZName:
                ZArg = GetCallArg( i );
                break;
            default:
                Matched = false;
        }

        if( Matched ) {
            MinArg = std::min( MinArg, i );
            MaxArg = std::max( MaxArg, i );
        }
    }

    if( !YArg ) {
        return;
    }

    const unsigned int NumCoordParams = ZArg ? 3 : 2;

    if( MaxArg - MinArg != NumCoordParams - 1 ) {
        // This means that the parameters are not contiguous, which means we
        // can't be sure we know what's going on.
        return;
    }

    const FunctionDecl *ContainingFunction = getContainingFunction(
                Result, Call ? static_cast<const Expr *>( Call ) : ConstructorCall );

    // Look for another overload of the called function with a point parameter
    // in the right spot.

    const FunctionDecl *NewCallee = nullptr;
    const DeclContext *Context = Callee->getDeclContext();
    for( const NamedDecl *OtherDecl : Context->lookup( Callee->getDeclName() ) ) {
        if( const FunctionDecl *OtherCallee = dyn_cast<FunctionDecl>( OtherDecl ) ) {
            if( OtherCallee == Callee || OtherCallee == ContainingFunction ) {
                continue;
            }

            if( doFunctionsMatch( Callee, OtherCallee, NumCoordParams, SkipArgs, MinArg,
                                  !!ZArg ) ) {
                NewCallee = OtherCallee;
                break;
            }
        }
        if( const FunctionTemplateDecl *OtherTmpl =
                dyn_cast<FunctionTemplateDecl>( OtherDecl ) ) {
            const FunctionTemplateDecl *Tmpl = Callee->getPrimaryTemplate();

            if( !Tmpl || Tmpl == OtherTmpl ) {
                continue;
            }

            if( doFunctionsMatch( Tmpl->getTemplatedDecl(), OtherTmpl->getTemplatedDecl(),
                                  NumCoordParams, SkipArgs, MinArg, !!ZArg ) ) {
                NewCallee = OtherTmpl->getTemplatedDecl();
                break;
            }
        }
    }

    if( !NewCallee ) {
        // No new overload available; no replacement to suggest
        return;
    }

    // Construct replacement text
    std::string Replacement =
        ( "point( " + getText( Result, XArg ) + ", " + getText( Result, YArg ) ).str();
    if( ZArg ) {
        Replacement = ( "tri" + Replacement + ", " + getText( Result, ZArg ) ).str();
    }
    Replacement += " )";

    // Construct range to be replaced
    while( isa<CXXDefaultArgExpr>( GetCallArg( MaxArg ) ) ) {
        --MaxArg;
        if( MaxArg == UINT_MAX ) {
            // We underflowed; that means every argument was defaulted.  In
            // this case, we don't want to change the call at all
            return;
        }
    }
    SourceRange SourceRangeToReplace( GetCallArg( MinArg )->getBeginLoc(),
                                      GetCallArg( MaxArg )->getEndLoc() );
    CharSourceRange CharRangeToReplace = Lexer::makeFileCharRange(
            CharSourceRange::getTokenRange( SourceRangeToReplace ), *Result.SourceManager,
            Check.getLangOpts() );

    std::string message =
        ZArg ? "Call to %0 could instead call overload using a tripoint parameter."
        : "Call to %0 could instead call overload using a point parameter.";

    Check.diag( CallBeginLoc, message )
            << Callee << FixItHint::CreateReplacement( CharRangeToReplace, Replacement );
    Check.diag( Callee->getLocation(), "current overload", DiagnosticIDs::Note );
    Check.diag( NewCallee->getLocation(), "alternate overload", DiagnosticIDs::Note );
}

void UsePointApisCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckCall( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
