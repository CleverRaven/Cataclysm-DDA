#include <clang/Basic/Version.h>
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>
#include <llvm/ADT/StringRef.h>

#include "AlmostNeverAutoCheck.h"
#include "AssertCheck.h"
#include "AvoidAlternativeTokensCheck.h"
#include "CombineLocalsIntoPointCheck.h"
#include "DeterminismCheck.h"
#include "HeaderGuardCheck.h"
#include "JsonTranslationInputCheck.h"
#include "LargeInlineFunctionCheck.h"
#include "LargeStackObjectCheck.h"
#include "NoLongCheck.h"
#include "NoStaticTranslationCheck.h"
#include "OtMatchCheck.h"
#include "PointInitializationCheck.h"
#include "RedundantParenthesesCheck.h"
#include "SerializeCheck.h"
#include "SimplifyPointConstructorsCheck.h"
#include "StaticDeclarationsCheck.h"
#include "StaticInitializationOrderCheck.h"
#include "StaticIntIdConstantsCheck.h"
#include "StaticStringIdConstantsCheck.h"
#include "TestFilenameCheck.h"
#include "TestsMustRestoreGlobalStateCheck.h"
#include "TextStyleCheck.h"
#include "TranslateStringLiteralCheck.h"
#include "TranslationsInDebugMessagesCheck.h"
#include "TranslatorCommentsCheck.h"
#include "U8PathCheck.h"
#include "UnitOverflowCheck.h"
#include "UnsequencedCallsCheck.h"
#include "UnusedStaticsCheck.h"
#include "UseLocalizedSortingCheck.h"
#include "UseMdarrayCheck.h"
#include "UseNamedPointConstantsCheck.h"
#include "UsePointApisCheck.h"
#include "UsePointArithmeticCheck.h"
#include "UseStringViewCheck.h"
#include "UTF8ToLowerUpperCheck.h"
#include "XYCheck.h"

#if defined( CATA_CLANG_TIDY_EXECUTABLE )
#include <clang-tidy/tool/ClangTidyMain.h>
#endif

namespace clang::tidy
{
namespace cata
{

class CataModule : public ClangTidyModule
{
    public:
        void addCheckFactories( ClangTidyCheckFactories &CheckFactories ) override {
            // Sanity check the clang version to verify that we're loaded into
            // the same version we linked against

            std::string RuntimeVersion = getClangFullVersion();
            if( !llvm::StringRef( RuntimeVersion ).contains( "clang version " CLANG_VERSION_STRING ) ) {
                llvm::report_fatal_error(
                    llvm::Twine( "clang version mismatch in CataTidyModule.  Compiled against "
                                 CLANG_VERSION_STRING " but loaded by ", RuntimeVersion ) );
                abort(); // NOLINT(cata-assert)
            }
            CheckFactories.registerCheck<AlmostNeverAutoCheck>( "cata-almost-never-auto" );
            CheckFactories.registerCheck<AssertCheck>( "cata-assert" );
            CheckFactories.registerCheck<AvoidAlternativeTokensCheck>(
                "cata-avoid-alternative-tokens" );
            CheckFactories.registerCheck<CombineLocalsIntoPointCheck>(
                "cata-combine-locals-into-point" );
            CheckFactories.registerCheck<DeterminismCheck>( "cata-determinism" );
            CheckFactories.registerCheck<CataHeaderGuardCheck>( "cata-header-guard" );
            CheckFactories.registerCheck<JsonTranslationInputCheck>( "cata-json-translation-input" );
            CheckFactories.registerCheck<LargeInlineFunctionCheck>( "cata-large-inline-function" );
            CheckFactories.registerCheck<LargeStackObjectCheck>( "cata-large-stack-object" );
            CheckFactories.registerCheck<NoLongCheck>( "cata-no-long" );
            CheckFactories.registerCheck<NoStaticTranslationCheck>( "cata-no-static-translation" );
            CheckFactories.registerCheck<OtMatchCheck>( "cata-ot-match" );
            CheckFactories.registerCheck<PointInitializationCheck>( "cata-point-initialization" );
            CheckFactories.registerCheck<RedundantParenthesesCheck>( "cata-redundant-parentheses" );
            CheckFactories.registerCheck<SerializeCheck>( "cata-serialize" );
            CheckFactories.registerCheck<SimplifyPointConstructorsCheck>(
                "cata-simplify-point-constructors" );
            CheckFactories.registerCheck<StaticInitializationOrderCheck>(
                "cata-static-initialization-order" );
            CheckFactories.registerCheck<StaticDeclarationsCheck>( "cata-static-declarations" );
            CheckFactories.registerCheck<StaticIntIdConstantsCheck>(
                "cata-static-int_id-constants" );
            CheckFactories.registerCheck<StaticStringIdConstantsCheck>(
                "cata-static-string_id-constants" );
            CheckFactories.registerCheck<TestFilenameCheck>( "cata-test-filename" );
            CheckFactories.registerCheck<TestsMustRestoreGlobalStateCheck>(
                "cata-tests-must-restore-global-state" );
            CheckFactories.registerCheck<TextStyleCheck>( "cata-text-style" );
            CheckFactories.registerCheck<TranslateStringLiteralCheck>(
                "cata-translate-string-literal" );
            CheckFactories.registerCheck<TranslationsInDebugMessagesCheck>(
                "cata-translations-in-debug-messages" );
            CheckFactories.registerCheck<TranslatorCommentsCheck>( "cata-translator-comments" );
            CheckFactories.registerCheck<U8PathCheck>( "cata-u8-path" );
            CheckFactories.registerCheck<UnitOverflowCheck>( "cata-unit-overflow" );
            CheckFactories.registerCheck<UnsequencedCallsCheck>( "cata-unsequenced-calls" );
            CheckFactories.registerCheck<UnusedStaticsCheck>( "cata-unused-statics" );
            CheckFactories.registerCheck<UseLocalizedSortingCheck>( "cata-use-localized-sorting" );
            CheckFactories.registerCheck<UseMdarrayCheck>( "cata-use-mdarray" );
            CheckFactories.registerCheck<UseNamedPointConstantsCheck>(
                "cata-use-named-point-constants" );
            CheckFactories.registerCheck<UsePointApisCheck>( "cata-use-point-apis" );
            CheckFactories.registerCheck<UsePointArithmeticCheck>( "cata-use-point-arithmetic" );
            CheckFactories.registerCheck<UseStringViewCheck>( "cata-use-string_view" );
            CheckFactories.registerCheck<UTF8ToLowerUpperCheck>( "cata-utf8-no-to-lower-to-upper" );
            CheckFactories.registerCheck<XYCheck>( "cata-xy" );
        }
};

} // namespace cata

// Register the MiscTidyModule using this statically initialized variable.
// NOLINTNEXTLINE(cata-unused-statics)
static ClangTidyModuleRegistry::Add<cata::CataModule>
X( "cata-module", "Adds Cataclysm-DDA checks." );

} // namespace clang::tidy

#if defined( CATA_CLANG_TIDY_EXECUTABLE )
int main( int argc, const char **argv )
{
    return clang::tidy::clangTidyMain( argc, argv );
}
#endif
