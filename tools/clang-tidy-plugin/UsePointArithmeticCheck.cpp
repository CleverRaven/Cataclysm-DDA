#include "UsePointArithmeticCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>
#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Utils.h"
#include "clang/AST/OperationKinds.h"
#include "clang/Basic/OperatorKinds.h"
#include "../../src/cata_assert.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static auto isRefactorableExpr( const std::string &member_ )
{
    return ignoringParenCasts(
               anyOf(
                   memberExpr(
                       member( hasName( member_ ) )
                   ),
                   binaryOperator(),
                   cxxOperatorCallExpr()
               )
           );
}

void UsePointArithmeticCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( isPointConstructor().bind( "constructorDecl" ) ),
            testWhetherConstructingTemporary(),
            argumentCountIs( 2 ),
            hasArgument( 0, expr( isRefactorableExpr( "x" ) ).bind( "xexpr" ) ),
            hasArgument( 1, expr( isRefactorableExpr( "y" ) ).bind( "yexpr" ) )
        ).bind( "constructorCall" ),
        this
    );
    Finder->addMatcher(
        cxxConstructExpr(
            hasDeclaration( isPointConstructor().bind( "constructorDecl" ) ),
            testWhetherConstructingTemporary(),
            anyOf(
                allOf(
                    argumentCountIs( 3 ),
                    hasArgument( 0, expr( isRefactorableExpr( "x" ) ).bind( "xexpr" ) ),
                    hasArgument( 1, expr( isRefactorableExpr( "y" ) ).bind( "yexpr" ) ),
                    hasArgument( 2, expr( isRefactorableExpr( "z" ) ).bind( "zexpr" ) )
                ),
                allOf(
                    argumentCountIs( 2 ),
                    hasArgument( 0, expr( isRefactorableExpr( "xy" ) ).bind( "xyexpr" ) ),
                    hasArgument( 1, expr( isRefactorableExpr( "z" ) ).bind( "zexpr" ) )
                )
            )
        ).bind( "constructorCall" ),
        this
    );
}

struct ExpressionComponent {
    ExpressionComponent( const MatchFinder::MatchResult &Result, const Expr *E ) :
        objectRef( getText( Result, E ) ),
        coefficient( 1 ),
        isMember( false ),
        isArrowRef( false ),
        isTripoint( false ) {
        complete_init();
    }

    ExpressionComponent( const MatchFinder::MatchResult &Result, const Expr *E,
                         const CXXRecordDecl *MemberOf, bool IsArrowRef ) :
        objectRef( getText( Result, E ) ),
        coefficient( 1 ),
        isMember( true ),
        isArrowRef( IsArrowRef ),
        isTripoint( MemberOf->getName() == "tripoint" ) {
        complete_init();
    }

    void complete_init() {
        if( StringRef( objectRef ).ends_with( "->" ) ) {
            objectRef.erase( objectRef.end() - 2, objectRef.end() );
        }
    }

    std::string objectRef;
    int coefficient;
    bool coefficientInvolvesMacro = false;
    bool isMember;
    bool isArrowRef;
    bool isTripoint;

    std::tuple<bool, std::string, bool> sortKey() const {
        return std::make_tuple( isMember, objectRef, isArrowRef );
    }

    bool canConsolidateWith( const ExpressionComponent &other ) const {
        return sortKey() == other.sortKey();
    }

    void consolidate( const ExpressionComponent &other ) {
        coefficient += other.coefficient;
    }
};

static bool operator<( const ExpressionComponent &l, const ExpressionComponent &r )
{
    // NOLINTNEXTLINE(cata-use-localized-sorting)
    return l.sortKey() < r.sortKey();
}

static bool operator>( const ExpressionComponent &l, const ExpressionComponent &r )
{
    return r < l;
}

static std::vector<ExpressionComponent> handleMultiply(
    const Expr *LhsE, const Expr *RhsE,
    std::vector<ExpressionComponent> LhsC, std::vector<ExpressionComponent> RhsC )
{
    auto updateComponents = [&]( std::vector<ExpressionComponent> &Components,
    const IntegerLiteral * CoeffExpr ) {
        int Value = CoeffExpr->getValue().getZExtValue();
        bool IsMacro = CoeffExpr->getBeginLoc().isMacroID();
        for( ExpressionComponent &Component : Components ) {
            Component.coefficient *= Value;
            Component.coefficientInvolvesMacro |= IsMacro;
        }
    };

    if( const IntegerLiteral *CoeffExpr = dyn_cast<IntegerLiteral>( LhsE ) ) {
        updateComponents( RhsC, CoeffExpr );
        return RhsC;
    }
    if( const IntegerLiteral *CoeffExpr = dyn_cast<IntegerLiteral>( RhsE ) ) {
        updateComponents( LhsC, CoeffExpr );
        return LhsC;
    }
    return {};
}

