/* malloc.c  -  Memory allocator  -  Public Domain  -  2016 Mattias Jansson
 *
 * This library provides a cross-platform lock free thread caching malloc implementation in C11.
 * The latest source code is always available at
 *
 * https://github.com/mjansson/rpmalloc
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

/* clang-format off */

//
// This file provides overrides for the standard library malloc entry points for C and new/delete operators for C++
// It also provides automatic initialization/finalization of process and threads
//

#if defined(__TINYC__)
#include <sys/types.h>
#endif

#if (defined(__GNUC__) || defined(__clang__))
#pragma GCC visibility push(default)
#endif

#define USE_IMPLEMENT 1
#define USE_INTERPOSE 0
#define USE_ALIAS 0

#if defined(__APPLE__)
#undef USE_INTERPOSE
#define USE_INTERPOSE 1

typedef struct interpose_t {
	void* new_func;
	void* orig_func;
} interpose_t;

#define MAC_INTERPOSE_PAIR(newf, oldf) 	{ (void*)newf, (void*)oldf }
#define MAC_INTERPOSE_SINGLE(newf, oldf) \
__attribute__((used)) static const interpose_t macinterpose##newf##oldf \
__attribute__ ((section("__DATA, __interpose"))) = MAC_INTERPOSE_PAIR(newf, oldf)

#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#undef USE_IMPLEMENT
#undef USE_ALIAS
#define USE_IMPLEMENT 0
#define USE_ALIAS 1
#endif

#ifdef _MSC_VER
#pragma warning (disable : 4100)
#undef malloc
#undef free
#undef calloc
#endif

#if ENABLE_OVERRIDE

typedef struct rp_nothrow_t { int __dummy; } rp_nothrow_t;

#if USE_INTERPOSE || USE_ALIAS

static void* rpmalloc_nothrow(size_t size, rp_nothrow_t t) { (void)sizeof(t); return rpmalloc(size); }
static void* rpaligned_alloc_reverse(size_t size, size_t align) { return rpaligned_alloc(align, size); }
static void* rpaligned_alloc_reverse_nothrow(size_t size, size_t align, rp_nothrow_t t) { (void)sizeof(t); return rpaligned_alloc(align, size); }
static void rpfree_size(void* p, size_t size) { (void)sizeof(size); rpfree(p); }
static void rpfree_aligned(void* p, size_t align) { (void)sizeof(align); rpfree(p); }
static void rpfree_size_aligned(void* p, size_t size, size_t align) { (void)sizeof(size); (void)sizeof(align); rpfree(p); }

#endif

extern inline void* RPMALLOC_CDECL rpvalloc(size_t size) {
	return rpaligned_alloc(os_page_size, size);
}

extern inline void* RPMALLOC_CDECL
rppvalloc(size_t size) {
	const size_t page_size = os_page_size;
	const size_t aligned_size = ((size + page_size - 1) / page_size) * page_size;
#if ENABLE_VALIDATE_ARGS
	if (aligned_size < size) {
		errno = EINVAL;
		return 0;
	}
#endif
	return rpaligned_alloc(os_page_size, aligned_size);
}

