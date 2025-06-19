#include "StaticStringIdConstantsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static auto isStringIdType()
{
    return cxxRecordDecl( hasName( "string_id" ) );
}

static auto isStringIdConstructor()
{
    return cxxConstructorDecl( ofClass( isStringIdType() ) );
}

static auto testWhetherInStaticMacro()
{
    return cxxConstructExpr(
               anyOf(
                   hasAncestor(
                       callExpr(
                           callee( decl( functionDecl( hasName( "static_argument_identity" ) ) ) )
                       ).bind( "staticArgument" )
                   ),
                   anything()
               )
           );
}

static auto isStringIdConstructExpr()
{
    return cxxConstructExpr(
               hasDeclaration( isStringIdConstructor().bind( "constructorDecl" ) ),
               testWhetherConstructingTemporary(),
               testWhetherParentIsVarDecl(),
               testWhetherGrandparentIsTranslationUnitDecl(),
               testWhetherInStaticMacro(),
               hasArgument( 0, stringLiteral().bind( "arg" ) )
           ).bind( "constructorCall" );
}

void StaticStringIdConstantsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        isStringIdConstructExpr(),
        this
    );
    Finder->addMatcher(
        declRefExpr(
            hasDeclaration( varDecl( hasInitializer( isStringIdConstructExpr() ) ) )
        ).bind( "declRef" ),
        this
    );
}

static std::string GetPrefixFor( const CXXRecordDecl *Type, StaticStringIdConstantsCheck &Check )
{
    const ClassTemplateSpecializationDecl *CTSDecl =
        dyn_cast<ClassTemplateSpecializationDecl>( Type );
    if( !CTSDecl ) {
        Check.diag( Type->getBeginLoc(),
                    "Declaration of string_id instantiation was not a tempalte specialization" );
        return {};
    }
    QualType ArgType = CTSDecl->getTemplateArgs()[0].getAsType();
    PrintingPolicy Policy( LangOptions{} );
    Policy.adjustForCPlusPlus();
    std::string TypeName = ArgType.getAsString( Policy );

    static const std::unordered_map<std::string, std::string> HardcodedPrefixes = {
        { "activity_type", "" },
        { "add_type", "addiction_" },
        { "ammunition_type", "ammo_" },
        { "bionic_data", "" },
        { "fault", "" },
        { "furn_t", "furn_" },
        { "ma_technique", "" },
        { "martialart", "" },
        { "MonsterGroup", "" },
        { "morale_type_data", "" },
        { "mtype", "" },
        { "mutation_branch", "trait_" },
        { "mutation_category_trait", "mutation_category_" },
        { "npc_class", "" },
        { "oter_t", "oter_" },
        { "oter_type_t", "oter_type_" },
        { "quality", "qual_" },
        { "Skill", "skill_" },
        { "ter_t", "ter_" },
        { "trap", "" },
        { "zone_type", "zone_type_" },
    };

    auto HardcodedPrefix = HardcodedPrefixes.find( TypeName );
    if( HardcodedPrefix != HardcodedPrefixes.end() ) {
        return HardcodedPrefix->second;
    }

    for( const char *Suffix : {
             "_type", "_info"
         } ) {
        if( StringRef( TypeName ).ends_with( Suffix ) ) {
            TypeName.erase( TypeName.end() - strlen( Suffix ), TypeName.end() );
        }
    }

    return TypeName + "_";
}

static std::string GetCanonicalName( const CXXRecordDecl *Type, const StringRef &Id,
                                     StaticStringIdConstantsCheck &Check )
{
    std::string Result = ( GetPrefixFor( Type, Check ) + Id ).str();

    for( char &c : Result ) {
        if( !isalnum( c ) ) {
            c = '_';
        }
    }

    static const std::string anon_prefix = "_anonymous_namespace___";
    if( StringRef( Result ).starts_with( anon_prefix ) ) {
        Result.erase( Result.begin(), Result.begin() + anon_prefix.size() );
    }

    // Remove double underscores since they are reserved identifiers
    while( true ) {
        size_t double_underscores = Result.find( "__" );
        if( double_underscores == std::string::npos ) {
            break;
        }
        Result.erase( Result.begin() + double_underscores );
    }

    return Result;
}

