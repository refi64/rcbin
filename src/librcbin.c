#define RCBIN_INTERNAL
#include <rcbin.h>


#ifdef _WIN32

#include <windows.h>


int rcbin_init() { return 1; }


void rcbin_lookup(const char* name, const void** data_out, size_t* sz_out) {
    HRSRC resinfo = FindResource(NULL, name, RT_RCDATA);
    if (resinfo == NULL) return;

    size_t sz = SizeofResource(NULL, resinfo);
    if (sz == 0) return;

    HGLOBAL resdata = LoadResource(NULL, resinfo);
    if (resdata == NULL) return;

    LPVOID res = LockResource(resdata);
    if (res == NULL) return;

    *data_out = res;
    *sz_out = sz;
}


#else

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


// This table will be modified by rcbin to contain pointers to the different resources.
char __rcbin_internal_entries[RCBIN_HEADER_SIZE]
                __attribute__((__section__("__rcbin_internal_entries"))) = {0};
// Contains the rcbin binary resource data of the executable file.
static void* current_exe_data = NULL;

#define ROOT_HEADER ((rcbin_root_header*)__rcbin_internal_entries)


static void read_executable_path(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) return;

    if (lseek(fd, ROOT_HEADER->offs, SEEK_SET) == -1) return;

    size_t total_sz = lseek(fd, 0, SEEK_END);
    if (total_sz == -1) return;

    if (total_sz < ROOT_HEADER->offs) return;
    if (lseek(fd, ROOT_HEADER->offs, SEEK_SET) == -1) return;

    size_t sz = total_sz - ROOT_HEADER->offs;

    void* buf = malloc(sz);
    if (read(fd, buf, sz) == -1) {
        free(buf);
        return;
    }

    current_exe_data = buf;
}


// Platform-specific executable finding...

#ifdef __APPLE__

#include <mach-o/dyld.h>

static void read_executable() {
    char path[PATH_MAX];
    size_t sz;
    if (_NSGetExecutablePath(path, &sz)) return;
    read_executable_path(path);
}

#elif __sun

static void read_executable() {
    char* path = getexecname();
    if (path) read_executable_path(path);
}

#elif __FreeBSD__

#include <sys/sysctl.h>

static void read_executable() {
    int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    char path[PATH_MAX];
    size_t sz;
    if (sysctl(name, 4, path, &sz, NULL, 0) == 0) {
        read_executable_path(path);
    }
}

#elif __NetBSD__

static void read_executable() {
    read_executable_path("/proc/curproc/exe");
}

#elif __DragonFly__

static void read_executable() {
    read_executable_path("/proc/curproc/file");
}

#elif __linux__

static void read_executable() {
    read_executable_path("/proc/self/exe");
}

#else

#error Unsupported platform.

#endif


int rcbin_init() {
    if (ROOT_HEADER->magic != RCBIN_MAGIC) return 0;

    read_executable();
    return current_exe_data != NULL;
}


void rcbin_lookup(const char* name, const void** data_out, size_t* sz_out) {
    const void* pos = __rcbin_internal_entries+sizeof(rcbin_root_header);
    size_t name_sz = strlen(name);

    // Scan through the internal entries.
    while (pos < (void*)(__rcbin_internal_entries+RCBIN_HEADER_SIZE)) {
        // Grab the header and check the name. If it's the one we're looking for, then
        // grab the data from the exe and return.
        rcbin_entry_header* eh = (rcbin_entry_header*)pos;
        const char* e_name_begin = pos+sizeof(rcbin_entry_header);
        if (eh->name_sz == name_sz && memcmp(e_name_begin, name, name_sz) == 0) {
            *data_out = current_exe_data+eh->offs;
            *sz_out = eh->data_sz;
            return;
        }
        pos += sizeof(rcbin_entry_header)+eh->name_sz;
    }
}

#endif
