#ifndef SIMBRICKS_ADAPTER_H
#define SIMBRICKS_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>

#include <simbricks/pcie/if.h>

#define VFIO_DIR_PATH "/dev/vfio"
#define PCI_DEVICE_PATH "/dev/vfio/noiommu-0"

static struct SimbricksBaseIfSHMPool pool;
static struct SimbricksPcieIf pcieif;
void simbricks_adapter_init(const char *socket_path, const char *shm_path, bool enable_sync);
void *simbricks_adapter_getevent(uint64_t ts);
bool simbricks_adapter_getread(void *ev, uint64_t *id, uint64_t *addr, uint8_t *len);
bool simbricks_adapter_getwrite(void *ev, uint64_t *id, uint64_t *addr, uint8_t *len, uint64_t *val);
void simbricks_adapter_eventdone(void *ev);
uint64_t simbricks_adapter_nextts(void);
void simbricks_adapter_complr(uint64_t id, uint64_t val, uint8_t len, uint64_t ts);
void simbricks_adapter_complw(uint64_t id, uint64_t ts);

#endif // SIMBRICKS_ADAPTER_H