static CharSourceRange RangeForDecl( const VarDecl *d, SourceManager &SM )
{
    SourceLocation begin = d->getBeginLoc();
    const char *data = SM.getCharacterData( begin );
    size_t decl_len = std::strcspn( data, "\n" );
    return CharSourceRange::getCharRange( begin, begin.getLocWithOffset( decl_len + 1 ) );
}

void StaticStringIdConstantsCheck::CheckConstructor( const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *ConstructorCall =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "constructorCall" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const StringLiteral *Arg = Result.Nodes.getNodeAs<StringLiteral>( "arg" );
    const VarDecl *VarDeclParent = Result.Nodes.getNodeAs<VarDecl>( "parentVarDecl" );
    const TranslationUnitDecl *TranslationUnit =
        Result.Nodes.getNodeAs<TranslationUnitDecl>( "grandparentTranslationUnit" );
    if( !ConstructorCall || !ConstructorDecl || !Arg ) {
        return;
    }

    const SourceManager &SM = *Result.SourceManager;

    // Ignore cases in header files for now
    if( !SM.isInMainFile( ConstructorCall->getBeginLoc() ) ) {
        return;
    }

    std::string CanonicalName =
        GetCanonicalName( ConstructorDecl->getParent(), Arg->getString(), *this );

    if( VarDeclParent && TranslationUnit ) {
        if( VarDeclParent->isStaticDataMember() ) {
            return;
        }

        const VarDecl *PreviousDecl = dyn_cast_or_null<VarDecl>( VarDeclParent->getPreviousDecl() );
        bool PreviousDeclIsExtern =
            PreviousDecl ? PreviousDecl->getStorageClass() == SC_Extern : false;

        // This is already a global-scope declaration.  Verify that it's const
        // and static.
        if( !VarDeclParent->getType().isConstQualified() ) {
            diag(
                ConstructorCall->getBeginLoc(),
                "Global declaration of %0 should be const."
            ) << VarDeclParent <<
              FixItHint::CreateInsertion( VarDeclParent->getTypeSpecStartLoc(), "const " );
            any_wrong_names_ = true;
        } else if( VarDeclParent->getStorageClass() != SC_Static && !PreviousDeclIsExtern ) {
            diag(
                ConstructorCall->getBeginLoc(),
                "Global declaration of %0 should be static."
            ) << VarDeclParent <<
              FixItHint::CreateInsertion( VarDeclParent->getSourceRange().getBegin(), "static " );
            any_wrong_names_ = true;
        }
        std::string CurrentName = VarDeclParent->getNameAsString();
        if( CurrentName != CanonicalName &&
            !PreviousDeclIsExtern &&
            !StringRef( CurrentName ).starts_with( "fuel_type_" ) ) {
            SourceRange Range =
                DeclarationNameInfo( VarDeclParent->getDeclName(), VarDeclParent->getLocation()
                                   ).getSourceRange();
            diag(
                ConstructorCall->getBeginLoc(),
                "Declaration of string_id %0 should be named '%1'."
            ) << VarDeclParent << CanonicalName <<
              FixItHint::CreateReplacement( Range, CanonicalName );
            any_wrong_names_ = true;
        }
        if( found_decls_set_.insert( VarDeclParent ).second ) {
            CharSourceRange DeclRange = RangeForDecl( VarDeclParent, *Result.SourceManager );
            StringRef DeclText = getText( Result, DeclRange );
            found_decls_.push_back(
                FoundDecl{ VarDeclParent, DeclRange, DeclText } );
        }
        return;
    }

    // At this point we are looking at a construction call which is not a
    // VarDecl at translation unit scope

    // First ignore anything in a STATIC macro call
    const CallExpr *IsInStaticMacro = Result.Nodes.getNodeAs<CallExpr>( "staticArgument" );
    unsigned offset = SM.getFileOffset( ConstructorCall->getBeginLoc() );
    if( !IsInStaticMacro && !CanonicalName.empty() && promotable_set_.insert( offset ).second ) {
        promotable_.push_back(
            PromotableCall{ VarDeclParent, ConstructorCall, CanonicalName, Arg->getString() } );
    }
}

