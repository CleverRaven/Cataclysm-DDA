// RUN: %check_clang_tidy -allow-stdinc %s cata-serialize %t -- --load=%cata_plugin -- -isystem %cata_include -isystem %cata_include/third-party

#include "json.h"

class A0
{
        int a;
        // CHECK-MESSAGES: warning: Function 'deserialize' appears to be a serialization function for class 'A0' but does not mention field 'a'. [cata-serialize]
        int b;
        // CHECK-MESSAGES: warning: Function 'serialize' appears to be a serialization function for class 'A0' but does not mention field 'b'. [cata-serialize]
        bool was_loaded = false;

        void serialize( JsonOut &jsout ) const {
            jsout.member( "a", a );
        }

        void deserialize( JsonObject &jso ) {
            jso.read( "b", b );
        }
};

class A1
{
        int a;
        int b;
        bool was_loaded = false;

        void serialize( JsonOut &jsout ) const {
            // If it doesn't mention *any* members then it's probably not a
            // serialization function
        }

        void deserialize( JsonObject &jso ) {
            // Can suppress warnings for a particular function by putting the
            // following string in a comment:
            // CATA_DO_NOT_CHECK_SERIALIZE
            jso.read( "b", b );
        }
};

struct A2 {
    int a;
    int b;
    // CHECK-MESSAGES: warning: Function 'serialize' appears to be a serialization function for class 'A2' but does not mention field 'b'. [cata-serialize]
    bool was_loaded = false;

    template<typename T>
    void serialize( T &out ) const {
        out.member( "a", a );
    }
};

void f2( JsonOut &jsout )
{
    A2 a2;
    a2.serialize( jsout );
}