extern inline void* RPMALLOC_CDECL
rpreallocarray(void* ptr, size_t count, size_t size) {
	size_t total;
#if ENABLE_VALIDATE_ARGS
#ifdef _MSC_VER
	int err = SizeTMult(count, size, &total);
	if ((err != S_OK) || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#else
	int err = __builtin_umull_overflow(count, size, &total);
	if (err || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
#else
	total = count * size;
#endif
	return rprealloc(ptr, total);
}

#if USE_IMPLEMENT

extern inline void* RPMALLOC_CDECL malloc(size_t size) { return rpmalloc(size); }
extern inline void* RPMALLOC_CDECL calloc(size_t count, size_t size) { return rpcalloc(count, size); }
extern inline void* RPMALLOC_CDECL realloc(void* ptr, size_t size) { return rprealloc(ptr, size); }
extern inline void* RPMALLOC_CDECL reallocf(void* ptr, size_t size) { return rprealloc(ptr, size); }
extern inline void* RPMALLOC_CDECL aligned_alloc(size_t alignment, size_t size) { return rpaligned_alloc(alignment, size); }
extern inline void* RPMALLOC_CDECL memalign(size_t alignment, size_t size) { return rpmemalign(alignment, size); }
extern inline int RPMALLOC_CDECL posix_memalign(void** memptr, size_t alignment, size_t size) { return rpposix_memalign(memptr, alignment, size); }
extern inline void RPMALLOC_CDECL free(void* ptr) { rpfree(ptr); }
extern inline void RPMALLOC_CDECL cfree(void* ptr) { rpfree(ptr); }
extern inline size_t RPMALLOC_CDECL malloc_usable_size(void* ptr) { return rpmalloc_usable_size(ptr); }
extern inline size_t RPMALLOC_CDECL malloc_size(void* ptr) { return rpmalloc_usable_size(ptr); }
extern inline void* RPMALLOC_CDECL valloc(size_t size) { return rpvalloc(size); }
extern inline void* RPMALLOC_CDECL pvalloc(size_t size) { return rppvalloc(size); }
extern inline void* RPMALLOC_CDECL reallocarray(void* ptr, size_t count, size_t size) { return rpreallocarray(ptr, count, size); }

#ifdef _WIN32
extern inline void* RPMALLOC_CDECL _malloc_base(size_t size) { return rpmalloc(size); }
extern inline void RPMALLOC_CDECL _free_base(void* ptr) { rpfree(ptr); }
extern inline void* RPMALLOC_CDECL _calloc_base(size_t count, size_t size) { return rpcalloc(count, size); }
extern inline size_t RPMALLOC_CDECL _msize(void* ptr) { return rpmalloc_usable_size(ptr); }
extern inline size_t RPMALLOC_CDECL _msize_base(void* ptr) { return rpmalloc_usable_size(ptr); }
extern inline void* RPMALLOC_CDECL _realloc_base(void* ptr, size_t size) { return rprealloc(ptr, size); }
#endif

#ifdef _WIN32
// For Windows, #include <rpnew.h> in one source file to get the C++ operator overrides implemented in your module
#else
// Overload the C++ operators using the mangled names (https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling)
// operators delete and delete[]
#define RPDEFVIS __attribute__((visibility("default")))
extern void _ZdlPv(void* p); void RPDEFVIS _ZdlPv(void* p) { rpfree(p); }
extern void _ZdaPv(void* p); void RPDEFVIS _ZdaPv(void* p) { rpfree(p); }
#if ARCH_64BIT
// 64-bit operators new and new[], normal and aligned
extern void* _Znwm(uint64_t size); void* RPDEFVIS _Znwm(uint64_t size) { return rpmalloc(size); }
extern void* _Znam(uint64_t size); void* RPDEFVIS _Znam(uint64_t size) { return rpmalloc(size); }
extern void* _Znwmm(uint64_t size, uint64_t align); void* RPDEFVIS _Znwmm(uint64_t size, uint64_t align) { return rpaligned_alloc(align, size); }
extern void* _Znamm(uint64_t size, uint64_t align); void* RPDEFVIS _Znamm(uint64_t size, uint64_t align) { return rpaligned_alloc(align, size); }
extern void* _ZnwmSt11align_val_t(uint64_t size, uint64_t align); void* RPDEFVIS _ZnwmSt11align_val_t(uint64_t size, uint64_t align) { return rpaligned_alloc(align, size); }
extern void* _ZnamSt11align_val_t(uint64_t size, uint64_t align); void* RPDEFVIS _ZnamSt11align_val_t(uint64_t size, uint64_t align) { return rpaligned_alloc(align, size); }
extern void* _ZnwmRKSt9nothrow_t(uint64_t size, rp_nothrow_t t); void* RPDEFVIS _ZnwmRKSt9nothrow_t(uint64_t size, rp_nothrow_t t) { (void)sizeof(t); return rpmalloc(size); }
extern void* _ZnamRKSt9nothrow_t(uint64_t size, rp_nothrow_t t); void* RPDEFVIS _ZnamRKSt9nothrow_t(uint64_t size, rp_nothrow_t t) { (void)sizeof(t); return rpmalloc(size); }
extern void* _ZnwmSt11align_val_tRKSt9nothrow_t(uint64_t size, uint64_t align, rp_nothrow_t t); void* RPDEFVIS _ZnwmSt11align_val_tRKSt9nothrow_t(uint64_t size, uint64_t align, rp_nothrow_t t) { (void)sizeof(t); return rpaligned_alloc(align, size); }
extern void* _ZnamSt11align_val_tRKSt9nothrow_t(uint64_t size, uint64_t align, rp_nothrow_t t); void* RPDEFVIS _ZnamSt11align_val_tRKSt9nothrow_t(uint64_t size, uint64_t align, rp_nothrow_t t) { (void)sizeof(t); return rpaligned_alloc(align, size); }
// 64-bit operators sized delete and delete[], normal and aligned
extern void _ZdlPvm(void* p, uint64_t size); void RPDEFVIS _ZdlPvm(void* p, uint64_t size) { rpfree(p); (void)sizeof(size); }
extern void _ZdaPvm(void* p, uint64_t size); void RPDEFVIS _ZdaPvm(void* p, uint64_t size) { rpfree(p); (void)sizeof(size); }
extern void _ZdlPvSt11align_val_t(void* p, uint64_t align); void RPDEFVIS _ZdlPvSt11align_val_t(void* p, uint64_t align) { rpfree(p); (void)sizeof(align); }
extern void _ZdaPvSt11align_val_t(void* p, uint64_t align); void RPDEFVIS _ZdaPvSt11align_val_t(void* p, uint64_t align) { rpfree(p); (void)sizeof(align); }
extern void _ZdlPvmSt11align_val_t(void* p, uint64_t size, uint64_t align); void RPDEFVIS _ZdlPvmSt11align_val_t(void* p, uint64_t size, uint64_t align) { rpfree(p); (void)sizeof(size); (void)sizeof(align); }
extern void _ZdaPvmSt11align_val_t(void* p, uint64_t size, uint64_t align); void RPDEFVIS _ZdaPvmSt11align_val_t(void* p, uint64_t size, uint64_t align) { rpfree(p); (void)sizeof(size); (void)sizeof(align); }
#else
// 32-bit operators new and new[], normal and aligned
extern void* _Znwj(uint32_t size); void* RPDEFVIS _Znwj(uint32_t size) { return rpmalloc(size); }
extern void* _Znaj(uint32_t size); void* RPDEFVIS _Znaj(uint32_t size) { return rpmalloc(size); }
extern void* _Znwjj(uint32_t size, uint32_t align); void* RPDEFVIS _Znwjj(uint32_t size, uint32_t align) { return rpaligned_alloc(align, size); }
extern void* _Znajj(uint32_t size, uint32_t align); void* RPDEFVIS _Znajj(uint32_t size, uint32_t align) { return rpaligned_alloc(align, size); }
extern void* _ZnwjSt11align_val_t(size_t size, size_t align); void* RPDEFVIS _ZnwjSt11align_val_t(size_t size, size_t align) { return rpaligned_alloc(align, size); }
extern void* _ZnajSt11align_val_t(size_t size, size_t align); void* RPDEFVIS _ZnajSt11align_val_t(size_t size, size_t align) { return rpaligned_alloc(align, size); }
extern void* _ZnwjRKSt9nothrow_t(size_t size, rp_nothrow_t t); void* RPDEFVIS _ZnwjRKSt9nothrow_t(size_t size, rp_nothrow_t t) { (void)sizeof(t); return rpmalloc(size); }
extern void* _ZnajRKSt9nothrow_t(size_t size, rp_nothrow_t t); void* RPDEFVIS _ZnajRKSt9nothrow_t(size_t size, rp_nothrow_t t) { (void)sizeof(t); return rpmalloc(size); }
extern void* _ZnwjSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t); void* RPDEFVIS _ZnwjSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t) { (void)sizeof(t); return rpaligned_alloc(align, size); }
extern void* _ZnajSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t); void* RPDEFVIS _ZnajSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t) { (void)sizeof(t); return rpaligned_alloc(align, size); }
// 32-bit operators sized delete and delete[], normal and aligned
extern void _ZdlPvj(void* p, uint64_t size); void RPDEFVIS _ZdlPvj(void* p, uint64_t size) { rpfree(p); (void)sizeof(size); }
extern void _ZdaPvj(void* p, uint64_t size); void RPDEFVIS _ZdaPvj(void* p, uint64_t size) { rpfree(p); (void)sizeof(size); }
extern void _ZdlPvSt11align_val_t(void* p, uint32_t align); void RPDEFVIS _ZdlPvSt11align_val_t(void* p, uint64_t a) { rpfree(p); (void)sizeof(align); }
extern void _ZdaPvSt11align_val_t(void* p, uint32_t align); void RPDEFVIS _ZdaPvSt11align_val_t(void* p, uint64_t a) { rpfree(p); (void)sizeof(align); }
extern void _ZdlPvjSt11align_val_t(void* p, uint32_t size, uint32_t align); void RPDEFVIS _ZdlPvjSt11align_val_t(void* p, uint64_t size, uint64_t align) { rpfree(p); (void)sizeof(size); (void)sizeof(a); }
extern void _ZdaPvjSt11align_val_t(void* p, uint32_t size, uint32_t align); void RPDEFVIS _ZdaPvjSt11align_val_t(void* p, uint64_t size, uint64_t align) { rpfree(p); (void)sizeof(size); (void)sizeof(a); }
#endif
#endif
#endif

