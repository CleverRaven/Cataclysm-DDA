#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "enum_conversions.h"
#include "string_formatter.h"
#include "translation_plural_evaluator.h"
#include "try_parse_integer.h"

using LexicalError = TranslationPluralRulesEvaluator::LexicalError;
using ParseError = TranslationPluralRulesEvaluator::ParseError;
using EvaluationError = TranslationPluralRulesEvaluator::EvaluationError;
using ExprTokenType = TranslationPluralRulesEvaluator::ExprTokenType;
using Token = TranslationPluralRulesEvaluator::ExprToken;
using ExprNode = TranslationPluralRulesEvaluator::ExprNode;

// Workaround to GCC 5.5 not being able to hash an enum class of primitive underlying type
namespace std
{
template<> struct hash<ExprTokenType> {
    std::size_t operator()( const ExprTokenType &token ) const {
        return static_cast<std::size_t>( token );
    }
};
} // namespace std

std::string TranslationPluralRulesEvaluator::ExprTokenTypeToString( ExprTokenType token )
{
    static const std::unordered_map<ExprTokenType, const char *> ExprTokenTypeToStringMap{
        { ExprTokenType::Variable, "n" },
        { ExprTokenType::Plus, "+" },
        { ExprTokenType::Minus, "-" },
        { ExprTokenType::Multiplication, "*" },
        { ExprTokenType::Division, "/" },
        { ExprTokenType::Remainder, "%" },
        { ExprTokenType::LessThan, "<" },
        { ExprTokenType::Equal, "==" },
        { ExprTokenType::GreaterThan, ">" },
        { ExprTokenType::LessOrEqual, "<=" },
        { ExprTokenType::GreaterOrEqual, ">=" },
        { ExprTokenType::NotEqual, "!=" },
        { ExprTokenType::Negation, "!" },
        { ExprTokenType::And, "&&" },
        { ExprTokenType::Or, "||" },
        { ExprTokenType::LeftBracket, "("},
        { ExprTokenType::RightBracket, ")" },
        { ExprTokenType::Condition, "?" },
        { ExprTokenType::Branch, ":" },
        { ExprTokenType::Termination, ";" }
    };
    auto it = ExprTokenTypeToStringMap.find( token );
    if( it == ExprTokenTypeToStringMap.end() ) {
        return "[?]";
    }
    return it->second;
}

bool TranslationPluralRulesEvaluator::IsOperandToken( ExprTokenType token )
{
    static const std::unordered_set<ExprTokenType> OperandTokens = {
        ExprTokenType::Variable,
        ExprTokenType::Constant
    };
    return OperandTokens.count( token ) == 1;
}

bool TranslationPluralRulesEvaluator::IsUnaryToken( ExprTokenType token )
{
    static const std::unordered_set<ExprTokenType> UnaryTokens = {
        ExprTokenType::Negation
    };
    return UnaryTokens.count( token );
}

bool TranslationPluralRulesEvaluator::IsBinaryToken( ExprTokenType token )
{
    static const std::unordered_set<ExprTokenType> BinaryTokens = {
        ExprTokenType::Plus,
        ExprTokenType::Minus,
        ExprTokenType::Multiplication,
        ExprTokenType::Division,
        ExprTokenType::Remainder,
        ExprTokenType::LessThan,
        ExprTokenType::Equal,
        ExprTokenType::GreaterThan,
        ExprTokenType::LessOrEqual,
        ExprTokenType::NotEqual,
        ExprTokenType::GreaterOrEqual,
        ExprTokenType::And,
        ExprTokenType::Or
    };
    return BinaryTokens.count( token );
}

bool TranslationPluralRulesEvaluator::IsTernaryToken( ExprTokenType token )
{
    static const std::unordered_set<ExprTokenType> TernaryTokens = {
        ExprTokenType::Condition,
        ExprTokenType::Branch
    };
    return TernaryTokens.count( token );
}

