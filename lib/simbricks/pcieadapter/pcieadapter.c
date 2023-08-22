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


void simbricks_adapter_init(struct SimbricksSmartNicIf *nicif,
                            const char *socket_path0,
                            const char *socket_path1,
                            const char *shm_path,
                            bool enable_sync)
{

  struct SimbricksBaseIfParams pcieParams0;
  struct SimbricksBaseIfParams pcieParams1;

  SimbricksPcieIfDefaultParams(&pcieParams0);
  SimbricksPcieIfDefaultParams(&pcieParams1);

  /** fill in essential PCI device info */
  struct SimbricksProtoPcieDevIntro di0;
  memset(&di0, 0, sizeof(di0));
  di0.bars[0].len = 0x10000;
  di0.bars[0].flags = SIMBRICKS_PROTO_PCIE_BAR_64;
  di0.pci_vendor_id = 0x9876;
  di0.pci_device_id = 0x1234;

  struct SimbricksProtoPcieDevIntro di1;
  memset(&di1, 0, sizeof(di1));
  di1.bars[0].len = 0x10000;
  di1.bars[0].flags = SIMBRICKS_PROTO_PCIE_BAR_64;
  di1.pci_vendor_id = 0x9876;
  di1.pci_device_id = 0x4321;

  pcieParams0.sock_path = socket_path0;
  pcieParams1.sock_path = socket_path1;
  
  // fprintf(stderr, "pcieParams0.sock_path is %s, pcieParams1.sock_path is %s\n", pcieParams0.sock_path, pcieParams1.sock_path); 

  if (SimbricksSmartNicIfInit(nicif, shm_path, &pcieParams0, &pcieParams1, &di0, &di1) != 0) {
    fprintf(stderr, "SimbricksSmartNicIfInit failed\n");
  }
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

void simbricks_adapter_forward_read(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t val, uint8_t len, uint64_t ts)
{
  fprintf(stderr, "simbricks_adapter_forward_read start\n");
  volatile union SimbricksProtoPcieD2H *msg = alloc_out(pcie, ts);
  memcpy((void *) msg->write.data, &val, len);
  msg->writecomp.req_id = id;
  msg->read.offset = TX_QUEUE_DESC_RING_SIZE_REG;
  msg->read.len = sizeof(uint64_t);

  fprintf(stderr, "forward offset %ld, len %d\n", msg->read.offset, msg->read.len);
  SimbricksPcieIfD2HOutSend(pcie, msg,
    SIMBRICKS_PROTO_PCIE_D2H_MSG_READ);
}

bool simbricks_adapter_getreadcomp(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr,
                               uint8_t *len)
{
  fprintf(stderr, "simbricks_adapter_getreadcomp start\n");
  volatile union SimbricksProtoPcieH2D *msg =
    (volatile union SimbricksProtoPcieH2D *) ev;

  fprintf(stderr, "!msg %d\n", !msg);
  fprintf(stderr, "!msg || SimbricksPcieIfH2DInType(pcie, msg) %d\n", !msg || SimbricksPcieIfH2DInType(pcie, msg)!= SIMBRICKS_PROTO_PCIE_H2D_MSG_READCOMP);
  if (msg){
    fprintf(stderr, "SimbricksPcieIfH2DInType(pcie, msg) %d\n", SimbricksPcieIfH2DInType(pcie, msg));
  }

  if (!msg || SimbricksPcieIfH2DInType(pcie, msg) !=
      SIMBRICKS_PROTO_PCIE_H2D_MSG_READCOMP) {
      return false;
  }

  *id = msg->read.req_id;
  *addr = msg->read.offset;
  *len = msg->read.len;

  fprintf(stderr, "simbricks_adapter_getreadcomp return TRUE, address is %ld\n", *addr);

  return true;
}

void simbricks_adapter_forward_write(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t ts)
{
  volatile union SimbricksProtoPcieD2H *msg = alloc_out(pcie, ts);
  msg->writecomp.req_id = id;
  SimbricksPcieIfD2HOutSend(pcie, msg,
    SIMBRICKS_PROTO_PCIE_D2H_MSG_WRITE);
}

bool simbricks_adapter_getwritecomp(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr,
                               uint8_t *len, uint64_t *val)
{
  fprintf(stderr, "simbricks_adapter_getwritecomp start\n");
  volatile union SimbricksProtoPcieH2D *msg =
    (volatile union SimbricksProtoPcieH2D *) ev;

  fprintf(stderr, "!msg %d\n", !msg);
  fprintf(stderr, "!msg || SimbricksPcieIfH2DInType(pcie, msg) %d\n", !msg || SimbricksPcieIfH2DInType(pcie, msg)!= SIMBRICKS_PROTO_PCIE_H2D_MSG_WRITECOMP);
  if (msg){
    fprintf(stderr, "SimbricksPcieIfH2DInType(pcie, msg) %d\n", SimbricksPcieIfH2DInType(pcie, msg));
  }

  if (!msg || SimbricksPcieIfH2DInType(pcie, msg) !=
      SIMBRICKS_PROTO_PCIE_H2D_MSG_WRITECOMP) {
      return false;
  }

  *id = msg->read.req_id;
  *addr = msg->read.offset;
  *len = msg->read.len;

  fprintf(stderr, "simbricks_adapter_getwritecomp return TRUE, address is %ld\n", *addr);

  return true;
}
