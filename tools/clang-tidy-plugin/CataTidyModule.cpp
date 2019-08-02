#include <llvm/ADT/StringRef.h>

#include "ClangTidyModule.h"
#include "ClangTidyModuleRegistry.h"
#include "NoLongCheck.h"
#include "PointInitializationCheck.h"
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
            CheckFactories.registerCheck<NoLongCheck>( "cata-no-long" );
            CheckFactories.registerCheck<PointInitializationCheck>( "cata-point-initialization" );
            CheckFactories.registerCheck<XYCheck>( "cata-xy" );
        }
};

} // namespace cata

// Register the MiscTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<cata::CataModule>
X( "cata-module", "Adds Cataclysm-DDA checks." );

} // namespace tidy
} // namespace clang
