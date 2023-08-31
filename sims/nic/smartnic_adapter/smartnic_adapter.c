/*
 * Copyright 2021 Max Planck Institute for Software Systems, and
 * National University of Singapore
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

#include <assert.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <pcap/pcap.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <simbricks/pcie/if.h>
#include <simbricks/pcieadapter/pcieadapter.h>
#include <simbricks/smartnicif/smartnicif.h>

typedef struct {
    uint64_t id;
    uint64_t addr;
    uint8_t len;
} ReadEvent;

typedef struct {
    uint64_t id;
    uint64_t value;
} ReadCompEvent;

typedef struct {
    uint64_t id;
    uint64_t addr;
    uint8_t len;
    uint64_t value;
} WriteEvent;

typedef struct {
    uint64_t id;
} WriteCompEvent;

static struct SimbricksSmartNicIf nicif;

int main(int argc, char *argv[]) {

    fprintf(stderr, "main is started, argc %d\n", argc);
    for (int i = 0; i < argc; i++){
        fprintf(stderr, "argv[%d]: %s\n", i, argv[i]);
    }

    if (argc < 4) {
        printf("Usage: %s <path_sock> <path_sock> <path_shm> <sync (y/n)>\n", argv[0]);
        return 1;
    }

    ReadEvent *read_ev0 = malloc(sizeof(ReadEvent));
    WriteEvent *write_ev0 = malloc(sizeof(WriteEvent));

    ReadEvent *read_ev1 = malloc(sizeof(ReadEvent));
    ReadCompEvent *read_compl_ev1 = malloc(sizeof(ReadCompEvent));
    WriteEvent *write_ev1 = malloc(sizeof(WriteEvent));
    WriteCompEvent *write_compl_ev1 = malloc(sizeof(WriteCompEvent));

    fprintf(stderr, "Initializing SimBricks connection...\n");
    simbricks_adapter_init(&nicif, argv[1], argv[2], argv[3], 0);

    fprintf(stderr, "Entering simulation loop...\n");

    uint64_t cur_time0 = 0;
    uint64_t cur_time1 = 0;

    while (1) {
        // PCIe0
        void *ev = simbricks_adapter_getevent(&nicif.pcie0, cur_time0);

        if (ev != NULL) {
            if (simbricks_adapter_getread(&nicif.pcie0, ev, &read_ev0->id, &read_ev0->addr, &read_ev0->len)) {
                fprintf(stderr, "adapter getread, forward from pcie0 to pcie1\n");
                simbricks_adapter_forward_read(&nicif.pcie1, read_ev0->id, &read_ev0->addr, read_ev0->len, cur_time1);
            } 
            else if (simbricks_adapter_getwrite(&nicif.pcie0, ev, &write_ev0->id, &write_ev0->addr, &write_ev0->len, &write_ev0->value)) {
                simbricks_adapter_forward_write(&nicif.pcie1, write_ev0->id, cur_time1);
            }
        }

        uint64_t next_time = simbricks_adapter_nextts(&nicif.pcie0);
        cur_time0 = next_time;
        simbricks_adapter_eventdone(&nicif.pcie0, ev);

        // PCIe1
        void *ev1 = simbricks_adapter_getevent(&nicif.pcie1, cur_time1);

        if (ev1 != NULL) {
            if (simbricks_adapter_getreadcomp(&nicif.pcie1, ev1, &read_compl_ev1->id, &read_compl_ev1->value)) {
                simbricks_adapter_complr(&nicif.pcie0, read_compl_ev1->id, read_compl_ev1->value, sizeof(read_compl_ev1->value), cur_time0);
            }
            else if (simbricks_adapter_getwritecomp(&nicif.pcie1, ev1, &write_compl_ev1->id)) {
                simbricks_adapter_complw(&nicif.pcie0, write_ev0->id, cur_time0);
            }
            else if (simbricks_adapter_getread(&nicif.pcie1, ev1, &read_ev1->id, &read_ev1->addr, &read_ev1->len)) {
                simbricks_adapter_complr(&nicif.pcie1, read_ev1->id, 42, read_ev1->len, cur_time1);
            } 
            else if (simbricks_adapter_getwrite(&nicif.pcie1, ev1, &write_ev1->id, &write_ev1->addr, &write_ev1->len, &write_ev1->value)) {
                simbricks_adapter_complw(&nicif.pcie1, write_ev1->id, cur_time1);
            }
        }

        uint64_t next_time1 = simbricks_adapter_nextts(&nicif.pcie1);
        cur_time1 = next_time1;
        simbricks_adapter_eventdone(&nicif.pcie1, ev1);
    }

    return 0;
}