static std::vector<ExpressionComponent> decomposeExpr( const Expr *E, const std::string &Member,
        const MatchFinder::MatchResult &Result )
{
    auto anyHaveMember = []( const std::vector<ExpressionComponent> &Components ) {
        auto isMember = []( const ExpressionComponent & C ) {
            return C.isMember;
        };
        return std::any_of( Components.begin(), Components.end(), isMember );
    };
    cata_assert( E );
    switch( E->getStmtClass() ) {
        case Stmt::BinaryOperatorClass: {
            const BinaryOperator *Binary = cast<BinaryOperator>( E );
            std::vector<ExpressionComponent> Lhs =
                decomposeExpr( Binary->getLHS(), Member, Result );
            std::vector<ExpressionComponent> Rhs =
                decomposeExpr( Binary->getRHS(), Member, Result );
            if( Lhs.empty() || Rhs.empty() ) {
                return {};
            }
            if( !anyHaveMember( Lhs ) && !anyHaveMember( Rhs ) ) {
                return { { Result, E } };
            }

            switch( Binary->getOpcode() ) {
                case BO_Add:
                    Lhs.insert( Lhs.end(), Rhs.begin(), Rhs.end() );
                    return Lhs;
                case BO_Sub:
                    for( ExpressionComponent &Component : Rhs ) {
                        Component.coefficient *= -1;
                    }
                    Lhs.insert( Lhs.end(), Rhs.begin(), Rhs.end() );
                    return Lhs;
                case BO_Mul:
                    return handleMultiply( Binary->getLHS(), Binary->getRHS(), Lhs, Rhs );
                default:
                    return {};
            }
        }
        case Stmt::CXXMemberCallExprClass: {
            const CXXMemberCallExpr *MemEx = cast<CXXMemberCallExpr>( E );
            const CXXMethodDecl *MemDecl = MemEx->getMethodDecl();
            const CXXRecordDecl *Record = dyn_cast<CXXRecordDecl>( MemDecl->getParent() );
            if( isPointType( Record ) && MemDecl->getName() == Member ) {
                const Expr *Object = MemEx->getImplicitObjectArgument();
                QualType ObjectType = Object->getType();
                bool IsArrowRef = ObjectType->isPointerType();
                return { { Result, Object, MemDecl->getParent(), IsArrowRef } };
            } else {
                return { { Result, E } };
            }
        }
        case Stmt::CXXOperatorCallExprClass: {
            const CXXOperatorCallExpr *Op = cast<CXXOperatorCallExpr>( E );
            if( Op->getNumArgs() == 1 ) {
                std::vector<ExpressionComponent> Inner =
                    decomposeExpr( Op->getArg( 0 ), Member, Result );
                if( Inner.empty() ) {
                    return {};
                }
                switch( Op->getOperator() ) {
                    case OO_Minus:
                        for( ExpressionComponent &Component : Inner ) {
                            Component.coefficient *= -1;
                        }
                        return Inner;
                    default:
                        return {};
                }
            } else if( Op->getNumArgs() == 2 ) {
                std::vector<ExpressionComponent> Lhs =
                    decomposeExpr( Op->getArg( 0 ), Member, Result );
                std::vector<ExpressionComponent> Rhs =
                    decomposeExpr( Op->getArg( 1 ), Member, Result );
                if( Lhs.empty() || Rhs.empty() ) {
                    return {};
                }
                switch( Op->getOperator() ) {
                    case OO_Plus:
                        Lhs.insert( Lhs.end(), Rhs.begin(), Rhs.end() );
                        return Lhs;
                    case OO_Minus:
                        for( ExpressionComponent &Component : Rhs ) {
                            Component.coefficient *= -1;
                        }
                        Lhs.insert( Lhs.end(), Rhs.begin(), Rhs.end() );
                        return Lhs;
                    case OO_Star:
                        return handleMultiply( Op->getArg( 0 ), Op->getArg( 1 ), Lhs, Rhs );
                    default:
                        return {};
                }
            } else {
                return {};
            }
        }
        case Stmt::ImplicitCastExprClass: {
            const ImplicitCastExpr *Implicit = cast<ImplicitCastExpr>( E );
            return decomposeExpr( Implicit->getSubExpr(), Member, Result );
        }
        case Stmt::MaterializeTemporaryExprClass: {
            const MaterializeTemporaryExpr *Temp = cast<MaterializeTemporaryExpr>( E );
            return decomposeExpr( Temp->getSubExpr(), Member, Result );
        }
        case Stmt::MemberExprClass: {
            const MemberExpr *MemEx = cast<MemberExpr>( E );
            const ValueDecl *MemDecl = MemEx->getMemberDecl();
            const FieldDecl *Field = dyn_cast<FieldDecl>( MemDecl );
            const CXXRecordDecl *Record =
                Field ? dyn_cast<CXXRecordDecl>( Field->getParent() ) : nullptr;
            if( isPointType( Record ) && MemDecl->getName() == Member ) {
                const Expr *Object = MemEx->getBase();
                return { { Result, Object, Record, MemEx->isArrow() } };
            } else {
                return { { Result, E } };
            }
        }
        case Stmt::ParenExprClass: {
            const ParenExpr *P = cast<ParenExpr>( E );
            std::vector<ExpressionComponent> result =
                decomposeExpr( P->getSubExpr(), Member, Result );
            if( !anyHaveMember( result ) ) {
                return { { Result, E } };
            }
            return result;
        }
        case Stmt::UnaryOperatorClass: {
            const UnaryOperator *Unary = cast<UnaryOperator>( E );
            std::vector<ExpressionComponent> Inner =
                decomposeExpr( Unary->getSubExpr(), Member, Result );
            if( Inner.empty() ) {
                return {};
            }
            if( !anyHaveMember( Inner ) ) {
                return { { Result, E } };
            }

            switch( Unary->getOpcode() ) {
                case UO_Minus:
                    for( ExpressionComponent &Component : Inner ) {
                        Component.coefficient *= -1;
                    }
                    return Inner;
                default:
                    return {};
            }
        }
        default:
            return { { Result, E } };
    }
}

