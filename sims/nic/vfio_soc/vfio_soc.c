/*
 * Copyright 2019 University of Washington, Max Planck Institute for
 * Software Systems, and The University of Texas at Austin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define __USE_LARGEFILE64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#include <simbricks/vfio/vfio.h>
#include <simbricks/vfio_mem/vfio_mem.h>

static void *alloc_base;
static uint64_t alloc_phys_base = 1ULL * 1024 * 1024 * 1024;
static size_t alloc_size = 512 * 1024 * 1024;
static size_t alloc_off = 0;

static void alloc_init() {
    if (alloc_base)
        return;

    fprintf(stderr, "vfio_SoC: initializing allocator\n");
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "opening devmem failed\n");
        abort();
    }

    fprintf(stderr, "fd %d, alloc_size %ld, alloc_phys_base %ld\n", fd, alloc_size, alloc_phys_base);
    void *mem = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                fd, alloc_phys_base);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "mmap devmem failed\n");
        perror("Error \n");
        abort();
    }
    alloc_base = mem;

    fprintf(stderr, "vfio_SoC: allocator initialized\n");
}

void* memAlloc(size_t size) {
    fprintf(stderr, "vfio_SoC: memAlloc( %ld )\n", size);
    alloc_init();

    if (alloc_off + size > alloc_size) {
        fprintf(stderr, "size is larger than alloc_size\n");
    }
    void *addr = (void *) ((uint8_t *) alloc_base + alloc_off);
    alloc_off += size;
    fprintf(stderr, "vfio_SoC:    =  %p\n", addr);
    return addr;
}

uintptr_t memGetPhyAddr(void* buf) {
    return alloc_phys_base + ((uintptr_t) buf - (uintptr_t) alloc_base);
}

void memCopyFromHost(void* dst, const void* src, size_t size) {
    // For SoC-based FPGAs that used shared memory with the CPU, use memcopy()
    memcpy(dst, src, size);
}

void memCopyToHost(void* dst, const void* src, size_t size) {
    // For SoC-based FPGAs that used shared memory with the CPU, use memcopy()
    memcpy(dst, src, size);
}

void writeMappedReg(void* base_addr, uint32_t offset, uint32_t val) {
  *((volatile uint32_t *) (char *)(base_addr) + offset) = val;
}

uint32_t readMappedReg(void* base_addr, uint32_t offset) {
  return *((volatile uint32_t *) (char *)(base_addr) + offset);
}

int main(int argc, char *argv[])
{
    struct vfio_dev dev;
    void *reg_bar;
    size_t reg_len;
    struct vfio_region_info reg;

    if (vfio_dev_open(&dev, argv[1], argv[2]) != 0) {
        fprintf(stderr, "open device failed\n");
        return -1;
    }

    if(vfio_region_map(&dev, 0, &reg_bar, &reg_len, &reg)) {
        fprintf(stderr, "mapping registers failed\n");
        return -1;
    }

    memAlloc(alloc_size);

    if(vfio_busmaster_enable(&dev)) {
        fprintf(stderr, "busmaster enable failed\n");
        return -1;
    }

    int* int_mapped_addr = (int*)(reg_bar);

    for (int i=0; i<2; i++) {
        usleep(10000);
    }

    return 0;
}