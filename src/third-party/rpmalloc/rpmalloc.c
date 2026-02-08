/* rpmalloc.c  -  Memory allocator  -  Public Domain  -  2016-2020 Mattias
 * Jansson
 *
 * This library provides a cross-platform lock free thread caching malloc
 * implementation in C11. The latest source code is always available at
 *
 * https://github.com/mjansson/rpmalloc
 *
 * This library is put in the public domain; you can redistribute it and/or
 * modify it without any restrictions.
 *
 */

#include "rpmalloc.h"

#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#if !defined(__has_builtin)
#define __has_builtin(b) 0
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-macros"
#pragma clang diagnostic ignored "-Wunused-function"
#if __has_warning("-Wreserved-identifier")
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#if __has_warning("-Wstatic-in-inline")
#pragma clang diagnostic ignored "-Wstatic-in-inline"
#endif
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-macros"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#define PLATFORM_POSIX 0
#else
#define PLATFORM_WINDOWS 0
#define PLATFORM_POSIX 1
#endif

#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

#if PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <fibersapi.h>
static DWORD fls_key;
#endif
#if PLATFORM_POSIX
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
static pthread_key_t pthread_key;
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#define MAP_HUGETLB MAP_ALIGNED_SUPER
#ifndef PROT_MAX
#define PROT_MAX(f) 0
#endif
#else
#define PROT_MAX(f) 0
#endif
#ifdef __sun
extern int
madvise(caddr_t, size_t, int);
#endif
#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0
#endif
#endif

#if defined(__linux__) || defined(__ANDROID__)
#include <sys/prctl.h>
#if !defined(PR_SET_VMA)
#define PR_SET_VMA 0x53564d41
#define PR_SET_VMA_ANON_NAME 0
#endif
#endif
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
#include <mach/mach_vm.h>
#include <mach/vm_statistics.h>
#endif
#include <pthread.h>
#endif
#if defined(__HAIKU__) || defined(__TINYC__)
#include <pthread.h>
#endif

#include <limits.h>
#if (INTPTR_MAX > INT32_MAX)
#define ARCH_64BIT 1
#define ARCH_32BIT 0
#else
#define ARCH_64BIT 0
#define ARCH_32BIT 1
#endif

#define pointer_offset(ptr, ofs) (void*)((char*)(ptr) + (ptrdiff_t)(ofs))
#define pointer_diff(first, second) (ptrdiff_t)((const char*)(first) - (const char*)(second))

////////////
///
/// Build time configurable limits
///
//////

#ifndef ENABLE_VALIDATE_ARGS
//! Enable validation of args to public entry points
#define ENABLE_VALIDATE_ARGS 0
#endif
#ifndef ENABLE_ASSERTS
//! Enable asserts
#define ENABLE_ASSERTS 0
#endif
#ifndef ENABLE_UNMAP
//! Enable unmapping memory pages
#define ENABLE_UNMAP 1
#endif
#ifndef ENABLE_DECOMMIT
//! Enable decommitting memory pages
#define ENABLE_DECOMMIT 1
#endif
#ifndef ENABLE_DYNAMIC_LINK
//! Enable building as dynamic library
#define ENABLE_DYNAMIC_LINK 0
#endif
#ifndef ENABLE_OVERRIDE
//! Enable standard library malloc/free/new/delete overrides
#define ENABLE_OVERRIDE 1
#endif
#ifndef ENABLE_STATISTICS
//! Enable statistics
#define ENABLE_STATISTICS 0
#endif

////////////
///
/// Built in size configurations
///
//////

#define PAGE_HEADER_SIZE 128
#define SPAN_HEADER_SIZE PAGE_HEADER_SIZE

#define SMALL_GRANULARITY 16

#define SMALL_BLOCK_SIZE_LIMIT (4 * 1024)
#define MEDIUM_BLOCK_SIZE_LIMIT (256 * 1024)
#define LARGE_BLOCK_SIZE_LIMIT (8 * 1024 * 1024)

#define SMALL_SIZE_CLASS_COUNT 73
#define MEDIUM_SIZE_CLASS_COUNT 24
#define LARGE_SIZE_CLASS_COUNT 20
#define SIZE_CLASS_COUNT (SMALL_SIZE_CLASS_COUNT + MEDIUM_SIZE_CLASS_COUNT + LARGE_SIZE_CLASS_COUNT)

#define SMALL_PAGE_SIZE_SHIFT 16
#define SMALL_PAGE_SIZE (1 << SMALL_PAGE_SIZE_SHIFT)
#define SMALL_PAGE_MASK (~((uintptr_t)SMALL_PAGE_SIZE - 1))
#define MEDIUM_PAGE_SIZE_SHIFT 22
#define MEDIUM_PAGE_SIZE (1 << MEDIUM_PAGE_SIZE_SHIFT)
#define MEDIUM_PAGE_MASK (~((uintptr_t)MEDIUM_PAGE_SIZE - 1))
#define LARGE_PAGE_SIZE_SHIFT 26
#define LARGE_PAGE_SIZE (1 << LARGE_PAGE_SIZE_SHIFT)
#define LARGE_PAGE_MASK (~((uintptr_t)LARGE_PAGE_SIZE - 1))

#define SPAN_SIZE (256 * 1024 * 1024)
#define SPAN_MASK (~((uintptr_t)(SPAN_SIZE - 1)))

#if ENABLE_VALIDATE_ARGS
//! Maximum allocation size to avoid integer overflow
#undef  MAX_ALLOC_SIZE
#define MAX_ALLOC_SIZE (((size_t)-1) - SPAN_SIZE)
#endif

////////////
///
/// Utility macros
///
//////

#if ENABLE_ASSERTS
#undef NDEBUG
#if defined(_MSC_VER) && !defined(_DEBUG)
#define _DEBUG
#endif
#include <assert.h>
#define RPMALLOC_TOSTRING_M(x) #x
#define RPMALLOC_TOSTRING(x) RPMALLOC_TOSTRING_M(x)
#define rpmalloc_assert(truth, message) \
	do {                                \
		if (!(truth)) {                 \
			assert((truth) && message); \
		}                               \
	} while (0)
#else
#define rpmalloc_assert(truth, message) \
	do {                                \
	} while (0)
#endif

#if defined(_MSC_VER)
#define rpmalloc_assume(cond) __assume(cond)
#elif defined(__clang__) && __has_builtin(__builtin_assume)
#define rpmalloc_assume(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#define rpmalloc_assume(cond)           \
	do {                                \
		if (!__builtin_expect(cond, 0)) \
			__builtin_unreachable();    \
	} while (0)
#else
#define rpmalloc_assume(cond) 0
#endif

////////////
///
/// Statistics
///
//////

#if ENABLE_STATISTICS

typedef struct rpmalloc_statistics_t {
	atomic_size_t page_mapped;
	atomic_size_t page_mapped_peak;
	atomic_size_t page_commit;
	atomic_size_t page_decommit;
	atomic_size_t page_active;
	atomic_size_t page_active_peak;
	atomic_size_t heap_count;
} rpmalloc_statistics_t;

static rpmalloc_statistics_t global_statistics;

#else

#endif

////////////
///
/// Low level abstractions
///
//////

static inline size_t
rpmalloc_clz(uintptr_t x) {
#if ARCH_64BIT
#if defined(_MSC_VER) && !defined(__clang__)
	return (size_t)__lzcnt64(x);
#else
	return (size_t)__builtin_clzll(x);
#endif
#else
#if defined(_MSC_VER) && !defined(__clang__)
	return (size_t)__lzcnt32(x);
#else
	return (size_t)__builtin_clzl(x);
#endif
#endif
}

static inline void
wait_spin(void) {
#if defined(_MSC_VER)
#if defined(_M_ARM64)
	__yield();
#else
	_mm_pause();
#endif
#elif (defined(__x86_64__) || defined(__i386__)) && !defined(_M_ARM64EC)
	__asm__ volatile("pause" ::: "memory");
#elif defined(__aarch64__) || (defined(__arm__) && __ARM_ARCH >= 7) || defined(_M_ARM64EC)
	__asm__ volatile("yield" ::: "memory");
#elif defined(__powerpc__) || defined(__powerpc64__)
	// No idea if ever been compiled in such archs but ... as precaution
	__asm__ volatile("or 27,27,27");
#elif defined(__sparc__)
	__asm__ volatile("rd %ccr, %g0 \n\trd %ccr, %g0 \n\trd %ccr, %g0");
#else
	struct timespec ts = {0};
	nanosleep(&ts, 0);
#endif
}

#if defined(__GNUC__) || defined(__clang__)

#define EXPECTED(x) __builtin_expect((x), 1)
#define UNEXPECTED(x) __builtin_expect((x), 0)

#else

#define EXPECTED(x) x
#define UNEXPECTED(x) x

#endif

#if defined(__GNUC__) || defined(__clang__)
#ifdef __has_builtin
#if __has_builtin(__builtin_memcpy_inline)
#define memcpy_const(x, y, s) __builtin_memcpy_inline(x, y, s)
#else
#define memcpy_const(x, y, s)                                                                                   \
	do {                                                                                                        \
		_Static_assert(__builtin_choose_expr(__builtin_constant_p(s), 1, 0), "len must be a constant integer"); \
		memcpy(x, y, s);                                                                                        \
	} while (0)
#endif

#if __has_builtin(__builtin_memset_inline)
#define memset_const(x, y, s) __builtin_memset_inline(x, y, s)
#else
#define memset_const(x, y, s)                                                                                   \
	do {                                                                                                        \
		_Static_assert(__builtin_choose_expr(__builtin_constant_p(s), 1, 0), "len must be a constant integer"); \
		memset(x, y, s);                                                                                        \
	} while (0)
#endif
#endif
#endif

#ifndef memcpy_const
#define memcpy_const(x, y, s) memcpy(x, y, s)
#define memset_const(x, y, s) memset(x, y, s)
#endif

////////////
///
/// Data types
///
//////

//! A memory heap, per thread
typedef struct heap_t heap_t;
//! Span of memory pages
typedef struct span_t span_t;
//! Memory page
typedef struct page_t page_t;
//! Memory block
typedef struct block_t block_t;
//! Size class for a memory block
typedef struct size_class_t size_class_t;

//! Memory page type
typedef enum page_type_t {
	PAGE_SMALL,   // 64KiB
	PAGE_MEDIUM,  // 4MiB
	PAGE_LARGE,   // 64MiB
	PAGE_HUGE
} page_type_t;

//! Block size class
struct size_class_t {
	//! Size of blocks in this class
	uint32_t block_size;
	//! Number of blocks in each chunk
	uint32_t block_count;
};