struct CompareDecls {
    auto as_tuple( const VarDecl *d ) const {
        return std::make_pair( d->getType().getLocalUnqualifiedType().getAsString(), d->getName() );
    }

    auto as_tuple( const FoundDecl &d ) const {
        return as_tuple( d.decl );
    }

    auto as_tuple( const PromotableCall &p ) const {
        return std::make_pair( p.construction->getType().getAsString(),
                               StringRef( p.canonical_name ) );
    }

    template<typename T, typename U>
    bool operator()( const T &l, const U &r ) const {
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        return as_tuple( l ) < as_tuple( r );
    }

    template<typename T, typename U>
    bool operator()( const T *l, const U *r ) const {
        return ( *this )( *l, *r );
    }
};

static CompareDecls compare_decls;

void StaticStringIdConstantsCheck::onEndOfTranslationUnit()
{
    if( any_wrong_names_ ) {
        // Can't safely perform these checks if anything else was corrected
        return;
    }

    // In certain corner cases the AST can visit the declarations in a
    // different order than they appear in the source file.  Thus, we need to
    // sort them by source file order before doing any further analysis.
    auto compare_text_positions = []( const FoundDecl & l, const FoundDecl & r ) {
        return l.text.data() < r.text.data();
    };
    std::sort( found_decls_.begin(), found_decls_.end(), compare_text_positions );

    const auto begin = found_decls_.begin();

    auto last_before_gap = std::adjacent_find(
                               begin, found_decls_.end(),
    []( const FoundDecl & decl1, const FoundDecl & decl2 ) {
        return decl2.decl != decl1.decl->getNextDeclInContext();
    } );

    if( last_before_gap == found_decls_.end() ) {
        --last_before_gap;
    }

    auto first_after_gap = last_before_gap + 1;

    auto out_of_order = []( const FoundDecl & l, const FoundDecl & r ) {
        return compare_decls( r, l );
    };

    // Check first that all the decls in the contiguous chunk are in sorted
    // order

    auto out_of_order_it = std::adjacent_find( begin, first_after_gap, out_of_order );
    if( out_of_order_it != first_after_gap ) {
        const VarDecl *wrong_pair_first = out_of_order_it[0].decl;
        const VarDecl *wrong_pair_second = out_of_order_it[1].decl;
        CharSourceRange RangeToReplace =
            CharSourceRange::getCharRange( found_decls_.front().range.getBegin(),
                                           last_before_gap->range.getEnd() );
        std::sort( begin, first_after_gap, compare_decls );
        std::string Replacement;
        QualType last_type;
        for( auto d = begin; d < first_after_gap; ++d ) {
            if( last_type != d->decl->getType() ) {
                if( last_type != QualType() ) {
                    Replacement += "\n";
                }
                last_type = d->decl->getType();
            }
            Replacement += d->text;
        }
        diag(
            found_decls_.front().decl->getBeginLoc(),
            "string_id declarations should be sorted."
        ) << FixItHint::CreateReplacement( RangeToReplace, Replacement );
        diag(
            wrong_pair_second->getBeginLoc(),
            "%0 should be before %1.", DiagnosticIDs::Note
        ) << wrong_pair_second << wrong_pair_first;
        return;
    }

    // Now we're in the case where the initial segment of declarations is
    // sorted, and we need to add any later ones in amongst them.
    if( first_after_gap != found_decls_.end() ) {
        std::unordered_map<ptrdiff_t, std::vector<const FoundDecl *>> to_insert;
        std::vector<FixItHint> fixits;
        for( auto decl_it = first_after_gap; decl_it != found_decls_.end(); ++decl_it ) {
            auto insert_at_it = std::lower_bound( begin, first_after_gap, *decl_it, compare_decls );
            ptrdiff_t insert_at_pos = insert_at_it - begin;
            to_insert[insert_at_pos].push_back( &*decl_it );
            fixits.push_back( FixItHint::CreateRemoval( decl_it->range ) );
        }

        for( std::pair<const ptrdiff_t, std::vector<const FoundDecl *>> &p : to_insert ) {
            const ptrdiff_t insert_at_pos = p.first;
            std::vector<const FoundDecl *> &decls_to_insert = p.second;
            const bool at_end = insert_at_pos == first_after_gap - begin;
            SourceLocation insert_at;
            QualType type_after;
            if( at_end ) {
                insert_at = last_before_gap->range.getEnd();
            } else {
                insert_at = found_decls_[insert_at_pos].range.getBegin();
                type_after = found_decls_[insert_at_pos].decl->getType();
            }
            QualType last_type;
            if( insert_at_pos != 0 ) {
                last_type = found_decls_[insert_at_pos - 1].decl->getType();
            }

            std::sort( decls_to_insert.begin(), decls_to_insert.end(), compare_decls );
            std::string to_insert;
            for( const FoundDecl *decl : decls_to_insert ) {
                if( last_type != decl->decl->getType() ) {
                    if( last_type != QualType() ) {
                        to_insert += "\n";
                    }
                    last_type = decl->decl->getType();
                }

                to_insert += decl->text;
            }
            if( !at_end && last_type != type_after ) {
                to_insert += "\n";
            }
            fixits.push_back( FixItHint::CreateInsertion( insert_at, to_insert ) );
        }

        diag(
            first_after_gap->decl->getBeginLoc(),
            "string_id declarations should be together."
        ) << fixits;
        int num_out_of_place = found_decls_.end() - first_after_gap - 1;
        diag(
            begin->decl->getBeginLoc(),
            "%0 (and %2 others) should be added to the group starting at %1.", DiagnosticIDs::Note
        ) << first_after_gap->decl << begin->decl << ( num_out_of_place - 1 );
        if( const NamedDecl *separating_decl =
                dyn_cast_or_null<NamedDecl>( last_before_gap->decl->getNextDeclInContext() ) ) {
            diag(
                separating_decl->getBeginLoc(), "They are currently separated by %0.",
                DiagnosticIDs::Note
            ) << separating_decl;
        }
        return;
    }

    // If we reached here then we know that all the static declarations are in
    // a single group and are sorted properly.  So it's safe to start promoting
    // temporaries to statics
    std::unordered_map<ptrdiff_t, std::vector<const PromotableCall *>> to_insert;
    for( const PromotableCall &prom : promotable_ ) {
        if( prom.decl ) {
            // Doing this automatically for local variable declarations is
            // hard, so don't try; just report a warning.
            if( prom.decl->getType().isConstQualified() ) {
                diag( prom.construction->getBeginLoc(),
                      "Local const string_id instances with fixed content should be promoted to a "
                      "static instance at global scope." );
            }
        } else if( found_decls_.empty() ) {
            // If no existing decls then we can't know where to put them, so
            // just report a warning.
            diag(
                prom.construction->getBeginLoc(),
                "Temporary string_id instances with fixed content should be promoted to a static "
                "instance at global scope."
            );
        } else {
            auto insert_at_it = std::upper_bound( begin, found_decls_.end(), prom, compare_decls );
            ptrdiff_t insert_at_pos = insert_at_it - begin;
            to_insert[insert_at_pos].push_back( &prom );
        }
    }

    auto decl_for_call = []( const PromotableCall * call ) {
        std::string result = "static const ";
        result += call->construction->getType().getAsString();
        result += " ";
        result += call->canonical_name;
        result += "( \"";
        result += call->string_literal_arg;
        result += "\" );\n";
        return result;
    };

    for( std::pair<const ptrdiff_t, std::vector<const PromotableCall *>> &p : to_insert ) {
        const ptrdiff_t insert_at_pos = p.first;
        std::vector<const PromotableCall *> &calls = p.second;

        const bool at_end = insert_at_pos == found_decls_.end() - begin;
        bool inserting_at_end = false;
        SourceLocation insert_at;
        QualType type_after;
        StringRef last_canonical_name;
        if( at_end ) {
            insert_at = last_before_gap->range.getEnd();
            inserting_at_end = true;
        } else {
            insert_at = found_decls_[insert_at_pos].range.getBegin();
            type_after = found_decls_[insert_at_pos].decl->getType().getLocalUnqualifiedType();
        }
        QualType last_type;
        if( insert_at_pos != 0 ) {
            const VarDecl *last_decl = found_decls_[insert_at_pos - 1].decl;
            last_type = last_decl->getType().getLocalUnqualifiedType();
            last_canonical_name = last_decl->getName();
        }
        if( last_type == calls.front()->construction->getType() ) {
            // When the type is the same as the one we're inserting after we
            // whouls switch to inserting at the end of the previous rather
            // than the beginning of the next
            insert_at = found_decls_[insert_at_pos - 1].range.getEnd();
            inserting_at_end = true;
        }

        std::sort( calls.begin(), calls.end(), compare_decls );

        std::string to_insert;
        std::vector<FixItHint> fixits;
        bool first = true;
        for( const PromotableCall *call : calls ) {
            fixits.push_back(
                FixItHint::CreateReplacement( call->construction->getSourceRange(),
                                              call->canonical_name ) );
            if( call->canonical_name != last_canonical_name ) {
                if( last_type != call->construction->getType() ) {
                    if( last_type != QualType() && ( !first || inserting_at_end ) ) {
                        to_insert += "\n";
                    }
                    last_type = call->construction->getType();
                }
                to_insert += decl_for_call( call );
                last_canonical_name = call->canonical_name;
            }
            first = false;
        }
        if( !inserting_at_end && last_type != type_after ) {
            to_insert += "\n";
        }
        fixits.push_back( FixItHint::CreateInsertion( insert_at, to_insert ) );
        diag(
            calls.front()->construction->getBeginLoc(),
            "Temporary string_id instances with fixed content should be promoted to a static "
            "instance at global scope."
        ) << fixits;
        for( auto call = calls.begin() + 1; call != calls.end(); ++call ) {
            diag(
                ( *call )->construction->getBeginLoc(),
                "Temporary string_id instances with fixed content should be promoted to a static "
                "instance at global scope."
            );
        }
    }
}

