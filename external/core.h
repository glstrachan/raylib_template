#pragma once
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>

#if defined(_WIN32) || defined(_WIN64)
    #define OC_PLATFORM_WINDOWS 1
#else
    #define OC_PLATFORM_UNIX 1
#endif

#if __STDC_VERSION__ == 201710L
    // #define __typeof__(a) int
#endif

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 202311L
#else
    #define typeof __typeof__
#endif

typedef unsigned char          uint8;
typedef signed char            sint8;
typedef unsigned short int     uint16;
typedef signed short int       sint16;
typedef unsigned int           uint32;
typedef signed int             sint32;
typedef unsigned long long int uint64;
typedef signed long long int   sint64;
typedef unsigned long long int uword;
typedef signed long long int   sword;

typedef struct {
    char* ptr;
    uword len;
} string;

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))

#if OC_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    // #include "Windows.h"
    // #define typeof decltype

    #define WINAPI __stdcall
    #define MEM_RESERVE                     0x00002000  
    #define MEM_COMMIT                      0x00001000  
    #define PAGE_READWRITE          0x04    
    #define PAGE_EXECUTE 0x10
    typedef void* HANDLE;

    #define STD_INPUT_HANDLE    ((sint32)-10)
    #define STD_OUTPUT_HANDLE   ((sint32)-11)
    #define STD_ERROR_HANDLE    ((sint32)-12)

    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING ((sint32)0x0004)

    _Noreturn void WINAPI ExitProcess(uint32 uExitCode);
    void* WINAPI VirtualAlloc(void* lpAddress, sword dwSize, sint32 flAllocationType, sint32 flProtect);
    uint32_t WINAPI VirtualFree(void* lpAddress, sword dwSize, sint32 dwFreeType);
    void* WINAPI VirtualProtect(void* BaseAddress, sword Size, sint32 flNewProtect, sint32* lpflOldProtect);

    HANDLE WINAPI GetStdHandle(sint32 nStdHandle);
    sint32 WINAPI GetFileType(HANDLE handle);

    sint32 WINAPI WriteFile(
        HANDLE hFile,
        const void* lpBuffer,
        unsigned long nNumberOfBytesToWrite,
        unsigned long* lpNumberOfBytesWritten,
        void* lpOverlapped
    );

    int WINAPI GetConsoleMode(HANDLE hConsoleHandle, sint32* lpMode);
    int WINAPI SetConsoleMode(HANDLE hConsoleHandle, sint32 lpMode);
#elif OC_PLATFORM_UNIX
    #include <sys/mman.h>
    #include <unistd.h>

    #ifdef __linux__
        #define MAP_UNINITIALIZED 0x4000000
    #else
        #define MAP_UNINITIALIZED 0x0000000
    #endif
#else
    #error "invalid platform"
#endif

#define typeinfo_gen_type_variants_ptr1(type) \
                   type*                : OC_TYPE_POINTER, \
    const          type*                : OC_TYPE_POINTER, \
    const volatile type*                : OC_TYPE_VOLATILE_POINTER, \
          volatile type*                : OC_TYPE_VOLATILE_POINTER, \
          
#if __STDC_VERSION__ >= 201112L
#define typeinfo_kind(x) _Generic((x),                 \
        char:               OC_TYPE_CHAR,              \
        unsigned char:      OC_TYPE_UNSIGNED_CHAR,     \
        unsigned short:     OC_TYPE_UNSIGNED_SHORT,    \
        short:              OC_TYPE_SHORT,             \
        unsigned int:       OC_TYPE_UNSIGNED_INT,      \
        int:                OC_TYPE_INT,               \
        unsigned long:      OC_TYPE_UNSIGNED_LONG,     \
        long:               OC_TYPE_LONG,              \
        unsigned long long: OC_TYPE_UNSIGNED_LONG_LONG,\
        long long:          OC_TYPE_LONG_LONG,         \
        float:              OC_TYPE_FLOAT,             \
        double:             OC_TYPE_DOUBLE,            \
        long double:        OC_TYPE_LONG_DOUBLE,       \
        char*:              OC_TYPE_CHAR_STRING,       \
        const char*:        OC_TYPE_CHAR_STRING,       \
        typeinfo_gen_type_variants_ptr1(unsigned char)      \
        typeinfo_gen_type_variants_ptr1(unsigned short)     \
        typeinfo_gen_type_variants_ptr1(short)              \
        typeinfo_gen_type_variants_ptr1(unsigned int)       \
        typeinfo_gen_type_variants_ptr1(int)                \
        typeinfo_gen_type_variants_ptr1(unsigned long)      \
        typeinfo_gen_type_variants_ptr1(long)               \
        typeinfo_gen_type_variants_ptr1(unsigned long long) \
        typeinfo_gen_type_variants_ptr1(long long)          \
        typeinfo_gen_type_variants_ptr1(float)              \
        typeinfo_gen_type_variants_ptr1(double)             \
        typeinfo_gen_type_variants_ptr1(long double)        \
        typeinfo_gen_type_variants_ptr1(void)        \
\
        char**:               OC_TYPE_POINTER_POINTER,               \
        unsigned char**:      OC_TYPE_POINTER_POINTER,               \
        unsigned short**:     OC_TYPE_POINTER_POINTER,               \
        short**:              OC_TYPE_POINTER_POINTER,               \
        unsigned int**:       OC_TYPE_POINTER_POINTER,               \
        int**:                OC_TYPE_POINTER_POINTER,               \
        unsigned long**:      OC_TYPE_POINTER_POINTER,               \
        long**:               OC_TYPE_POINTER_POINTER,               \
        unsigned long long**: OC_TYPE_POINTER_POINTER,               \
        long long**:          OC_TYPE_POINTER_POINTER,               \
        float**:              OC_TYPE_POINTER_POINTER,               \
        double**:             OC_TYPE_POINTER_POINTER,               \
        long double**:        OC_TYPE_POINTER_POINTER,               \
        void**:               OC_TYPE_POINTER_POINTER,               \
\
        string: OC_TYPE_STRING, \
        string*: OC_TYPE_POINTER, \
        string**: OC_TYPE_POINTER_POINTER, \
        default:  oc_assert(false && "unsupported type: "#x) \
    )
#else
#error "unsupported typeinfo"
#define typeinfo_kind(x) 0
#endif

typedef enum {
    OC_TYPE_CHAR,
    OC_TYPE_UNSIGNED_CHAR,
    OC_TYPE_SIGNED_CHAR,
    OC_TYPE_UNSIGNED_SHORT,
    OC_TYPE_SHORT,
    OC_TYPE_UNSIGNED_INT,
    OC_TYPE_INT,
    OC_TYPE_UNSIGNED_LONG,
    OC_TYPE_LONG,
    OC_TYPE_UNSIGNED_LONG_LONG,
    OC_TYPE_LONG_LONG,
    OC_TYPE_FLOAT,
    OC_TYPE_DOUBLE,
    OC_TYPE_LONG_DOUBLE,
    OC_TYPE_STRING,
    OC_TYPE_CHAR_STRING,
    OC_TYPE_POINTER,
    OC_TYPE_VOLATILE_POINTER,
    OC_TYPE_POINTER_VOLATILE,
    OC_TYPE_VOLATILE_POINTER_VOLATILE,
    OC_TYPE_POINTER_POINTER,
} Oc_Type_Kind;

typedef struct {
    Oc_Type_Kind kind;
} Oc_Type_Info;

typedef struct {
    Oc_Type_Kind kind;
    void* data;
} Oc_Generic;

typedef uword (*WriteFunction)(void* writer, const void* data, uword data_size);

typedef struct oc_writer {
    WriteFunction write;
} Oc_Writer;

typedef struct oc_arena_chunk {
    // this must be word aligned
    uword used, size;
    struct oc_arena_chunk* next;
    uword data[];
} Oc_Arena_Chunk;

typedef struct {
    Oc_Arena_Chunk* head;
    Oc_Arena_Chunk* current;
} Oc_Arena;

typedef struct {
    Oc_Arena_Chunk* chunk;
    uword used;
} Oc_Arena_Save;