//! A memory block
struct block_t {
	//! Next block in list
	block_t* next;
};

//! A page contains blocks of a given size
struct page_t {
	//! Size class of blocks
	uint32_t size_class;
	//! Block size
	uint32_t block_size;
	//! Block count
	uint32_t block_count;
	//! Block initialized count
	uint32_t block_initialized;
	//! Block used count
	uint32_t block_used;
	//! Page type
	page_type_t page_type;
	//! Flag set if part of heap full list
	uint32_t is_full : 1;
	//! Flag set if part of heap free list
	uint32_t is_free : 1;
	//! Flag set if blocks are zero initialied
	uint32_t is_zero : 1;
	//! Flag set if memory pages have been decommitted
	uint32_t is_decommitted : 1;
	//! Flag set if containing aligned blocks
	uint32_t has_aligned_block : 1;
	//! Fast combination flag for either huge, fully allocated or has aligned blocks
	uint32_t generic_free : 1;
	//! Local free list count
	uint32_t local_free_count;
	//! Local free list
	block_t* local_free;
	//! Owning heap
	heap_t* heap;
	//! Next page in list
	page_t* next;
	//! Previous page in list
	page_t* prev;
	//! Multithreaded free list, block index is in low 32 bit, list count is high 32 bit
	atomic_ullong thread_free;
};

//! A span contains pages of a given type
struct span_t {
	//! Page header
	page_t page;
	//! Owning heap
	heap_t* heap;
	//! Page address mask
	uintptr_t page_address_mask;
	//! Number of pages initialized
	uint32_t page_initialized;
	//! Number of pages in use
	uint32_t page_count;
	//! Number of bytes per page
	uint32_t page_size;
	//! Page type
	page_type_t page_type;
	//! Offset to start of mapped memory region
	uint32_t offset;
	//! Mapped size
	uint64_t mapped_size;
	//! Next span in list
	span_t* next;
};

// Control structure for a heap, either a thread heap or a first class heap if enabled
struct heap_t {
	//! Owning thread ID
	uintptr_t owner_thread;
	//! Heap local free list for small size classes
	block_t* local_free[SIZE_CLASS_COUNT];
	//! Available non-full pages for each size class
	page_t* page_available[SIZE_CLASS_COUNT];
	//! Free pages for each page type
	page_t* page_free[3];
	//! Free but still committed page count for each page tyoe
	uint32_t page_free_commit_count[3];
	//! Multithreaded free list
	atomic_uintptr_t thread_free[3];
	//! Available partially initialized spans for each page type
	span_t* span_partial[3];
	//! Spans in full use for each page type
	span_t* span_used[4];
	//! Next heap in queue
	heap_t* next;
	//! Previous heap in queue
	heap_t* prev;
	//! Heap ID
	uint32_t id;
	//! Finalization state flag
	uint32_t finalize;
	//! Memory map region offset
	uint32_t offset;
	//! Memory map size
	size_t mapped_size;
#if RPMALLOC_HEAP_STATISTICS
	struct rpmalloc_heap_statistics_t stats;
#endif
};

_Static_assert(sizeof(page_t) <= PAGE_HEADER_SIZE, "Invalid page header size");
_Static_assert(sizeof(span_t) <= SPAN_HEADER_SIZE, "Invalid span header size");
_Static_assert(sizeof(heap_t) <= 4096, "Invalid heap size");

////////////
///
/// Global data
///
//////

//! Fallback heap
static RPMALLOC_CACHE_ALIGNED heap_t global_heap_fallback;
//! Default heap
static heap_t* global_heap_default = &global_heap_fallback;
//! Available heaps
static heap_t* global_heap_queue;
//! In use heaps
static heap_t* global_heap_used;
//! Lock for heap queue
static atomic_uintptr_t global_heap_lock;
//! Heap ID counter
static atomic_uint global_heap_id = 1;
//! Initialized flag
static int global_rpmalloc_initialized;
//! Memory interface
static rpmalloc_interface_t* global_memory_interface;
//! Default memory interface
static rpmalloc_interface_t global_memory_interface_default;
//! Current configuration
static rpmalloc_config_t global_config = {0};
//! Main thread ID
static uintptr_t global_main_thread_id;

//! Size classes
#define SCLASS(n) \
	{ (n * SMALL_GRANULARITY), (SMALL_PAGE_SIZE - PAGE_HEADER_SIZE) / (n * SMALL_GRANULARITY) }
#define MCLASS(n) \
	{ (n * SMALL_GRANULARITY), (MEDIUM_PAGE_SIZE - PAGE_HEADER_SIZE) / (n * SMALL_GRANULARITY) }
#define LCLASS(n) \
	{ (n * SMALL_GRANULARITY), (LARGE_PAGE_SIZE - PAGE_HEADER_SIZE) / (n * SMALL_GRANULARITY) }
static const size_class_t global_size_class[SIZE_CLASS_COUNT] = {
    SCLASS(1),      SCLASS(1),      SCLASS(2),      SCLASS(3),      SCLASS(4),      SCLASS(5),      SCLASS(6),
    SCLASS(7),      SCLASS(8),      SCLASS(9),      SCLASS(10),     SCLASS(11),     SCLASS(12),     SCLASS(13),
    SCLASS(14),     SCLASS(15),     SCLASS(16),     SCLASS(17),     SCLASS(18),     SCLASS(19),     SCLASS(20),
    SCLASS(21),     SCLASS(22),     SCLASS(23),     SCLASS(24),     SCLASS(25),     SCLASS(26),     SCLASS(27),
    SCLASS(28),     SCLASS(29),     SCLASS(30),     SCLASS(31),     SCLASS(32),     SCLASS(33),     SCLASS(34),
    SCLASS(35),     SCLASS(36),     SCLASS(37),     SCLASS(38),     SCLASS(39),     SCLASS(40),     SCLASS(41),
    SCLASS(42),     SCLASS(43),     SCLASS(44),     SCLASS(45),     SCLASS(46),     SCLASS(47),     SCLASS(48),
    SCLASS(49),     SCLASS(50),     SCLASS(51),     SCLASS(52),     SCLASS(53),     SCLASS(54),     SCLASS(55),
    SCLASS(56),     SCLASS(57),     SCLASS(58),     SCLASS(59),     SCLASS(60),     SCLASS(61),     SCLASS(62),
    SCLASS(63),     SCLASS(64),     SCLASS(80),     SCLASS(96),     SCLASS(112),    SCLASS(128),    SCLASS(160),
    SCLASS(192),    SCLASS(224),    SCLASS(256),    MCLASS(320),    MCLASS(384),    MCLASS(448),    MCLASS(512),
    MCLASS(640),    MCLASS(768),    MCLASS(896),    MCLASS(1024),   MCLASS(1280),   MCLASS(1536),   MCLASS(1792),
    MCLASS(2048),   MCLASS(2560),   MCLASS(3072),   MCLASS(3584),   MCLASS(4096),   MCLASS(5120),   MCLASS(6144),
    MCLASS(7168),   MCLASS(8192),   MCLASS(10240),  MCLASS(12288),  MCLASS(14336),  MCLASS(16384),  LCLASS(20480),
    LCLASS(24576),  LCLASS(28672),  LCLASS(32768),  LCLASS(40960),  LCLASS(49152),  LCLASS(57344),  LCLASS(65536),
    LCLASS(81920),  LCLASS(98304),  LCLASS(114688), LCLASS(131072), LCLASS(163840), LCLASS(196608), LCLASS(229376),
    LCLASS(262144), LCLASS(327680), LCLASS(393216), LCLASS(458752), LCLASS(524288)};

//! Threshold number of pages for when free pages are decommitted
static uint32_t global_page_free_overflow[4] = {16, 8, 2, 0};

//! Number of pages to retain when free page threshold overflows
static uint32_t global_page_free_retain[4] = {4, 2, 1, 0};

//! OS huge page support
static int os_huge_pages;
//! OS memory map granularity
static size_t os_map_granularity;
//! OS memory page size
static size_t os_page_size;

////////////
///
/// Thread local heap and ID
///
//////

//! Current thread heap
#if defined(_MSC_VER) && !defined(__clang__)
#define TLS_MODEL
#define _Thread_local __declspec(thread)
#elif defined(__ANDROID__)
#if __ANDROID_API__ >= 29 && \
    ((defined(__clang__) && (__clang_major__ >= 17)) || (defined(__NDK_MAJOR__) && (__NDK_MAJOR__ >= 26)))
#define TLS_MODEL __attribute__((tls_model("local-dynamic")))
#else
#define TLS_MODEL
#endif
#else
#define TLS_MODEL __attribute__((tls_model("initial-exec")))
// #define TLS_MODEL
#endif
static _Thread_local heap_t* global_thread_heap TLS_MODEL = &global_heap_fallback;

static heap_t*
heap_allocate(int first_class);

static void
heap_page_free_decommit(heap_t* heap, uint32_t page_type, uint32_t page_retain_count);

//! Fast thread ID
static inline uintptr_t
get_thread_id(void) {
#if defined(_WIN32)
	return (uintptr_t)((void*)NtCurrentTeb());
#elif !defined(__APPLE__) && !defined(__CYGWIN__) &&                                                \
    ((defined(__clang__) && (__clang_major__ >= 7)) || ((defined(__GNUC__) && (__GNUC__ >= 5)))) && \
    (defined(__aarch64__) || defined(__x86_64__) || defined(__loongarch__))  // Unsure of other archs, needs testing
	void* thp = __builtin_thread_pointer();
	return (uintptr_t)thp;
#else
	uintptr_t tid;
#if defined(__i386__)
	__asm__("movl %%gs:0, %0" : "=r"(tid) : :);
#elif defined(__x86_64__)
#if defined(__MACH__)
	__asm__("movq %%gs:0, %0" : "=r"(tid) : :);
#else
	__asm__("movq %%fs:0, %0" : "=r"(tid) : :);
#endif
#elif defined(__arm__)
	__asm__ volatile("mrc p15, 0, %0, c13, c0, 3" : "=r"(tid));
#elif defined(__aarch64__)
#if defined(__MACH__)
	// tpidr_el0 likely unused, always return 0 on iOS
	__asm__ volatile("mrs %0, tpidrro_el0" : "=r"(tid));
#else
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(tid));
#endif
#else
	tid = (uintptr_t)&global_thread_heap;
#endif
	return tid;
#endif
}

//! Set the current thread heap
static void
set_thread_heap(heap_t* heap) {
	global_thread_heap = heap;
	if (heap && (heap->id != 0)) {
		rpmalloc_assert(heap->id != 0, "Default heap being used");
		heap->owner_thread = get_thread_id();
	}
#if PLATFORM_WINDOWS
	FlsSetValue(fls_key, heap);
#else
	pthread_setspecific(pthread_key, heap);
#endif
}

