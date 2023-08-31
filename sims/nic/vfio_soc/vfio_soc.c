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

#include <simbricks/vfio/vfio.h>

int main(int argc, char *argv[])
{
    struct vfio_dev dev;
    void *mapped_addr;
    size_t reg_len;
    struct vfio_region_info reg;

    if (vfio_dev_open(&dev, argv[1], argv[2]) != 0) {
        fprintf(stderr, "open device failed\n");
        return -1;
    }

    if(vfio_region_map(&dev, 0, &mapped_addr, &reg_len, &reg)) {
        fprintf(stderr, "mapping registers failed\n");
        return -1;
    }

    if(vfio_busmaster_enable(&dev)) {
        fprintf(stderr, "busmaster enable failed\n");
        return -1;
    }

    int* int_mapped_addr = (int*)(mapped_addr);

    for (int i=0; i<2; i++) {
        usleep(10000);
    }

    return 0;
}