static std::vector<ExpressionComponent> consolidateComponents(
    std::vector<ExpressionComponent> components )
{
    std::sort( components.begin(), components.end() );
    std::vector<ExpressionComponent> result;
    for( const ExpressionComponent &component : components ) {
        if( !result.empty() && result.back().canConsolidateWith( component ) ) {
            result.back().consolidate( component );
        } else {
            result.push_back( component );
        }
    }
    return result;
}

// The code needed to extract the given coordinates from (first) a point and
// (second) a tripoint.
static const std::unordered_map<std::string, std::pair<std::string, std::string>>
MemberAccessor = {
    { "x", { ".x", ".x" } },
    { "y", { ".y", ".y" } },
    { "z", { ".z", ".z" } },
    { "xy", { "", ".xy()" } },
    { "xyz", { "", "" } },
};

static void appendCoefficient( std::string &Result, int coefficient )
{
    switch( coefficient ) {
        case -1:
            if( Result.empty() ) {
                Result += "-";
            } else {
                Result += " - ";
            }
            break;
        case 0:
            break;
        case 1:
            if( !Result.empty() ) {
                Result += " + ";
            }
            break;
        default:
            if( Result.empty() ) {
                Result += std::to_string( coefficient ) + " * ";
            } else if( coefficient < 0 ) {
                Result += " - " + std::to_string( -coefficient ) + " * ";
            } else {
                Result += " + " + std::to_string( coefficient ) + " * ";
            }
    }
}

static std::string writeConstructor( const StringRef TypeName,
                                     const std::set<std::string> &Keys,
                                     std::map<std::string, std::string> Args )
{
    std::string Result = TypeName.str() + "( ";
    bool AnyLeftovers = false;
    for( const auto &Key : Keys ) {
        std::string &Leftover = Args[Key];
        if( Leftover.empty() ) {
            Leftover = "0";
        } else {
            AnyLeftovers = true;
        }
        Result += Leftover + ", ";
    }

    if( AnyLeftovers ) {
        Result.erase( Result.end() - 2, Result.end() );
        return Result + " )";
    }

    return "";
}