static heap_t*
get_thread_heap_allocate(void) {
	heap_t* heap = heap_allocate(0);
	set_thread_heap(heap);
	return heap;
}

//! Get the current thread heap
static inline heap_t*
get_thread_heap(void) {
	return global_thread_heap;
}

//! Get the size class from given size in bytes for tiny blocks (below 16 times the minimum granularity)
static inline uint32_t
get_size_class_tiny(size_t size) {
	return (((uint32_t)size + (SMALL_GRANULARITY - 1)) / SMALL_GRANULARITY);
}

//! Get the size class from given size in bytes
static inline uint32_t
get_size_class(size_t size) {
	uintptr_t minblock_count = (size + (SMALL_GRANULARITY - 1)) / SMALL_GRANULARITY;
	// For sizes up to 64 times the minimum granularity (i.e 1024 bytes) the size class is equal to number of such
	// blocks
	if (size <= (SMALL_GRANULARITY * 64)) {
		rpmalloc_assert(global_size_class[minblock_count].block_size >= size, "Size class misconfiguration");
		return (uint32_t)(minblock_count ? minblock_count : 1);
	}
	--minblock_count;
	// Calculate position of most significant bit, since minblock_count now guaranteed to be > 64 this position is
	// guaranteed to be >= 6
#if ARCH_64BIT
	const uint32_t most_significant_bit = (uint32_t)(63 - (int)rpmalloc_clz(minblock_count));
#else
	const uint32_t most_significant_bit = (uint32_t)(31 - (int)rpmalloc_clz(minblock_count));
#endif
	// Class sizes are of the bit format [..]000xxx000[..] where we already have the position of the most significant
	// bit, now calculate the subclass from the remaining two bits
	const uint32_t subclass_bits = (minblock_count >> (most_significant_bit - 2)) & 0x03;
	const uint32_t class_idx = (uint32_t)((most_significant_bit << 2) + subclass_bits) + 41;
	rpmalloc_assert((class_idx >= SIZE_CLASS_COUNT) || (global_size_class[class_idx].block_size >= size),
	                "Size class misconfiguration");
	rpmalloc_assert((class_idx >= SIZE_CLASS_COUNT) || (global_size_class[class_idx - 1].block_size < size),
	                "Size class misconfiguration");
	return class_idx;
}

static inline page_type_t
get_page_type(uint32_t size_class) {
	if (size_class < SMALL_SIZE_CLASS_COUNT)
		return PAGE_SMALL;
	else if (size_class < (SMALL_SIZE_CLASS_COUNT + MEDIUM_SIZE_CLASS_COUNT))
		return PAGE_MEDIUM;
	else if (size_class < SIZE_CLASS_COUNT)
		return PAGE_LARGE;
	return PAGE_HUGE;
}

static inline size_t
get_page_aligned_size(size_t size) {
	size_t unalign = size % global_config.page_size;
	if (unalign)
		size += global_config.page_size - unalign;
	return size;
}

////////////
///
/// OS entry points
///
//////

static void
os_set_page_name(void* address, size_t size) {
#if defined(__linux__) || defined(__ANDROID__)
	const char* name = os_huge_pages ? global_config.huge_page_name : global_config.page_name;
	if ((address == MAP_FAILED) || !name)
		return;
	// If the kernel does not support CONFIG_ANON_VMA_NAME or if the call fails
	// (e.g. invalid name) it is a no-op basically.
	(void)prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, (uintptr_t)address, size, (uintptr_t)name);
#else
	(void)sizeof(size);
	(void)sizeof(address);
#endif
}

static void*
os_mmap(size_t size, size_t alignment, size_t* offset, size_t* mapped_size) {
	size_t map_size = size + alignment;
#if PLATFORM_WINDOWS
	// Ok to MEM_COMMIT - according to MSDN, "actual physical pages are not allocated unless/until the virtual addresses
	// are actually accessed". But if we enable decommit it's better to not immediately commit and instead commit per
	// page to avoid saturating the OS commit limit
#if ENABLE_DECOMMIT
	DWORD do_commit = 0;
	if (global_config.disable_decommit)
	    do_commit = MEM_COMMIT;
#else
	DWORD do_commit = MEM_COMMIT;
#endif
	void* ptr =
	    VirtualAlloc(0, map_size, (os_huge_pages ? MEM_LARGE_PAGES : 0) | MEM_RESERVE | do_commit, PAGE_READWRITE);
#else
	int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED;
#if defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
	int fd = (int)VM_MAKE_TAG(240U);
	if (os_huge_pages)
		fd |= VM_FLAGS_SUPERPAGE_SIZE_2MB;
	void* ptr = mmap(0, map_size, PROT_READ | PROT_WRITE, flags, fd, 0);
#elif defined(MAP_HUGETLB)
	void* ptr = mmap(0, map_size, PROT_READ | PROT_WRITE | PROT_MAX(PROT_READ | PROT_WRITE),
	                 (os_huge_pages ? MAP_HUGETLB : 0) | flags, -1, 0);
#if defined(MADV_HUGEPAGE)
	// In some configurations, huge pages allocations might fail thus
	// we fallback to normal allocations and promote the region as transparent huge page
	if ((ptr == MAP_FAILED || !ptr) && os_huge_pages) {
		ptr = mmap(0, map_size, PROT_READ | PROT_WRITE, flags, -1, 0);
		if (ptr && ptr != MAP_FAILED) {
			int prm = madvise(ptr, size, MADV_HUGEPAGE);
			(void)prm;
			rpmalloc_assert((prm == 0), "Failed to promote the page to transparent huge page");
		}
	}
#endif
	os_set_page_name(ptr, map_size);
#elif defined(MAP_ALIGNED)
	const size_t align = (sizeof(size_t) * 8) - (size_t)(__builtin_clzl(size - 1));
	void* ptr = mmap(0, map_size, PROT_READ | PROT_WRITE, (os_huge_pages ? MAP_ALIGNED(align) : 0) | flags, -1, 0);
#elif defined(MAP_ALIGN)
	caddr_t base = (os_huge_pages ? (caddr_t)(4 << 20) : 0);
	void* ptr = mmap(base, map_size, PROT_READ | PROT_WRITE, (os_huge_pages ? MAP_ALIGN : 0) | flags, -1, 0);
#else
	void* ptr = mmap(0, map_size, PROT_READ | PROT_WRITE, flags, -1, 0);
#endif
	if (ptr == MAP_FAILED)
		ptr = 0;
#endif
	if (!ptr) {
		if (global_memory_interface->map_fail_callback && global_memory_interface->map_fail_callback(map_size))
			return os_mmap(size, alignment, offset, mapped_size);
		rpmalloc_assert(ptr != 0, "Failed to map more virtual memory");
		return 0;
	}
	if (alignment) {
		size_t padding = ((uintptr_t)ptr & (uintptr_t)(alignment - 1));
		if (padding)
			padding = alignment - padding;
		rpmalloc_assert(padding <= alignment, "Internal failure in padding");
		rpmalloc_assert(!(padding % 8), "Internal failure in padding");
		ptr = pointer_offset(ptr, padding);
		*offset = padding;
	}
	*mapped_size = map_size;
#if ENABLE_STATISTICS
	size_t page_count = map_size / global_config.page_size;
	size_t page_mapped_current =
	    atomic_fetch_add_explicit(&global_statistics.page_mapped, page_count, memory_order_relaxed) + page_count;
	size_t page_mapped_peak = atomic_load_explicit(&global_statistics.page_mapped_peak, memory_order_relaxed);
	while (page_mapped_current > page_mapped_peak) {
		if (atomic_compare_exchange_weak_explicit(&global_statistics.page_mapped_peak, &page_mapped_peak,
		                                          page_mapped_current, memory_order_relaxed, memory_order_relaxed))
			break;
	}
#endif
	return ptr;
}

static int
os_mcommit(void* address, size_t size) {
#if ENABLE_DECOMMIT
	if (global_config.disable_decommit) {
		return 0;
	}
#if PLATFORM_WINDOWS
	if (!VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE)) {
		if (global_memory_interface->map_fail_callback && global_memory_interface->map_fail_callback(size))
			return os_mcommit(address, size);
		rpmalloc_assert(0, "Failed to commit virtual memory block");
		return 1;
	}
#else
	/*
	if (mprotect(address, size, PROT_READ | PROT_WRITE)) {
		rpmalloc_assert(0, "Failed to commit virtual memory block");
	}
	*/
#endif
#if ENABLE_STATISTICS
	size_t page_count = size / global_config.page_size;
	atomic_fetch_add_explicit(&global_statistics.page_commit, page_count, memory_order_relaxed);
	size_t page_active_current =
	    atomic_fetch_add_explicit(&global_statistics.page_active, page_count, memory_order_relaxed) + page_count;
	size_t page_active_peak = atomic_load_explicit(&global_statistics.page_active_peak, memory_order_relaxed);
	while (page_active_current > page_active_peak) {
		if (atomic_compare_exchange_weak_explicit(&global_statistics.page_active_peak, &page_active_peak,
		                                          page_active_current, memory_order_relaxed, memory_order_relaxed))
			break;
	}
#endif
#endif
	(void)sizeof(address);
	(void)sizeof(size);
	return 0;
}