#if USE_INTERPOSE

__attribute__((used)) static const interpose_t macinterpose_malloc[]
__attribute__ ((section("__DATA, __interpose"))) = {
	//new and new[]
	MAC_INTERPOSE_PAIR(rpmalloc, _Znwm),
	MAC_INTERPOSE_PAIR(rpmalloc, _Znam),
	MAC_INTERPOSE_PAIR(rpaligned_alloc_reverse, _Znwmm),
	MAC_INTERPOSE_PAIR(rpaligned_alloc_reverse, _Znamm),
	MAC_INTERPOSE_PAIR(rpmalloc_nothrow, _ZnwmRKSt9nothrow_t),
	MAC_INTERPOSE_PAIR(rpmalloc_nothrow, _ZnamRKSt9nothrow_t),
	MAC_INTERPOSE_PAIR(rpaligned_alloc_reverse, _ZnwmSt11align_val_t),
	MAC_INTERPOSE_PAIR(rpaligned_alloc_reverse, _ZnamSt11align_val_t),
	MAC_INTERPOSE_PAIR(rpaligned_alloc_reverse_nothrow, _ZnwmSt11align_val_tRKSt9nothrow_t),
	MAC_INTERPOSE_PAIR(rpaligned_alloc_reverse_nothrow, _ZnamSt11align_val_tRKSt9nothrow_t),
	//delete and delete[]
	MAC_INTERPOSE_PAIR(rpfree, _ZdlPv),
	MAC_INTERPOSE_PAIR(rpfree, _ZdaPv),
	MAC_INTERPOSE_PAIR(rpfree_size, _ZdlPvm),
	MAC_INTERPOSE_PAIR(rpfree_size, _ZdaPvm),
	MAC_INTERPOSE_PAIR(rpfree_aligned, _ZdlPvSt11align_val_t),
	MAC_INTERPOSE_PAIR(rpfree_aligned, _ZdaPvSt11align_val_t),
	MAC_INTERPOSE_PAIR(rpfree_size_aligned, _ZdlPvmSt11align_val_t),
	MAC_INTERPOSE_PAIR(rpfree_size_aligned, _ZdaPvmSt11align_val_t),
	//libc entry points
	MAC_INTERPOSE_PAIR(rpmalloc, malloc),
	MAC_INTERPOSE_PAIR(rpmalloc, calloc),
	MAC_INTERPOSE_PAIR(rprealloc, realloc),
	MAC_INTERPOSE_PAIR(rprealloc, reallocf),
	MAC_INTERPOSE_PAIR(rpvalloc, valloc),
#if defined(__MAC_10_15) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_15
	MAC_INTERPOSE_PAIR(rpaligned_alloc, aligned_alloc),
#endif
	MAC_INTERPOSE_PAIR(rpmemalign, memalign),
	MAC_INTERPOSE_PAIR(rpposix_memalign, posix_memalign),
	MAC_INTERPOSE_PAIR(rpfree, free),
	MAC_INTERPOSE_PAIR(rpfree, cfree),
	MAC_INTERPOSE_PAIR(rpmalloc_usable_size, malloc_usable_size),
	MAC_INTERPOSE_PAIR(rpmalloc_usable_size, malloc_size)
};

