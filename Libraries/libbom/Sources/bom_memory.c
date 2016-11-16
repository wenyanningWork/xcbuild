/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

/* Enable access to ftruncate(). */
#define _POSIX_C_SOURCE 200809L

#include <bom/bom.h>

#include <string.h>
#include <assert.h>

#if _WIN32
// TODO mmap
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static void
_bom_context_memory_realloc(struct bom_context_memory *memory, size_t size)
{
    memory->size = size;
    memory->data = realloc(memory->data, memory->size);
}

static void
_bom_context_memory_free(struct bom_context_memory *memory)
{
    free(memory->data);
}

struct bom_context_memory
bom_context_memory(void const *data, size_t size)
{
    void *new = malloc(size);

    if (data != NULL) {
        memcpy(new, data, size);
    } else {
        memset(new, 0, size);
    }

    return (struct bom_context_memory){
        .data = new,
        .size = size,
        .resize = _bom_context_memory_realloc,
        .free = _bom_context_memory_free,
        .ctx = NULL,
    };
}

struct _bom_context_memory_mmap_context {
#if _WIN32
    // TODO: mmap
#else
    int fd;
#endif
    bool writeable;
};

static void
_bom_context_memory_mremap(struct bom_context_memory *memory, size_t size)
{
    struct _bom_context_memory_mmap_context *context = memory->ctx;

#if _WIN32
    // TODO: mmap
    (void)context;
#else
    munmap(memory->data, memory->size);
    if (size > memory->size) {
        int ret = ftruncate(context->fd, size);
        assert(ret == 0);
        (void)ret;
    }

    memory->size = size;

    int prot = context->writeable ? PROT_READ | PROT_WRITE : PROT_READ;
    memory->data = mmap(NULL, size, prot, MAP_SHARED, context->fd, 0);
    assert((intptr_t)memory->data != -1);
#endif
}

static void
_bom_context_memory_munmap(struct bom_context_memory *memory)
{
    struct _bom_context_memory_mmap_context *context = memory->ctx;

#if _WIN32
    // TODO: mmap
#else
    munmap(memory->data, memory->size);
    close(context->fd);
#endif

    free(context);
}

struct bom_context_memory
bom_context_memory_file(const char *fn, bool writeable, size_t minimum_size)
{
#if _WIN32
    // TODO: mmap
    (void)_bom_context_memory_mremap;
    (void)_bom_context_memory_munmap;
    return (struct bom_context_memory) {
        .data = NULL,
        .size = 0,
        .resize = NULL,
        .free = NULL,
        .ctx = NULL,
    };
#else
    int fd = open(fn, (writeable ? (O_RDWR | O_CREAT) : O_RDONLY), 0755);

    if (fd < 0) {
        return (struct bom_context_memory) {
            .data = NULL,
            .size = 0,
            .resize = NULL,
            .free = NULL,
            .ctx = NULL,
        };
    }

    struct stat st;
    fstat(fd, &st);

    // Expand file to minimum_size
    if (st.st_size < (off_t)minimum_size) {
        int result = ftruncate(fd, minimum_size);
        if (result != 0) {
            close(fd);
            return (struct bom_context_memory) {
                .data = NULL,
                .size = 0,
                .resize = NULL,
                .free = NULL,
                .ctx = NULL,
            };
        }
    }

    struct _bom_context_memory_mmap_context *context = malloc(sizeof(*context));
    context->fd = fd;
    context->writeable = writeable;

    int prot = context->writeable ? PROT_READ | PROT_WRITE : PROT_READ;
    size_t size = st.st_size < (off_t)minimum_size ? minimum_size : st.st_size;
    void *data = mmap(NULL, size, prot, (writeable ? MAP_SHARED : MAP_PRIVATE), context->fd, 0);

    return (struct bom_context_memory) {
        .data = data,
        .size = size,
        .resize = _bom_context_memory_mremap,
        .free = _bom_context_memory_munmap,
        .ctx = context,
    };
#endif
}