static int
os_mdecommit(void* address, size_t size) {
#if ENABLE_DECOMMIT
	if (global_config.disable_decommit)
		return 1;
#if PLATFORM_WINDOWS
	if (!VirtualFree(address, size, MEM_DECOMMIT)) {
		rpmalloc_assert(0, "Failed to decommit virtual memory block");
		return 1;
	}
#else
	/*
	if (mprotect(address, size, PROT_NONE)) {
		rpmalloc_assert(0, "Failed to decommit virtual memory block");
	}
	*/
#if defined(MADV_DONTNEED)
	if (madvise(address, size, MADV_DONTNEED)) {
#elif defined(MADV_FREE_REUSABLE)
	int ret;
	while ((ret = madvise(address, size, MADV_FREE_REUSABLE)) == -1 && (errno == EAGAIN))
		errno = 0;
	if ((ret == -1) && (errno != 0)) {
#elif defined(MADV_PAGEOUT)
	if (madvise(address, size, MADV_PAGEOUT)) {
#elif defined(MADV_FREE)
	if (madvise(address, size, MADV_FREE)) {
#else
	if (posix_madvise(address, size, POSIX_MADV_DONTNEED)) {
#endif
		rpmalloc_assert(0, "Failed to decommit virtual memory block");
		return 1;
	}
#endif
#if ENABLE_STATISTICS
	size_t page_count = size / global_config.page_size;
	atomic_fetch_add_explicit(&global_statistics.page_decommit, page_count, memory_order_relaxed);
	size_t page_active_current =
	    atomic_fetch_sub_explicit(&global_statistics.page_active, page_count, memory_order_relaxed);
	rpmalloc_assert(page_active_current >= page_count, "Decommit counter out of sync");
	(void)sizeof(page_active_current);
#endif
#else
	(void)sizeof(address);
	(void)sizeof(size);
#endif
	return 0;
}

static void
os_munmap(void* address, size_t offset, size_t mapped_size) {
	(void)sizeof(mapped_size);
	address = pointer_offset(address, -(int32_t)offset);
#if ENABLE_UNMAP
#if PLATFORM_WINDOWS
	if (!VirtualFree(address, 0, MEM_RELEASE)) {
		rpmalloc_assert(0, "Failed to unmap virtual memory block");
	}
#else
	if (munmap(address, mapped_size))
		rpmalloc_assert(0, "Failed to unmap virtual memory block");
#endif
#if ENABLE_STATISTICS
	size_t page_count = mapped_size / global_config.page_size;
	atomic_fetch_sub_explicit(&global_statistics.page_mapped, page_count, memory_order_relaxed);
#endif
#endif
}

////////////
///
/// Page interface
///
//////

static inline span_t*
page_get_span(page_t* page) {
	return (span_t*)((uintptr_t)page & SPAN_MASK);
}

static inline size_t
page_get_size(page_t* page) {
	if (page->page_type == PAGE_SMALL)
		return SMALL_PAGE_SIZE;
	else if (page->page_type == PAGE_MEDIUM)
		return MEDIUM_PAGE_SIZE;
	else if (page->page_type == PAGE_LARGE)
		return LARGE_PAGE_SIZE;
	else
		return page_get_span(page)->page_size;
}

static inline int
page_is_thread_heap(page_t* page) {
#if RPMALLOC_FIRST_CLASS_HEAPS
	return (!page->heap->owner_thread || (page->heap->owner_thread == get_thread_id()));
#else
	return (page->heap->owner_thread == get_thread_id());
#endif
}

static inline block_t*
page_block_start(page_t* page) {
	return pointer_offset(page, PAGE_HEADER_SIZE);
}

static inline block_t*
page_block(page_t* page, uint32_t block_index) {
	return pointer_offset(page, PAGE_HEADER_SIZE + (page->block_size * block_index));
}

static inline uint32_t
page_block_index(page_t* page, block_t* block) {
	block_t* block_first = page_block_start(page);
	return (uint32_t)pointer_diff(block, block_first) / page->block_size;
}

static inline uint32_t
page_block_from_thread_free_list(page_t* page, uint64_t token, block_t** block) {
	uint32_t block_index = (uint32_t)(token & 0xFFFFFFFFULL);
	uint32_t list_count = (uint32_t)((token >> 32ULL) & 0xFFFFFFFFULL);
	*block = list_count ? page_block(page, block_index) : 0;
	return list_count;
}

static inline uint64_t
page_block_to_thread_free_list(page_t* page, uint32_t block_index, uint32_t list_count) {
	(void)sizeof(page);
	return ((uint64_t)list_count << 32ULL) | (uint64_t)block_index;
}

static inline block_t*
page_block_realign(page_t* page, block_t* block) {
	void* blocks_start = page_block_start(page);
	uint32_t block_offset = (uint32_t)pointer_diff(block, blocks_start);
	return pointer_offset(block, -(int32_t)(block_offset % page->block_size));
}

static block_t*
page_get_local_free_block(page_t* page) {
	block_t* block = page->local_free;
	page->local_free = block->next;
	--page->local_free_count;
	++page->block_used;
	return block;
}

static inline void
page_decommit_memory_pages(page_t* page) {
	if (page->is_decommitted)
		return;
	void* extra_page = pointer_offset(page, global_config.page_size);
	size_t extra_page_size = page_get_size(page) - global_config.page_size;
	if (global_memory_interface->memory_decommit(extra_page, extra_page_size) != 0)
		return;
#if RPMALLOC_HEAP_STATISTICS && ENABLE_DECOMMIT
	if (page->heap)
		page->heap->stats.committed_size -= extra_page_size;
#endif
	page->is_decommitted = 1;
}

static inline int
page_commit_memory_pages(page_t* page) {
	if (!page->is_decommitted)
		return 0;
	void* extra_page = pointer_offset(page, global_config.page_size);
	size_t extra_page_size = page_get_size(page) - global_config.page_size;
	if (global_memory_interface->memory_commit(extra_page, extra_page_size) != 0)
		return 1;
	page->is_decommitted = 0;
#if ENABLE_DECOMMIT
#if RPMALLOC_HEAP_STATISTICS
	if (page->heap)
		page->heap->stats.committed_size += extra_page_size;
#endif
#if !defined(__APPLE__)
	// When page is recommitted, the blocks in the second memory page and forward
	// will be zeroed out by OS - take advantage in zalloc/calloc calls and make sure
	// blocks in first page is zeroed out
	void* first_page = pointer_offset(page, PAGE_HEADER_SIZE);
	memset(first_page, 0, global_config.page_size - PAGE_HEADER_SIZE);
	page->is_zero = 1;
#endif
#endif
	return 0;
}

static void
page_available_to_free(page_t* page) {
	rpmalloc_assert(page->is_full == 0, "Page full flag internal failure");
	rpmalloc_assert(page->is_decommitted == 0, "Page decommitted flag internal failure");
	heap_t* heap = page->heap;
	if (heap->page_available[page->size_class] == page) {
		heap->page_available[page->size_class] = page->next;
	} else {
		page->prev->next = page->next;
		if (page->next)
			page->next->prev = page->prev;
	}
	page->is_free = 1;
	page->is_zero = 0;
	page->next = heap->page_free[page->page_type];
	heap->page_free[page->page_type] = page;
	if (++heap->page_free_commit_count[page->page_type] >= global_page_free_overflow[page->page_type])
		heap_page_free_decommit(heap, page->page_type, global_page_free_retain[page->page_type]);
}

static void
page_full_to_available(page_t* page) {
	rpmalloc_assert(page->is_full == 1, "Page full flag internal failure");
	rpmalloc_assert(page->is_decommitted == 0, "Page decommitted flag internal failure");
	heap_t* heap = page->heap;
	page->next = heap->page_available[page->size_class];
	if (page->next)
		page->next->prev = page;
	heap->page_available[page->size_class] = page;
	page->is_full = 0;
	if (page->has_aligned_block == 0)
		page->generic_free = 0;
}

static void
page_full_to_free_on_new_heap(page_t* page, heap_t* heap) {
	rpmalloc_assert(heap->id, "Page full to free on default heap");
	rpmalloc_assert(page->is_full == 1, "Page full flag internal failure");
	rpmalloc_assert(page->is_decommitted == 0, "Page decommitted flag internal failure");
	page->is_full = 0;
	page->is_free = 1;
	page->heap = heap;
	atomic_store_explicit(&page->thread_free, 0, memory_order_release);
	page->next = heap->page_free[page->page_type];
	heap->page_free[page->page_type] = page;
	if (++heap->page_free_commit_count[page->page_type] >= global_page_free_overflow[page->page_type])
		heap_page_free_decommit(heap, page->page_type, global_page_free_retain[page->page_type]);
}

static void
page_available_to_full(page_t* page) {
	heap_t* heap = page->heap;
	if (heap->page_available[page->size_class] == page) {
		heap->page_available[page->size_class] = page->next;
	} else {
		page->prev->next = page->next;
		if (page->next)
			page->next->prev = page->prev;
	}
	page->is_full = 1;
	page->is_zero = 0;
	page->generic_free = 1;
}

static inline void
page_put_local_free_block(page_t* page, block_t* block) {
	block->next = page->local_free;
	page->local_free = block;
	++page->local_free_count;
	if (UNEXPECTED(--page->block_used == 0)) {
		page_available_to_free(page);
	} else if (UNEXPECTED(page->is_full != 0)) {
		page_full_to_available(page);
	}
}

static NOINLINE void
page_adopt_thread_free_block_list(page_t* page) {
	if (page->local_free)
		return;
	unsigned long long thread_free = atomic_load_explicit(&page->thread_free, memory_order_acquire);
	if (thread_free != 0) {
		// Other threads can only replace with another valid list head, this will never change to 0 in other threads
		while (!atomic_compare_exchange_weak_explicit(&page->thread_free, &thread_free, 0, memory_order_acquire,
		                                              memory_order_relaxed))
			wait_spin();
		page->local_free_count = page_block_from_thread_free_list(page, thread_free, &page->local_free);
		rpmalloc_assert(page->local_free_count <= page->block_used, "Page thread free list count internal failure");
		page->block_used -= page->local_free_count;
	}
}

static NOINLINE void
page_put_thread_free_block(page_t* page, block_t* block) {
	atomic_thread_fence(memory_order_acquire);
	if (page->is_full) {
		// Page is full, put the block in the heap thread free list instead, otherwise
		// the heap will not pick up the free blocks until a thread local free happens
		heap_t* heap = page->heap;
		uintptr_t prev_head = atomic_load_explicit(&heap->thread_free[page->page_type], memory_order_relaxed);
		block->next = (void*)prev_head;
		while (!atomic_compare_exchange_weak_explicit(&heap->thread_free[page->page_type], &prev_head, (uintptr_t)block,
		                                              memory_order_release, memory_order_relaxed)) {
			block->next = (void*)prev_head;
			wait_spin();
		}
	} else {
		unsigned long long prev_thread_free = atomic_load_explicit(&page->thread_free, memory_order_relaxed);
		uint32_t block_index = page_block_index(page, block);
		rpmalloc_assert(page_block(page, block_index) == block, "Block pointer is not aligned to start of block");
		uint32_t list_size = page_block_from_thread_free_list(page, prev_thread_free, &block->next) + 1;
		uint64_t thread_free = page_block_to_thread_free_list(page, block_index, list_size);
		while (!atomic_compare_exchange_weak_explicit(&page->thread_free, &prev_thread_free, thread_free,
		                                              memory_order_release, memory_order_relaxed)) {
			list_size = page_block_from_thread_free_list(page, prev_thread_free, &block->next) + 1;
			thread_free = page_block_to_thread_free_list(page, block_index, list_size);
			wait_spin();
		}
	}
}

static void
page_push_local_free_to_heap(page_t* page) {
	// Push the page free list as the fast track list of free blocks for heap
	page->heap->local_free[page->size_class] = page->local_free;
	page->block_used += page->local_free_count;
	page->local_free = 0;
	page->local_free_count = 0;
}

static NOINLINE void*
page_initialize_blocks(page_t* page) {
	rpmalloc_assert(page->block_initialized < page->block_count, "Block initialization internal failure");
	block_t* block = page_block(page, page->block_initialized);
	++page->block_initialized;
	++page->block_used;

	if ((page->page_type == PAGE_SMALL) && (page->block_size < (global_config.page_size >> 1))) {
		// Link up until next memory page in free list
		void* memory_page_start = (void*)((uintptr_t)block & ~(uintptr_t)(global_config.page_size - 1));
		void* memory_page_next = pointer_offset(memory_page_start, global_config.page_size);
		block_t* free_block = pointer_offset(block, page->block_size);
		block_t* first_block = free_block;
		block_t* last_block = free_block;
		uint32_t list_count = 0;
		uint32_t max_list_count = page->block_count - page->block_initialized;
		while (((void*)free_block < memory_page_next) && (list_count < max_list_count)) {
			last_block = free_block;
			free_block->next = pointer_offset(free_block, page->block_size);
			free_block = free_block->next;
			++list_count;
		}
		if (list_count) {
			last_block->next = 0;
			page->local_free = first_block;
			page->block_initialized += list_count;
			page->local_free_count = list_count;
		}
	}

	return block;
}

static inline RPMALLOC_ALLOCATOR void*
page_allocate_block(page_t* page, unsigned int zero) {
	unsigned int is_zero = 0;
	block_t* block = (page->local_free != 0) ? page_get_local_free_block(page) : 0;
	if (UNEXPECTED(block == 0)) {
		if (atomic_load_explicit(&page->thread_free, memory_order_acquire) != 0) {
			page_adopt_thread_free_block_list(page);
			block = (page->local_free != 0) ? page_get_local_free_block(page) : 0;
		}
		if (block == 0) {
			block = page_initialize_blocks(page);
			is_zero = page->is_zero;
		}
	}

	rpmalloc_assert(page->block_used <= page->block_count, "Page block use counter out of sync");
	if (page->local_free && !page->heap->local_free[page->size_class])
		page_push_local_free_to_heap(page);

	// The page might be full when free list has been pushed to heap local free list,
	// check if there is a thread free list to adopt
	if (page->block_used == page->block_count)
		page_adopt_thread_free_block_list(page);

	if (page->block_used == page->block_count) {
		// Page is now fully utilized
		rpmalloc_assert(!page->is_full, "Page block use counter out of sync with full flag");
		page_available_to_full(page);
	}

	if (zero) {
		if (!is_zero)
			memset(block, 0, page->block_size);
		else
			*(uintptr_t*)block = 0;
	}

	return block;
}

////////////
///
/// Span interface
///
//////

static inline int
span_is_thread_heap(span_t* span) {
#if RPMALLOC_FIRST_CLASS_HEAPS
	return (!span->heap->owner_thread || (span->heap->owner_thread == get_thread_id()));
#else
	return (span->heap->owner_thread == get_thread_id());
#endif
}

static inline page_t*
span_get_page_from_block(span_t* span, void* block) {
	return (page_t*)((uintptr_t)block & span->page_address_mask);
}

//! Find or allocate a page from the given span
static inline page_t*
span_allocate_page(span_t* span) {
	// Allocate path, initialize a new chunk of memory for a page in the given span
	rpmalloc_assert(span->page_initialized < span->page_count, "Page initialization internal failure");
	heap_t* heap = span->heap;
	page_t* page = pointer_offset(span, span->page_size * span->page_initialized);

#if ENABLE_DECOMMIT
	// The first page is always committed on initial span map of memory
	if (span->page_initialized) {
		if (global_memory_interface->memory_commit(page, span->page_size) != 0)
			return 0;
#if RPMALLOC_HEAP_STATISTICS
		heap->stats.committed_size += span->page_size;
#endif
	}
#endif
	++span->page_initialized;

	page->page_type = span->page_type;
	page->is_zero = 1;
	page->heap = heap;
	rpmalloc_assert(page_is_thread_heap(page), "Page owner thread mismatch");

	if (span->page_initialized == span->page_count) {
		// Span fully utilized
		rpmalloc_assert(span == heap->span_partial[span->page_type], "Span partial tracking out of sync");
		heap->span_partial[span->page_type] = 0;

		span->next = heap->span_used[span->page_type];
		heap->span_used[span->page_type] = span;
	}

	return page;
}

static NOINLINE void
span_deallocate_block(span_t* span, page_t* page, void* block) {
	if (UNEXPECTED(page->page_type == PAGE_HUGE)) {
#if RPMALLOC_HEAP_STATISTICS
		if (span->heap) {
			span->heap->stats.mapped_size -= span->mapped_size;
#if ENABLE_DECOMMIT
			span->heap->stats.committed_size -= span->page_count * span->page_size;
#else
			span->heap->stats.committed_size -= mapped_size;
#endif
		}
#endif
		global_memory_interface->memory_unmap(span, span->offset, span->mapped_size);
		return;
	}

	if (page->has_aligned_block) {
		// Realign pointer to block start
		block = page_block_realign(page, block);
	}

	int is_thread_local = page_is_thread_heap(page);
	if (EXPECTED(is_thread_local != 0)) {
		page_put_local_free_block(page, block);
	} else {
		// Multithreaded deallocation, push to deferred deallocation list.
		page_put_thread_free_block(page, block);
	}
}

////////////
///
/// Block interface
///
//////

static inline span_t*
block_get_span(block_t* block) {
	return (span_t*)((uintptr_t)block & SPAN_MASK);
}

static inline void
block_deallocate(block_t* block) {
	span_t* span = (span_t*)((uintptr_t)block & SPAN_MASK);
	page_t* page = span_get_page_from_block(span, block);
	const int is_thread_local = page_is_thread_heap(page);

#if RPMALLOC_HEAP_STATISTICS
	heap_t* heap = span->heap;
	if (heap) {
		if (span->page_type <= PAGE_LARGE)
			heap->stats.allocated_size -= page->block_size;
		else
			heap->stats.allocated_size -= ((size_t)span->page_size * (size_t)span->page_count);
	}
#endif

	// Optimized path for thread local free with non-huge block in page
	// that has no aligned blocks
	if (EXPECTED(is_thread_local != 0)) {
		if (EXPECTED(page->generic_free == 0)) {
			// Page is not huge, not full and has no aligned block - fast path
			block->next = page->local_free;
			page->local_free = block;
			++page->local_free_count;
			if (UNEXPECTED(--page->block_used == 0))
				page_available_to_free(page);
		} else {
			span_deallocate_block(span, page, block);
		}
	} else {
		span_deallocate_block(span, page, block);
	}
}

static inline size_t
block_usable_size(block_t* block) {
	span_t* span = (span_t*)((uintptr_t)block & SPAN_MASK);
	if (EXPECTED(span->page_type <= PAGE_LARGE)) {
		page_t* page = span_get_page_from_block(span, block);
		void* blocks_start = pointer_offset(page, PAGE_HEADER_SIZE);
		return page->block_size - ((size_t)pointer_diff(block, blocks_start) % page->block_size);
	} else {
		return ((size_t)span->page_size * (size_t)span->page_count) - (size_t)pointer_diff(block, span);
	}
}

////////////
///
/// Heap interface
///
//////

static inline void
heap_lock_acquire(void) {
	uintptr_t lock = 0;
	uintptr_t this_lock = get_thread_id();
	while (!atomic_compare_exchange_strong(&global_heap_lock, &lock, this_lock)) {
		lock = 0;
		wait_spin();
	}
}

static inline void
heap_lock_release(void) {
	rpmalloc_assert((uintptr_t)atomic_load_explicit(&global_heap_lock, memory_order_relaxed) == get_thread_id(),
	                "Bad heap lock");
	atomic_store_explicit(&global_heap_lock, 0, memory_order_release);
}

static inline heap_t*
heap_initialize(void* block) {
	heap_t* heap = block;
	memset_const(heap, 0, sizeof(heap_t));
	heap->id = 1 + atomic_fetch_add_explicit(&global_heap_id, 1, memory_order_relaxed);
	return heap;
}

static heap_t*
heap_allocate_new(void) {
	if (!global_config.page_size)
		rpmalloc_initialize(0);
	size_t heap_size = get_page_aligned_size(sizeof(heap_t));
	size_t offset = 0;
	size_t mapped_size = 0;
	block_t* block = global_memory_interface->memory_map(heap_size, 0, &offset, &mapped_size);
#if ENABLE_DECOMMIT
	if (global_memory_interface->memory_commit(block, heap_size) != 0)
		return 0;
#endif
	heap_t* heap = heap_initialize((void*)block);
	heap->offset = (uint32_t)offset;
	heap->mapped_size = mapped_size;
#if ENABLE_STATISTICS
	atomic_fetch_add_explicit(&global_statistics.heap_count, 1, memory_order_relaxed);
#endif
	return heap;
}

static void
heap_unmap(heap_t* heap) {
	global_memory_interface->memory_unmap(heap, heap->offset, heap->mapped_size);
}

static heap_t*
heap_allocate(int first_class) {
	heap_t* heap = 0;
	if (!first_class) {
		heap_lock_acquire();
		heap = global_heap_queue;
		global_heap_queue = heap ? heap->next : 0;
		heap_lock_release();
	}
	if (!heap)
		heap = heap_allocate_new();
	if (heap) {
		uintptr_t current_thread_id = get_thread_id();
		heap_lock_acquire();
		heap->next = global_heap_used;
		heap->prev = 0;
		if (global_heap_used)
			global_heap_used->prev = heap;
		global_heap_used = heap;
		heap_lock_release();
		heap->owner_thread = current_thread_id;
	}
	return heap;
}

static inline void
heap_release(heap_t* heap) {
	heap_lock_acquire();
	if (heap->prev)
		heap->prev->next = heap->next;
	if (heap->next)
		heap->next->prev = heap->prev;
	if (global_heap_used == heap)
		global_heap_used = heap->next;
	heap->next = global_heap_queue;
	global_heap_queue = heap;
	heap_lock_release();
}

static void
heap_page_free_decommit(heap_t* heap, uint32_t page_type, uint32_t page_retain_count) {
	page_t* page = heap->page_free[page_type];
	while (page && page_retain_count) {
		page = page->next;
		--page_retain_count;
	}
	while (page && (page->is_decommitted == 0)) {
		page_decommit_memory_pages(page);
		--heap->page_free_commit_count[page_type];
		page = page->next;
	}
}

static inline int
heap_make_free_page_available(heap_t* heap, uint32_t size_class, page_t* page) {
	page->size_class = size_class;
	page->block_size = global_size_class[size_class].block_size;
	page->block_count = global_size_class[size_class].block_count;
	page->block_used = 0;
	page->block_initialized = 0;
	page->local_free = 0;
	page->local_free_count = 0;
	page->is_full = 0;
	page->is_free = 0;
	page->has_aligned_block = 0;
	page->generic_free = 0;
	page->heap = heap;
	page_t* head = heap->page_available[size_class];
	page->next = head;
	page->prev = 0;
	atomic_store_explicit(&page->thread_free, 0, memory_order_release);
	if (head)
		head->prev = page;
	heap->page_available[size_class] = page;
	if (page->is_decommitted != 0)
		return page_commit_memory_pages(page);
	return 0;
}

//! Find or allocate a span for the given page type with the given size class
static inline span_t*
heap_get_span(heap_t* heap, page_type_t page_type) {
	// Fast path, available span for given page type
	if (EXPECTED(heap->span_partial[page_type] != 0))
		return heap->span_partial[page_type];

	// Fallback path, map more memory
	size_t offset = 0;
	size_t mapped_size = 0;
	span_t* span = global_memory_interface->memory_map(SPAN_SIZE, SPAN_SIZE, &offset, &mapped_size);
#if RPMALLOC_HEAP_STATISTICS
	heap->stats.mapped_size += mapped_size;
#endif
	if (EXPECTED(span != 0)) {
		uint32_t page_count = 0;
		uint32_t page_size = 0;
		uintptr_t page_address_mask = 0;
		if (page_type == PAGE_SMALL) {
			page_count = SPAN_SIZE / SMALL_PAGE_SIZE;
			page_size = SMALL_PAGE_SIZE;
			page_address_mask = SMALL_PAGE_MASK;
		} else if (page_type == PAGE_MEDIUM) {
			page_count = SPAN_SIZE / MEDIUM_PAGE_SIZE;
			page_size = MEDIUM_PAGE_SIZE;
			page_address_mask = MEDIUM_PAGE_MASK;
		} else {
			page_count = SPAN_SIZE / LARGE_PAGE_SIZE;
			page_size = LARGE_PAGE_SIZE;
			page_address_mask = LARGE_PAGE_MASK;
		}
#if ENABLE_DECOMMIT
		if (global_memory_interface->memory_commit(span, page_size) != 0)
			return 0;
#endif
#if RPMALLOC_HEAP_STATISTICS
#if ENABLE_DECOMMIT
		heap->stats.committed_size += page_size;
#else
		heap->stats.committed_size += mapped_size;
#endif
#endif
		span->heap = heap;
		span->page_type = page_type;
		span->page_count = page_count;
		span->page_size = page_size;
		span->page_address_mask = page_address_mask;
		span->offset = (uint32_t)offset;
		span->mapped_size = mapped_size;

		heap->span_partial[page_type] = span;
	}

	return span;
}

static page_t*
heap_get_page(heap_t* heap, uint32_t size_class);

static void
block_deallocate(block_t* block);

static page_t*
heap_get_page_generic(heap_t* heap, uint32_t size_class) {
	page_type_t page_type = get_page_type(size_class);

	// Check if there is a free page from multithreaded deallocations
	uintptr_t block_mt = atomic_load_explicit(&heap->thread_free[page_type], memory_order_acquire);
	if (UNEXPECTED(block_mt != 0)) {
		while (!atomic_compare_exchange_weak_explicit(&heap->thread_free[page_type], &block_mt, 0, memory_order_release,
		                                              memory_order_relaxed)) {
			wait_spin();
		}
		block_t* block = (void*)block_mt;
		while (block) {
			block_t* next_block = block->next;
			block_deallocate(block);
			block = next_block;
		}
		// Retry after processing deferred thread frees
		return heap_get_page(heap, size_class);
	}

	// Check if there is a free page
	page_t* page = heap->page_free[page_type];
	if (EXPECTED(page != 0)) {
		heap->page_free[page_type] = page->next;
		if (page->is_decommitted == 0) {
			rpmalloc_assert(heap->page_free_commit_count[page_type] > 0, "Free committed page count out of sync");
			--heap->page_free_commit_count[page_type];
		}
		if (heap_make_free_page_available(heap, size_class, page) != 0)
			return 0;
		return page;
	}
	rpmalloc_assert(heap->page_free_commit_count[page_type] == 0, "Free committed page count out of sync");

	if (heap->id == 0) {
		// Thread has not yet initialized, assign heap and try again
		rpmalloc_initialize(0);
		return heap_get_page(get_thread_heap(), size_class);
	}

	// Fallback path, find or allocate span for given size class
	// If thread was not initialized, the heap for the new span
	// will be different from the local heap variable in this scope
	// (which is the default heap) - so use span page heap instead
	span_t* span = heap_get_span(heap, page_type);
	if (EXPECTED(span != 0)) {
		page = span_allocate_page(span);
		if (heap_make_free_page_available(page->heap, size_class, page) != 0)
			return 0;
	}

	return page;
}

//! Find or allocate a page for the given size class
static page_t*
heap_get_page(heap_t* heap, uint32_t size_class) {
	// Fast path, available page for given size class
	page_t* page = heap->page_available[size_class];
	if (EXPECTED(page != 0))
		return page;
	return heap_get_page_generic(heap, size_class);
}

//! Pop a block from the heap local free list
static inline RPMALLOC_ALLOCATOR void*
heap_pop_local_free(heap_t* heap, uint32_t size_class) {
	block_t** free_list = heap->local_free + size_class;
	block_t* block = *free_list;
	if (EXPECTED(block != 0))
		*free_list = block->next;
	return block;
}

//! Generic allocation path from heap pages, spans or new mapping
static NOINLINE RPMALLOC_ALLOCATOR void*
heap_allocate_block_small_to_large(heap_t* heap, uint32_t size_class, unsigned int zero) {
	page_t* page = heap_get_page(heap, size_class);
	if (EXPECTED(page != 0))
		return page_allocate_block(page, zero);
	return 0;
}

//! Generic allocation path from heap pages, spans or new mapping
static NOINLINE RPMALLOC_ALLOCATOR void*
heap_allocate_block_huge(heap_t* heap, size_t size, unsigned int zero) {
	if (heap->id == 0) {
		// Thread has not yet initialized, assign heap and try again
		rpmalloc_initialize(0);
		heap = get_thread_heap();
	}
	size_t alloc_size = get_page_aligned_size(size + SPAN_HEADER_SIZE);
	size_t offset = 0;
	size_t mapped_size = 0;
	void* block = global_memory_interface->memory_map(alloc_size, SPAN_SIZE, &offset, &mapped_size);
	if (block) {
		span_t* span = block;
#if ENABLE_DECOMMIT
		if (global_memory_interface->memory_commit(span, alloc_size) != 0)
			return 0;
#endif
#if RPMALLOC_HEAP_STATISTICS
		heap->stats.mapped_size += mapped_size;
#if ENABLE_DECOMMIT
		heap->stats.committed_size += alloc_size;
#else
		heap->stats.committed_size += mapped_size;
#endif
#endif
		span->heap = heap;
		span->page_type = PAGE_HUGE;
		span->page_size = (uint32_t)global_config.page_size;
		span->page_count = (uint32_t)(alloc_size / global_config.page_size);
		span->page_address_mask = LARGE_PAGE_MASK;
		span->offset = (uint32_t)offset;
		span->mapped_size = mapped_size;
		span->page.heap = heap;
		span->page.is_full = 1;
		span->page.generic_free = 1;
		span->page.page_type = PAGE_HUGE;
		// Keep track of span if first class heap
		if (!heap->owner_thread) {
			span->next = heap->span_used[PAGE_HUGE];
			heap->span_used[PAGE_HUGE] = span;
		}
		void* ptr = pointer_offset(block, SPAN_HEADER_SIZE);
		if (zero)
			memset(ptr, 0, size);
#if RPMALLOC_HEAP_STATISTICS
		heap->stats.allocated_size += size;
#endif
		return ptr;
	}
	return 0;
}

static RPMALLOC_ALLOCATOR NOINLINE void*
heap_allocate_block_generic(heap_t* heap, size_t size, unsigned int zero) {
	uint32_t size_class = get_size_class(size);
	if (EXPECTED(size_class < SIZE_CLASS_COUNT)) {
#if RPMALLOC_HEAP_STATISTICS
		heap->stats.allocated_size += global_size_class[size_class].block_size;
#endif

		block_t* block = heap_pop_local_free(heap, size_class);
		if (EXPECTED(block != 0)) {
			// Fast track with small block available in heap level local free list
			if (zero)
				memset(block, 0, global_size_class[size_class].block_size);
			return block;
		}

		return heap_allocate_block_small_to_large(heap, size_class, zero);
	}

	return heap_allocate_block_huge(heap, size, zero);
}

//! Find or allocate a block of the given size
static inline RPMALLOC_ALLOCATOR void*
heap_allocate_block(heap_t* heap, size_t size, unsigned int zero) {
	if (size <= (SMALL_GRANULARITY * 64)) {
		uint32_t size_class = get_size_class_tiny(size);
		block_t* block = heap_pop_local_free(heap, size_class);
		if (EXPECTED(block != 0)) {
			// Fast track with small block available in heap level local free list
			if (zero)
				memset(block, 0, global_size_class[size_class].block_size);
#if RPMALLOC_HEAP_STATISTICS
			heap->stats.allocated_size += global_size_class[size_class].block_size;
#endif
			return block;
		}
	}
	return heap_allocate_block_generic(heap, size, zero);
}

static RPMALLOC_ALLOCATOR void*
heap_allocate_block_aligned(heap_t* heap, size_t alignment, size_t size, unsigned int zero) {
	if (alignment <= SMALL_GRANULARITY)
		return heap_allocate_block(heap, size, zero);

#if ENABLE_VALIDATE_ARGS
	if ((size + alignment) < size) {
		errno = EINVAL;
		return 0;
	}
	if (alignment & (alignment - 1)) {
		errno = EINVAL;
		return 0;
	}
#endif
	if (alignment >= RPMALLOC_MAX_ALIGNMENT) {
		errno = EINVAL;
		return 0;
	}

	size_t align_mask = alignment - 1;
	block_t* block = heap_allocate_block(heap, size + alignment, zero);
	if ((uintptr_t)block & align_mask) {
		block = (void*)(((uintptr_t)block & ~(uintptr_t)align_mask) + alignment);
		// Mark as having aligned blocks
		span_t* span = block_get_span(block);
		page_t* page = span_get_page_from_block(span, block);
		page->has_aligned_block = 1;
		page->generic_free = 1;
	}
	return block;
}

static void*
heap_reallocate_block(heap_t* heap, void* block, size_t size, size_t old_size, unsigned int flags) {
	if (block) {
		// Grab the span using guaranteed span alignment
		span_t* span = block_get_span(block);
		if (EXPECTED(span->page_type <= PAGE_LARGE)) {
			// Normal sized block
			page_t* page = span_get_page_from_block(span, block);
			void* blocks_start = pointer_offset(page, PAGE_HEADER_SIZE);
			uint32_t block_offset = (uint32_t)pointer_diff(block, blocks_start);
			uint32_t block_idx = block_offset / page->block_size;
			void* block_origin = pointer_offset(blocks_start, (size_t)block_idx * page->block_size);
			if (!old_size)
				old_size = (size_t)((ptrdiff_t)page->block_size - pointer_diff(block, block_origin));
			if ((size_t)page->block_size >= size) {
				// Still fits in block, never mind trying to save memory, but preserve data if alignment changed
				if ((block != block_origin) && !(flags & RPMALLOC_NO_PRESERVE))
					memmove(block_origin, block, old_size);
				return block_origin;
			}
		} else {
			// Huge block
			void* block_start = pointer_offset(span, SPAN_HEADER_SIZE);
			if (!old_size)
				old_size = ((size_t)span->page_size * (size_t)span->page_count) - SPAN_HEADER_SIZE;
			if ((size < old_size) && (size > LARGE_BLOCK_SIZE_LIMIT)) {
				// Still fits in block and still huge, never mind trying to save memory,
				// but preserve data if alignment changed
				if ((block_start != block) && !(flags & RPMALLOC_NO_PRESERVE))
					memmove(block_start, block, old_size);
				return block_start;
			}
		}
	} else {
		old_size = 0;
	}

	if (!!(flags & RPMALLOC_GROW_OR_FAIL))
		return 0;

	// Size is greater than block size or saves enough memory to resize, need to allocate a new block
	// and deallocate the old. Avoid hysteresis by overallocating if increase is small (below 37%)
	size_t lower_bound = old_size + (old_size >> 2) + (old_size >> 3);
	size_t new_size = (size > lower_bound) ? size : ((size > old_size) ? lower_bound : size);
	void* old_block = block;
	block = heap_allocate_block(heap, new_size, 0);
	if (block && old_block) {
		if (!(flags & RPMALLOC_NO_PRESERVE))
			memcpy(block, old_block, old_size < new_size ? old_size : new_size);
		block_deallocate(old_block);
	}

	return block;
}

static void*
heap_reallocate_block_aligned(heap_t* heap, void* block, size_t alignment, size_t size, size_t old_size,
                              unsigned int flags) {
	if (alignment <= SMALL_GRANULARITY)
		return heap_reallocate_block(heap, block, size, old_size, flags);

	int no_alloc = !!(flags & RPMALLOC_GROW_OR_FAIL);
	size_t usable_size = (block ? block_usable_size(block) : 0);
	if ((usable_size >= size) && !((uintptr_t)block & (alignment - 1))) {
		if (no_alloc || (size >= (usable_size / 2)))
			return block;
	}
	// Aligned alloc marks span as having aligned blocks
	void* old_block = block;
	block = (!no_alloc ? heap_allocate_block_aligned(heap, alignment, size, 0) : 0);
	if (EXPECTED(block != 0)) {
		if (!(flags & RPMALLOC_NO_PRESERVE) && old_block) {
			if (!old_size)
				old_size = usable_size;
			memcpy(block, old_block, old_size < size ? old_size : size);
		}
		if (EXPECTED(old_block != 0))
			block_deallocate(old_block);
	}
	return block;
}

static void
heap_free_all(heap_t* heap) {
	for (int itype = 0; itype < 3; ++itype) {
		span_t* span = heap->span_partial[itype];
		while (span) {
			span_t* span_next = span->next;
			global_memory_interface->memory_unmap(span, span->offset, span->mapped_size);
			span = span_next;
		}
		heap->span_partial[itype] = 0;
		heap->page_free[itype] = 0;
		heap->page_free_commit_count[itype] = 0;
		atomic_store_explicit(&heap->thread_free[itype], 0, memory_order_release);
	}
	for (int itype = 0; itype < 4; ++itype) {
		span_t* span = heap->span_used[itype];
		while (span) {
			span_t* span_next = span->next;
			global_memory_interface->memory_unmap(span, span->offset, span->mapped_size);
			span = span_next;
		}
		heap->span_used[itype] = 0;
	}
	memset(heap->local_free, 0, sizeof(heap->local_free));
	memset(heap->page_available, 0, sizeof(heap->page_available));

#if ENABLE_STATISTICS
	// TODO: Fix
#endif
}

////////////
///
/// Extern interface
///
//////

int
rpmalloc_is_thread_initialized(void) {
	return (get_thread_heap() != global_heap_default) ? 1 : 0;
}

extern inline RPMALLOC_ALLOCATOR void*
rpmalloc(size_t size) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return 0;
	}
#endif
	heap_t* heap = get_thread_heap();
	return heap_allocate_block(heap, size, 0);
}

extern inline RPMALLOC_ALLOCATOR void*
rpzalloc(size_t size) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return 0;
	}
#endif
	heap_t* heap = get_thread_heap();
	return heap_allocate_block(heap, size, 1);
}