typedef struct {
    Oc_Writer writer;
    Oc_Arena* arena;
    char* items;
    uword count, capacity;
} Oc_String_Builder;

typedef struct {
    int base;
    int base_upper;
    int size;
} Oc_Format_Config;

#ifndef NULL
#define NULL     ((void*)0)
#endif
#ifndef INFINITY
#define INFINITY __builtin_inff()
#endif
#ifndef NAN
#define NAN      __builtin_nanf("")
#endif
#define lit(s)   ((string){ s, sizeof(s)-1 })

#ifndef max
#define max(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#endif
#ifndef min
#define min(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#endif

#define _oc_generic_tmp(___y) typeof(_Generic((___y), float: (float)0, double: (double)0, long double: (long double)0, string: ((string){0}), default: (uword)0))
#define _oc_generic_cast(x) _Generic((x), string: (x), default: (_oc_generic_tmp(x))(x))

#define OC_MAKE_GENERIC1(x, n) _oc_generic_tmp(x) ___p_ ## n = _oc_generic_cast(x); 
#define OC_MAKE_GENERIC1_PARAM(x, n) , ((Oc_Generic){ typeinfo_kind(x), & ___p_ ## n })

#define OC_VA_NARGS(...) OC_VA_NARGS_IMPL(, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define OC_VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, N, ...) N

#define OC_CONCAT(a, b) OC_CONCAT_IMPL(a, b)
#define OC_CONCAT_IMPL(a, b) a##b

// Map each argument to a transformation macro
#define OC_MAP(transform_macro, ...) OC_CONCAT(OC_MAP_, OC_VA_NARGS(__VA_ARGS__))(transform_macro, __VA_ARGS__)

#define OC_MAP_0(transform_macro)
#define OC_MAP_1(transform_macro, a1) , transform_macro(a1)
#define OC_MAP_2(transform_macro, a1, a2) , transform_macro(a1), transform_macro(a2)
#define OC_MAP_3(transform_macro, a1, a2, a3) , transform_macro(a1), transform_macro(a2), transform_macro(a3)
#define OC_MAP_4(transform_macro, a1, a2, a3, a4) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4)
#define OC_MAP_5(transform_macro, a1, a2, a3, a4, a5) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5)
#define OC_MAP_6(transform_macro, a1, a2, a3, a4, a5, a6) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6)
#define OC_MAP_7(transform_macro, a1, a2, a3, a4, a5, a6, a7) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7)
#define OC_MAP_8(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8)
#define OC_MAP_9(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9)
#define OC_MAP_10(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10)
#define OC_MAP_11(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10), transform_macro(a11)
#define OC_MAP_12(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10), transform_macro(a11), transform_macro(a12)
#define OC_MAP_13(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10), transform_macro(a11), transform_macro(a12), transform_macro(a13)
#define OC_MAP_14(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10), transform_macro(a11), transform_macro(a12), transform_macro(a13), transform_macro(a14)
#define OC_MAP_15(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10), transform_macro(a11), transform_macro(a12), transform_macro(a13), transform_macro(a14), transform_macro(a15)
#define OC_MAP_16(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) , transform_macro(a1), transform_macro(a2), transform_macro(a3), transform_macro(a4), transform_macro(a5), transform_macro(a6), transform_macro(a7), transform_macro(a8), transform_macro(a9), transform_macro(a10), transform_macro(a11), transform_macro(a12), transform_macro(a13), transform_macro(a14), transform_macro(a15), transform_macro(a16)

#define OC_MAP_SEQ(transform_macro, ...) OC_CONCAT(OC_MAP_SEQ_, OC_VA_NARGS(__VA_ARGS__))(transform_macro, ##__VA_ARGS__)
#define OC_MAP_SEQ_0(transform_macro, ...)
#define OC_MAP_SEQ_1(transform_macro, a1)                                                                      OC_MAP_SEQ_0(transform_macro) transform_macro(a1, 1)
#define OC_MAP_SEQ_2(transform_macro, a1, a2)                                                                  OC_MAP_SEQ_1(transform_macro, a1)  transform_macro(a2, 2)
#define OC_MAP_SEQ_3(transform_macro, a1, a2, a3)                                                              OC_MAP_SEQ_2(transform_macro, a1, a2)  transform_macro(a3, 3)
#define OC_MAP_SEQ_4(transform_macro, a1, a2, a3, a4)                                                          OC_MAP_SEQ_3(transform_macro, a1, a2, a3)  transform_macro(a4, 4)
#define OC_MAP_SEQ_5(transform_macro, a1, a2, a3, a4, a5)                                                      OC_MAP_SEQ_4(transform_macro, a1, a2, a3, a4)  transform_macro(a5, 5)
#define OC_MAP_SEQ_6(transform_macro, a1, a2, a3, a4, a5, a6)                                                  OC_MAP_SEQ_5(transform_macro, a1, a2, a3, a4, a5)  transform_macro(a6, 6)
#define OC_MAP_SEQ_7(transform_macro, a1, a2, a3, a4, a5, a6, a7)                                              OC_MAP_SEQ_6(transform_macro, a1, a2, a3, a4, a5, a6)  transform_macro(a7, 7)
#define OC_MAP_SEQ_8(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8)                                          OC_MAP_SEQ_7(transform_macro, a1, a2, a3, a4, a5, a6, a7)  transform_macro(a8, 8)
#define OC_MAP_SEQ_9(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9)                                      OC_MAP_SEQ_8(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8)  transform_macro(a9, 9)
#define OC_MAP_SEQ_10(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)                                OC_MAP_SEQ_9(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9)  transform_macro(a10, 10)
#define OC_MAP_SEQ_11(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)                           OC_MAP_SEQ_10(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)  transform_macro(a11, 11)
#define OC_MAP_SEQ_12(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)                      OC_MAP_SEQ_11(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)  transform_macro(a12, 12)
#define OC_MAP_SEQ_13(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)                 OC_MAP_SEQ_12(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)  transform_macro(a13, 13)
#define OC_MAP_SEQ_14(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)            OC_MAP_SEQ_13(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)  transform_macro(a14, 14)
#define OC_MAP_SEQ_15(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)       OC_MAP_SEQ_14(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)  transform_macro(a15, 15)
#define OC_MAP_SEQ_16(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)  OC_MAP_SEQ_15(transform_macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)  transform_macro(a16, 16)


#define OC_MAP_SEPARATOR(transform_macro, sep, ...) OC_CONCAT(OC_MAP_SEPARATOR_, OC_VA_NARGS(__VA_ARGS__))(transform_macro, sep, ##__VA_ARGS__)
#define OC_MAP_SEPARATOR_0(transform_macro, sep, ...)
#define OC_MAP_SEPARATOR_1(transform_macro, sep, a1)                                                                      OC_MAP_SEPARATOR_0(transform_macro, sep) transform_macro(a1, 1)
#define OC_MAP_SEPARATOR_2(transform_macro, sep, a1, a2)                                                                  OC_MAP_SEPARATOR_1(transform_macro, sep, a1) sep transform_macro(a2, 2)
#define OC_MAP_SEPARATOR_3(transform_macro, sep, a1, a2, a3)                                                              OC_MAP_SEPARATOR_2(transform_macro, sep, a1, a2) sep transform_macro(a3, 3)
#define OC_MAP_SEPARATOR_4(transform_macro, sep, a1, a2, a3, a4)                                                          OC_MAP_SEPARATOR_3(transform_macro, sep, a1, a2, a3) sep transform_macro(a4, 4)
#define OC_MAP_SEPARATOR_5(transform_macro, sep, a1, a2, a3, a4, a5)                                                      OC_MAP_SEPARATOR_4(transform_macro, sep, a1, a2, a3, a4) sep transform_macro(a5, 5)
#define OC_MAP_SEPARATOR_6(transform_macro, sep, a1, a2, a3, a4, a5, a6)                                                  OC_MAP_SEPARATOR_5(transform_macro, sep, a1, a2, a3, a4, a5) sep transform_macro(a6, 6)
#define OC_MAP_SEPARATOR_7(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7)                                              OC_MAP_SEPARATOR_6(transform_macro, sep, a1, a2, a3, a4, a5, a6) sep transform_macro(a7, 7)
#define OC_MAP_SEPARATOR_8(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8)                                          OC_MAP_SEPARATOR_7(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7) sep transform_macro(a8, 8)
#define OC_MAP_SEPARATOR_9(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9)                                      OC_MAP_SEPARATOR_8(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8) sep transform_macro(a9, 9)
#define OC_MAP_SEPARATOR_10(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)                                OC_MAP_SEPARATOR_9(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9) sep transform_macro(a10, 10)
#define OC_MAP_SEPARATOR_11(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)                           OC_MAP_SEPARATOR_10(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) sep transform_macro(a11, 11)
#define OC_MAP_SEPARATOR_12(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)                      OC_MAP_SEPARATOR_11(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) sep transform_macro(a12, 12)
#define OC_MAP_SEPARATOR_13(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)                 OC_MAP_SEPARATOR_12(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) sep transform_macro(a13, 13)
#define OC_MAP_SEPARATOR_14(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)            OC_MAP_SEPARATOR_13(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) sep transform_macro(a14, 14)
#define OC_MAP_SEPARATOR_15(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)       OC_MAP_SEPARATOR_14(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) sep transform_macro(a15, 15)
#define OC_MAP_SEPARATOR_16(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)  OC_MAP_SEPARATOR_15(transform_macro, sep, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) sep transform_macro(a16, 16)

