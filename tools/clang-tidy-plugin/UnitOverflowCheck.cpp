#include "UnitOverflowCheck.h"
#include "Utils.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>

#include <map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

struct QuantityUnit {
    std::string_view Unit;
    std::int64_t ConversionFactor;
};

static const std::map<std::string_view, QuantityUnit> FunctionAndQuantityTypes {
    {"from_joule", {"energy", 1'000LL}},
    {"from_kilojoule", {"energy", 1'000'000LL}},
    {"from_watt", {"power", 1'000LL}},
    {"from_kilowatt", {"power", 1'000'000LL}},
};

void UnitOverflowCheck::registerMatchers( ast_matchers::MatchFinder *Finder )
{
    for( const auto &[functionName, unit] : FunctionAndQuantityTypes ) {
        Finder->addMatcher(
            callExpr( callee( functionDecl( hasName( functionName ) ).bind( "func" ) ),
                      hasArgument( 0, expr( hasType( isInteger() ) ).bind( "arg" ) ) ),
            this
        );
    }
}

void UnitOverflowCheck::check( const ast_matchers::MatchFinder::MatchResult &Result )
{
    const FunctionDecl *func = Result.Nodes.getNodeAs<FunctionDecl>( "func" );
    const Expr *arg = Result.Nodes.getNodeAs<Expr>( "arg" );
    if( !func || !arg ) {
        return;
    }
    const QualType type = arg->getType();
    const std::int64_t width = Result.Context->getTypeInfo( type ).Width;
    if( width >= 64 ) {
        return;
    }
    const SourceManager &sourceManager = *Result.SourceManager;
    if( sourceManager.getFilename( arg->getBeginLoc() ).ends_with( "src/units.h" ) ) {
        return;
    }
    const std::string functionName = func->getNameAsString();
    if( const IntegerLiteral *literal = dyn_cast<IntegerLiteral>( arg ) ) {
        const bool isSigned = literal->getType()->isSignedIntegerType();
        std::int64_t minVal = 0;
        std::int64_t maxVal = 0;
        if( isSigned ) {
            minVal = llvm::APInt::getSignedMinValue( width ).getSExtValue();
            maxVal = llvm::APInt::getSignedMaxValue( width ).getSExtValue();
        } else {
            minVal = llvm::APInt::getMinValue( width ).getSExtValue();
            maxVal = llvm::APInt::getMaxValue( width ).getSExtValue();
        }
        const std::int64_t multiplier = FunctionAndQuantityTypes.at( functionName ).ConversionFactor;
        const std::int64_t val = literal->getValue().getSExtValue() * multiplier;
        if( val < minVal || val > maxVal ) {
            emitDiag( arg->getBeginLoc(), FunctionAndQuantityTypes.at( functionName ).Unit, type.getAsString(),
                      functionName, FixItHint::CreateReplacement( arg->getSourceRange(),
                              ( getText( Result, arg ) + Twine( "LL" ) ).str() ) );
        }
    } else {
        emitDiag( arg->getBeginLoc(), FunctionAndQuantityTypes.at( functionName ).Unit, type.getAsString(),
                  functionName, FixItHint::CreateReplacement( arg->getSourceRange(),
                          ( Twine( "static_cast<std::int64_t>( " ) + getText( Result, arg ) + " )" ).str() ) );
    }
}

void UnitOverflowCheck::emitDiag( const SourceLocation &loc, const std::string_view QuantityType,
                                  const std::string_view ValueType,
                                  const std::string_view FunctionName, const clang::FixItHint &fix )
{
    diag( loc,
          "constructing %0 quantity from '%1' can overflow in '%2' in multiplying with the conversion factor" )
            << QuantityType << ValueType << FunctionName << fix;
}

} // namespace clang::tidy::cata
