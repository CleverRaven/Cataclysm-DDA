// RUN: %check_clang_tidy %s cata-serialize %t -- -plugins=%cata_plugin --

struct JsonIn {
    bool read( int & );
};

struct JsonOut {
    void member( const char *, int );
};

class A
{
        int a;
        // CHECK-MESSAGES: warning: Function 'deserialize' appears to be a serialization function for class 'A' but does not mention field 'a'. [cata-serialize]
        int b;
        // CHECK-MESSAGES: warning: Function 'serialize' appears to be a serialization function for class 'A' but does not mention field 'b'. [cata-serialize]
        bool was_loaded = false;

        void serialize( JsonOut &jsout ) const {
            jsout.member( "a", a );
        }

        void deserialize( JsonIn &jsin ) {
            jsin.read( b );
        }
};