static void appendComponent( std::string &Result, const ExpressionComponent &Component,
                             const std::string &MemberKey )
{
    appendCoefficient( Result, Component.coefficient );

    std::string Prefix;
    std::string Accessor;

    if( Component.isMember ) {
        const std::pair<std::string, std::string> &Accessors = MemberAccessor.at( MemberKey );
        Accessor = Component.isTripoint ? Accessors.second : Accessors.first;
        if( Component.isArrowRef ) {
            if( Accessor.empty() ) {
                Prefix = "*";
            } else {
                cata_assert( Accessor[0] == '.' );
                Accessor.erase( Accessor.begin() );
                Accessor = "->" + Accessor;
            }
        }
    }

    Result += Prefix + Component.objectRef + Accessor;
}

static void appendMismatchedComponent(
    std::string &Result,
    const std::map<std::string, std::vector<ExpressionComponent>> &Components,
    const std::map<std::string, std::vector<ExpressionComponent>::const_iterator> &Positions,
    const ExpressionComponent &MinComponent, const std::string &Members )
{
    appendCoefficient( Result, 1 );
    std::string TypeName = Members == "xy" ? "point" : "tripoint";
    std::map<std::string, std::string> Coordinates;
    std::set<std::string> Keys;
    for( const auto &Position : Positions ) {
        const std::string &Key = Position.first;
        Keys.insert( Key );
        bool AtEnd = Position.second == Components.at( Key ).end();
        if( !AtEnd && Position.second->sortKey() == MinComponent.sortKey() ) {
            appendComponent( Coordinates[Key], *Position.second, Position.first );
        }
    }

    Result += writeConstructor( TypeName, Keys, Coordinates );
}

