#define RCBIN_INTERNAL
#include <rcbin.h>

#include <stdio.h>


typedef struct rcbin_context rcbin_context;


#ifdef _WIN32

#include <windows.h>


struct rcbin_context {
    HANDLE h;
};


int ctx_init(rcbin_context* ctx, const char* path) {
    ctx->h = BeginUpdateResource(path, FALSE);
    return ctx->h != NULL;
}


int ctx_add(rcbin_context* ctx, const char* name, void* data, size_t data_sz) {
    size_t name_sz = strlen(name);
    size_t prefix_sz = strlen(RCBIN_PLATFORM_PREFIX);

    char* prefixed_name = malloc(prefix_sz+name_sz+1);
    memcpy(prefixed_name, RCBIN_PLATFORM_PREFIX, prefix_sz);
    memcpy(prefixed_name+prefix_sz, name, name_sz+1);

    BOOL status = UpdateResource(ctx->h, RT_RCDATA, prefixed_name,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), data,
                                 data_sz);

    free(prefixed_name);
    return status == TRUE;
}


int ctx_save(rcbin_context* ctx) {
    return EndUpdateResource(ctx->h, FALSE) == TRUE;
}


#else

#include <libelf.h>
#include <gelf.h>

#include <unistd.h>
#include <fcntl.h>
#include <math.h>


#define NEXT_POWER_OF_2(x) ((x) == 0 ? 2 : pow(2, ceil(log2(x))))


struct rcbin_context {
    int fd;
    Elf* e;
    size_t root_in_file_offs, data_in_file_offs;
    rcbin_root root;

    void* data;
    size_t data_sz, data_offs;
};


int ctx_init(rcbin_context* ctx, const char* path) {
    if (elf_version(EV_CURRENT) == EV_NONE) return 0;

    ctx->fd = open(path, O_RDWR);
    if (ctx->fd == -1) return 0;

    size_t file_sz = lseek(ctx->fd, 0, SEEK_END);
    if (file_sz == -1) return 0;
    ctx->data_in_file_offs = file_sz;

    lseek(ctx->fd, 0, SEEK_SET);

    ctx->e = elf_begin(ctx->fd, ELF_C_READ, NULL);
    if (ctx->e == NULL) return 0;
    if (elf_kind(ctx->e) != ELF_K_ELF) return 0;

    size_t shstrndx;
    elf_getshdrstrndx(ctx->e, &shstrndx);

    ctx->root.magic = 0;

    // Find the rcbin entries section.
    Elf_Scn* scn = NULL;
    while ((scn = elf_nextscn(ctx->e, scn))) {
        GElf_Shdr shdr;
        gelf_getshdr(scn, &shdr);

        if (strcmp(elf_strptr(ctx->e, shstrndx, shdr.sh_name), RCBIN_SECTION) == 0) {
            // Grab the root info.
            Elf_Data* root_data = elf_getdata(scn, NULL);

            rcbin_root* prev_r = root_data->d_buf;
            if (prev_r->magic == RCBIN_MAGIC) {
                // rcbin has already been run once; keep the file offsets identical.
                ctx->data_in_file_offs = prev_r->offs;
            }

            // Save the root header.
            ctx->root.magic = RCBIN_MAGIC;
            ctx->root.offs = ctx->data_in_file_offs;
            ctx->root_in_file_offs = shdr.sh_offset + root_data->d_off;
        }
    }

    ctx->data = malloc(2);
    ctx->data_sz = 2;
    ctx->data_offs = 0;

    return ctx->root.magic != 0;
}


int ctx_grow(rcbin_context* ctx, int expand) {
    ctx->data_offs += expand;

    // If growing is necessary, grow up till the next power of two.
    if (ctx->data_offs > ctx->data_sz) {
        ctx->data_sz = NEXT_POWER_OF_2(ctx->data_offs);
        ctx->data = realloc(ctx->data, ctx->data_sz);
        if (ctx->data == NULL) return 0;
    }

    return 1;
}


int ctx_add(rcbin_context* ctx, const char* name, void* data, size_t data_sz) {
    size_t current_offset = ctx->data_offs;
    size_t name_sz = strlen(name);
    if (!ctx_grow(ctx, sizeof(rcbin_entry_header) + name_sz + data_sz)) return 0;

    // Setup the entry header.
    struct rcbin_entry_header eh;

    eh.name_sz = name_sz;
    eh.data_sz = data_sz;

    // Write the header and data.
    memcpy(ctx->data+current_offset, &eh, sizeof(eh));
    current_offset += sizeof(eh);
    memcpy(ctx->data+current_offset, name, name_sz);
    current_offset += name_sz;
    memcpy(ctx->data+current_offset, data, data_sz);
    current_offset += data_sz;

    return 1;
}


int ctx_save(rcbin_context* ctx) {
    size_t current_offset = ctx->data_offs;
    if (!ctx_grow(ctx, sizeof(uint64_t))) return 0;

    // a name_sz of 0 signifies the end
    uint64_t tail = 0;
    memcpy(ctx->data+current_offset, &tail, sizeof(uint64_t));

    if (lseek(ctx->fd, ctx->root_in_file_offs, SEEK_SET) == -1) return 0;
    if (write(ctx->fd, &ctx->root, sizeof(rcbin_root)) == -1) return 0;

    if (lseek(ctx->fd, ctx->data_in_file_offs, SEEK_SET) == -1) return 0;
    if (write(ctx->fd, ctx->data, ctx->data_offs) == -1) return 0;

    fsync(ctx->fd);
    close(ctx->fd);

    free(ctx->data);
    return 1;
}


#endif


void usage(const char* self) {
    printf("usage: %s executable-file [resource-name resource-value...]\n", self);
    puts("resource-value should be =value for string value, @value for file contents.");
    printf("example: %s myapp myresource @myfile\n", self);
    printf("example: %s myapp myresource =mystring\n", self);
}


int read_file_data(const char* path, void** data_out, size_t* sz_out) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return 0;

    if (fseek(fp, 0, SEEK_END) != 0) return 0;
    *sz_out = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *data_out = malloc(*sz_out);
    if (fread(*data_out, 1, *sz_out, fp) == -1) return 0;

    fclose(fp);
    return 1;
}


#define ISEVEN(n) ((n) % 2 == 0)


int main(int argc, char** argv) {
    if (argc < 2 || !ISEVEN(argc)) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0 ||
        strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return 0;
    }

    rcbin_context ctx;
    if (!ctx_init(&ctx, argv[1])) {
        puts("failed to read file; are you sure it exists and is a valid ELF file?");
        return 1;
    }

    for (int i=2; i<argc; i+=2) {
        char* name = argv[i];
        char* descr = argv[i+1];
        size_t descr_sz = strlen(descr);

        void* data;
        size_t data_sz;

        switch (descr[0]) {
        case '=': data = descr+1; data_sz = descr_sz-1; break;
        case '@':
            if (!read_file_data(descr+1, &data, &data_sz)) {
                printf("error reading resource: %s\n", descr+1);
                return 1;
            }
            break;
        default:
            printf("invalid resource description: %s\n", descr);
            return 1;
        }

        if (!ctx_add(&ctx, name, data, data_sz)) {
            puts("failed to add resource");
            return 1;
        }

        if (descr[0] == '@') free(data);
    }

    if (!ctx_save(&ctx)) {
        puts("failed to save output file");
        return 1;
    }

    return 0;
}
