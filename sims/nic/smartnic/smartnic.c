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
#include <simbricks/smartnicif/smartnicif.h>

static volatile int exiting = 0;
uint64_t main_time = 0;
static struct SimbricksSmartNicIf nicif;     // Dummy PCIe Interface Simulator
static uint64_t clock_period = 4 * 1000ULL;  // 4ns -> 250MHz

static void sigint_handler(int dummy) {
  exiting = 1;
}

static void sigusr1_handler(int dummy) {
  fprintf(stderr, "main_time = %lu\n", main_time);
}

static void move_pkt(struct SimbricksPcieIf *from, struct SimbricksPcieIf *to) {

  fprintf(stderr, "debug_smartnic SmartNic move pkt start\n");

  volatile union SimbricksProtoPcieH2D *msg_from =
      SimbricksPcieIfH2DInPoll(from, main_time);
      
  volatile union SimbricksProtoPcieD2H *msg_to = SimbricksPcieIfD2HOutAlloc(&nicif.pcie1, main_time);
  volatile struct SimbricksProtoPcieD2HRead *read = &msg_to->read;
    read->req_id = (uint64_t) 0;
    read->offset = (uint64_t) 0;
    read->len = (uint16_t) 0;

  SimbricksPcieIfD2HOutSend(to, msg_to,
                            SIMBRICKS_PROTO_PCIE_D2H_MSG_READ);

  SimbricksPcieIfH2DInDone(from, msg_from);
}

int main(int argc, char *argv[]) {
  fprintf(stderr, "debug_smartnic Starting Smartnic Interface\n");

  if (argc < 4 && argc > 10) {
    fprintf(stderr,
            "Usage: smartnic_interface PCI-SOCKET PCI-SOCKET "
            "SHM [SYNC-MODE] [START-TICK] [SYNC-PERIOD] [PCI-LATENCY] "
            "[ETH-LATENCY] [CLOCK-FREQ-MHZ]\n");
    return EXIT_FAILURE;
  }

  struct SimbricksBaseIfParams pcieParams0;
  struct SimbricksBaseIfParams pcieParams1;

  SimbricksPcieIfDefaultParams(&pcieParams0);
  SimbricksPcieIfDefaultParams(&pcieParams1);

  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);

  struct SimbricksProtoPcieDevIntro di;
  memset(&di, 0, sizeof(di));
  di.bars[0].len = 1 << 24;
  di.bars[0].flags = SIMBRICKS_PROTO_PCIE_BAR_64;
  di.pci_vendor_id = 0x5543;
  di.pci_device_id = 0x1001;
  di.pci_class = 0x02;
  di.pci_subclass = 0x00;
  di.pci_revision = 0x00;
  di.pci_msi_nvecs = 32;

  pcieParams0.sock_path = argv[1];
  pcieParams1.sock_path = argv[2];

  if (SimbricksSmartNicIfInit(&nicif, argv[3], &pcieParams0, &pcieParams1, &di) != 0) {
    fprintf(stderr, "SimbricksSmartNicIfInit failed\n");
    return -1;
  }

  fprintf(stderr, "debug_smartnic SimbricksSmartNicIfInit completed\n");

  int sync_pci0 = SimbricksBaseIfSyncEnabled(&nicif.pcie0.base);
  int sync_pci1 = SimbricksBaseIfSyncEnabled(&nicif.pcie1.base);

  while (!exiting) {
    if (SimbricksPcieIfD2HOutSync(&nicif.pcie0, main_time) < 0) {
      fprintf(stderr, "SimbricksPcieIfD2HOutSync(nicif.pcie0) failed\n");
      abort();
    }
    if (SimbricksPcieIfD2HOutSync(&nicif.pcie1, main_time) != 0) {
      fprintf(stderr, "SimbricksPcieIfD2HOutSync(nicif.pcie1) failed\n");
      abort();
    }

    do {
      move_pkt(&nicif.pcie0, &nicif.pcie1);
      main_time = SimbricksPcieIfH2DInTimestamp(&nicif.pcie0);
    } while (!exiting &&
             ((sync_pci0 && SimbricksPcieIfD2HInTimestamp(&nicif.pcie0) <= main_time) || 
             (sync_pci1 && SimbricksPcieIfD2HInTimestamp(&nicif.pcie1) <= main_time)));

    main_time += clock_period / 2;
    exiting = 1;
  }

  return 0;
}