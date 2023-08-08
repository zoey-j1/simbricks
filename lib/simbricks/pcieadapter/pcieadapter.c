#include "pcieadapter.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void abort_iffail(bool success, const char *msg)
{
  if (success)
    return;
  fprintf(stderr, "failed: %s\n", msg);
  abort();
}


void simbricks_adapter_init(struct SimbricksSmartAdapter *nicif,
                            const char *socket_path0,
                            const char *socket_path1,
                            const char *shm_path,
                            bool enable_sync)
{

  /** fill in essential PCI device info */
  struct SimbricksProtoPcieDevIntro di0;
  memset(&di0, 0, sizeof(di0));
  di0.bars[0].len = 0x10000;
  di0.bars[0].flags = SIMBRICKS_PROTO_PCIE_BAR_64;
  di0.pci_vendor_id = 0x9876;
  di0.pci_device_id = 0x1234;

  /** fill in essential PCI device info */
  struct SimbricksProtoPcieDevIntro di1;
  memset(&di1, 0, sizeof(di1));
  di1.bars[0].len = 0x10000;
  di1.bars[0].flags = SIMBRICKS_PROTO_PCIE_BAR_64;
  di1.pci_vendor_id = 0x9876;
  di1.pci_device_id = 0x4321;

  struct SimbricksBaseIfParams pcieParams0;
  struct SimbricksBaseIfParams pcieParams1;
  SimbricksPcieIfDefaultParams(&pcieParams0);
  SimbricksPcieIfDefaultParams(&pcieParams1);

  pcieParams0.blocking_conn = true;
  pcieParams0.sock_path = socket_path0;
  pcieParams0.sync_mode = (enable_sync ?
      kSimbricksBaseIfSyncRequired : kSimbricksBaseIfSyncDisabled);

  pcieParams1.blocking_conn = true;
  pcieParams1.sock_path = socket_path1;
  pcieParams1.sync_mode = (enable_sync ?
      kSimbricksBaseIfSyncRequired : kSimbricksBaseIfSyncDisabled);

  abort_iffail(!SimbricksBaseIfSHMPoolCreate(&nicif->pool, shm_path, SimbricksBaseIfSHMSize(&pcieParams0) + SimbricksBaseIfSHMSize(&pcieParams1)), "pool create");

  struct SimbricksBaseIf *bif0 = &nicif->pcie0.base;
  abort_iffail(!SimbricksBaseIfInit(bif0, &pcieParams0), "base init");
  abort_iffail(!SimbricksBaseIfListen(bif0, &nicif->pool), "listen");
  abort_iffail(!SimbricksBaseIfIntroSend(bif0, &di0, sizeof(di0)), "intro send");
  struct SimbricksProtoPcieHostIntro hi0;
  size_t hl0 = sizeof(hi0);
  abort_iffail(!SimbricksBaseIfIntroRecv(bif0, &hi0, &hl0), "intro recv");

  struct SimbricksBaseIf *bif1 = &nicif->pcie1.base;
  abort_iffail(!SimbricksBaseIfInit(bif1, &pcieParams1), "base init");
  abort_iffail(!SimbricksBaseIfListen(bif1, &nicif->pool), "listen");
  abort_iffail(!SimbricksBaseIfIntroSend(bif1, &di1, sizeof(di1)), "intro send");
  struct SimbricksProtoPcieHostIntro hi1;
  size_t hl1 = sizeof(hi1);
  abort_iffail(!SimbricksBaseIfIntroRecv(bif1, &hi1, &hl1), "intro recv");
}

void *simbricks_adapter_getevent(struct SimbricksPcieIf *pcie, uint64_t ts)
{
  while (SimbricksPcieIfD2HOutSync(pcie, ts)) {
    fprintf(stderr, "warning: sync failed\n");
  }

  return (void *) SimbricksPcieIfH2DInPoll(pcie, ts);
}

