# Tests

There are a number of test cases in `tests/`. The test framework is [Catch](https://github.com/philsquared/Catch). For tutorials, please visit [https://github.com/philsquared/Catch/blob/master/docs/tutorial.md](https://github.com/philsquared/Catch/blob/master/docs/tutorial.md).

## Guidelines
When creating tests, insure that any objects used (directly or indirectly) are fully reset befre testing.
A number of tests have been rendered flaky by either properties of randomly generated objects or by interactions between tests via global objects (mostly the player object).

When generating objects with json definitions, use REQUIRE statements to assert the properties of the objects that the test needs.
This protects the test from shifting json definitions, by making it apparent what about the object changed to cause the test to break.