/* --------  macro api -------- */

#define OC_DEFAULT_MAP_ENTRY_COUNT 512
#define OC_DEFAULT_MAP_SEED 0xf8abc103ba79eb85LLu
#define OC_ARENA_CHUNK_SIZE (4096 * 80)

#define OC_FD_INPUT  (0u)
#define OC_FD_OUTPUT (1u)
#define OC_FD_ERROR  (2u)

// #ifdef __x86_64__
//     #define oc_breakpoint() __asm__("int3")
// #elif __aarch64__
//     #define oc_breakpoint() __asm__("trap")
// #endif
#define oc_breakpoint() __builtin_debugtrap()

#ifndef _NDEBUG
#define oc_assert(expr) ((expr) ? 1 : _oc_assert_fail(#expr, __FILE__, __LINE__, __func__))
#else
#define oc_assert(expr) (void)(0)
#endif
#define oc_unreachable(expr) (_oc_assert_fail(#expr, __FILE__, __LINE__, __func__))
#define wprint(writer, fmt, ...) do { OC_MAP_SEQ(OC_MAKE_GENERIC1, __VA_ARGS__); _oc_printw((writer), fmt OC_MAP_SEQ(OC_MAKE_GENERIC1_PARAM, __VA_ARGS__)); } while (0)
#define print(fmt, ...) do { OC_MAP_SEQ(OC_MAKE_GENERIC1, __VA_ARGS__); _oc_printw(&stdout_writer, fmt OC_MAP_SEQ(OC_MAKE_GENERIC1_PARAM, __VA_ARGS__)); } while (0)
#define eprint(fmt, ...) do { OC_MAP_SEQ(OC_MAKE_GENERIC1, __VA_ARGS__); _oc_printw(&stderr_writer, fmt OC_MAP_SEQ(OC_MAKE_GENERIC1_PARAM, __VA_ARGS__)); } while (0)
#define oc_todo(fmt, ...) do { OC_MAP_SEQ(OC_MAKE_GENERIC1, __VA_ARGS__); print("oc_todo - {}:{} - ", __FILE__, __LINE__); _oc_printw(&stdout_writer, fmt OC_MAP_SEQ(OC_MAKE_GENERIC1_PARAM, __VA_ARGS__)); oc_breakpoint(); oc_exit(-1); } while (0)
#define oc_len(arr) (sizeof(arr)/sizeof((arr)[0]))
#define oc_pun(value, type) ({ __typeof__(value) _v = (value); *(type*)&_v; })
#define oc_oom() do { print("Out of memory: {}:{}\n", __FILE__, __LINE__); oc_exit(-1); } while (0)
#define oc_format(arena, fmt, ...) ({        \
    Oc_String_Builder sb;                    \
    oc_sb_init(&sb, arena);                  \
    wprint(&sb.writer, fmt, ## __VA_ARGS__); \
    oc_sb_to_string(&sb);                    \
})

#define Enum(_name, _type, ...) typedef _type _name; enum { __VA_ARGS__ }
#define Hash_Map(K, V) struct { struct { K _key; V _value; uint32 filled; }* entries; uint32 count_filled, capacity; }
#define hash_map_get_from_hash(arena, hm, key, hash) ({                                             \
        __typeof__((hm)->entries[0]._value)* result = NULL;                          \
        if ((hm)->capacity) { \
            uint32 index = hash % (hm)->capacity; \
            uint32 until_index = index;                                                 \
            do {                                                                        \
                typeof(key) key__ = key; \
                if ((hm)->entries[index].filled && MAP_DEFAULT_EQL_FN(&(hm)->entries[index]._key, &key__)) {                \
                    result = &(hm)->entries[index]._value;                               \
                    break;                                                              \
                }                                                                       \
                index = (index + 1) % (hm)->capacity;                                   \
            } while ((hm)->entries[index].filled && index != until_index);              \
        } \
        result;                                                                     \
    })

#define hash_map_get(arena, hm, key) ({                                             \
        __typeof__((hm)->entries[0]._value)* result = NULL;                          \
        if ((hm)->capacity) { \
            typeof(key) key__ = key; \
            uint32 index = MAP_DEFAULT_HASH_FN(&key__, sizeof(key), MAP_DEFAULT_SEED) % (hm)->capacity; \
            uint32 until_index = index;                                                 \
            do {                                                                        \
                if ((hm)->entries[index].filled && MAP_DEFAULT_EQL_FN(&(hm)->entries[index]._key, &key__, sizeof(key))) {                \
                    result = &(hm)->entries[index]._value;                               \
                    break;                                                              \
                }                                                                       \
                index = (index + 1) % (hm)->capacity;                                   \
            } while ((hm)->entries[index].filled && index != until_index);              \
        } \
        result;                                                                     \
    })

#define hash_map_reserve(arena, hm, reserve_capacity) do {                                                                                              \
        __typeof__((hm)->entries) new_entries = oc_arena_realloc((arena), (hm)->entries, (hm)->capacity * sizeof(*(hm)->entries), (reserve_capacity) * sizeof(*(hm)->entries)); \
        memset(new_entries, 0, (reserve_capacity) * sizeof(*(hm)->entries)); \
        if ((hm)->capacity) {                                                                                                                           \
            for (uint32 i = 0; i < ((hm)->capacity); ++i) {                                                                                               \
                if (!(hm)->entries[i].filled) continue;                                                                                               \
                uint32 index = MAP_DEFAULT_HASH_FN(&(hm)->entries[i]._key, sizeof((hm)->entries[i]._key), MAP_DEFAULT_SEED) % (reserve_capacity);                                   \
                while (new_entries[index].filled) {                                                                                               \
                    if (MAP_DEFAULT_EQL_FN(&new_entries[index]._key, &(hm)->entries[i]._key, sizeof((hm)->entries[i]._key))) {                                                                       \
                        break;                                                                                                                      \
                    }                                                                                                                               \
                    index = (index + 1) % (reserve_capacity);                                                                                       \
                }                                                                                                                                   \
                new_entries[index]._key = (hm)->entries[i]._key;                                                                                                    \
                new_entries[index]._value = (hm)->entries[i]._value;                                                                                                \
                new_entries[index].filled = 1;                                                                                                    \
            }                                                                                                                                           \
        }                                                                                                                                               \
        (hm)->entries = new_entries;                                                                                                                        \
        (hm)->capacity = (reserve_capacity);                                                                                                            \
    } while (0)

