#ifndef SIMBRICKS_ADAPTER_H
#define SIMBRICKS_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>

#include <simbricks/pcie/if.h>
#include <simbricks/smartnicif/smartnicif.h>

#define QUEUE_SIZE 4096 // Adjust the queue size as needed

#define TX_QUEUE_DESC_RING_ADDR_REG 0x1000  // starting address of the descriptor ring for the TX queue
#define TX_QUEUE_DESC_RING_SIZE_REG 0x1004  // size of the descriptor ring for the TX queue
#define TX_QUEUE_CONTROL_REG 0x1010         // register is used to control the behavior of the TX queue
#define TX_QUEUE_ENABLE_BIT 0x1             // when set to 1, enables the TX queue for packet transmission

#define RX_QUEUE_DESC_RING_ADDR_REG 0x2000  // starting address of the descriptor ring for the RX queue
#define RX_QUEUE_DESC_RING_SIZE_REG 0x2004  // size of the descriptor ring for the RX queue
#define RX_QUEUE_CONTROL_REG 0x2010         // control the behavior of the RX queue
#define RX_QUEUE_ENABLE_BIT 0x1             // when set to 1, enables the RX queue for receiving packets

void simbricks_adapter_init(struct SimbricksSmartNicIf *nicif, const char *socket_path0, const char *socket_path1, const char *shm_path, bool enable_sync);
void *simbricks_adapter_getevent(struct SimbricksPcieIf *pcie, uint64_t ts);
bool simbricks_adapter_getread(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr, uint8_t *len);
bool simbricks_adapter_getwrite(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr, uint8_t *len, uint64_t *val);
void simbricks_adapter_eventdone(struct SimbricksPcieIf *pcie, void *ev);
uint64_t simbricks_adapter_nextts(struct SimbricksPcieIf *pcie);
void simbricks_adapter_complr(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t val, uint8_t len, uint64_t ts);
void simbricks_adapter_complw(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t ts);
void simbricks_adapter_forward(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t val, uint8_t len, uint64_t ts);

#endif // SIMBRICKS_ADAPTER_H