#endif

#if USE_ALIAS

#define RPALIAS(fn) __attribute__((alias(#fn), used, visibility("default")));

// Alias the C++ operators using the mangled names (https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling)

// operators delete and delete[]
void _ZdlPv(void* p) RPALIAS(rpfree)
void _ZdaPv(void* p) RPALIAS(rpfree)

#if ARCH_64BIT
// 64-bit operators new and new[], normal and aligned
void* _Znwm(uint64_t size) RPALIAS(rpmalloc)
void* _Znam(uint64_t size) RPALIAS(rpmalloc)
void* _Znwmm(uint64_t size, uint64_t align) RPALIAS(rpaligned_alloc_reverse)
void* _Znamm(uint64_t size, uint64_t align) RPALIAS(rpaligned_alloc_reverse)
void* _ZnwmSt11align_val_t(size_t size, size_t align) RPALIAS(rpaligned_alloc_reverse)
void* _ZnamSt11align_val_t(size_t size, size_t align) RPALIAS(rpaligned_alloc_reverse)
void* _ZnwmRKSt9nothrow_t(size_t size, rp_nothrow_t t) RPALIAS(rpmalloc_nothrow)
void* _ZnamRKSt9nothrow_t(size_t size, rp_nothrow_t t) RPALIAS(rpmalloc_nothrow)
void* _ZnwmSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t) RPALIAS(rpaligned_alloc_reverse_nothrow)
void* _ZnamSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t) RPALIAS(rpaligned_alloc_reverse_nothrow)
// 64-bit operators delete and delete[], sized and aligned
void _ZdlPvm(void* p, size_t n) RPALIAS(rpfree_size)
void _ZdaPvm(void* p, size_t n) RPALIAS(rpfree_size)
void _ZdlPvSt11align_val_t(void* p, size_t a) RPALIAS(rpfree_aligned)
void _ZdaPvSt11align_val_t(void* p, size_t a) RPALIAS(rpfree_aligned)
void _ZdlPvmSt11align_val_t(void* p, size_t n, size_t a) RPALIAS(rpfree_size_aligned)
void _ZdaPvmSt11align_val_t(void* p, size_t n, size_t a) RPALIAS(rpfree_size_aligned)
#else
// 32-bit operators new and new[], normal and aligned
void* _Znwj(uint32_t size) RPALIAS(rpmalloc)
void* _Znaj(uint32_t size) RPALIAS(rpmalloc)
void* _Znwjj(uint32_t size, uint32_t align) RPALIAS(rpaligned_alloc_reverse)
void* _Znajj(uint32_t size, uint32_t align) RPALIAS(rpaligned_alloc_reverse)
void* _ZnwjSt11align_val_t(size_t size, size_t align) RPALIAS(rpaligned_alloc_reverse)
void* _ZnajSt11align_val_t(size_t size, size_t align) RPALIAS(rpaligned_alloc_reverse)
void* _ZnwjRKSt9nothrow_t(size_t size, rp_nothrow_t t) RPALIAS(rpmalloc_nothrow)
void* _ZnajRKSt9nothrow_t(size_t size, rp_nothrow_t t) RPALIAS(rpmalloc_nothrow)
void* _ZnwjSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t) RPALIAS(rpaligned_alloc_reverse_nothrow)
void* _ZnajSt11align_val_tRKSt9nothrow_t(size_t size, size_t align, rp_nothrow_t t) RPALIAS(rpaligned_alloc_reverse_nothrow)
// 32-bit operators delete and delete[], sized and aligned
void _ZdlPvj(void* p, size_t n) RPALIAS(rpfree_size)
void _ZdaPvj(void* p, size_t n) RPALIAS(rpfree_size)
void _ZdlPvSt11align_val_t(void* p, size_t a) RPALIAS(rpfree_aligned)
void _ZdaPvSt11align_val_t(void* p, size_t a) RPALIAS(rpfree_aligned)
void _ZdlPvjSt11align_val_t(void* p, size_t n, size_t a) RPALIAS(rpfree_size_aligned)
void _ZdaPvjSt11align_val_t(void* p, size_t n, size_t a) RPALIAS(rpfree_size_aligned)
#endif