#define hash_map_put(arena, hm, key, value) do {                                                                                                 \
        if ((hm)->count_filled >= (hm)->capacity / 2) {                                                                                          \
            uint32 new_cap = (hm)->capacity ? (hm)->capacity * 4 : 32;                                                                          \
            hash_map_reserve(arena, hm, new_cap);                                                                                         \
        }                                                                                                                                        \
        typeof(key) key__ = key; \
        uint32 index = MAP_DEFAULT_HASH_FN(&key__, sizeof(key), MAP_DEFAULT_SEED) % (hm)->capacity;                                                                \
        while ((hm)->entries[index].filled) {                                                                                                      \
            if (MAP_DEFAULT_EQL_FN(&(hm)->entries[index]._key, &key__, sizeof(key))) {                                                                               \
                break;                                                                                                                           \
            }                                                                                                                                    \
            index = (index + 1) % (hm)->capacity;                                                                                                  \
        }                                                                                                                                        \
        (hm)->entries[index]._key = key;                                                                                               \
        (hm)->entries[index]._value = value;                                                                                               \
        (hm)->entries[index].filled = 1;                                                                                               \
        (hm)->count_filled++; \
    } while(0)

#define hash_map_iterator(hm) struct { typeof((hm)->entries[0]._value)* value; typeof((hm)->entries[0]._key)* key; typeof((hm)->capacity) index; }
#define hash_map_iterator_next(hm, _it) ((_it).index = 0; (_it).index < (hm)->capacity; ++(_it).index) if (((_it).key = &(hm)->entries[(_it).index]._key, (_it).value = &(hm)->entries[(_it).index]._value, (hm)->entries[(_it).index].filled))


#define Array(V) struct { V* items; uint32 count, capacity; }
#define oc_array_aligned_append(arena, array, alignment, value)                                                     \
    do {                                                                                         \
        if ((array)->count + 1 > (array)->capacity) {                                            \
            uword new_cap = (array)->capacity ? (array)->capacity * 2 : 16;                      \
            void* new_ptr = oc_arena_aligned_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items), (alignment)); \
            (array)->items = new_ptr;                                                            \
            (array)->capacity = new_cap;                                                         \
        }                                                                                        \
        (array)->items[(array)->count++] = value;                                                \
    } while (0)
#define oc_array_aligned_append_many(arena, array, alignment, ptr, len)                                             \
    do {                                                                                         \
        if ((array)->count + (len) > (array)->capacity) {                                            \
            uword new_cap = (array)->capacity ? ((array)->count + (len) + (array)->capacity * 2) : (((array)->count + (len)) * 8); \
            void* new_ptr = oc_arena_aligned_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items), (alignment)); \
            (array)->items = new_ptr;                                                            \
            (array)->capacity = new_cap;                                                         \
        }                                                                                        \
        memcpy((array)->items + (array)->count, ptr, (len) * sizeof(*(array)->items));  \
        (array)->count += (len);                              \
    } while (0)
#define oc_array_aligned_reserve(arena, array, alignment, _count)                                                     \
    do {                                                                                           \
        if ((_count) > (array)->capacity) {                                                        \
            uword new_cap = (_count) * 2;                                                           \
            void* new_ptr = oc_arena_aligned_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items), (alignment));   \
            (array)->items = new_ptr;                                                              \
            (array)->capacity = new_cap;                                                           \
        }                                                                                          \
        (array)->count = (_count);                                                                 \
    } while (0)
#define oc_array_aligned_extend_count_unint(arena, array, alignment, plus_count)                                                     \
	do {                                                                                           \
		if ((array)->count + (plus_count) > (array)->capacity) {                                                        \
			uword new_cap = ((array)->count + (plus_count)) * 2;                                                           \
			void* new_ptr = oc_arena_aligned_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items), (alignment));   \
			(array)->items = new_ptr;                                                              \
			(array)->capacity = new_cap;                                                           \
		}                                                                                          \
		(array)->count += (plus_count);                                                                 \
	} while (0)

#define oc_array_append(arena, array, value)                                                     \
    do {                                                                                         \
        if ((array)->count + 1 > (array)->capacity) {                                            \
            uword new_cap = (array)->capacity ? (array)->capacity * 2 : 16;                      \
            void* new_ptr = oc_arena_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items)); \
            (array)->items = new_ptr;                                                            \
            (array)->capacity = new_cap;                                                         \
        }                                                                                        \
        (array)->items[(array)->count++] = value;                                                \
    } while (0)
#define oc_array_append_many(arena, array, ptr, len)                                             \
    do {                                                                                         \
        if ((array)->count + (len) > (array)->capacity) {                                            \
            uword new_cap = (array)->capacity ? ((array)->count + (len) + (array)->capacity * 2) : (((array)->count + (len)) * 8); \
            void* new_ptr = oc_arena_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items)); \
            (array)->items = new_ptr;                                                            \
            (array)->capacity = new_cap;                                                         \
        }                                                                                        \
        memcpy((array)->items + (array)->count, ptr, (len) * sizeof(*(array)->items));  \
        (array)->count += (len);                              \
    } while (0)
#define oc_array_reserve(arena, array, _count)                                                     \
    do {                                                                                           \
        if ((_count) > (array)->capacity) {                                                        \
            uword new_cap = (_count) * 2;                                                           \
            void* new_ptr = oc_arena_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items));   \
            (array)->items = new_ptr;                                                              \
            (array)->capacity = new_cap;                                                           \
        }                                                                                          \
        (array)->count = (_count);                                                                 \
    } while (0)
#define oc_array_extend_count_unint(arena, array, plus_count)                                                     \
	do {                                                                                           \
		if ((array)->count + (plus_count) > (array)->capacity) {                                                        \
			uword new_cap = ((array)->count + (plus_count)) * 2;                                                           \
			void* new_ptr = oc_arena_realloc(arena, (array)->items, (array)->capacity * sizeof(*(array)->items), new_cap * sizeof(*(array)->items));   \
			(array)->items = new_ptr;                                                              \
			(array)->capacity = new_cap;                                                           \
		}                                                                                          \
		(array)->count += (plus_count);                                                                 \
	} while (0)
#define oc_array_unordered_remove(arena, array, index)              \
	do {                                                            \
        (array)->items[index] = (array)->items[(array)->count - 1]; \
		(array)->count--;                                           \
	} while (0)

static inline uword oc_align_forward(uword value, uword alignment_in_bytes) {
    return (value + alignment_in_bytes - 1) & ~(alignment_in_bytes - 1);
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --------    libc forwards  -------- */
// void *memset(void *s, int c, size_t n);
// void *memcpy(void *dest, const void *src, size_t n);
// int memcmp(const void *a, const void *b, size_t n);
// int strncmp(const char *a, const char *b, size_t n);
// int strcmp(const char *a, const char *b);
// size_t strlen(const char *s);
// __declspec(dllimport) _Noreturn void exit(int status);
// typedef void FILE;
// int fopen_s(FILE**, const char*, const char*);
// int fseek(FILE*, int, int);
// size_t ftell(FILE*);
// unsigned long long fwrite(const void *, unsigned long long a, unsigned long long b, FILE *);
// unsigned long long fread(void *, unsigned long long a, unsigned long long b, FILE *);
// void fclose(FILE*);
// __declspec(dllimport) void* realloc(void *, size_t);
// __declspec(dllimport) void* malloc(size_t);
// __declspec(dllimport) void* alloca(size_t);
// #define SEEK_CUR    1
// #define SEEK_END    2
// #define SEEK_SET    0

/* -------- Platform Specific -------- */
void* oc_allocate_pages(uword required_size);
uword stdout_write(void* writer, const uint8* data, uword data_size);
uword stderr_write(void* writer, const uint8* data, uword data_size);
bool oc_is_tty(uint32 fd);
bool oc_fd_supports_color(uint32 fd);

/* --------     Others        -------- */
_Noreturn int _oc_assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);
Oc_Arena_Chunk* oc_arena_new_chunk(Oc_Arena* arena, uword size_in_bytes);
Oc_Arena_Save oc_arena_save(Oc_Arena* arena);
void oc_arena_restore(Oc_Arena* arena, Oc_Arena_Save restore_point);
void oc_arena_reset(Oc_Arena* arena);
void* oc_arena_alloc_aligned(Oc_Arena* arena, uint64 size, uint64 alignment);
void* oc_arena_aligned_realloc(Oc_Arena* arena, void* old_ptr, uint64 old_size, uint64 size, uint64 alignment);
void* oc_arena_alloc(Oc_Arena* arena, uint64 size);
void* oc_arena_realloc(Oc_Arena* arena, void* old_ptr, uint64 old_size, uint64 size);
void* oc_arena_dup(Oc_Arena* arena, void* data, uword size);
string oc_sprintf(Oc_Arena* arena, const char* fmt, ...);
void oc_sb_append_char(Oc_String_Builder* sb, char c);
void oc_sb_append_string(Oc_String_Builder* sb, string s);
void oc_sb_append_char_str(Oc_String_Builder* sb, const char* c);
void oc_sb_append_char_str_len(Oc_String_Builder* sb, const char* c, uword len);
string oc_sb_to_string(Oc_String_Builder* sb);
uword oc_sb_writer_write(void* writer, const void* data, uword data_size);
void oc_sb_init(Oc_String_Builder* sb, Oc_Arena* arena);
void oc_writer_format_and_write_int(Oc_Writer *writer, Oc_Format_Config cfg, uint64 ivalue);
void oc_writer_format_and_write_float(Oc_Writer *writer, Oc_Format_Config cfg, double fvalue);
void _oc_printw(void *writer, const char* fmt, ...);
void _oc_vprintw(void *writer, const char* fmt, va_list args);
_Noreturn void oc_exit(int status);
void oc_hex_dump(void* data, int count, int indent, int mark_mod);

