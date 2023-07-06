**Project Milestones**
1. &#9744; Create dummy PCIe interface simulator that just connects to host and SmartNIC SoC simulator. Create python orchestration script for running this simulation. Check that PCIe devices appear correctly in host.
    - create dummy PCIe interface simulator in sims/nic/smartnic 
    - create python orchestration script smartnic_test.py in experiments/pyexps
2. &#9744; Implement MMIO functionality in PCIe interface, along with the required driver functionality for both host and SmartNIC SoC, and a simple test application (e.p. Poking some registers on the host, and fulfilling those requests from the SmartNIC).
3. &#9744; Implement DMA functionality in PCIe interface, along with the required driver functionality for SoC, and a simple use-case (e.g. memcopy application offload).
4. &#9744; (Optional) Implement interrupt functionality in PCIe interface along with the required driver functionality for host and SoC and a simple use-case (e.g. add completion notification to memcopy app from before)
5. &#9744; Implement a smartNIC application that just implements basic NIC functionality: receive packets via regular RAW sockets from the SmartNIC NIC, and then add them to a simple polled receive queue on the host, same thing for the host. (write code for SoC and host applications).