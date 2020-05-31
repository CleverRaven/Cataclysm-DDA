#include "StaticStringIdConstantsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

static auto isStringIdType()
{
    return cxxRecordDecl( hasName( "string_id" ) );
}

static auto isStringIdConstructor()
{
    return cxxConstructorDecl( ofClass( isStringIdType() ) );
}

static auto isStringIdConstructExpr()
{
    return cxxConstructExpr(
               hasDeclaration( isStringIdConstructor().bind( "constructorDecl" ) ),
               testWhetherConstructingTemporary(),
               testWhetherParentIsVarDecl(),
               testWhetherGrandparentIsTranslationUnitDecl(),
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

static std::string GetPrefixFor( const CXXRecordDecl *Type )
{
    const ClassTemplateSpecializationDecl *CTSDecl =
        dyn_cast<ClassTemplateSpecializationDecl>( Type );
    QualType ArgType = CTSDecl->getTemplateArgs()[0].getAsType();
    PrintingPolicy Policy( LangOptions{} );
    Policy.SuppressTagKeyword = true;
    std::string TypeName = ArgType.getAsString( Policy );

    static const std::unordered_map<std::string, std::string> HardcodedPrefixes = {
        { "activity_type", "" },
        { "ammunition_type", "ammo_" },
        { "bionic_data", "" },
        { "fault", "" },
        { "ma_technique", "" },
        { "martialart", "" },
        { "MonsterGroup", "" },
        { "morale_type_data", "" },
        { "mtype", "" },
        { "mutation_branch", "trait_" },
        { "npc_class", "" },
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
        if( StringRef( TypeName ).endswith( Suffix ) ) {
            TypeName.erase( TypeName.end() - strlen( Suffix ), TypeName.end() );
        }
    }

    return TypeName + "_";
}

static std::string GetCanonicalName( const CXXRecordDecl *Type, const StringRef &Id )
{
    std::string Result = ( GetPrefixFor( Type ) + Id ).str();

    for( char &c : Result ) {
        if( !isalnum( c ) ) {
            c = '_';
        }
    }

    return Result;
}

static void CheckConstructor( StaticStringIdConstantsCheck &Check,
                              const MatchFinder::MatchResult &Result )
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

    std::string CanonicalName = GetCanonicalName( ConstructorDecl->getParent(), Arg->getString() );

    if( VarDeclParent && TranslationUnit ) {
        const VarDecl *PreviousDecl = dyn_cast_or_null<VarDecl>( VarDeclParent->getPreviousDecl() );
        bool PreviousDeclIsExtern =
            PreviousDecl ? PreviousDecl->getStorageClass() == SC_Extern : false;

        // This is already a global-scope declaration.  Verify that it's const
        // and static.
        if( !VarDeclParent->getType().isConstQualified() ) {
            Check.diag(
                ConstructorCall->getBeginLoc(),
                "Global declaration of %0 should be const."
            ) << VarDeclParent <<
              FixItHint::CreateInsertion( VarDeclParent->getTypeSpecStartLoc(), "const " );
        } else if( VarDeclParent->getStorageClass() != SC_Static && !PreviousDeclIsExtern ) {
            Check.diag(
                ConstructorCall->getBeginLoc(),
                "Global declaration of %0 should be static."
            ) << VarDeclParent <<
              FixItHint::CreateInsertion( VarDeclParent->getSourceRange().getBegin(), "static " );
        }
        std::string CurrentName = VarDeclParent->getNameAsString();
        if( CurrentName != CanonicalName &&
            !PreviousDeclIsExtern &&
            !StringRef( CurrentName ).startswith( "fuel_type_" ) ) {
            SourceRange Range = DeclarationNameInfo(
                                    VarDeclParent->getDeclName(), VarDeclParent->getLocation() ).getSourceRange();
            Check.diag(
                ConstructorCall->getBeginLoc(),
                "Declaration of string_id %0 should be named '%1'."
            ) << VarDeclParent << CanonicalName <<
              FixItHint::CreateReplacement( Range, CanonicalName );
        }
        return;
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

    std::string CanonicalName = GetCanonicalName( ConstructorDecl->getParent(), Arg->getString() );
    std::string CurrentName = VarDeclParent->getNameAsString();

    if( CurrentName != CanonicalName &&
        !StringRef( CurrentName ).startswith( "fuel_type_" ) ) {
        Check.diag(
            Ref->getBeginLoc(),
            "Use of string_id %0 should be named '%1'."
        ) << Ref->getDecl() << CanonicalName <<
          FixItHint::CreateReplacement( Ref->getSourceRange(), CanonicalName );
    }
}

void StaticStringIdConstantsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( *this, Result );
    CheckDeclRef( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