size_t stbds_hash_string(string* str, size_t len, size_t seed);
size_t stbds_hash_string_atom(string* str, size_t len, size_t seed);
size_t stbds_siphash_bytes(void *p, size_t len, size_t seed);

// size_t oc_hash_int(uint64 i, size_t seed);
// size_t oc_eql_int(uint64 i, size_t seed);

static inline bool bytes_eql(void* a, void* b, size_t len) {
    return memcmp(a, b, len) == 0;
}

#define STBDS_SIZE_T_BITS           ((sizeof (size_t)) * 8)
#define STBDS_ROTATE_LEFT(val, n)   (((val) << (n)) | ((val) >> (STBDS_SIZE_T_BITS - (n))))
#define STBDS_ROTATE_RIGHT(val, n)  (((val) >> (n)) | ((val) << (STBDS_SIZE_T_BITS - (n))))

#define MAP_DEFAULT &cc->arena, MAP_DEFAULT_HASH_FN, MAP_DEFAULT_EQL_FN, MAP_DEFAULT_SEED
#define MAP_DEFAULT_HASH_FN(key, len, value) _Generic((key),	\
        string : stbds_hash_string_atom,					\
        default : stbds_siphash_bytes						\
    )(key, len, value)

#define MAP_DEFAULT_EQL_FN(a, b, len) _Generic((a),				\
        string : string_atom_eql,						\
        default : bytes_eql						\
    )(a, b, len)

#define MAP_DEFAULT_SEED (16u)

// start is clamped to zero if negative
// end < 0 means to use the end of s
static inline string string_slice(string s, sword start, sword end) {
    oc_assert(start >= 0);
    oc_assert(end >= start);
    // if (start < 0) start = 0;
    // if (end < 0) end = s.len;

    s.ptr += start;
    s.len = end - start;
    return s;
}

// start is clamped to zero if negative
static inline string string_slice_count(string s, sword start, sword count) {
    if (start < 0) start = 0;

    s.ptr += start;
    s.len = count;
    return s;
}

static inline bool string_atom_eql(string a, string b) {
    return a.ptr == b.ptr;
}

static inline bool string_eql(string a, string b) {
    if (a.len != b.len) return false;
    return strncmp(a.ptr, b.ptr, a.len) == 0;
}

static inline bool string_starts_with(string haystack, string needle) {
    if (needle.len > haystack.len) return false;
    return memcmp(haystack.ptr, needle.ptr, needle.len) == 0;
}


extern Oc_Writer stdout_writer;
extern Oc_Writer stderr_writer;

#include <errno.h>
#ifdef OC_CORE_IMPLEMENTATION
    #ifdef OC_PLATFORM_WINDOWS
        void* oc_allocate_pages(uword required_size) {
            return VirtualAlloc(NULL, required_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        }

        uword stdout_write(void* writer, const uint8* data, uword data_size) {
            (void)writer;
            HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
            uword total_written = 0;

            while (total_written < data_size) {
                uint32 written = 0;
                if (!WriteFile(handle, data, data_size, (unsigned long*)&written, NULL)) {
                    // eprint("WriteFile failed\n");
                    return 0;
                }
                total_written += written;
            }
            return total_written;
        }

        uword stderr_write(void* writer, const uint8* data, uword data_size) {
            (void)writer;
            HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
            uword total_written = 0;

            while (total_written < data_size) {
                uint32 written = 0;
                if (!WriteFile(handle, data, data_size, (unsigned long*)&written, NULL)) {
                    // eprint("WriteFile failed\n");
                    return 0;
                }
                total_written += written;
            }
            return total_written;
        }

        _Noreturn void oc_exit(int status)  {
            ExitProcess(status);
        }

        bool oc_is_tty(uint32 fd) {
            static const sint32 h[] = { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };
            oc_assert(fd < oc_len(h));
            HANDLE handle = GetStdHandle(h[fd]);
            sint32 file_type = GetFileType(handle);
            return file_type == 0x0002;
        }

        bool oc_fd_supports_color(uint32 fd) {
            static const sint32 h[] = { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };
            oc_assert(fd < oc_len(h));
            HANDLE handle = GetStdHandle(h[fd]);
            if (handle == (void*)-1ll) return false;

            sint32 mode = 0;
            GetConsoleMode(handle, &mode);

            if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) return true;

            if (SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                return true;
            }

            return false;
        }
    #else
        int fopen_s(FILE** f, const char* name, const char* mode) {
            int ret = 0;
            oc_assert(f);
            *f = fopen(name, mode);
            /* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
            if (!*f)
                ret = errno;
            return ret;
        }

        void* oc_allocate_pages(uword required_size) {
            return mmap(NULL, required_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED, -1, 0);
        }

        uword stdout_write(void* writer, const uint8* data, uword data_size) {
            (void)writer;
            return write(STDOUT_FILENO, data, (size_t)data_size);
        }

        uword stderr_write(void* writer, const uint8* data, uword data_size) {
            (void)writer;
            return write(STDERR_FILENO, data, (size_t)data_size);
        }

        _Noreturn void oc_exit(int status)  {
            exit(status);
        }

        bool oc_fd_supports_color(uint32 fd) {
            static const sint32 h[] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
            oc_assert(fd < oc_len(h));

            return isatty(h[fd]);
        }

    #endif


_Noreturn int _oc_assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)  {
    (void)function;
    print("assert fail({}:{}): {}\n", file, line, assertion);

    oc_breakpoint();
    exit(1);
}

Oc_Arena_Chunk* oc_arena_new_chunk(Oc_Arena* arena, uword size_in_bytes) {
    // Oc_Arena_Chunk* chunk = malloc(OC_ARENA_CHUNK_SIZE /* should be word aligned */);
    // uword aligned_bytes = oc_align_forward(size_in_bytes + sizeof(Oc_Arena_Chunk), OC_ARENA_CHUNK_SIZE);
    uword aligned_bytes = (size_in_bytes + OC_ARENA_CHUNK_SIZE - 1) / OC_ARENA_CHUNK_SIZE * OC_ARENA_CHUNK_SIZE;
    Oc_Arena_Chunk* chunk = oc_allocate_pages(aligned_bytes);
    if (arena->current && chunk == (void*)(arena->current->data + chunk->size)) {
        // if new chunk is right after current chunk, just extend current chunk
        chunk->size += aligned_bytes;
        return arena->current;
    } else {
        chunk->used = 0;
        chunk->size = (aligned_bytes - sizeof(Oc_Arena_Chunk)) / sizeof(uword);
        chunk->next = NULL;
        return chunk;
    }
}

Oc_Arena_Save oc_arena_save(Oc_Arena* arena) {
    return (Oc_Arena_Save) {
        .chunk = arena->current,
        .used = arena->current ? arena->current->used : 0,
    };
}

void oc_arena_restore(Oc_Arena* arena, Oc_Arena_Save restore_point) {
    arena->current = restore_point.chunk;
    if (arena->current) arena->current->used = restore_point.used;
}

