# Standard Library Implementation

To support build environment without C++ STL, snmalloc can optionally use self-vendored STL functionalities.
To use self-vendored implementations, one need to set `SNMALLOC_USE_SELF_VENDORED_STL` to `ON` when configuring cmake.
