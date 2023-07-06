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

#include "lib/simbricks/smartnicif/smartnicif.h"

#include <poll.h>
#include <stdio.h>
#include <string.h>

int SimbricksSmartNicIfInit(struct SimbricksSmartNicIf *nicif, const char *shm_path,
                       struct SimbricksBaseIfParams *pcieParams0,
                       struct SimbricksBaseIfParams *pcieParams1,
                       struct SimbricksProtoPcieDevIntro *di) {
  struct SimbricksBaseIf *pcieif0 = &nicif->pcie0.base;
  struct SimbricksBaseIf *pcieif1 = &nicif->pcie1.base;

  // first allocate pool
  size_t shm_size = 0;
  if (pcieParams0) {
    shm_size += pcieParams0->in_num_entries * pcieParams0->in_entries_size;
    shm_size += pcieParams0->out_num_entries * pcieParams0->out_entries_size;
  }
  if (pcieParams1) {
    shm_size += pcieParams1->in_num_entries * pcieParams1->in_entries_size;
    shm_size += pcieParams1->out_num_entries * pcieParams1->out_entries_size;
  }
  if (SimbricksBaseIfSHMPoolCreate(&nicif->pool, shm_path, shm_size)) {
    perror("SimbricksNicIfInit: SimbricksBaseIfSHMPoolCreate failed");
    return -1;
  }

  struct SimBricksBaseIfEstablishData ests[2];
  struct SimbricksProtoPcieHostIntro pcie_h_intro0;
  struct SimbricksProtoPcieHostIntro pcie_h_intro1;
  unsigned n_bifs = 0;
  if (pcieParams0) {
    if (SimbricksBaseIfInit(pcieif0, pcieParams0)) {
      perror("SimbricksSmartNicIfInit: SimbricksBaseIfInit pcie failed");
      return -1;
    }

    if (SimbricksBaseIfListen(pcieif0, &nicif->pool)) {
      perror("SimbricksSmartNicIfInit: SimbricksBaseIfListen pcie failed");
      return -1;
    }

    memset(&pcie_h_intro0, 0, sizeof(pcie_h_intro0));
    ests[n_bifs].base_if = pcieif0;
    ests[n_bifs].tx_intro = di;
    ests[n_bifs].tx_intro_len = sizeof(*di);
    ests[n_bifs].rx_intro = &pcie_h_intro0;
    ests[n_bifs].rx_intro_len = sizeof(pcie_h_intro0);
    n_bifs++;
  }

  if (pcieParams1) {
    if (SimbricksBaseIfInit(pcieif1, pcieParams1)) {
      perror("SimbricksNicIfInit: SimbricksBaseIfInit pcie failed");
      return -1;
    }

    if (SimbricksBaseIfListen(pcieif1, &nicif->pool)) {
      perror("SimbricksNicIfInit: SimbricksBaseIfListen pcie failed");
      return -1;
    }

    memset(&pcie_h_intro1, 0, sizeof(pcie_h_intro1));
    ests[n_bifs].base_if = pcieif1;
    ests[n_bifs].tx_intro = di;
    ests[n_bifs].tx_intro_len = sizeof(*di);
    ests[n_bifs].rx_intro = &pcie_h_intro1;
    ests[n_bifs].rx_intro_len = sizeof(pcie_h_intro1);
    n_bifs++;
  }

  return SimBricksBaseIfEstablish(ests, n_bifs);
}  // NOLINT(whitespace/indent)

int SimbricksSmartNicIfCleanup(struct SimbricksSmartNicIf *nicif) {
  SimbricksBaseIfClose(&nicif->pcie0.base);
  SimbricksBaseIfClose(&nicif->pcie1.base);
  /* TODO: unlink? */
  return -1;
}