void oc_arena_reset(Oc_Arena* arena) {
    arena->current = arena->head;
    arena->current->used = 0;
}

void* oc_arena_alloc_aligned(Oc_Arena* arena, uint64 size, uint64 alignment) {
#ifdef OC_ENABLE_ASAN_MALLOC
    return _aligned_malloc(size, alignment);
#else
    if (size == 0) return NULL;
    alignment = max(8, alignment);
    // 1 -> 0 -> 0 -> 1
    // 2 -> 1 -> 0 -> 1
    // 7 -> 6 -> 0 -> 1
    // 8 -> 7 -> 0 -> 1
    // 9 -> 8 -> 1 -> 2
    uword words = (size - 1) / sizeof(uword) + 1;

    if (!arena->current) {
        arena->head = arena->current = oc_arena_new_chunk(arena, max(size, alignment));
        if (!arena->head) oc_oom();
    }

    void *result, *aligned_result;
    do {
        if (arena->current->used + words > arena->current->size) {
            if (arena->current->next == NULL) {
                arena->current->next = oc_arena_new_chunk(arena, size);
                if (!arena->current->next) oc_oom();
            } else {
                arena->current->next->used = 0;
            }
            arena->current = arena->current->next;
        }

        result = arena->current->data + arena->current->used;
        aligned_result = (void*)oc_align_forward((uword)result, (uword)alignment);
    } while ((arena->current->used + words + ((aligned_result - result) / sizeof(uword))) > arena->current->size);

    arena->current->used += words + (aligned_result - result) / sizeof(uword);

    return aligned_result;
#endif
}

void* oc_arena_alloc(Oc_Arena* arena, uint64 size) {
#ifdef OC_ENABLE_ASAN_MALLOC
    return malloc(size);
#else
    if (size == 0) return NULL;
    // 1 -> 0 -> 0 -> 1
    // 2 -> 1 -> 0 -> 1
    // 7 -> 6 -> 0 -> 1
    // 8 -> 7 -> 0 -> 1
    // 9 -> 8 -> 1 -> 2
    uword words = (size - 1) / sizeof(uword) + 1;
    if (!arena->current) {
        arena->head = arena->current = oc_arena_new_chunk(arena, size);
        if (!arena->head) oc_oom();
    } else {
        if (arena->current->used + words > arena->current->size) {
            if (arena->current->next == NULL) {
                arena->current->next = oc_arena_new_chunk(arena, size);
                if (!arena->current->next) oc_oom();
            } else {
                arena->current->next->used = 0;
            }
            arena->current = arena->current->next;
        }
    }

    void* result = arena->current->data + arena->current->used;
    arena->current->used += words;
    return result;
#endif
}

void* oc_arena_aligned_realloc(Oc_Arena* arena, void* old_ptr, uint64 old_size, uint64 size, uint64 alignment) {
#ifdef OC_ENABLE_ASAN_MALLOC
    return _aligned_realloc(old_ptr, size, alignment);
#else
    oc_assert(arena != NULL);
    // oc_assert(old_ptr != NULL);
    if (old_ptr == NULL)  return oc_arena_alloc_aligned(arena, size, alignment);
    if (size <= old_size) return old_ptr;

    uword old_words = (old_size - 1) / sizeof(uword) + 1;
    uword new_words = (size - 1) / sizeof(uword) + 1;

    // print("oc_arena_realloc: old_ptr = {}, old_size = {}, size = {}\n", old_ptr, old_size, size);
    oc_assert(arena->current);
    if ((uint8*)old_ptr + old_size == (uint8*)(arena->current->data + arena->current->used)) {
        if (arena->current->used + (new_words - old_words) <= arena->current->size) {
            // print("oc_arena_realloc: Reusing memory :)\n");
            // reuse memory
            arena->current->used += new_words - old_words;
            return old_ptr;
        }
    } else if (arena->current->used + new_words <= arena->current->size) {
        // print("oc_arena_realloc: Reusing chunk, but copying old data :|\n");
        // copy old data
        void* new_ptr = arena->current->data + arena->current->used;
        memcpy(new_ptr, old_ptr, old_size);
        arena->current->used += new_words;
        return new_ptr;
    }


    void* new_ptr = oc_arena_alloc_aligned(arena, size, alignment);
    if (!new_ptr) oc_oom();

    void* end_of_last_used = arena->current->data + arena->current->used;
    if ((uint8*)old_ptr + old_size == end_of_last_used && new_ptr == (uint8*)old_ptr + old_size) {
        // print("oc_arena_realloc: Reusing extended chunk :)\n");
        // If the new pointer is right after the old pointer, we can just extend it
        arena->current->used += new_words - old_words;
        return old_ptr;
    } else {
        // print("oc_arena_realloc: New chunk, copying data :(\n");
        memcpy(new_ptr, old_ptr, old_size);
        arena->current->used += new_words;
        return new_ptr;
    }
#endif
}

void* oc_arena_realloc(Oc_Arena* arena, void* old_ptr, uint64 old_size, uint64 size) {
#ifdef OC_ENABLE_ASAN_MALLOC
    return realloc(old_ptr, size);
#else
    oc_assert(arena != NULL);
    // oc_assert(old_ptr != NULL);
    if (old_ptr == NULL)  return oc_arena_alloc(arena, size);
    if (size <= old_size) return old_ptr;

    uword old_words = (old_size - 1) / sizeof(uword) + 1;
    uword new_words = (size - 1) / sizeof(uword) + 1;

    // print("oc_arena_realloc: old_ptr = {}, old_size = {}, size = {}\n", old_ptr, old_size, size);
    oc_assert(arena->current);
    if ((uint8*)old_ptr + old_size == (uint8*)(arena->current->data + arena->current->used)) {
        if (arena->current->used + (new_words - old_words) <= arena->current->size) {
            // print("oc_arena_realloc: Reusing memory :)\n");
            // reuse memory
            arena->current->used += new_words - old_words;
            return old_ptr;
        }
    } else if (arena->current->used + new_words <= arena->current->size) {
        // print("oc_arena_realloc: Reusing chunk, but copying old data :|\n");
        // copy old data
        void* new_ptr = arena->current->data + arena->current->used;
        memcpy(new_ptr, old_ptr, old_size);
        arena->current->used += new_words;
        return new_ptr;
    }


    void* new_ptr = oc_arena_alloc(arena, size);
    if (!new_ptr) oc_oom();

    void* end_of_last_used = arena->current->data + arena->current->used;
    if ((uint8*)old_ptr + old_size == end_of_last_used && new_ptr == (uint8*)old_ptr + old_size) {
        // print("oc_arena_realloc: Reusing extended chunk :)\n");
        // If the new pointer is right after the old pointer, we can just extend it
        arena->current->used += new_words - old_words;
        return old_ptr;
    } else {
        // print("oc_arena_realloc: New chunk, copying data :(\n");
        memcpy(new_ptr, old_ptr, old_size);
        arena->current->used += new_words;
        return new_ptr;
    }
#endif
}

void* oc_arena_dup(Oc_Arena* arena, void* data, uword size) {
    if (size == 0) return NULL;
    void* new_ptr = oc_arena_alloc(arena, size);
    memcpy(new_ptr, data, size);
    return new_ptr;
}

string oc_sprintf(Oc_Arena* arena, const char* fmt, ...) {
    Oc_String_Builder b;
    oc_sb_init(&b, arena);

    va_list args;
    va_start(args, fmt);
    _oc_vprintw(&b.writer, fmt, args);
    va_end(args);

    return oc_sb_to_string(&b);
}

void oc_sb_append_char(Oc_String_Builder* sb, char c) {
    oc_assert(sb->arena != NULL);
    if (sb->count + 1 > sb->capacity) {
        // Resize the buffer
        uword new_cap = sb->capacity ? sb->capacity * 2 : 16;
        char* new_ptr = oc_arena_realloc(sb->arena, sb->items, sb->capacity, new_cap);
        if (!new_ptr) oc_oom();
        sb->items = new_ptr;
        sb->capacity = new_cap;
    }
    sb->items[sb->count++] = c;
}

