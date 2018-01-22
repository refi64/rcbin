#ifndef RCBIN_H
#define RCBIN_H


#include <stdlib.h>
#include <string.h>


#ifdef RCBIN_NO_THREAD_LOCAL
#define RCBIN_THREAD_LOCAL
#elif _MSC_VER
#define RCBIN_THREAD_LOCAL __declspec(thread)
#elif __GNUC__
#define RCBIN_THREAD_LOCAL __thread
#else
#error Unsupported platform/compiler for thread local. \
       Try passing -DRCBIN_NO_THREAD_LOCAL.
#endif

#ifdef _WIN32

#define RCBIN_PLATFORM_PREFIX "rcbin_res_"

#else // !_WIN32

#include <inttypes.h>

#define RCBIN_PLATFORM_PREFIX ""
#define RCBIN_MAGIC 0x3CB1


#ifdef RCBIN_INTERNAL

#define RCBIN_SECTION "__rcbin_internal_root_section"

typedef struct rcbin_root rcbin_root;
typedef struct rcbin_entry_header rcbin_entry_header;

struct rcbin_root {
    uint32_t magic;
    uint64_t offs;
};

struct rcbin_entry_header {
    uint64_t name_sz, data_sz;
};



#endif // RCBIN_INTERNAL

#endif // _WIN32


typedef struct rcbin_entry rcbin_entry;

struct rcbin_entry {
    size_t sz;
    const void* data;
};


int rcbin_init();
void* rcbin_alloc(size_t sz);
void rcbin_lookup(const char* name, const void** data_out, size_t* sz_out);


#define RCBIN_IMPORT(name) \
    rcbin_entry name() { \
        static RCBIN_THREAD_LOCAL size_t rcbin_sz_cache=-1; \
        static RCBIN_THREAD_LOCAL const void* rcbin_data_cache=NULL; \
        if (rcbin_data_cache == NULL) \
            rcbin_lookup(RCBIN_PLATFORM_PREFIX #name, &rcbin_data_cache, \
                         &rcbin_sz_cache); \
        rcbin_entry e; e.sz = rcbin_sz_cache; e.data = rcbin_data_cache; \
        return e; }

#endif // RCBIN_H