static void CheckDeclRef( StaticStringIdConstantsCheck &Check,
                          const MatchFinder::MatchResult &Result )
{
    const DeclRefExpr *Ref = Result.Nodes.getNodeAs<DeclRefExpr>( "declRef" );
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructorDecl" );
    const StringLiteral *Arg = Result.Nodes.getNodeAs<StringLiteral>( "arg" );
    const VarDecl *VarDeclParent = Result.Nodes.getNodeAs<VarDecl>( "parentVarDecl" );
    const TranslationUnitDecl *TranslationUnit =
        Result.Nodes.getNodeAs<TranslationUnitDecl>( "grandparentTranslationUnit" );

    if( !Ref || !ConstructorDecl || !Arg || !VarDeclParent || !TranslationUnit ) {
        return;
    }

    const SourceManager &SM = *Result.SourceManager;

    // Ignore cases in header files for now
    if( !SM.isInMainFile( VarDeclParent->getBeginLoc() ) ) {
        return;
    }

    std::string CanonicalName =
        GetCanonicalName( ConstructorDecl->getParent(), Arg->getString(), Check );
    std::string CurrentName = VarDeclParent->getNameAsString();

    if( CurrentName != CanonicalName &&
        !StringRef( CurrentName ).starts_with( "fuel_type_" ) ) {
        Check.diag(
            Ref->getBeginLoc(),
            "Use of string_id %0 should be named '%1'."
        ) << Ref->getDecl() << CanonicalName <<
          FixItHint::CreateReplacement( Ref->getSourceRange(), CanonicalName );
    }
}

void StaticStringIdConstantsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( Result );
    CheckDeclRef( *this, Result );
}

} // namespace clang::tidy::cata
