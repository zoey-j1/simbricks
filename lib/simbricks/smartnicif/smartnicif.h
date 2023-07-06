/*
 * Copyright 2022 Max Planck Institute for Software Systems, and
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

#ifndef SIMBRICKS_SMARTNICIF_H_
#define SIMBRICKS_SMARTNICIF_H_

#include <stddef.h>
#include <stdint.h>

#include <simbricks/pcie/if.h>

struct SimbricksSmartNicIf {
  struct SimbricksBaseIfSHMPool pool;
  struct SimbricksPcieIf pcie0;
  struct SimbricksPcieIf pcie1;
};

int SimbricksSmartNicIfInit(struct SimbricksSmartNicIf *nicif, const char *shmPath,
                       struct SimbricksBaseIfParams *pcieParams0,
                       struct SimbricksBaseIfParams *pcieParams1,
                       struct SimbricksProtoPcieDevIntro *di);

int SimbricksSmartNicIfCleanup(struct SimbricksSmartNicIf *nicif);

static inline int SimbricksSmartNicIfSync(struct SimbricksSmartNicIf *nicif,
                                     uint64_t cur_ts) {
  return ((SimbricksPcieIfD2HOutSync(&nicif->pcie0, cur_ts) == 0 &&
           SimbricksPcieIfD2HOutSync(&nicif->pcie1, cur_ts) == 0)
              ? 0
              : -1);
}

static inline uint64_t SimbricksSmartNicIfNextTimestamp(
    struct SimbricksSmartNicIf *nicif) {
  uint64_t pcie0_in = SimbricksPcieIfH2DInTimestamp(&nicif->pcie0);
  uint64_t pcie0_out = SimbricksPcieIfD2HOutNextSync(&nicif->pcie0);
  uint64_t pcie0 = (pcie0_in <= pcie0_out ? pcie0_in : pcie0_out);

  uint64_t pcie1_in = SimbricksPcieIfH2DInTimestamp(&nicif->pcie1);
  uint64_t pcie1_out = SimbricksPcieIfD2HOutNextSync(&nicif->pcie1);
  uint64_t pcie1 = (pcie1_in <= pcie1_out ? pcie1_in : pcie1_out);

  return (pcie0 < pcie1 ? pcie0 : pcie1);
}

#endif  // SIMBRICKS_NICIF_NICIF_H_