int TranslationPluralRulesEvaluator::OperatorPrecedence( ExprTokenType token )
{
    static const std::unordered_map<ExprTokenType, int> OperatorPrecedenceMap = {
        { ExprTokenType::Plus, 5 },
        { ExprTokenType::Minus, 5 },
        { ExprTokenType::Multiplication, 6 },
        { ExprTokenType::Division, 6 },
        { ExprTokenType::Remainder, 6 },
        { ExprTokenType::LessThan, 4 },
        { ExprTokenType::Equal, 3 },
        { ExprTokenType::GreaterThan, 4 },
        { ExprTokenType::LessOrEqual, 4 },
        { ExprTokenType::GreaterOrEqual, 4 },
        { ExprTokenType::NotEqual, 3 },
        { ExprTokenType::Negation, 7 },
        { ExprTokenType::And, 2 },
        { ExprTokenType::Or, 1 },
        { ExprTokenType::Condition, 0 },
        { ExprTokenType::Branch, 0 }
    };
    return OperatorPrecedenceMap.at( token );
}

std::string TranslationPluralRulesEvaluator::ExprToken::ToString() const
{
    if( type == ExprTokenType::Constant ) {
        return std::to_string( value );
    } else {
        return ExprTokenTypeToString( type );
    }
}

Token TranslationPluralRulesEvaluator::GetNextToken( const char *&p )
{
    while( *p != '\0' && *p == ' ' ) {
        ++p;
    }
    if( *p == '\0' || *p == ';' ) {
        return ExprToken( ExprTokenType::Termination, 0 );
    } else if( *p == 'n' ) {
        ++p;
        return ExprToken( ExprTokenType::Variable, 0 );
    } else if( *p == '+' ) {
        ++p;
        return ExprToken( ExprTokenType::Plus, 0 );
    } else if( *p == '-' ) {
        ++p;
        return ExprToken( ExprTokenType::Minus, 0 );
    } else if( *p == '*' ) {
        ++p;
        return ExprToken( ExprTokenType::Multiplication, 0 );
    } else if( *p == '/' ) {
        ++p;
        return ExprToken( ExprTokenType::Division, 0 );
    } else if( *p == '%' ) {
        ++p;
        return ExprToken( ExprTokenType::Remainder, 0 );
    } else if( *p == '<' && *( p + 1 ) == '=' ) {
        p += 2;
        return ExprToken( ExprTokenType::LessOrEqual, 0 );
    } else if( *p == '=' && *( p + 1 ) == '=' ) {
        p += 2;
        return ExprToken( ExprTokenType::Equal, 0 );
    } else if( *p == '>' && *( p + 1 ) == '=' ) {
        p += 2;
        return ExprToken( ExprTokenType::GreaterOrEqual, 0 );
    } else if( *p == '!' && *( p + 1 ) == '=' ) {
        p += 2;
        return ExprToken( ExprTokenType::NotEqual, 0 );
    } else if( *p == '<' ) {
        ++p;
        return ExprToken( ExprTokenType::LessThan, 0 );
    } else if( *p == '>' ) {
        ++p;
        return ExprToken( ExprTokenType::GreaterThan, 0 );
    } else if( *p == '!' ) {
        ++p;
        return ExprToken( ExprTokenType::Negation, 0 );
    } else if( *p == '&' && *( p + 1 ) == '&' ) {
        p += 2;
        return ExprToken( ExprTokenType::And, 0 );
    } else if( *p == '|' && *( p + 1 ) == '|' ) {
        p += 2;
        return ExprToken( ExprTokenType::Or, 0 );
    }    else if( *p == '(' ) {
        ++p;
        return ExprToken( ExprTokenType::LeftBracket, 0 );
    } else if( *p == ')' ) {
        ++p;
        return ExprToken( ExprTokenType::RightBracket, 0 );
    } else if( *p == '?' ) {
        ++p;
        return ExprToken( ExprTokenType::Condition, 0 );
    } else if( *p == ':' ) {
        ++p;
        return ExprToken( ExprTokenType::Branch, 0 );
    } else if( std::isdigit( *p ) ) {
        int value = 0;
        while( std::isdigit( *p ) ) {
            value = value * 10 + static_cast<int>( *p - '0' );
            ++p;
        }
        return ExprToken( ExprTokenType::Constant, value );
    } else {
        throw LexicalError( string_format( "Unexpected character %c in expression", *p ) );
    }
}

std::vector<Token> TranslationPluralRulesEvaluator::Lexer( const std::string &expr )
{
    std::vector<ExprToken> tokens;
    const char *p = &expr[0];
    ExprToken token;
    while( ( token = GetNextToken( p ) ).type != ExprTokenType::Termination ) {
        tokens.emplace_back( token );
    }
    return tokens;
}

