# Testing Cataclysm

When you `make` Cataclysm from source, an executable `tests/cata_test` is built
from test cases found in the `tests/` directory. These tests are written in the
[Catch2 framework](https://github.com/catchorg/Catch2).

Run `tests/cata_test --help` to see the available command-line options and/or
consult the [Catch2 tutorial](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md)
for a more thorough introduction.


## Guidelines

When creating tests, ensure that all objects used (directly or indirectly) are
fully reset before testing. Several tests have been rendered flaky by
properties of randomly generated objects or interactions between tests via
global objects (often the player object). As a general guideline, test cases
should be standalone (one test should not rely on the output of another).

When generating objects with json definitions, use REQUIRE statements to assert
the properties of the objects that the test needs. This protects the test from
shifting json definitions by making it apparent what about the object changed
to cause the test to break.


## Writing test cases

You can choose several ways to organize and express your tests, but the basic
unit is a `TEST_CASE`. Each test `.cpp` file should define at least one test
case, with a name, and an optional (but strongly encouraged) list of tags:

```cpp
    TEST_CASE( "sweet junk food", "[food][junk][sweet]" )
    {
        // ...
    }
```

Within the `TEST_CASE`, the Catch2 framework allows a number of different
macros for logically grouping related parts of the test together. One approach
that encourages a high level of readability is the
[BDD](https://en.wikipedia.org/wiki/Behavior-driven_development)
(behavior-driven-development) style using `GIVEN`, `WHEN`, and `THEN` sections.
Here's an outline of what a test might look like using those:

```cpp
    TEST_CASE( "sweet junk food", "[food][junk][sweet]" )
    {
        GIVEN( "character has a sweet tooth" ) {

            WHEN( "they eat some junk food" ) {

                THEN( "they get a morale bonus from its sweetness" ) {
                }
            }
        }
    }
```

Thinking in these terms may help you understand the logical progression from
setting up the test and initializing the test data (usually expressed by the
`GIVEN` part), performing some operation that generates a result you want to
test (often contained in the `WHEN` part), and verifying this result meets your
expectations (the `THEN` part, naturally).

Filling in the above with actual test code might look like this:

```cpp
    TEST_CASE( "sweet junk food", "[food][junk][sweet]" )
    {
        avatar dummy;
        dummy.clear_morale();

        GIVEN( "character has a sweet tooth" ) {
            dummy.toggle_trait( trait_PROJUNK );

            WHEN( "they eat some junk food" ) {
                item necco( "neccowafers" );
                dummy.eat( necco );

                THEN( "they get a morale bonus from its sweetness" ) {
                    CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) >= 5 );
                }
            }
        }
    }
```

Let's look at each part in turn to see what's going on. First, we declare an
`avatar`, representing the character or player. This test is going to check the
player's morale, so we clear it to ensure a clean slate:

```cpp
    avatar dummy;
    dummy.clear_morale();
```

Inside the `GIVEN`, we want some code that implements what the `GIVEN` is
saying - that the character has a sweet tooth. In the game's code, this is
represented with the `PROJUNK` trait, so we can set that using `toggle_trait`:

```cpp
    GIVEN( "character has a sweet tooth" ) {
        dummy.toggle_trait( trait_PROJUNK );
```

Now, notice we are nested inside the `GIVEN` - for the rest of the scope of
that `GIVEN`, the `dummy` will have this trait. For this simple test it will
only affect a couple more lines, but when your tests become larger and more
complex (which they will), you will need to be aware of these nested scopes and
how you can use them to avoid cross-pollution between your tests.

Anyway, now that our `dummy` has a sweet tooth, we want them to eat something
sweet, so we can spawn the `neccowafers` item and tell them to eat some:

```cpp
    WHEN( "they eat some junk food" ) {
        dummy.eat( item( "neccowafers" ) );
```

The function(s) you invoke at this point are often the focus of your testing;
the goal is to exercise some pathway through those function(s) in such a way
that your code will be reached, and thus covered by the test. The `eat`
function is used as an example here, but that is quite a high-level, complex
function itself, with many behaviors and sub-behaviors. Since this test case is
only interested in the morale effect, a better test would invoke a lower-level
function that `eat` invokes, such as `modify_morale`.

Our `dummy` has eaten the `neccowafers`, but did it do anything? Because they
have a sweet tooth, they should get a specific morale bonus known as
`MORALE_SWEETTOOTH`, and it should be at least `5` in magnitude:

```cpp
    THEN( "they get a morale bonus from its sweetness" ) {
        CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) >= 5 );
    }
```

This `CHECK` macro takes a boolean expression, failing the test if the
expression is false. Likewise, you can use `CHECK_FALSE`, which will fail if
the expression is true.


## Requiring or Checking

While the `CHECK` and `CHECK_FALSE` macros make assertions about the truth or
falsity of expressions, they still allow the test to continue, even when they
fail. This lets you do several `CHECK`s, and be informed if one *or more* of
them do not meet your expectations.

Another kind of assertion is the `REQUIRE` (and its counterpart
`REQUIRE_FALSE`). Unlike the `CHECK` assertions, `REQUIRE` will not continue if
it fails - this assertion is considered essential for the test to continue.

A `REQUIRE` is useful when you wish to double-check your assumptions after
making some change to the system state. For example, here are a couple of
`REQUIRE`s added to the sweet-tooth test, to ensure our `dummy` really has the
desired trait, and that the `neccowafers` really are junk food:

```cpp
    GIVEN( "character has a sweet tooth" ) {
        dummy.toggle_trait( trait_PROJUNK );
        REQUIRE( dummy.has_trait( trait_PROJUNK ) );

        WHEN( "they eat some junk food" ) {
            item necco( "neccowafers" );
            REQUIRE( necco.has_flag( "ALLERGEN_JUNK" ) );

            dummy.eat( necco );

            THEN( "they get a morale bonus from its sweetness" ) {
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) >= 5 );
            }
        }
    }
```

We use `REQUIRE` here, because there is no reason to continue the test if these
fail. If our assumptions are wrong, nothing that follows is valid. Clearly, if
`toggle_trait` failed to give the character the `PROJUNK` trait, or if the
`neccowafers` turn out not to be made of sugar after all, then our test of the
morale bonus is meaningless.

You can think of `REQUIRE` as being a prerequisite for the test, while `CHECK`
is looking at the results of the test.
