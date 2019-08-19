#include "UseNamedPointConstantsCheck.h"

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

static auto isInteger( const std::string &bind )
{
    return expr(
               anyOf(
                   integerLiteral(),
                   unaryOperator(
                       anyOf( hasOperatorName( "+" ), hasOperatorName( "-" ) ),
                       hasUnaryOperand( integerLiteral() )
                   )
               )
           ).bind( bind );
}

static auto testWhetherParentIsVarDecl()
{
    return expr(
               anyOf(
                   hasParent( varDecl().bind( "parentVarDecl" ) ),
                   anything()
               )
           );
}

void UseNamedPointConstantsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( isPointConstructor().bind( "constructorDecl" ) ),
            testWhetherConstructingTemporary(),
            testWhetherParentIsVarDecl(),
            argumentCountIs( 2 ),
            hasArgument( 0, isInteger( "xexpr" ) ),
            hasArgument( 1, isInteger( "yexpr" ) )
        ).bind( "constructorCall" ),
        this
    );
    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( isPointConstructor().bind( "constructorDecl" ) ),
            testWhetherConstructingTemporary(),
            testWhetherParentIsVarDecl(),
            argumentCountIs( 3 ),
            hasArgument( 0, isInteger( "xexpr" ) ),
            hasArgument( 1, isInteger( "yexpr" ) ),
            hasArgument( 2, isInteger( "zexpr" ) )
        ).bind( "constructorCall" ),
        this
    );
}

static const std::map<std::pair<int, int>, std::string> PointConstants = {
    { { 0, 0 }, "point_zero" },
    { { 0, -1 }, "point_north" },
    { { 1, -1 }, "point_north_east" },
    { { 1, 0 }, "point_east" },
    { { 1, 1 }, "point_south_east" },
    { { 0, 1 }, "point_south" },
    { { -1, 1 }, "point_south_west" },
    { { -1, 0 }, "point_west" },
    { { -1, -1 }, "point_north_west" },
};

static const std::map<std::tuple<int, int, int>, std::string> TripointConstants = {
    { std::make_tuple( 0, 0, 0 ), "tripoint_zero" },
    { std::make_tuple( 0, 0, 1 ), "tripoint_above" },
    { std::make_tuple( 0, 0, -1 ), "tripoint_below" },
    { std::make_tuple( 0, -1, 0 ), "tripoint_north" },
    { std::make_tuple( 1, -1, 0 ), "tripoint_north_east" },
    { std::make_tuple( 1, 0, 0 ), "tripoint_east" },
    { std::make_tuple( 1, 1, 0 ), "tripoint_south_east" },
    { std::make_tuple( 0, 1, 0 ), "tripoint_south" },
    { std::make_tuple( -1, 1, 0 ), "tripoint_south_west" },
    { std::make_tuple( -1, 0, 0 ), "tripoint_west" },
    { std::make_tuple( -1, -1, 0 ), "tripoint_north_west" },
};

static void CheckConstructor( UseNamedPointConstantsCheck &Check,
                              const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCall" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const Expr *XExpr = Result.Nodes.getNodeAs<Expr>( "xexpr" );
    const Expr *YExpr = Result.Nodes.getNodeAs<Expr>( "yexpr" );
    const Expr *ZExpr = Result.Nodes.getNodeAs<Expr>( "zexpr" );
    const Expr *TempParent = Result.Nodes.getNodeAs<Expr>( "temp" );
    const VarDecl *VarDeclParent = Result.Nodes.getNodeAs<VarDecl>( "parentVarDecl" );
    if( !ConstructorCall || !ConstructorDecl || !XExpr || !YExpr ) {
        return;
    }

    std::map<std::string, int> Args;

    auto insertArgIf = [&]( const Expr * E, const char *Key ) {
        int Value;
        if( E == nullptr ) {
            return;
        } else if( const IntegerLiteral *Literal = dyn_cast<IntegerLiteral>( E ) ) {
            Value = Literal->getValue().getZExtValue();
        } else if( const UnaryOperator *UOp = dyn_cast<UnaryOperator>( E ) ) {
            const IntegerLiteral *Literal = dyn_cast<IntegerLiteral>( UOp->getSubExpr() );
            Value = Literal->getValue().getZExtValue();
            if( UOp->getOpcode() == UO_Minus ) {
                Value = -Value;
            }
        } else {
            assert( false );
        }
        Args.insert( { Key, Value } );
    };
    insertArgIf( XExpr, "x" );
    insertArgIf( YExpr, "y" );
    insertArgIf( ZExpr, "z" );

    std::string Replacement;

    if( ZExpr ) {
        auto it = TripointConstants.find( std::make_tuple( Args["x"], Args["y"], Args["z"] ) );
        if( it != TripointConstants.end() ) {
            Replacement = it->second;
        }
    } else {
        auto it = PointConstants.find( std::make_pair( Args["x"], Args["y"] ) );
        if( it != PointConstants.end() ) {
            Replacement = it->second;
        }
    }

    if( Replacement.empty() ) {
        return;
    }

    // Avoid replacing the definitions of the constants themselves
    if( VarDeclParent && VarDeclParent->getName() == Replacement ) {
        return;
    }

    const std::string UserVisibleReplacement = Replacement;

    const unsigned int LastArg = ConstructorCall->getNumArgs() - 1;
    SourceRange SourceRangeToReplace( ConstructorCall->getArg( 0 )->getBeginLoc(),
                                      ConstructorCall->getArg( LastArg )->getEndLoc() );

    if( TempParent ) {
        SourceRangeToReplace = ConstructorCall->getSourceRange();
        // Work around buggy source range for default parameters
        const std::string ReplacedText = getText( Result, ConstructorCall );
        if( ReplacedText.size() >= 2 && ReplacedText.substr( 0, 2 ) == "= " ) {
            Replacement = "= " + Replacement;
        }
    }

    CharSourceRange CharRangeToReplace = Lexer::makeFileCharRange(
            CharSourceRange::getTokenRange( SourceRangeToReplace ), *Result.SourceManager,
            Check.getLangOpts() );

    Check.diag(
        ConstructorCall->getBeginLoc(),
        "Prefer constructing %0 from named constant '%1' rather than explicit integer arguments."
    ) << ConstructorDecl->getParent() << UserVisibleReplacement <<
      FixItHint::CreateReplacement( CharRangeToReplace, Replacement );
}

void UseNamedPointConstantsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