void TranslationPluralRulesEvaluator::ExprNode::AddChild( ExprNode *child )
{
    if( n_children == 3 ) {
        throw ParseError( "Children nodes exceeded maximum of 3." );
    }
    children[n_children++] = child;
}

int TranslationPluralRulesEvaluator::ExprNode::Evaluate( std::size_t n )
{
    if( n_children == 0 ) {
        switch( token.type ) {
            case ExprTokenType::Constant:
                return token.value;
            case ExprTokenType::Variable:
                return n;
            default:
                throw EvaluationError( string_format( "Unexpected token %s at a node with 0 child.",
                                                      ExprTokenTypeToString( token.type ) ) );
        }
    } else if( n_children == 1 ) {
        switch( token.type ) {
            case ExprTokenType::Negation:
                return children[0]->Evaluate( n ) == 0;
            default:
                throw EvaluationError( string_format( "Unexpected token %s at a node with 1 child.",
                                                      ExprTokenTypeToString( token.type ) ) );
        }
    } else if( n_children == 2 ) {
        const int a = children[0]->Evaluate( n );
        const int b = children[1]->Evaluate( n );
        switch( token.type ) {
            case ExprTokenType::Plus:
                return a + b;
            case ExprTokenType::Minus:
                return a - b;
            case ExprTokenType::Multiplication:
                return a * b;
            case ExprTokenType::Division:
                if( b == 0 ) {
                    throw EvaluationError( "Division by zero" );
                }
                return a / b;
            case ExprTokenType::Remainder:
                if( b == 0 ) {
                    throw EvaluationError( "Division by zero" );
                }
                return a % b;
            case ExprTokenType::LessThan:
                return a < b;
            case ExprTokenType::Equal:
                return a == b;
            case ExprTokenType::GreaterThan:
                return a > b;
            case ExprTokenType::NotEqual:
                return a != b;
            case ExprTokenType::LessOrEqual:
                return a <= b;
            case ExprTokenType::GreaterOrEqual:
                return a >= b;
            case ExprTokenType::Or:
                return a || b;
            case ExprTokenType::And:
                return a && b;
            default:
                throw EvaluationError( string_format( "Unexpected token %s at a node with 2 children.",
                                                      ExprTokenTypeToString( token.type ) ) );
        }
    } else if( n_children == 3 ) {
        switch( token.type ) {
            case ExprTokenType::Condition:
                return children[0]->Evaluate( n ) ? children[1]->Evaluate( n ) : children[2]->Evaluate( n );
            default:
                throw EvaluationError( string_format( "Unexpected token %s at a node with 3 children.",
                                                      ExprTokenTypeToString( token.type ) ) );
        }
    } else {
        throw EvaluationError( string_format( "Unexpected %zu children in a node.", n_children ) );
    }
}

void TranslationPluralRulesEvaluator::UnwindStacks( std::stack<ExprNode *> &operands,
        std::stack<ExprNode *> &operators,
        int precedence )
{
    while( not operators.empty() &&
           OperatorPrecedence( operators.top()->token.type ) >= precedence &&
           ( OperatorPrecedence( operators.top()->token.type ) > precedence ||
             not IsTernaryToken( operators.top()->token.type ) ) ) {
        if( IsUnaryToken( operators.top()->token.type ) ) {
            if( operands.empty() ) {
                throw ParseError( "Insufficient operands" );
            }
            ExprNode *a = operands.top();
            operands.pop();
            ExprNode *op = operators.top();
            operators.pop();
            op->AddChild( a );
            operands.push( op );
        } else if( IsBinaryToken( operators.top()->token.type ) ) {
            if( operands.size() < 2 ) {
                throw ParseError( "Insufficient operands" );
            }
            ExprNode *b = operands.top();
            operands.pop();
            ExprNode *a = operands.top();
            operands.pop();
            ExprNode *op = operators.top();
            operators.pop();
            op->AddChild( a );
            op->AddChild( b );
            operands.push( op );
        } else if( IsTernaryToken( operators.top()->token.type ) ) {
            if( operands.size() < 3 || operands.size() < 2 ) {
                break;
            }
            ExprNode *c = operands.top();
            operands.pop();
            ExprNode *b = operands.top();
            operands.pop();
            ExprNode *a = operands.top();
            operands.pop();
            ExprNode *op2 = operators.top();
            operators.pop();
            ExprNode *op1 = operators.top();
            operators.pop();
            op1->AddChild( a );
            op1->AddChild( b );
            op1->AddChild( c );
            operands.push( op1 );
            delete op2;
        }
    }
}

