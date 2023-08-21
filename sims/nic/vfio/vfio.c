/*
 * Copyright 2019 University of Washington, Max Planck Institute for
 * Software Systems, and The University of Texas at Austin
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
#define __USE_LARGEFILE64
#include "vfio.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <linux/pci.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include "vfio.h"

int vfio_dev_open(struct vfio_dev *dev, const char *groupname, const char *pci_dev) {
    dev->containerfd = open("/dev/vfio/vfio", O_RDWR);
    if (dev->containerfd < 0) {
        perror("Error opening VFIO container");
        return -1;
    }

    if (ioctl(dev->containerfd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
        perror("Incompatible VFIO API version");
        close(dev->containerfd);
        return -1;
    }

    if (ioctl(dev->containerfd, VFIO_CHECK_EXTENSION, VFIO_NOIOMMU_IOMMU) != 1) {
        perror("vfio_dev_open not available");
        close(dev->containerfd);
        return -1;
    }

    // Open the VFIO group
    dev->groupfd = open(groupname, O_RDWR);
    if (dev->groupfd < 0) {
        perror("Error opening VFIO group");
        close(dev->containerfd);
        return -1;
    }

    struct vfio_group_status group_status = { .argsz = sizeof(group_status) };
    if (ioctl(dev->groupfd, VFIO_GROUP_GET_STATUS, &group_status)) {
        perror("Error getting VFIO group status");
        close(dev->groupfd);
        close(dev->containerfd);
        return -1;
    }

    if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
        perror("VFIO group is not viable (device not attached)");
        close(dev->groupfd);
        close(dev->containerfd);
        return -1;
    }

    // Add the group to the container
    if (ioctl(dev->groupfd, VFIO_GROUP_SET_CONTAINER, &(dev->containerfd))) {
        perror("Error adding VFIO group to container");
        close(dev->groupfd);
        close(dev->containerfd);
        return -1;
    }

    // Enable the NOIOMMU
    if (ioctl(dev->containerfd, VFIO_SET_IOMMU, VFIO_NOIOMMU_IOMMU)) {
        perror("Error enabling NOIOMMU");
        close(dev->groupfd);
        close(dev->containerfd);
        return -1;
    }

    // Get the device file descriptor
    dev->devfd = ioctl(dev->groupfd, VFIO_GROUP_GET_DEVICE_FD, pci_dev);
    if (dev->devfd < 0) {
        perror("Error getting VFIO device file descriptor");
        close(dev->groupfd);
        close(dev->containerfd);
        return -1;
    }

    // Get device information
    dev->info.argsz = sizeof(dev->info);
    if (ioctl(dev->devfd, VFIO_DEVICE_GET_INFO, &(dev->info))) {
        perror("Error getting VFIO device info");
        close(dev->devfd);
        close(dev->groupfd);
        close(dev->containerfd);
        return -1;
    }

    return 0;
}

void vfio_dev_close(struct vfio_dev *dev) {
    close(dev->devfd);
    close(dev->groupfd);
    close(dev->containerfd);
}

void* vfio_map_queue_buffer(int devfd, off_t offset) {

    fprintf(stderr, "devfd is %d, offset is %ld, queue size is %d !!!\n", devfd, offset, QUEUE_SIZE);

    void* queue_buf = mmap(NULL, QUEUE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, offset);
    if (queue_buf == MAP_FAILED) {
        perror("Error mapping queue buffer");
        return NULL;
    }
    return queue_buf;
}

void vfio_unmap_queue_buffer(void *queue_buf, size_t size) {
    munmap(queue_buf, size);
}

ssize_t vfio_read_rx_queue(void *rx_queue_buf, void *data, size_t size) {
    // ... (implementation to read data from RX queue, device-specific)
    if (size > QUEUE_SIZE) {
        fprintf(stderr, "Error: Size exceeds queue buffer size\n");
        return -1;
    }

    // Copy data from the RX queue buffer into the 'data' buffer
    memcpy(data, (uint8_t*)rx_queue_buf, size);

    return size;
}

ssize_t vfio_write_tx_queue(void *tx_queue_buf, const void *data, size_t size) {
    // ... (implementation to write data to TX queue, device-specific)
    if (size > QUEUE_SIZE) {
        fprintf(stderr, "Error: Size exceeds queue buffer size\n");
        return -1;
    }

    // Copy data from the 'data' buffer into the TX queue buffer
    memcpy(tx_queue_buf, data, size);

    return size;
}

int main(int argc, char *argv[]) {
    struct vfio_dev dev;
    memset(&dev, 0, sizeof(dev));

    // Open VFIO device
    if (vfio_dev_open(&dev, argv[1], argv[2]) != 0) {
        fprintf(stderr, "Error opening VFIO device\n");
        return 1;
    }

    // Map device regions
    void *tx_queue_buf = vfio_map_queue_buffer(dev.devfd, TX_QUEUE_DESC_RING_ADDR_REG);
    if (tx_queue_buf == NULL) {
        vfio_dev_close(&dev);
        return 1;
    }

    // TODO: vfio_map_queue_buffer(dev.devfd, QUEUE_SIZE) get Invalid argument error
    void *rx_queue_buf = vfio_map_queue_buffer(dev.devfd, RX_QUEUE_DESC_RING_ADDR_REG);
    if (rx_queue_buf == NULL) {
        vfio_unmap_queue_buffer(tx_queue_buf, QUEUE_SIZE);
        vfio_dev_close(&dev);
        return 1;
    }

    // Perform read and write operations on the queues
    char data[] = "Hello, VFIO!";
    size_t data_size = strlen(data) + 1;

    // Read data from the RX queue
    char rx_data[QUEUE_SIZE];
    ssize_t read_result = vfio_read_rx_queue(rx_queue_buf, rx_data, data_size);
    if (read_result < 0) {
        perror("Error reading from RX queue");
    } else {
        printf("Data read from RX queue: %s\n", rx_data);
    }

    printf("read data from rx queue: ");
    int i = 0;
    while (rx_data[i] != '\0') {
        putchar(rx_data[i]);
        i++;
    }
    printf("\n");

    // Write data to the TX queue
    ssize_t write_result = vfio_write_tx_queue(tx_queue_buf, data, data_size);
    if (write_result < 0) {
        perror("Error writing to TX queue");
    } else {
        printf("Data written to TX queue: %s\n", data);
    }

    // Clean up
    vfio_unmap_queue_buffer(tx_queue_buf, QUEUE_SIZE);
    vfio_unmap_queue_buffer(rx_queue_buf, QUEUE_SIZE);
    vfio_dev_close(&dev);

    // sleep(10);

    return 0;
}
