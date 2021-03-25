// RUN: %check_clang_tidy %s cata-tests-must-restore-global-state %t -- -plugins=%cata_plugin -- -isystem %cata_include

bool fov_3d;
int fov_3d_z_range;

// The check identifies test files as being those which define the TEST_CASE
// macro.
#define TEST_CASE(name)

template<typename T>
class restore_on_out_of_scope
{
    public:
        explicit restore_on_out_of_scope( T &t_in );
};

void f0()
{
    fov_3d = true;
    // CHECK-MESSAGES: warning: Test alters global variable 'fov_3d'. You must ensure it is restored using 'restore_on_out_of_scope'. [cata-tests-must-restore-global-state]
}

void f1()
{
    restore_on_out_of_scope<int> restore( fov_3d_z_range );
    fov_3d_z_range = 1;
}