void* malloc(size_t size) RPALIAS(rpmalloc)
void* calloc(size_t count, size_t size) RPALIAS(rpcalloc)
void* realloc(void* ptr, size_t size) RPALIAS(rprealloc)
void* reallocf(void* ptr, size_t size) RPALIAS(rprealloc)
void* aligned_alloc(size_t alignment, size_t size) RPALIAS(rpaligned_alloc)
void* memalign(size_t alignment, size_t size) RPALIAS(rpmemalign)
int posix_memalign(void** memptr, size_t alignment, size_t size) RPALIAS(rpposix_memalign)
void free(void* ptr) RPALIAS(rpfree)
void cfree(void* ptr) RPALIAS(rpfree)
void* reallocarray(void* ptr, size_t count, size_t size) RPALIAS(rpreallocarray)
void* valloc(size_t size) RPALIAS(rpvalloc)
void* pvalloc(size_t size) RPALIAS(rppvalloc)
#if defined(__ANDROID__) || defined(__FreeBSD__)
size_t malloc_usable_size(const void* ptr) RPALIAS(rpmalloc_usable_size)
#else
size_t malloc_usable_size(void* ptr) RPALIAS(rpmalloc_usable_size)
#endif
size_t malloc_size(void* ptr) RPALIAS(rpmalloc_usable_size)

