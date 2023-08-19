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
    uint64_t addr;
    uint8_t len;
    uint64_t value;
} WriteEvent;

static struct SimbricksSmartNicIf nicif;

int main(int argc, char *argv[]) {

    printf("main is started, argc %d\n", argc);
    for (int i = 0; i < argc; i++){
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    // if (argc < 4) {
    //     printf("Usage: %s <path_sock> <path_sock> <path_shm> <sync (y/n)>\n", argv[0]);
    //     return 1;
    // }

    ReadEvent *read_ev = malloc(sizeof(ReadEvent));
    WriteEvent *write_ev = malloc(sizeof(WriteEvent));

    printf("Initializing SimBricks connection...\n");
    simbricks_adapter_init(&nicif, argv[1], argv[2], argv[3], 0);

    printf("Entering simulation loop...\n");
    uint64_t cur_time = 0;
    while (1) {
        void *ev = simbricks_adapter_getevent(&nicif.pcie0, cur_time);
        simbricks_adapter_eventdone(&nicif.pcie0, ev);

        if (ev != NULL) {
            if (simbricks_adapter_getread(&nicif.pcie0, ev, &read_ev->id, &read_ev->addr, &read_ev->len)) {
                simbricks_adapter_complr(&nicif.pcie0, read_ev->id, 42, read_ev->len, cur_time);
            } 
            else if (simbricks_adapter_getwrite(&nicif.pcie0, ev, &write_ev->id, &write_ev->addr, &write_ev->len, &write_ev->value)) {
                simbricks_adapter_complw(&nicif.pcie0, write_ev->id, cur_time);
                simbricks_adapter_forward(&nicif.pcie0, write_ev->id, 42, write_ev->len, cur_time);
            }
        }

        uint64_t next_time = simbricks_adapter_nextts(&nicif.pcie0);
        cur_time = next_time;

        void *ev1 = simbricks_adapter_getevent(&nicif.pcie1, cur_time);
        simbricks_adapter_eventdone(&nicif.pcie1, ev1);

        if (ev1 != NULL) {
            if (simbricks_adapter_getread(&nicif.pcie1, ev1, &read_ev->id, &read_ev->addr, &read_ev->len)) {
                simbricks_adapter_complr(&nicif.pcie1, read_ev->id, 42, read_ev->len, cur_time);
            } 
            else if (simbricks_adapter_getwrite(&nicif.pcie1, ev1, &write_ev->id, &write_ev->addr, &write_ev->len, &write_ev->value)) {
                simbricks_adapter_complw(&nicif.pcie1, write_ev->id, cur_time);
                simbricks_adapter_forward(&nicif.pcie1, write_ev->id, 42, write_ev->len, cur_time);
            }
        }

        uint64_t next_time1 = simbricks_adapter_nextts(&nicif.pcie1);
        cur_time = next_time1;
    }

    return 0;
}
