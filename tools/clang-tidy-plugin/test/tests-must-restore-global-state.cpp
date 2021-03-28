// RUN: %check_clang_tidy %s cata-tests-must-restore-global-state %t -- -plugins=%cata_plugin -- -isystem %cata_include

bool fov_3d;
int fov_3d_z_range;

namespace N
{
bool another_option;
}

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

void f1b()
{
    fov_3d_z_range = 1;
    // CHECK-MESSAGES: warning: Test alters global variable 'fov_3d_z_range'. You must ensure it is restored using 'restore_on_out_of_scope'. [cata-tests-must-restore-global-state]
}

void f2()
{
    int local_var;
    local_var = 1;
}

void f3()
{
    N::another_option = true;
    // CHECK-MESSAGES: warning: Test alters global variable 'another_option'. You must ensure it is restored using 'restore_on_out_of_scope'. [cata-tests-must-restore-global-state]
}

struct weather_type_id {};

struct weather_manager {
    weather_type_id weather_override;
};

weather_manager &get_weather();

void f4()
{
    get_weather().weather_override = {};
    // CHECK-MESSAGES: warning: Test assigns to weather_override.  It should instead use scoped_weather_override. [cata-tests-must-restore-global-state]
}