bool simbricks_adapter_getread(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr,
                               uint8_t *len)
{
  volatile union SimbricksProtoPcieH2D *msg =
    (volatile union SimbricksProtoPcieH2D *) ev;

  if (!msg || SimbricksPcieIfH2DInType(pcie, msg) !=
      SIMBRICKS_PROTO_PCIE_H2D_MSG_READ) {
      return false;
  }

  *id = msg->read.req_id;
  *addr = msg->read.offset;
  *len = msg->read.len;

  fprintf(stderr, "simbricks_adapter_getread return TRUE, address is %ld\n", *addr);

  return true;
}

bool simbricks_adapter_getwrite(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr,
                                uint8_t *len, uint64_t *val)
{
  volatile union SimbricksProtoPcieH2D *msg =
    (volatile union SimbricksProtoPcieH2D *) ev;

  if (!msg || SimbricksPcieIfH2DInType(pcie, msg) !=
      SIMBRICKS_PROTO_PCIE_H2D_MSG_WRITE) {
      return false;
  }

  *id = msg->write.req_id;
  *addr = msg->write.offset;
  *len = msg->write.len;

  *val = 0;
  memcpy(val, (void *) msg->write.data, *len);
  char charval[*len]; 
  memcpy(charval, (void *) msg->write.data, *len);

  fprintf(stderr, "simbricks_adapter_getwrite return TRUE val is %ld, address is %ld\n", *val, *addr);

  // Printing as individual characters:
  fprintf(stderr, "val as individual characters: ");
  for (int i = 0; i < *len; i++) {
    fprintf(stderr, "%c", charval[i]);
  }
  fprintf(stderr, "\n");

  return true;
}

void simbricks_adapter_eventdone(struct SimbricksPcieIf *pcie, void *ev)
{
  volatile union SimbricksProtoPcieH2D *msg =
    (volatile union SimbricksProtoPcieH2D *) ev;
  if (!msg) {
    return;
  }

  SimbricksPcieIfH2DInDone(pcie, msg);
}

uint64_t simbricks_adapter_nextts(struct SimbricksPcieIf *pcie)
{
  if (!SimbricksBaseIfSyncEnabled(&pcie->base))
    return 0;

  /* next thing due might be an incoming event or a sync we have to send */
  uint64_t rx_next = SimbricksPcieIfH2DInTimestamp(pcie);
  uint64_t sync_next = SimbricksPcieIfD2HOutNextSync(pcie);
  return (rx_next <= sync_next ? rx_next : sync_next);
}

static inline volatile union SimbricksProtoPcieD2H *alloc_out(struct SimbricksPcieIf *pcie, uint64_t ts)
{
  volatile union SimbricksProtoPcieD2H *msg;

  bool first = true;
  while ((msg = SimbricksPcieIfD2HOutAlloc(pcie, ts)) == NULL) {
    if (first) {
      first = false;
    }
  }

  if (!first)
    fprintf(stderr, "SimbricksPcieIfD2HOutAlloc succeeded\n");

  return msg;
}

void simbricks_adapter_complr(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t val, uint8_t len, uint64_t ts)
{
  volatile union SimbricksProtoPcieD2H *msg = alloc_out(pcie, ts);
  memcpy((void *) msg->readcomp.data, &val, len);
  msg->readcomp.req_id = id;
  SimbricksPcieIfD2HOutSend(pcie, msg,
    SIMBRICKS_PROTO_PCIE_D2H_MSG_READCOMP);
}

void simbricks_adapter_complw(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t ts)
{
  volatile union SimbricksProtoPcieD2H *msg = alloc_out(pcie, ts);
  msg->writecomp.req_id = id;
  SimbricksPcieIfD2HOutSend(pcie, msg,
    SIMBRICKS_PROTO_PCIE_D2H_MSG_WRITECOMP);

}

void simbricks_adapter_forward(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t val, uint8_t len, uint64_t ts)
{
  volatile union SimbricksProtoPcieD2H *msg = alloc_out(pcie, ts);
  memcpy((void *) msg->write.data, &val, len);
  msg->writecomp.req_id = id;
  SimbricksPcieIfD2HOutSend(pcie, msg,
    SIMBRICKS_PROTO_PCIE_D2H_MSG_WRITE);
}