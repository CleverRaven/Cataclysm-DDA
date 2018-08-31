# Tests

There are a number of test cases in [`tests/`](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/tests). The test framework is [Catch](https://github.com/philsquared/Catch). For tutorials, please visit [https://github.com/philsquared/Catch/blob/master/docs/tutorial.md](https://github.com/philsquared/Catch/blob/master/docs/tutorial.md).

## Guidelines
When creating tests, ensure that all objects used (directly or indirectly) are fully reset before testing. Several tests have been rendered flaky by properties of randomly generated objects or interactions between tests via global objects (often the player object). As a general guideline, test cases should be standalone (one test should not rely on the output of another).

When generating objects with json definitions, use REQUIRE statements to assert the properties of the objects that the test needs.
This protects the test from shifting json definitions by making it apparent what about the object changed to cause the test to break.
