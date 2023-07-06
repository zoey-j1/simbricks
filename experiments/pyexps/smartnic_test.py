# Copyright 2021 Max Planck Institute for Software Systems, and
# National University of Singapore
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# SOC qemu (H)  -(pcie)-> (D) Interface (D) <-(pcie)-  (H) Host qemu

import simbricks.orchestration.experiments as exp
import simbricks.orchestration.nodeconfig as node
import simbricks.orchestration.simulators as sim


experiments = []

# for h in ['qk', 'gk']:
for h in ['qk']:
    e = exp.Experiment('femutest-' + h)
    e.checkpoint = False

    node_config = node.LinuxFEMUNode()
    node_config.app = node.NVMEFsTest()
    node_config.cores = 1
    node_config.app.is_sleep = 1

    if h == 'gk':
        host = sim.Gem5Host(node_config)
        host.cpu_type = 'X86KvmCPU'
    elif h == 'qk':
        soc = sim.QemuHost(node_config)
        host = sim.QemuHost(node_config)

    soc.name = 'soc.0'
    e.add_host(soc)
    soc.wait = True

    host.name = 'host.0'
    e.add_host(host)
    host.wait = True

    smart = sim.SMARTDev()
    smart.name = "smart0"
    e.add_pcidev(smart)

    soc.add_pcidev(smart)
    host.add_pcidev(smart)

    smart.add_host(soc)
    smart.add_host(host)

    experiments.append(e)
