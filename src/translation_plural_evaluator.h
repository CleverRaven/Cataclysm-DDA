#pragma once
#ifndef CATA_SRC_TRANSLATION_PLURAL_EVALUATOR_H
#define CATA_SRC_TRANSLATION_PLURAL_EVALUATOR_H

#include <limits>
#include <stack>
#include <vector>

class TranslationPluralRulesEvaluator
{
    public:

        class Error : public std::exception
        {
            private:
                std::string message;
            public:
                explicit Error( const std::string &message ) : message( message ) {}
                const char *what() const noexcept override {
                    return message.c_str();
                }
        };

        class HeaderError : public Error
        {
            public:
                explicit HeaderError( const std::string &message ) : Error( message ) {}
        };

        class LexicalError : public Error
        {
            public:
                explicit LexicalError( const std::string &message ) : Error( message ) {}
        };

        class ParseError : public Error
        {
            public:
                explicit ParseError( const std::string &message ) : Error( message ) {}
        };

        class EvaluationError : public Error
        {
            public:
                explicit EvaluationError( const std::string &message ) : Error( message ) {}
        };

        enum class ExprTokenType : int {
            Variable,
            Constant,
            Plus,
            Minus,
            Multiplication,
            Division,
            Remainder,
            LessThan,
            Equal,
            GreaterThan,
            LessOrEqual,
            GreaterOrEqual,
            NotEqual,
            Negation,
            And,
            Or,
            LeftBracket,
            RightBracket,
            Condition,
            Branch,
            Termination,
            Default
        };

        static std::string ExprTokenTypeToString( ExprTokenType token );
        static bool IsOperandToken( ExprTokenType token );
        static bool IsUnaryToken( ExprTokenType token );
        static bool IsBinaryToken( ExprTokenType token );
        static bool IsTernaryToken( ExprTokenType token );
        static int OperatorPrecedence( ExprTokenType token );

        struct ExprToken {
            ExprTokenType type;
            int value;
            ExprToken() : type( ExprTokenType::Default ), value( 0 ) {}
            ExprToken( ExprTokenType type, int value ) : type( type ), value( value ) {}
            std::string ToString() const;
        };

        static ExprToken GetNextToken( const char *&p );
        static std::vector<ExprToken> Lexer( const std::string &expr );

        struct ExprNode {
            std::size_t n_children;
            ExprNode *children[3];
            ExprToken token;
            ExprNode() : n_children( 0 ), children{ nullptr, nullptr, nullptr } {}
            void AddChild( ExprNode *child );
            int Evaluate( std::size_t n );
        };

        static void UnwindStacks( std::stack<ExprNode *> &operands, std::stack<ExprNode *> &operators,
                                  int precedence = std::numeric_limits<int>::min() );
        static ExprNode *ConstructExprTreeRecursive( std::vector<ExprToken>::const_iterator &it,
                const std::vector<ExprToken>::const_iterator &end );
        void ConstructExprTree( const std::vector<ExprToken> &tokens );
        static void DisposeExprNodeRecursive( ExprNode *node );
        void DisposeExprTree();

        std::size_t Evaluate( const std::size_t n ) const;
        void ParsePluralRules( const std::string &plural_forms );
        explicit TranslationPluralRulesEvaluator( const std::string &plural_forms );
        ~TranslationPluralRulesEvaluator();

        TranslationPluralRulesEvaluator( TranslationPluralRulesEvaluator && ) = delete;
        TranslationPluralRulesEvaluator &operator=( TranslationPluralRulesEvaluator && ) = delete;

    private:
        std::size_t n_plurals;
        ExprNode *root;
};

#endif // CATA_SRC_TRANSLATION_PLURAL_EVALUATOR_H
