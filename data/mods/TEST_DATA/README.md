# Test Data pseudo-mod #

This mod is purely for loading data to be used by `tests/cata_test`. It is
automatically loaded by `tests/test_main.cpp`, so any items, recipes, or other
content defined in the mod will be available to everything in `tests/`.

The benefit of using this mod for test data is that it allows a clean
separation of tests from in-game content. Instead of testing with content in
the main `data/json` directory, functional tests can use `TEST_DATA` content
to ensure a more stable and controllable set of example data.