ExprNode *TranslationPluralRulesEvaluator::ConstructExprTreeRecursive(
    std::vector<ExprToken>::const_iterator &it,
    const std::vector<ExprToken>::const_iterator &end )
{
    std::stack<ExprNode *> operands;
    std::stack<ExprNode *> operators;
    while( it != end && it->type != ExprTokenType::Termination ) {
        if( it->type == ExprTokenType::LeftBracket ) {
            operands.push( ConstructExprTreeRecursive( ++it, end ) );
        } else if( it->type == ExprTokenType::RightBracket ) {
            ++it;
            break;
        } else {
            ExprNode *node = new ExprNode;
            node->token = *it;
            if( IsOperandToken( it->type ) ) {
                operands.push( node );
            } else {
                UnwindStacks( operands, operators, OperatorPrecedence( it->type ) );
                operators.push( node );
            }
            ++it;
        }
    }
    UnwindStacks( operands, operators );
    if( operands.size() == 1 && operators.empty() ) {
        return operands.top();
    } else if( operators.size() == 1 && operands.empty() ) {
        return operators.top();
    }
    throw ParseError( "Stacks not unwinded during expression tree construction." );
}

void TranslationPluralRulesEvaluator::ConstructExprTree( const std::vector<ExprToken> &tokens )
{
    auto it = tokens.begin();
    root = ConstructExprTreeRecursive( it, tokens.end() );
}

void TranslationPluralRulesEvaluator::DisposeExprNodeRecursive( ExprNode *node )
{
    for( std::size_t i = 0; i < node->n_children; i++ ) {
        DisposeExprNodeRecursive( node->children[i] );
        delete node->children[i];
        node->children[i] = nullptr;
    }
}

void TranslationPluralRulesEvaluator::DisposeExprTree()
{
    if( root != nullptr ) {
        DisposeExprNodeRecursive( root );
        delete root;
        root = nullptr;
    }
}

std::size_t TranslationPluralRulesEvaluator::Evaluate( const std::size_t n ) const
{
    std::size_t result = root->Evaluate( n );
    if( result >= n_plurals ) {
        throw EvaluationError( "Plural expression exceeded number of plural forms." );
    }
    return result;
}

void TranslationPluralRulesEvaluator::ParsePluralRules( const std::string &plural_forms )
{
    const std::string nplurals_header = "nplurals=";
    std::size_t nplurals_header_pos = plural_forms.find( nplurals_header );
    if( nplurals_header_pos == std::string::npos ) {
        throw HeaderError( string_format( "Expect \"%s\" in %s", nplurals_header, plural_forms ) );
    }
    nplurals_header_pos += nplurals_header.length();
    std::size_t first_semicolon = plural_forms.find( ';', nplurals_header_pos );
    if( first_semicolon == std::string::npos ) {
        throw HeaderError( string_format( "Expect semicolon in %s", plural_forms ) );
    }
    ret_val<int> n_plurals_optional = try_parse_integer<int>( plural_forms.substr(
                                          nplurals_header_pos, first_semicolon - nplurals_header_pos ), false );
    if( !n_plurals_optional.success() || n_plurals_optional.value() < 1 ) {
        throw HeaderError( string_format( "Invalid number of plural forms in %s", plural_forms ) );
    }
    n_plurals = static_cast<std::size_t>( n_plurals_optional.value() );
    const std::string plural_header = "plural=";
    std::size_t plural_header_pos = plural_forms.find( plural_header );
    if( plural_header_pos == std::string::npos ) {
        throw HeaderError( string_format( "Expect \"%s\" in %s", plural_header, plural_forms ) );
    }
    plural_header_pos += plural_header.length();
    std::size_t second_semicolon = plural_forms.find( ';', plural_header_pos );
    const std::string expr = plural_forms.substr( plural_header_pos,
                             second_semicolon - plural_header_pos );
    const std::vector<ExprToken> tokens = Lexer( expr );
    ConstructExprTree( tokens );
}

TranslationPluralRulesEvaluator::TranslationPluralRulesEvaluator( const std::string &plural_forms )
{
    ParsePluralRules( plural_forms );
}

TranslationPluralRulesEvaluator::~TranslationPluralRulesEvaluator()
{
    DisposeExprTree();
}
