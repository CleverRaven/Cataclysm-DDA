#include <llvm/ADT/StringRef.h>

#include "ClangTidyModule.h"
#include "ClangTidyModuleRegistry.h"
#include "DeterminismCheck.h"
#include "HeaderGuardCheck.h"
#include "JsonTranslationInputCheck.h"
#include "NoLongCheck.h"
#include "NoStaticGettextCheck.h"
#include "PointInitializationCheck.h"
#include "SimplifyPointConstructorsCheck.h"
#include "TestFilenameCheck.h"
#include "TextStyleCheck.h"
#include "TranslatorCommentsCheck.h"
#include "UseLocalizedSortingCheck.h"
#include "UseNamedPointConstantsCheck.h"
#include "UsePointApisCheck.h"
#include "UsePointArithmeticCheck.h"
#include "XYCheck.h"

namespace clang
{
namespace tidy
{
namespace cata
{

class CataModule : public ClangTidyModule
{
    public:
        void addCheckFactories( ClangTidyCheckFactories &CheckFactories ) override {
            CheckFactories.registerCheck<DeterminismCheck>( "cata-determinism" );
            CheckFactories.registerCheck<CataHeaderGuardCheck>( "cata-header-guard" );
            CheckFactories.registerCheck<JsonTranslationInputCheck>( "cata-json-translation-input" );
            CheckFactories.registerCheck<NoLongCheck>( "cata-no-long" );
            CheckFactories.registerCheck<NoStaticGettextCheck>( "cata-no-static-gettext" );
            CheckFactories.registerCheck<PointInitializationCheck>( "cata-point-initialization" );
            CheckFactories.registerCheck<SimplifyPointConstructorsCheck>(
                "cata-simplify-point-constructors" );
            CheckFactories.registerCheck<TestFilenameCheck>( "cata-test-filename" );
            CheckFactories.registerCheck<TextStyleCheck>( "cata-text-style" );
            CheckFactories.registerCheck<TranslatorCommentsCheck>( "cata-translator-comments" );
            CheckFactories.registerCheck<UseLocalizedSortingCheck>( "cata-use-localized-sorting" );
            CheckFactories.registerCheck<UseNamedPointConstantsCheck>(
                "cata-use-named-point-constants" );
            CheckFactories.registerCheck<UsePointApisCheck>( "cata-use-point-apis" );
            CheckFactories.registerCheck<UsePointArithmeticCheck>( "cata-use-point-arithmetic" );
            CheckFactories.registerCheck<XYCheck>( "cata-xy" );
        }
};

} // namespace cata

// Register the MiscTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<cata::CataModule>
X( "cata-module", "Adds Cataclysm-DDA checks." );

} // namespace tidy
} // namespace clang
