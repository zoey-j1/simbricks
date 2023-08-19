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

# SOC qemu (H)  -(pcie)-> (D)          Interface          (D) <-(pcie)-  (H) Host qemu
# SOC qemu (H)  -(pcie)-> (D) CV (net) -(wire)- (net) CV  (D) <-(pcie)-  (H) Host qemu

import os
import simbricks.orchestration.experiments as exp
import simbricks.orchestration.nodeconfig as node
import simbricks.orchestration.simulators as sim


# Set this to true to enable synchronization with qemu + instruction counting
# synchronized = True
synchronized = False

experiments = []

for h in ['qk']:
    e = exp.Experiment('cps-' + h)
    e.checkpoint = False

    # host
    server_config = node.AdapterHostNode()
    server_config.app = node.AdapterHostApp()
    server = sim.QemuHost(server_config)
    server.name = 'host'
    server.sync = synchronized
    server.wait = True

    adapter = sim.SmartAdapter()
    adapter.name = 'adapter'
    adapter.sync = synchronized
    server.add_pcidev(adapter)

    e.add_pcidev(adapter)
    e.add_host(server)

    experiments.append(e)