extern inline void
rpfree(void* ptr) {
	if (UNEXPECTED(ptr == 0))
		return;
	block_deallocate(ptr);
}

extern inline RPMALLOC_ALLOCATOR void*
rpcalloc(size_t num, size_t size) {
	size_t total;
#if ENABLE_VALIDATE_ARGS
#if PLATFORM_WINDOWS
	int err = SizeTMult(num, size, &total);
	if ((err != S_OK) || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#else
	int err = __builtin_umull_overflow(num, size, &total);
	if (err || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
#else
	total = num * size;
#endif
	heap_t* heap = get_thread_heap();
	return heap_allocate_block(heap, total, 1);
}

extern inline RPMALLOC_ALLOCATOR void*
rprealloc(void* ptr, size_t size) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return ptr;
	}
#endif
	heap_t* heap = get_thread_heap();
	return heap_reallocate_block(heap, ptr, size, 0, 0);
}

extern RPMALLOC_ALLOCATOR void*
rpaligned_realloc(void* ptr, size_t alignment, size_t size, size_t oldsize, unsigned int flags) {
#if ENABLE_VALIDATE_ARGS
	if ((size + alignment < size) || (alignment > SMALL_PAGE_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
	heap_t* heap = get_thread_heap();
	return heap_reallocate_block_aligned(heap, ptr, alignment, size, oldsize, flags);
}

extern RPMALLOC_ALLOCATOR void*
rpaligned_alloc(size_t alignment, size_t size) {
	heap_t* heap = get_thread_heap();
	return heap_allocate_block_aligned(heap, alignment, size, 0);
}

extern RPMALLOC_ALLOCATOR void*
rpaligned_zalloc(size_t alignment, size_t size) {
	heap_t* heap = get_thread_heap();
	return heap_allocate_block_aligned(heap, alignment, size, 1);
}

extern inline RPMALLOC_ALLOCATOR void*
rpaligned_calloc(size_t alignment, size_t num, size_t size) {
	size_t total;
#if ENABLE_VALIDATE_ARGS
#if PLATFORM_WINDOWS
	int err = SizeTMult(num, size, &total);
	if ((err != S_OK) || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#else
	int err = __builtin_umull_overflow(num, size, &total);
	if (err || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
#else
	total = num * size;
#endif
	heap_t* heap = get_thread_heap();
	return heap_allocate_block_aligned(heap, alignment, total, 1);
}

extern inline RPMALLOC_ALLOCATOR void*
rpmemalign(size_t alignment, size_t size) {
	heap_t* heap = get_thread_heap();
	return heap_allocate_block_aligned(heap, alignment, size, 0);
}

extern inline int
rpposix_memalign(void** memptr, size_t alignment, size_t size) {
	heap_t* heap = get_thread_heap();
	if (memptr)
		*memptr = heap_allocate_block_aligned(heap, alignment, size, 0);
	else
		return EINVAL;
	return *memptr ? 0 : ENOMEM;
}

extern inline size_t
rpmalloc_usable_size(void* ptr) {
	return (ptr ? block_usable_size(ptr) : 0);
}

////////////
///
/// Initialization and finalization
///
//////

static void
rpmalloc_thread_destructor(void* value) {
	// If this is called on main thread assume it means rpmalloc_finalize
	// has not been called and shutdown is forced (through _exit) or unclean
	if (get_thread_id() == global_main_thread_id)
		return;
	if (value)
		rpmalloc_thread_finalize();
}

extern int
rpmalloc_initialize_config(rpmalloc_interface_t* memory_interface, rpmalloc_config_t* config) {
	if (global_rpmalloc_initialized) {
		rpmalloc_thread_initialize();
		if (config)
			*config = global_config;
		return 0;
	}

	if (config)
		global_config = *config;

	int result = rpmalloc_initialize(memory_interface);

	if (config)
		*config = global_config;

	return result;
}

extern int
rpmalloc_initialize(rpmalloc_interface_t* memory_interface) {
	if (global_rpmalloc_initialized) {
		rpmalloc_thread_initialize();
		return 0;
	}

	global_rpmalloc_initialized = 1;

	global_memory_interface = memory_interface ? memory_interface : &global_memory_interface_default;
	if (!global_memory_interface->memory_map || !global_memory_interface->memory_unmap) {
		global_memory_interface->memory_map = os_mmap;
		global_memory_interface->memory_commit = os_mcommit;
		global_memory_interface->memory_decommit = os_mdecommit;
		global_memory_interface->memory_unmap = os_munmap;
	}

#if PLATFORM_WINDOWS
	SYSTEM_INFO system_info;
	memset(&system_info, 0, sizeof(system_info));
	GetSystemInfo(&system_info);
	os_map_granularity = system_info.dwAllocationGranularity;
#else
	os_map_granularity = (size_t)sysconf(_SC_PAGESIZE);
#endif

#if PLATFORM_WINDOWS
	os_page_size = system_info.dwPageSize;
#else
	os_page_size = os_map_granularity;
#endif
	if (global_config.enable_huge_pages) {
#if PLATFORM_WINDOWS
		HANDLE token = 0;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM)
		size_t large_page_minimum = GetLargePageMinimum();
#else
		size_t large_page_minimum = 2 * 1024 * 1024;
#endif
		if (large_page_minimum)
			OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);
		if (token) {
			LUID luid;
			if (LookupPrivilegeValue(0, SE_LOCK_MEMORY_NAME, &luid)) {
				TOKEN_PRIVILEGES token_privileges;
				memset(&token_privileges, 0, sizeof(token_privileges));
				token_privileges.PrivilegeCount = 1;
				token_privileges.Privileges[0].Luid = luid;
				token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				if (AdjustTokenPrivileges(token, FALSE, &token_privileges, 0, 0, 0)) {
					if (GetLastError() == ERROR_SUCCESS)
						os_huge_pages = 1;
				}
			}
			CloseHandle(token);
		}
		if (os_huge_pages) {
			if (large_page_minimum > os_page_size)
				os_page_size = large_page_minimum;
			if (large_page_minimum > os_map_granularity)
				os_map_granularity = large_page_minimum;
		}
#elif defined(__linux__)
		size_t huge_page_size = 0;
		FILE* meminfo = fopen("/proc/meminfo", "r");
		if (meminfo) {
			char line[128];
			while (!huge_page_size && fgets(line, sizeof(line) - 1, meminfo)) {
				line[sizeof(line) - 1] = 0;
				if (strstr(line, "Hugepagesize:"))
					huge_page_size = (size_t)strtol(line + 13, 0, 10) * 1024;
			}
			fclose(meminfo);
		}
		if (huge_page_size) {
			os_huge_pages = 1;
			os_page_size = huge_page_size;
			os_map_granularity = huge_page_size;
		}
#elif defined(__FreeBSD__)
		int rc;
		size_t sz = sizeof(rc);

		if (sysctlbyname("vm.pmap.pg_ps_enabled", &rc, &sz, NULL, 0) == 0 && rc == 1) {
			os_huge_pages = 1;
			os_page_size = 2 * 1024 * 1024;
			os_map_granularity = os_page_size;
		}
#elif defined(__APPLE__) || defined(__NetBSD__)
		os_huge_pages = 1;
		os_page_size = 2 * 1024 * 1024;
		os_map_granularity = os_page_size;
#endif
	} else {
		os_huge_pages = 0;
	}

	global_config.enable_huge_pages = os_huge_pages;

	if (!memory_interface || (global_config.page_size < os_page_size))
		global_config.page_size = os_page_size;

	if (global_config.enable_huge_pages || global_config.page_size > (256 * 1024))
		global_config.disable_decommit = 1;

#if defined(__linux__) || defined(__ANDROID__)
	if (global_config.disable_thp)
		(void)prctl(PR_SET_THP_DISABLE, 1, 0, 0, 0);
#endif

#ifdef _WIN32
	fls_key = FlsAlloc(&rpmalloc_thread_destructor);
#else
	pthread_key_create(&pthread_key, rpmalloc_thread_destructor);
#endif

	global_main_thread_id = get_thread_id();

	rpmalloc_thread_initialize();

	return 0;
}

extern const rpmalloc_config_t*
rpmalloc_config(void) {
	return &global_config;
}

extern void
rpmalloc_finalize(void) {
	rpmalloc_thread_finalize();

	if (global_config.unmap_on_finalize) {
		heap_t* heap = global_heap_queue;
		global_heap_queue = 0;
		while (heap) {
			heap_t* heap_next = heap->next;
			heap_free_all(heap);
			heap_unmap(heap);
			heap = heap_next;
		}
		heap = global_heap_used;
		global_heap_used = 0;
		while (heap) {
			heap_t* heap_next = heap->next;
			heap_free_all(heap);
			heap_unmap(heap);
			heap = heap_next;
		}
#if ENABLE_STATISTICS
		memset(&global_statistics, 0, sizeof(global_statistics));
#endif
	}

#ifdef _WIN32
	FlsFree(fls_key);
	fls_key = 0;
#else
	pthread_key_delete(pthread_key);
	pthread_key = 0;
#endif

	global_main_thread_id = 0;
	global_rpmalloc_initialized = 0;
}

extern void
rpmalloc_thread_initialize(void) {
	if (get_thread_heap() == global_heap_default)
		get_thread_heap_allocate();
}

extern void
rpmalloc_thread_finalize(void) {
	heap_t* heap = get_thread_heap();
	if (heap != global_heap_default) {
		heap_release(heap);
		set_thread_heap(global_heap_default);
	}
}

extern void
rpmalloc_thread_collect(void) {
}

void
rpmalloc_dump_statistics(void* file) {
#if ENABLE_STATISTICS
	fprintf(file, "Mapped pages:        %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.page_mapped, memory_order_relaxed));
	fprintf(file, "Mapped pages (peak): %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.page_mapped_peak, memory_order_relaxed));
	fprintf(file, "Active pages:        %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.page_active, memory_order_relaxed));
	fprintf(file, "Active pages (peak): %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.page_active_peak, memory_order_relaxed));
	fprintf(file, "Pages committed:     %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.page_commit, memory_order_relaxed));
	fprintf(file, "Pages decommitted:   %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.page_decommit, memory_order_relaxed));
	fprintf(file, "Heaps created:       %llu\n",
	        (unsigned long long)atomic_load_explicit(&global_statistics.heap_count, memory_order_relaxed));
#else
	(void)sizeof(file);
#endif
}

void
rpmalloc_global_statistics(rpmalloc_global_statistics_t* stats) {
#if ENABLE_STATISTICS
    stats->mapped = global_config.page_size * atomic_load_explicit(&global_statistics.page_mapped, memory_order_relaxed);
    stats->mapped_peak = global_config.page_size * atomic_load_explicit(&global_statistics.page_mapped_peak, memory_order_relaxed);
    stats->committed = global_config.page_size * atomic_load_explicit(&global_statistics.page_commit, memory_order_relaxed);
    stats->decommitted = global_config.page_size * atomic_load_explicit(&global_statistics.page_decommit, memory_order_relaxed);
    stats->active = global_config.page_size * atomic_load_explicit(&global_statistics.page_active, memory_order_relaxed);
    stats->active_peak = global_config.page_size * atomic_load_explicit(&global_statistics.page_active_peak, memory_order_relaxed);
    stats->heap_count = atomic_load_explicit(&global_statistics.heap_count, memory_order_relaxed);
#else
    memset(stats, 0, sizeof(rpmalloc_global_statistics_t));
#endif
}

#if RPMALLOC_FIRST_CLASS_HEAPS

rpmalloc_heap_t*
rpmalloc_heap_acquire(void) {
	// Must be a pristine heap from newly mapped memory pages, or else memory blocks
	// could already be allocated from the heap which would (wrongly) be released when
	// heap is cleared with rpmalloc_heap_free_all(). Also heaps guaranteed to be
	// pristine from the dedicated orphan list can be used.
	heap_t* heap = heap_allocate(1);
	rpmalloc_assume(heap != 0);
	heap->owner_thread = 0;
	return heap;
}

void
rpmalloc_heap_release(rpmalloc_heap_t* heap) {
	if (heap)
		heap_release(heap);
}

RPMALLOC_ALLOCATOR void*
rpmalloc_heap_alloc(rpmalloc_heap_t* heap, size_t size) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return 0;
	}
#endif
	return heap_allocate_block(heap, size, 0);
}

RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_alloc(rpmalloc_heap_t* heap, size_t alignment, size_t size) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return 0;
	}
#endif
	return heap_allocate_block_aligned(heap, alignment, size, 0);
}

RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_zalloc(rpmalloc_heap_t* heap, size_t alignment, size_t size) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return 0;
	}
#endif
	return heap_allocate_block_aligned(heap, alignment, size, 1);
}

RPMALLOC_ALLOCATOR void*
rpmalloc_heap_calloc(rpmalloc_heap_t* heap, size_t num, size_t size) {
	size_t total;
#if ENABLE_VALIDATE_ARGS
#if PLATFORM_WINDOWS
	int err = SizeTMult(num, size, &total);
	if ((err != S_OK) || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#else
	int err = __builtin_umull_overflow(num, size, &total);
	if (err || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
#else
	total = num * size;
#endif
	return heap_allocate_block(heap, total, 1);
}

extern inline RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_calloc(rpmalloc_heap_t* heap, size_t alignment, size_t num, size_t size) {
	size_t total;
#if ENABLE_VALIDATE_ARGS
#if PLATFORM_WINDOWS
	int err = SizeTMult(num, size, &total);
	if ((err != S_OK) || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#else
	int err = __builtin_umull_overflow(num, size, &total);
	if (err || (total >= MAX_ALLOC_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
#else
	total = num * size;
#endif
	return heap_allocate_block_aligned(heap, alignment, total, 1);
}

RPMALLOC_ALLOCATOR void*
rpmalloc_heap_realloc(rpmalloc_heap_t* heap, void* ptr, size_t size, unsigned int flags) {
#if ENABLE_VALIDATE_ARGS
	if (size >= MAX_ALLOC_SIZE) {
		errno = EINVAL;
		return ptr;
	}
#endif
	return heap_reallocate_block(heap, ptr, size, 0, flags);
}

RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_realloc(rpmalloc_heap_t* heap, void* ptr, size_t alignment, size_t size, unsigned int flags) {
#if ENABLE_VALIDATE_ARGS
	if ((size + alignment < size) || (alignment > SMALL_PAGE_SIZE)) {
		errno = EINVAL;
		return 0;
	}
#endif
	return heap_reallocate_block_aligned(heap, ptr, alignment, size, 0, flags);
}

void
rpmalloc_heap_free(rpmalloc_heap_t* heap, void* ptr) {
	(void)sizeof(heap);
	block_deallocate(ptr);
}

//! Free all memory allocated by the heap
void
rpmalloc_heap_free_all(rpmalloc_heap_t* heap) {
	heap_free_all(heap);
}

struct rpmalloc_heap_statistics_t
rpmalloc_heap_statistics(rpmalloc_heap_t* heap) {
#if RPMALLOC_HEAP_STATISTICS
	if (heap) {
		return heap->stats;
	}
#endif
	(void)sizeof(heap);
	struct rpmalloc_heap_statistics_t stats = {0};
	return stats;
}

extern inline void
rpmalloc_heap_thread_set_current(rpmalloc_heap_t* heap) {
	heap_t* prev_heap = get_thread_heap();
	if (prev_heap != heap) {
		set_thread_heap(heap);
		if (prev_heap)
			heap_release(prev_heap);
	}
}

rpmalloc_heap_t*
rpmalloc_get_heap_for_ptr(void* ptr) {
	// Grab the span, and then the heap from the span
	span_t* span = (span_t*)((uintptr_t)ptr & SPAN_MASK);
	if (span)
		return span_get_page_from_block(span, ptr)->heap;
	return 0;
}

#endif

#include "malloc.c"