void oc_sb_append_string(Oc_String_Builder* sb, string s) {
    oc_assert(sb->arena != NULL);
    if (sb->count + s.len > sb->capacity) {
        // Resize the buffer
        uword new_cap = sb->capacity ? max(s.len, sb->capacity * 2) : 16;
        char* new_ptr = oc_arena_realloc(sb->arena, sb->items, sb->capacity, new_cap);
        if (!new_ptr) oc_oom();
        sb->items = new_ptr;
        sb->capacity = new_cap;
    }
    memcpy(sb->items + sb->count, s.ptr, s.len);
    sb->count += s.len;
}

void oc_sb_append_char_str(Oc_String_Builder* sb, const char* c) {
    oc_assert(sb->arena != NULL);
    uword len = strlen(c);
    oc_sb_append_string(sb, (string) { .ptr = (char*)c, .len = len });
}

void oc_sb_append_char_str_len(Oc_String_Builder* sb, const char* c, uword len) {
    oc_assert(sb->arena != NULL);
    oc_sb_append_string(sb, (string) { .ptr = (char*)c, .len = len });
}

string oc_sb_to_string(Oc_String_Builder* sb) {
    oc_sb_append_char(sb, 0);
    return (string) { .ptr = sb->items, .len = sb->count - 1 };
}


uword oc_sb_writer_write(void* writer, const void* data, uword data_size) {
    Oc_String_Builder* w = writer;
    oc_sb_append_string(w, (string) { .ptr = (char*)data, .len = data_size });
    return data_size;
}

void oc_sb_init(Oc_String_Builder* sb, Oc_Arena* arena) {
    memset(sb, 0, sizeof(Oc_String_Builder));
    sb->writer.write = oc_sb_writer_write;
    sb->arena = arena;
}

Oc_Writer stdout_writer = {
    .write = (WriteFunction)stdout_write,
};

Oc_Writer stderr_writer = {
    .write = (WriteFunction)stderr_write,
};

void oc_writer_format_and_write_int(Oc_Writer *writer, Oc_Format_Config cfg, uint64 ivalue) {
    static char int_buffer[128];
    uword int_buffer_offset = sizeof(int_buffer) - 1;
    char base_base = cfg.base_upper ? 'A' : 'a';

    // int size = 0;
    // if (cfg.size > 0) {
    //     if (ivalue == 0) {
    //         size = 1;
    //     } else {
    //         uint64 value = ivalue;
    //         while (value) {
    //             uint64 current = value % cfg.base;
    //             char c = current < 10 ? (current + '0') : (current - 10 + base_base);
    //             int_buffer[int_buffer_offset--] = c;
    //             value /= base;
    //         }
    //     }
    // }

    if (ivalue == 0) {
        if (cfg.size > 1) {
            for (int i = 0; i < cfg.size; i++)
                writer->write(writer, "0", 1);
        } else {
            writer->write(writer, "0", 1);
        }
    } else {
        uint64 value = ivalue;
        while (value) {
            uint64 current = value % cfg.base;
            char c = current < 10 ? (current + '0') : (current - 10 + base_base);
            int_buffer[int_buffer_offset--] = c;
            value /= cfg.base;
        }
        while (int_buffer_offset + cfg.size + 1 > sizeof(int_buffer)) {
            int_buffer[int_buffer_offset--] = '0';
        }
        writer->write(writer, int_buffer + int_buffer_offset + 1, sizeof(int_buffer) - int_buffer_offset - 1);
    }
}

void oc_writer_format_and_write_float(Oc_Writer *writer, Oc_Format_Config cfg, double fvalue) {
    (void)cfg;
    char int_buffer[128];
    uword int_buffer_offset = 0;

    int_buffer_offset = sizeof(int_buffer) - 1;
    uint64 ivalue = fvalue;
    double dvalue = fvalue - ivalue;

    if (ivalue == 0) {
        writer->write(writer, "0.0", 1);
    } else {
        while (ivalue) {
            uint64 current = ivalue % 10;
            char c = current < 10 ? (current + '0') : (current - 10 + 'a');
            int_buffer[int_buffer_offset--] = c;
            ivalue /= 10;
        }
        writer->write(writer, int_buffer + int_buffer_offset + 1, sizeof(int_buffer) - int_buffer_offset - 1);
    }

    int decimal_places = cfg.size ? cfg.size : 6;
    double e = 0.5;
    for (int i = 0; i < decimal_places; ++i) e /= 10.0;
    // dvalue += e; why was this here??

    uint64 multiply = 10;
    double shifted = dvalue;
    double acc = 0.0;
    int_buffer_offset = 0;
    int_buffer[int_buffer_offset++] = '.';
    int decimals = 0;
    if ((dvalue - acc) > e) {
        while ((dvalue - acc) > e) {
            double d = shifted * 10;
            uint64 i = (uint64)d;
            acc += (double)i / (double)multiply;
            int_buffer[int_buffer_offset++] = i + '0';
            shifted = d - i;
            multiply *= 10;
            decimals++;
        }
        if (cfg.size && decimals < cfg.size) {
            while (decimals < cfg.size) {
                int_buffer[int_buffer_offset++] = '0';
                decimals++;
            }
        } else if (cfg.size && decimals > cfg.size) {
            int_buffer_offset -= (decimals - cfg.size);
        }
    } else {
        int_buffer[int_buffer_offset++] = '0';
    }
    writer->write(writer, int_buffer, int_buffer_offset);
}

void _oc_printw(void *writer, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _oc_vprintw(writer, fmt, args);
    va_end(args);
}