static void CheckConstructor( UsePointArithmeticCheck &Check,
                              const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCall" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const Expr *XExpr = Result.Nodes.getNodeAs<Expr>( "xexpr" );
    const Expr *YExpr = Result.Nodes.getNodeAs<Expr>( "yexpr" );
    const Expr *ZExpr = Result.Nodes.getNodeAs<Expr>( "zexpr" );
    const Expr *XYExpr = Result.Nodes.getNodeAs<Expr>( "xyexpr" );
    const Expr *TempParent = Result.Nodes.getNodeAs<Expr>( "temp" );
    if( !ConstructorCall || !ConstructorDecl ) {
        return;
    }

    std::map<std::string, std::vector<ExpressionComponent>> Components;

    auto insertComponentsIf = [&]( const Expr * E, const char *Key ) {
        if( E ) {
            Components.insert(
            { Key, consolidateComponents( decomposeExpr( E, Key, Result ) ) } );
        }
    };
    insertComponentsIf( XExpr, "x" );
    insertComponentsIf( YExpr, "y" );
    insertComponentsIf( ZExpr, "z" );
    insertComponentsIf( XYExpr, "xy" );

    if( Components.size() < 2 ) {
        return;
    }

    // Don't mess with the methods of point and tripoint themselves
    if( isPointMethod( getContainingFunction( Result, ConstructorCall ) ) ) {
        return;
    }

    std::string Joined;
    std::map<std::string, std::string> Leftovers;
    std::map<std::string, std::vector<ExpressionComponent>::const_iterator> Positions;

    bool AnyWithMultipleComponents = false;
    for( auto const &Component : Components ) {
        if( Component.second.size() > 1 ) {
            AnyWithMultipleComponents = true;
        }
        Positions.insert( { Component.first, Component.second.begin() } );
    }

    if( !AnyWithMultipleComponents ) {
        return;
    }

    while( true ) {
        bool XYAtEnd = false;
        for( auto const &Component : Components ) {
            if( Component.first == "z" ) {
                continue;
            }
            if( Positions.at( Component.first ) == Component.second.end() ) {
                XYAtEnd = true;
                break;
            }
        }
        if( XYAtEnd ) {
            // We have finished at least one of the lists, so there will be no
            // more matches.  We can simply append all the remainder to the
            // leftovers
            for( auto const &Component : Components ) {
                const std::string &Key = Component.first;
                auto Position = Positions.at( Key );
                while( Position != Component.second.end() ) {
                    appendComponent( Leftovers[Key], *Position, Key );
                    ++Position;
                }
            }
            break;
        }

        // Find the minimum element amongst the next elements
        std::string MinKey = Positions.begin()->first;
        const ExpressionComponent *CurrentMin = &*Positions.begin()->second;
        for( auto const &Position : Positions ) {
            const std::string &Key = Position.first;
            if( Position.second != Components.at( Key ).end() &&
                *Position.second < *CurrentMin ) {
                MinKey = Key;
                CurrentMin = &*Position.second;
            }
        }

        int AnyCoefficient = CurrentMin->coefficient;
        bool AllEqual = true;
        bool XYEqual = true;
        bool HasZ = false;
        bool CoefficientsEqual = true;
        bool InvolvesMacro = false;
        for( auto const &Position : Positions ) {
            const std::string &Key = Position.first;
            bool AtEnd = Position.second == Components.at( Key ).end();
            if( !AtEnd && *Position.second < *CurrentMin ) {
                abort(); // NOLINT(cata-assert)
            } else if( AtEnd || *Position.second > *CurrentMin ) {
                AllEqual = false;
                if( Key != "z" ) {
                    XYEqual = false;
                }
            } else {
                if( Key == "z" ) {
                    HasZ = true;
                }
                if( Position.second->coefficientInvolvesMacro ) {
                    InvolvesMacro = true;
                }
                if( Position.second->coefficient != AnyCoefficient ) {
                    CoefficientsEqual = false;
                }
            }
        }

        if( InvolvesMacro ) {
            Check.diag(
                ConstructorCall->getBeginLoc(),
                "Construction of %0 where a coefficient computation involves multiplication by "
                "a macro constant.  Should you be using one of the coordinate transformation "
                "functions?" ) <<
                               ConstructorDecl->getParent();
            return;
        }

        if( CurrentMin->isMember && XYEqual ) {
            // There is some level of equality, so try to output a joined
            // expression
            if( CoefficientsEqual ) {
                if( AllEqual && HasZ ) {
                    appendComponent( Joined, *CurrentMin, "xyz" );
                } else {
                    appendComponent( Joined, *CurrentMin, "xy" );
                }
            } else {
                if( AllEqual && HasZ ) {
                    appendMismatchedComponent(
                        Joined, Components, Positions, *CurrentMin, "xyz" );
                } else {
                    appendMismatchedComponent( Joined, Components, Positions, *CurrentMin, "xy" );
                }
            }
            for( auto &Position : Positions ) {
                const std::string &Key = Position.first;
                bool AtEnd = Position.second == Components.at( Key ).end();
                if( !AtEnd && Position.second->sortKey() == CurrentMin->sortKey() ) {
                    ++Position.second;
                }
            }
        } else {
            // Otherwise just emit a single-coordinate piece
            appendComponent( Leftovers[MinKey], *CurrentMin, MinKey );
            ++Positions.at( MinKey );
        }
    }

    if( Joined.empty() ) {
        return;
    }

    std::set<std::string> Keys;
    for( const auto &Component : Components ) {
        Keys.insert( Component.first );
    }

    StringRef TargetTypeName;
    if( Leftovers["z"].empty() ) {
        TargetTypeName = "point";
        Keys.erase( "z" );
    } else {
        TargetTypeName = ConstructorDecl->getParent()->getName();
    }
    std::string AccumulatedLeftovers = writeConstructor( TargetTypeName, Keys, Leftovers );

    if( !AccumulatedLeftovers.empty() ) {
        Joined += " + " + AccumulatedLeftovers;
    }

    const unsigned int LastArg = ConstructorCall->getNumArgs() - 1;
    SourceRange SourceRangeToReplace( ConstructorCall->getArg( 0 )->getBeginLoc(),
                                      ConstructorCall->getArg( LastArg )->getEndLoc() );

    if( TempParent ) {
        SourceRangeToReplace = ConstructorCall->getSourceRange();
    }

    CharSourceRange CharRangeToReplace = Lexer::makeFileCharRange(
            CharSourceRange::getTokenRange( SourceRangeToReplace ), *Result.SourceManager,
            Check.getLangOpts() );

    Check.diag(
        ConstructorCall->getBeginLoc(),
        "Construction of %0 can be simplified using overloaded arithmetic operators." ) <<
                ConstructorDecl->getParent() <<
                FixItHint::CreateReplacement( CharRangeToReplace, Joined );
}

void UsePointArithmeticCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( *this, Result );
}

} // namespace clang::tidy::cata
