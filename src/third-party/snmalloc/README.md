Include hierarchy
-----------------

The `snmalloc/` include path contains all of the snmalloc headers.
These are arranged in a hierarchy such that each of the directories may include ones below, in the following order, starting at the bottom:

 - `ds_core/` provides core data structures that depend on the C++ implementation and nothing else.
   This directory includes a number of things that abstract over different language extensions (for example, different built-in function names in different compilers).
 - `aal/` provides the architecture abstraction layer (AAL).
   This layer provides abstractions over CPU-specific intrinsics and defines things such as the virtual address-space size.
   There is a single AAL for an snmalloc instantiation.
 - `ds_aal/` provides data structures that depend on the AAL.
 - `pal/` provides the platform abstraction layer (PAL).
   This exposes OS- or environment-specific abstractions into the rest of the code.
   An snmalloc instantiation may use more than one PAL, including ones provided by the user.
 - `ds/` includes data structures that may depend on platform services or on features specific to the current CPU.
 - `mem/` provides the core allocator abstractions.
   The code here is templated over a back-end, which defines a particular embedding of snmalloc.
 - `backend_helpers/` provides helper classes for use in defining a back end.
   This includes data structures such as pagemap implementations (efficient maps from a chunk address to associated metadata) and buddy allocators for managing address-space ranges.
 - `backend/` provides some example implementations for snmalloc embeddings that provide a global memory allocator for an address space.
   Users may ignore this entirely and use the types in `mem/` with a custom back end to expose an snmalloc instance with specific behaviour.
   Layers above this can be used with a custom configuration by defining `SNMALLOC_PROVIDE_OWN_CONFIG` and exporting a type as `snmalloc::Config` that defines the configuration.
 - `global/` provides some front-end components that assume that snmalloc is available in a global configuration.
 - `override/` builds on top of `global/` to provide specific implementations with compatibility with external specifications (for example C `malloc`, C++ `operator new`, jemalloc's `*allocx`, or Rust's `std::alloc`).

Each layer until `backend_helpers/` provides a single header with the same name as the directory.
Files in higher layers should depend only on the single-file version.
This allows specific files to be moved to a lower layer if appropriate, without too much code churn.

There is only one exception to this rule: `backend/globalconfig.h`.
This file defines either the default configuration *or* nothing, depending on whether the user has defined `SNMALLOC_PROVIDE_OWN_CONFIG`.
The layers above the back end should include only this file, so that there is a single interception point for externally defined back ends.

External code should include only the following files:

 - `snmalloc/snmalloc_core.h` includes everything up to `backend_helpers`.
   This provides the building blocks required to assemble an snmalloc instance, but does not assume any global configuration.
 - `snmalloc/snmalloc_front.h` assumes a global configuration (either user-provided or the default from `snmalloc/backend/globalconfig.h` and exposes all of the functionality that depends on both.
 - `snmalloc/snmalloc.h` is a convenience wrapper that includes both of the above files.
 - `snmalloc/override/*.cc` can be compiled as-is or included after `snmalloc/snmalloc_core.h` and a custom global allocator definition to provide specific languages' global memory allocator APIs with a custom snmalloc embedding.
