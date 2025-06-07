#include "U8PathCheck.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void U8PathCheck::registerMatchers( MatchFinder *Finder )
{
    // (::.*)? matches ::__cxx11 from glibcxx
    auto declIsPath = matchesName( "^::(std|ghc)::filesystem(::.*)?::path$" );

    auto notInFsNamespace =
        unless( hasAncestor( namespaceDecl( matchesName( "^::(std|ghc)::filesystem$" ) ) ) );

    auto firstParamIsPath =
        hasParameter( 0, hasType( references( hasUnqualifiedDesugaredType( recordType(
                                      hasDeclaration( cxxRecordDecl( declIsPath ) ) ) ) ) ) );

    auto pathConversionConstructorDecl =
        cxxConstructorDecl(
            ofClass( declIsPath ),
            // Default construction
            unless( parameterCountIs( 0 ) ),
            // Copy/move construction
            unless( allOf( parameterCountIs( 1 ), firstParamIsPath ) )
        );

    auto pathConvertModifyOperatorDecl =
        cxxMethodDecl(
            hasAnyOverloadedOperatorName( "=", "/=", "+=" ),
            ofClass( declIsPath ),
            unless( firstParamIsPath )
        );

    auto pathConvertModifyMethodDecl =
        cxxMethodDecl(
            hasAnyName( "assign", "append", "concat" ),
            ofClass( declIsPath )
        );

    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( pathConversionConstructorDecl ),
            notInFsNamespace
        ).bind( "constructExpr" ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( pathConvertModifyOperatorDecl ),
            anyOf(
                allOf(
                    // lhs OP rhs
                    cxxOperatorCallExpr(),
                    hasArgument( 1, expr().bind( "operatorCallArg" ) )
                ),
                allOf(
                    // lhs.operator OP( rhs )
                    cxxMemberCallExpr(),
                    hasArgument( 0, expr().bind( "operatorCallArg" ) )
                )
            ),
            notInFsNamespace
        ),
        this
    );
    Finder->addMatcher(
        cxxMemberCallExpr(
            callee( pathConvertModifyMethodDecl ),
            notInFsNamespace
        ).bind( "memberCallExpr" ),
        this
    );
    Finder->addMatcher(
        declRefExpr(
            declRefExpr( to( pathConvertModifyOperatorDecl ) ).bind( "operatorRefExpr" ),
            // Do not match `OP` in `lhs OP rhs`
            unless( hasParent( expr( hasParent( cxxOperatorCallExpr( callee( expr( has( declRefExpr(
                                         equalsBoundNode( "operatorRefExpr" ) ) ) ) ) ) ) ) ) ),
            notInFsNamespace
        ),
        this
    );
    Finder->addMatcher(
        declRefExpr(
            to( pathConvertModifyMethodDecl ),
            notInFsNamespace
        ).bind( "methodRefExpr" ),
        this
    );
}

void U8PathCheck::check( const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *constructExpr =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructExpr" );
    const Expr *operatorCallArg =
        Result.Nodes.getNodeAs<Expr>( "operatorCallArg" );
    const CXXMemberCallExpr *memberCallExpr =
        Result.Nodes.getNodeAs<CXXMemberCallExpr>( "memberCallExpr" );
    const DeclRefExpr *operatorRefExpr =
        Result.Nodes.getNodeAs<DeclRefExpr>( "operatorRefExpr" );
    const DeclRefExpr *methodRefExpr =
        Result.Nodes.getNodeAs<DeclRefExpr>( "methodRefExpr" );

    SourceRange diagRange;
    std::string diagMessage;
    if( constructExpr ) {
        diagRange = constructExpr->getSourceRange();
        diagMessage = "Construct `std::filesystem::path` by passing UTF-8 string to "
                      "`std::filesystem::u8path` to ensure the correct path encoding.";
    } else if( operatorCallArg ) {
        diagRange = operatorCallArg->getSourceRange();
        diagMessage = "Modify `std::filesystem::path` using parameter constructed with "
                      "`std::filesystem::u8path` and UTF-8 string to ensure the correct path "
                      "encoding.";
    } else if( operatorRefExpr ) {
        diagRange = operatorRefExpr->getSourceRange();
        diagMessage = "Use the operator overload with `std::filesystem::path` parameter and "
                      "call it using parameter constructed with `std::filesystem::u8path` and "
                      "UTF-8 string to ensure the correct path encoding.";
    } else if( memberCallExpr ) {
        diagRange = memberCallExpr->getSourceRange();
        diagMessage = "Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter "
                      "constructed with `std::filesystem::u8path` and UTF-8 string to ensure "
                      "the correct path encoding.";
    } else if( methodRefExpr ) {
        diagRange = methodRefExpr->getSourceRange();
        diagMessage = "Use the `=`, `/=`, or `+=` operator overload with "
                      "`std::filesystem::path` parameter and call it using parameter "
                      "constructed with `std::filesystem::u8path` and UTF-8 string to ensure "
                      "the correct path encoding.";
    } else {
        return;
    }

    diag( diagRange.getBegin(), diagMessage ) << diagRange;
}

} // namespace clang::tidy::cata
