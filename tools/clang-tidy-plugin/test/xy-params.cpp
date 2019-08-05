// RUN: %check_clang_tidy %s cata-xy %t -- -plugins=%cata_plugin --

void f0( int x, int y );
// CHECK-MESSAGES: warning: 'f0' has parameters 'x' and 'y'.  Consider combining into a single point parameter. [cata-xy]

void f1( int foox, int fooy );
// CHECK-MESSAGES: warning: 'f1' has parameters 'foox' and 'fooy'.  Consider combining into a single point parameter. [cata-xy]

void f2( int foox, int bary );

void f3( int foox, int bary, int fooy );
// CHECK-MESSAGES: warning: 'f3' has parameters 'foox' and 'fooy'.  Consider combining into a single point parameter. [cata-xy]

void f4( int foox, int fooy, int barx, int bary );
// CHECK-MESSAGES: warning: 'f4' has parameters 'barx' and 'bary'.  Consider combining into a single point parameter. [cata-xy]
// CHECK-MESSAGES: warning: 'f4' has parameters 'foox' and 'fooy'.  Consider combining into a single point parameter. [cata-xy]

void g( int x, int y, int z );
// CHECK-MESSAGES: warning: 'g' has parameters 'x', 'y', and 'z'.  Consider combining into a single tripoint parameter. [cata-xy]

template<int>
int h( int x, int y );
// CHECK-MESSAGES: warning: 'h' has parameters 'x' and 'y'.  Consider combining into a single point parameter. [cata-xy]

int x0 = h<0>( 0, 0 );
int x1 = h<1>( 0, 0 );

// Verify that there are no warnings for non-int types
void i( float x, float y );