// end USE_ALIAS
#endif

#if defined(__GLIBC__) && defined(__linux__) && 0

void* __libc_malloc(size_t size) RPALIAS(rpmalloc)
void* __libc_calloc(size_t count, size_t size) RPALIAS(rpcalloc)
void* __libc_realloc(void* p, size_t size) RPALIAS(rprealloc)
void __libc_free(void* p) RPALIAS(rpfree)
void __libc_cfree(void* p) RPALIAS(rpfree)

extern void* __libc_valloc(size_t size);
extern void* __libc_pvalloc(size_t size);

void*
__libc_valloc(size_t size) {
	return valloc(size);
}

void*
__libc_pvalloc(size_t size) {
	return pvalloc(size);
}
void* __libc_memalign(size_t align, size_t size) RPALIAS(rpmemalign)

int __posix_memalign(void** p, size_t align, size_t size) RPALIAS(rpposix_memalign)

#endif

// end ENABLE_OVERRIDE
#endif

#if ENABLE_DYNAMIC_LINK

#ifdef _WIN32

extern __declspec(dllexport) BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved);

extern __declspec(dllexport) BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	(void)sizeof(reserved);
	(void)sizeof(instance);
	if (reason == DLL_PROCESS_ATTACH)
		rpmalloc_initialize(0);
	else if (reason == DLL_PROCESS_DETACH)
		rpmalloc_finalize();
	else if (reason == DLL_THREAD_ATTACH)
		rpmalloc_thread_initialize();
	else if (reason == DLL_THREAD_DETACH)
		rpmalloc_thread_finalize();
	return TRUE;
}

#endif

// end ENABLE_DYNAMIC_LINK
#endif

#if (defined(__GNUC__) || defined(__clang__))
#pragma GCC visibility pop
#endif

/* clang-format on */
