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


class AdapterHostNode(node.NodeConfig):

    def prepare_pre_cp(self):
        l = []
        l.append('mount -t proc proc /proc')
        l.append('mount -t sysfs sysfs /sys')
        l.append('echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode')
        return super().prepare_pre_cp() + l

    def prepare_post_cp(self):
        l = []
        l.append('lspci -vvv')
        l.append('echo 9876 1234 >/sys/bus/pci/drivers/vfio-pci/new_id')
        return super().prepare_post_cp() + l


class AdapterHostApp(node.AppConfig):
    def config_files(self):
        # copy cps impl binary into host image during prep
        m = {'controller_impl': open('../sims/nic/vfio/vfio', 'rb')}
        return {**m, **super().config_files()}

    def run_cmds(self, node):
        # application command to run once the host is booted up
        return ['/tmp/guest/controller_impl /dev/vfio/noiommu-0 0000:00:02.0']


class AdapterSocNode(node.NodeConfig):

    def prepare_pre_cp(self):
        l = []
        l.append('mount -t proc proc /proc')
        l.append('mount -t sysfs sysfs /sys')
        l.append('echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode')
        return super().prepare_pre_cp() + l

    def prepare_post_cp(self):
        l = []
        l.append('lspci -vvv')
        l.append('echo 9876 4321 >/sys/bus/pci/drivers/vfio-pci/new_id')
        return super().prepare_post_cp() + l


class AdapterSocApp(node.AppConfig):
    def config_files(self):
        # copy cps impl binary into host image during prep
        m = {'controller_impl': open('../sims/nic/vfio/vfio', 'rb')}
        return {**m, **super().config_files()}

    def run_cmds(self, node):
        # application command to run once the host is booted up
        return ['/tmp/guest/controller_impl /dev/vfio/noiommu-0 0000:00:02.0']



synchronized = False

experiments = []

for h in ['qk']:
    e = exp.Experiment('adapter-' + h)
    e.checkpoint = False

    host_config = AdapterHostNode()
    host_config.app = AdapterHostApp()
    host = sim.QemuHost(host_config)
    host.name = 'host'
    host.sync = synchronized
    host.wait = True

    soc_config = AdapterSocNode()
    soc_config.app = AdapterSocApp()
    soc = sim.QemuHost(soc_config)
    soc.name = 'soc'
    soc.sync = synchronized
    soc.wait = True

    adapter = sim.SMARTDev()
    adapter.name = 'adapter'
    adapter.sync = synchronized

    host.add_pcidev(adapter)
    soc.add_pcidev(adapter)

    adapter.add_host(host)
    adapter.add_host(soc)

    e.add_pcidev(adapter)
    e.add_host(host)
    e.add_host(soc)

    experiments.append(e)
