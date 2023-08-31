#ifndef SIMBRICKS_ADAPTER_H
#define SIMBRICKS_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>

#include <simbricks/pcie/if.h>
#include <simbricks/smartnicif/smartnicif.h>

void simbricks_adapter_init(struct SimbricksSmartNicIf *nicif, const char *socket_path0, const char *socket_path1, const char *shm_path, bool enable_sync);
void *simbricks_adapter_getevent(struct SimbricksPcieIf *pcie, uint64_t ts);
bool simbricks_adapter_getread(struct SimbricksPcieIf *pcie, void *ev, volatile struct SimbricksProtoPcieH2DRead read);
bool simbricks_adapter_getwrite(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr, uint8_t *len, uint64_t *val);
void simbricks_adapter_eventdone(struct SimbricksPcieIf *pcie, void *ev);
uint64_t simbricks_adapter_nextts(struct SimbricksPcieIf *pcie);
void simbricks_adapter_complr(struct SimbricksPcieIf *pcie, uint64_t ts, volatile struct SimbricksProtoPcieH2DRead read);
void simbricks_adapter_complw(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t ts);
void simbricks_adapter_forward_read(struct SimbricksPcieIf *pcie, uint64_t ts, volatile struct SimbricksProtoPcieH2DRead read);
bool simbricks_adapter_getreadcomp(struct SimbricksPcieIf *pcie, void *ev, volatile struct SimbricksProtoPcieH2DRead read);
void simbricks_adapter_forward_write(struct SimbricksPcieIf *pcie, uint64_t id, uint64_t ts);
bool simbricks_adapter_getwritecomp(struct SimbricksPcieIf *pcie, void *ev, uint64_t *id, uint64_t *addr, uint8_t *len, uint64_t *val);

#endif // SIMBRICKS_ADAPTER_H