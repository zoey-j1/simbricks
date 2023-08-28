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

import os
import simbricks.orchestration.experiments as exp
import simbricks.orchestration.nodeconfig as node
import simbricks.orchestration.simulators as sim
import time


synchronized = False
experiments = []
total = 4

start_time = time.time()

for h in ['qk']:
    e = exp.Experiment('time-' + h)
    e.checkpoint = False

    host_config = node.AdapterHostNode()
    host_config.app = node.AdapterHostApp()
    host = sim.QemuHost(host_config)
    host.name = 'host'
    host.sync = synchronized
    host.wait = True
    e.add_host(host)

    for i in range(total):
        soc_config = node.AdapterSocNode()
        soc_config.app = node.AdapterSocApp()
        soc = sim.QemuHost(soc_config)
        soc.name = f'soc{i}'
        soc.sync = synchronized
        soc.wait = True

        adapter = sim.SMARTDev()
        adapter.name = f'adapter{i}'
        adapter.sync = synchronized

        host.add_pcidev(adapter)
        soc.add_pcidev(adapter)
        adapter.add_host(host)
        adapter.add_host(soc)
        e.add_pcidev(adapter)
        e.add_host(soc)

    experiments.append(e)

    end_time = time.time()
    total_elapsed_time = end_time - start_time
    print(f"total time used {total_elapsed_time}")