void _oc_vprintw(void *writer, const char* fmt, va_list args) {
    Oc_Writer* w = writer;

    while (*fmt) {
        char c = *fmt;
        if (c == '{') {
            if (*(fmt + 1) == '{') {
                w->write(w, "{", 1);
                fmt += 2;
                continue;
            }
            Oc_Generic value = va_arg(args, Oc_Generic);

            int f = 0;
            static const int f_size = 1;
            static const int f_base = 2;
            int f_size_accum = 0;
            int f_base_accum = 10;
            int f_base_upper = 0;
            for (; *fmt && *fmt != '}'; fmt++) {
                if ((f & f_size) == 0 && *fmt >= '0' && *fmt <= '9') {
                    f_size_accum *= 10;
                    f_size_accum += (*fmt - '0');
                    f |= f_size;
                }
                if ((f & f_base) == 0 && *fmt == 'x') {
                    f_base_accum = 16;
                    f |= f_base;
                    f_base_upper = 0;
                }
                if ((f & f_base) == 0 && *fmt == 'X') {
                    f_base_accum = 16;
                    f |= f_base;
                    f_base_upper = 1;
                }
            }
            Oc_Format_Config cfg = {
                .base = f_base_accum,
                .base_upper = f_base_upper,
                .size = f_size_accum,
            };

            switch (value.kind) {
            case OC_TYPE_CHAR:
                w->write(w, (uint8*)value.data, 1);
                break;
            case OC_TYPE_UNSIGNED_CHAR: {
                unsigned char ivalue = *(unsigned char*)value.data;
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_SIGNED_CHAR: {
                int ivalue = *(signed char*)value.data;
                if (ivalue < 0) {
                    ivalue = -ivalue;
                    w->write(w, "-", 1);
                }
                oc_writer_format_and_write_int(w, cfg, (uint64)ivalue & 0xFF);
            } break;
            case OC_TYPE_UNSIGNED_SHORT: {
                unsigned short ivalue = *(unsigned short*)value.data;
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_SHORT: {
                int ivalue = *(signed short*)value.data;
                if (ivalue < 0) {
                    ivalue = -ivalue;
                    w->write(w, "-", 1);
                }
                oc_writer_format_and_write_int(w, cfg, (uint64)ivalue & 0xFFFF);
            } break;
            case OC_TYPE_UNSIGNED_INT: {
                unsigned int ivalue = *(unsigned int*)value.data;
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_INT: {
                int ivalue = *(signed int*)value.data;
                if (ivalue < 0) {
                    ivalue = -ivalue;
                    w->write(w, "-", 1);
                }
                oc_writer_format_and_write_int(w, cfg, ((uint64)ivalue) & 0xFFFFFFFFu);
            } break;
            case OC_TYPE_UNSIGNED_LONG: {
                uint64 ivalue = *(unsigned long*)value.data;
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_LONG: {
                sint64 ivalue = *(signed long*)value.data;
                if (ivalue < 0) {
                    ivalue = -ivalue;
                    w->write(w, "-", 1);
                }
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_UNSIGNED_LONG_LONG: {
                uint64 ivalue = *(unsigned long long*)value.data;
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_LONG_LONG: {
                sint64 ivalue = *(signed long long*)value.data;
                if (ivalue < 0) {
                    ivalue = -ivalue;
                    w->write(w, "-", 1);
                }
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_FLOAT: {
                float dvalue = *(float*)value.data;
                if (__builtin_signbit(dvalue)) {
                    dvalue = -dvalue;
                    w->write(w, "-", 1);
                }
                if (__builtin_isinf(dvalue)) {
                    w->write(w, "inf", 3);
                    break;
                } else if (__builtin_isnan(dvalue)) {
                    w->write(w, "nan", 3);
                    break;
                }
                oc_writer_format_and_write_float(w, cfg, dvalue);
            } break;
            case OC_TYPE_DOUBLE: {
                double dvalue = *(double*)value.data;
                if (__builtin_signbit(dvalue)) {
                    dvalue = -dvalue;
                    w->write(w, "-", 1);
                }
                if (__builtin_isinf(dvalue)) {
                    w->write(w, "inf", 3);
                    break;
                } else if (__builtin_isnan(dvalue)) {
                    w->write(w, "nan", 3);
                    break;
                }
                oc_writer_format_and_write_float(w, cfg, dvalue);
            } break;
            case OC_TYPE_LONG_DOUBLE: {
                long double dvalue = *(long double*)value.data;
                if (__builtin_signbit(dvalue)) {
                    dvalue = -dvalue;
                    w->write(w, "-", 1);
                }
                if (__builtin_isinf(dvalue)) {
                    w->write(w, "inf", 3);
                    break;
                } else if (__builtin_isnan(dvalue)) {
                    w->write(w, "nan", 3);
                    break;
                }
                oc_writer_format_and_write_float(w, cfg, dvalue);
            } break;
            case OC_TYPE_STRING: {
                string* str = (string*)value.data;
                w->write(w, str->ptr, str->len);
            } break;
            case OC_TYPE_CHAR_STRING: {
                const char* str = *(const char**)value.data;
                w->write(w, str, strlen(str));
            } break;
            case OC_TYPE_POINTER: {
                uint64 ivalue = *(uword*)value.data;
                cfg.base = 16;
                w->write(w, "0x", 2);
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_VOLATILE_POINTER: {
                uint64 ivalue = *(volatile uword*)value.data;
                cfg.base = 16;
                w->write(w, "0x", 2);
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_POINTER_VOLATILE: {
                uint64 ivalue = *(volatile uword*)value.data;
                cfg.base = 16;
                w->write(w, "0x", 2);
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_VOLATILE_POINTER_VOLATILE: {
                uint64 ivalue = *(volatile uword* volatile)value.data;
                cfg.base = 16;
                w->write(w, "0x", 2);
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            case OC_TYPE_POINTER_POINTER: {
                uint64 ivalue = *(uword*)value.data;
                cfg.base = 16;
                w->write(w, "0x", 2);
                oc_writer_format_and_write_int(w, cfg, ivalue);
            } break;
            }

        } else {
            w->write(w, (uint8*)fmt, 1);
        }
        fmt++;
    }
}

void oc_hex_dump(void* data, int count, int indent, int mark_mod) {
    unsigned char *bytes = (unsigned char*)data;
    for (int i = 0; i < indent; i++) {
        print("    ");
    }
    for (int i = 0; i < count; i++) {
        if (mark_mod > -1) {
            if ((i % 16) == 0 && (i > 0) && (i % mark_mod) == 0) {
                print("\n");
                for (int j = 0; j < indent; j++) {
                    print("    ");
                }
            }
        } else {
            if (i % 16 == 0 && i > 0) {
                print("\n");
                for (int j = 0; j < indent; j++) {
                    print("    ");
                }
            } else if (i % 8 == 0 && i > 0) {
                print(" ");
            }
        }

        if (bytes[i] > 0) {
            print("\x1b[44m{2x}\x1b[0m", bytes[i]);
        } else {
            print("{2x}", bytes[i]);
        }

        if (mark_mod > 0 && (i + 1) % mark_mod == 0 && (i + 1) > 0) {
            print("|");
        } else {
            print(" ");
        }
    }
    print("\n");
}


size_t stbds_hash_string(string* str, size_t len, size_t seed)
{
    size_t hash = seed;
    while (str->len-- > 0)
        hash = STBDS_ROTATE_LEFT(hash, 9) + (unsigned char) *str->ptr++;

    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ STBDS_ROTATE_RIGHT(hash,31);
    hash = hash * 21;
    hash ^= hash ^ STBDS_ROTATE_RIGHT(hash,11);
    hash += (hash << 6);
    hash ^= STBDS_ROTATE_RIGHT(hash,22);
    return hash+seed;
}

size_t stbds_hash_string_atom(string* str, size_t len, size_t seed) {
    (void)len;
    (void)seed;
    size_t key = (size_t)str->ptr;
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

#define STBDS_SIPHASH_C_ROUNDS 1
#define STBDS_SIPHASH_D_ROUNDS 1

size_t stbds_siphash_bytes(void *p, size_t len, size_t seed)
{
  unsigned char *d = (unsigned char *) p;
  size_t i,j;
  size_t v0,v1,v2,v3, data;

  // hash that works on 32- or 64-bit registers without knowing which we have
  // (computes different results on 32-bit and 64-bit platform)
  // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4 64-bit
  v0 = ((((size_t) 0x736f6d65 << 16) << 16) + 0x70736575) ^  seed;
  v1 = ((((size_t) 0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
  v2 = ((((size_t) 0x6c796765 << 16) << 16) + 0x6e657261) ^  seed;
  v3 = ((((size_t) 0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

  #define STBDS_SIPROUND() \
    do {                   \
      v0 += v1; v1 = STBDS_ROTATE_LEFT(v1, 13);  v1 ^= v0; v0 = STBDS_ROTATE_LEFT(v0,STBDS_SIZE_T_BITS/2); \
      v2 += v3; v3 = STBDS_ROTATE_LEFT(v3, 16);  v3 ^= v2;                                                 \
      v2 += v1; v1 = STBDS_ROTATE_LEFT(v1, 17);  v1 ^= v2; v2 = STBDS_ROTATE_LEFT(v2,STBDS_SIZE_T_BITS/2); \
      v0 += v3; v3 = STBDS_ROTATE_LEFT(v3, 21);  v3 ^= v0;                                                 \
    } while (0)

  for (i=0; i+sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
    data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    data |= (size_t) (d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // discarded if size_t == 4

    v3 ^= data;
    for (j=0; j < STBDS_SIPHASH_C_ROUNDS; ++j)
      STBDS_SIPROUND();
    v0 ^= data;
  }
  data = len << (STBDS_SIZE_T_BITS-8);
  switch (len - i) {
    case 7: data |= ((size_t) d[6] << 24) << 24; // fall through
    case 6: data |= ((size_t) d[5] << 20) << 20; // fall through
    case 5: data |= ((size_t) d[4] << 16) << 16; // fall through
    case 4: data |= (d[3] << 24); // fall through
    case 3: data |= (d[2] << 16); // fall through
    case 2: data |= (d[1] << 8); // fall through
    case 1: data |= d[0]; // fall through
    case 0: break;
  }
  v3 ^= data;
  for (j=0; j < STBDS_SIPHASH_C_ROUNDS; ++j)
    STBDS_SIPROUND();
  v0 ^= data;
  v2 ^= 0xff;
  for (j=0; j < STBDS_SIPHASH_D_ROUNDS; ++j)
    STBDS_SIPROUND();

  return v1^v2^v3; // slightly stronger since v0^v3 in above cancels out final round operation? I tweeted at the authors of SipHash about this but they didn't reply
}

#endif // OC_CORE_IMPLEMENTATION
