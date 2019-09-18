#include <llvm/ADT/StringRef.h>

#include "ClangTidyModule.h"
#include "ClangTidyModuleRegistry.h"
#include "NoStaticGettextCheck.h"
#include "NoLongCheck.h"
#include "PointInitializationCheck.h"
#include "SimplifyPointConstructorsCheck.h"
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
            CheckFactories.registerCheck<NoStaticGettext>( "cata-no-static-gettext" );
            CheckFactories.registerCheck<NoLongCheck>( "cata-no-long" );
            CheckFactories.registerCheck<PointInitializationCheck>( "cata-point-initialization" );
            CheckFactories.registerCheck<SimplifyPointConstructorsCheck>(
                "cata-simplify-point-constructors" );
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
