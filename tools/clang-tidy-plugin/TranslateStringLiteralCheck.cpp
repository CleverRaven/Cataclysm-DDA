#include <algorithm>
#include <cstring>
#include <unordered_set>

#include "TranslateStringLiteralCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/DiagnosticIDs.h>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void TranslateStringLiteralCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            allOf(
                unless(
                    anyOf(
                        hasDescendant(
                            callExpr(
                                callee(
                                    functionDecl(
                                        hasAnyName(
                                            "_",
                                            "translation_argument_identity",
                                            "pgettext",
                                            "n_gettext",
                                            "npgettext"
                                        )
                                    )
                                )
                            )
                        ),
                        hasAncestor(
                            functionDecl(
                                anyOf(
                                    hasAnyName(
                                        "check",
                                        "finalize"
                                    ),
                                    matchesName( "check_" ),
                                    matchesName( "debug" ),
                                    matchesName( "finalize_" )
                                )
                            )
                        ),
                        hasAncestor(
                            ifStmt(
                                hasCondition(
                                    hasDescendant(
                                        declRefExpr(
                                            to(
                                                    varDecl(
                                                            matchesName( "debug" )
                                                    )
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                ),
                anyOf(
                    allOf(
                        hasArgument( 0, stringLiteral().bind( "stringLiteral" ) ),
                        callee(
                            functionDecl(
                                hasName( "string_format" )
                            )
                        ),
                        unless(
                            hasAncestor(
                                callExpr(
                                    callee(
                                        functionDecl(
                                            anyOf(
                                                    hasName( "no_translation" ),
                                                    matchesName( "[Dd]ebug" ),
                                                    matchesName( "[Ee]rror" ),
                                                    hasName( "check" ),
                                                    hasName( "finalize" )
                                            )
                                        )
                                    )
                                )
                            )
                        ),
                        unless(
                            hasAncestor(
                                cxxOperatorCallExpr(
                                    isAssignmentOperator(),
                                    hasLHS(
                                        memberExpr(
                                            hasDeclaration(
                                                    namedDecl(
                                                            anyOf(
                                                                    matchesName( "[Dd]ebug" ),
                                                                    matchesName( "[Ee]rror" )
                                                            )
                                                    )
                                            )
                                        )
                                    )
                                )
                            )
                        ),
                        unless(
                            hasAncestor(
                                cxxOperatorCallExpr(
                                    hasOverloadedOperatorName( "<<" ),
                                    hasLHS(
                                        hasDescendant(
                                            callExpr(
                                                    callee(
                                                            functionDecl(
                                                                    hasName( "DebugLog" )
                                                            )
                                                    )
                                            )
                                        )
                                    )
                                )
                            )
                        ),
                        unless(
                            allOf(
                                hasAncestor(
                                    returnStmt()
                                ),
                                hasAncestor(
                                    functionDecl(
                                        anyOf(
                                            matchesName( "file" ),
                                            matchesName( "dir" ),
                                            matchesName( "path" )
                                        )
                                    )
                                )
                            )
                        ),
                        unless(
                            hasAncestor(
                                cxxConstructExpr(
                                    hasAncestor(
                                        varDecl(
                                            anyOf(
                                                    hasName( "var" ),
                                                    matchesName( "dir" ),
                                                    matchesName( "file" ),
                                                    matchesName( "path" ),
                                                    matchesName( "error" )
                                            )
                                        )
                                    )
                                )
                            )
                        ),
                        unless(
                            hasAncestor(
                                cxxThrowExpr()
                            )
                        )
                    ),
                    allOf(
                        hasArgument( 0, stringLiteral().bind( "stringLiteral" ) ),
                        callee(
                            functionDecl(
                                hasAnyName(
                                    "popup",
                                    "add_msg",
                                    "add_msg_if_player",
                                    "add_msg_player_or_npc"
                                )
                            )
                        )
                    ),
                    allOf(
                        hasArgument( 1, stringLiteral().bind( "stringLiteral" ) ),
                        callee(
                            functionDecl(
                                hasAnyName(
                                    "add_msg",
                                    "add_msg_if_player",
                                    "add_msg_if_player_sees",
                                    "add_msg_player_or_npc"
                                )
                            )
                        )
                    )
                )
            )
        ),
        this
    );
}

std::string TranslateStringLiteralCheck::pruneFormatStrings( const std::string_view str )
{
    std::string result;
    result.reserve( str.length() );
    bool in_format_string = false;
    static const std::unordered_set<char> format_chars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                                           ' ', '%', '$', '*', '.', '+', '-',
                                                           'c', 's', 'd', 'f', 'l', 'z', 'u', 'i'
                                                         };
    static const std::unordered_set<char> format_terminators = { 'c', 's', 'd', 'f', 'u', 'i' };
    for( const char ch : str ) {
        if( in_format_string ) {
            if( format_chars.count( ch ) == 0 ) {
                in_format_string = false;
                result.push_back( ch );
            } else if( format_terminators.count( ch ) == 1 ) {
                in_format_string = false;
            } else if( ch == '%' ) {
                in_format_string = false;
                result.push_back( '%' );
            }
        } else {
            if( ch == '%' ) {
                in_format_string = true;
            } else {
                result.push_back( ch );
            }
        }
    }
    return result;
}

std::string TranslateStringLiteralCheck::pruneTags( const std::string &str )
{
    if( str.find( "</" ) != std::string::npos && str.find( '>' ) != std::string::npos ) {
        std::string result;
        result.reserve( str.length() );
        bool in_a_tag = false;
        for( const char ch : str ) {
            if( ch == '<' ) {
                in_a_tag = true;
            }
            if( !in_a_tag ) {
                result.push_back( ch );
            }
            if( ch == '>' ) {
                in_a_tag = false;
            }
        }
        return result;
    }
    return str;
}

std::string TranslateStringLiteralCheck::removeSubstrings( const std::string &str,
        const std::unordered_set<std::string> &substrings )
{
    std::string result = str;
    bool contains_substring = false;
    do {
        contains_substring = false;
        for( const std::string &substring : substrings ) {
            const std::size_t pos = result.find( substring );
            if( pos != std::string::npos ) {
                contains_substring = true;
                result = result.substr( 0, pos ) + result.substr( pos + substring.length() );
            }
        }
    } while( contains_substring );
    return result;
}

std::string TranslateStringLiteralCheck::extractText( const std::string_view str )
{
    std::string result;
    std::copy_if( str.begin(), str.end(), std::back_inserter( result ), []( const char ch ) {
        return std::isalpha( ch );
    } );
    return result;
}

bool TranslateStringLiteralCheck::isUnit( const std::string &str )
{
    static const std::unordered_set<std::string> units = {
        // mass
        "mg",
        "g",
        "kg",
        // volume
        "ml",
        "L",
        // energy
        "mJ",
        "J",
        "kJ",
        // radiation
        "rad",
        // time
        "ms",
        "s",
        "m",
        "h",
        "d",
        // length
        "cm",
        "m"
    };
    return units.count( str );
}

bool TranslateStringLiteralCheck::containsTranslatableText( const std::string_view str )
{
    std::string text = extractText( str );
    if( text.empty() ) {
        return false;
    }
    return !isUnit( text );
}

void TranslateStringLiteralCheck::check( const MatchFinder::MatchResult &Result )
{
    static const std::unordered_set<std::string> special_tags = {
        "<color_%s>",
        "<num>"
    };
    static const std::unordered_set<std::string> ignore_source_files = {
        "/tests/"
    };
    const StringLiteral *stringLiteral = Result.Nodes.getNodeAs<StringLiteral>( "stringLiteral" );
    if( stringLiteral == nullptr ) {
        return;
    }
    const SourceManager *sourceManager = Result.SourceManager;
    const SourceLocation sourceLocation = stringLiteral->getBeginLoc();
    const std::string sourceLocationString = sourceLocation.printToString( *sourceManager );
    for( const auto &ignore_source_file : ignore_source_files ) {
        if( sourceLocationString.find( ignore_source_file ) != std::string::npos ) {
            return;
        }
    }
    const std::size_t length = stringLiteral->getByteLength();
    std::string literal( stringLiteral->getBytes().data(), length );
    literal = removeSubstrings( literal, special_tags );
    literal = pruneTags( literal );
    literal = pruneFormatStrings( literal );
    if( containsTranslatableText( literal ) ) {
        diag( sourceLocation,
              "String literal should be translated if it will be displayed in user interface.  "
              "If this is a false alarm, use //NOLINT or //NOLINTNEXTLINE to suppress the warning." );
    }
}

} // namespace clang::tidy::cata
