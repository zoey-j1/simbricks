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

import simbricks.orchestration.experiments as exp
import simbricks.orchestration.nodeconfig as node
import simbricks.orchestration.simulators as sim
from simbricks.orchestration.simulator_utils import create_dctcp_hosts

# iperf TCP_multi_client test
# naming convention following host-nic-net-app
# host: qemu/gem5-timing
# nic:  cv/cb/ib
# net:  switch/dumbbell/bridge
# app: DCTCPm

types_of_host = ['qemu', 'qt', 'gt', 'gO3']
types_of_nic = ['cv', 'cb', 'ib']
types_of_net = ['dumbbell']
types_of_app = ['DCTCPm']
types_of_mtu = [1500, 4000, 9000]

num_pairs = 2
max_k = 199680
k_step = 8320
#k_step = 16640
link_rate_opt = '--LinkRate=10Gb/s '  # don't forget space at the end
link_latency_opt = '--LinkLatency=500ns '
cpu_freq = '5GHz'
cpu_freq_qemu = '2GHz'
#mtu = 4000
sys_clock = '1GHz'  # if not set, default 1GHz

ip_start = '192.168.64.1'

experiments = []

# set network sim
NetClass = sim.NS3DumbbellNet

for mtu in types_of_mtu:
    for host in types_of_host:
        for nic in types_of_nic:
            for k_val in range(0, max_k + 1, k_step):

                net = NetClass()
                net.opt = link_rate_opt + link_latency_opt + f'--EcnTh={k_val}'

                e = exp.Experiment(
                    host + '-' + nic + '-' + 'dumbbell' + '-' + 'DCTCPm' +
                    f'{k_val}' + f'-{mtu}'
                )
                e.add_network(net)

                freq = cpu_freq
                # host
                if host == 'qemu':
                    HostClass = sim.QemuHost
                elif host == 'qt':
                    freq = cpu_freq_qemu

                    def qemu_timing(node_config: node.NodeConfig):
                        h = sim.QemuHost(node_config)
                        h.sync = True
                        return h

                    HostClass = qemu_timing
                elif host == 'gt':

                    def gem5_timing(node_config: node.NodeConfig):
                        h = sim.Gem5Host(node_config)
                        #h.sys_clock = sys_clock
                        return h

                    HostClass = gem5_timing
                    e.checkpoint = True
                elif host == 'gO3':

                    def gem5_o3(node_config: node.NodeConfig):
                        h = sim.Gem5Host(node_config)
                        h.cpu_type = 'DerivO3CPU'
                        h.sys_clock = sys_clock
                        return h

                    HostClass = gem5_o3
                    e.checkpoint = True
                else:
                    raise NameError(host)

                # nic
                if nic == 'ib':
                    NicClass = sim.I40eNIC
                    NcClass = node.I40eDCTCPNode
                elif nic == 'cb':
                    NicClass = sim.CorundumBMNIC
                    NcClass = node.CorundumDCTCPNode
                elif nic == 'cv':
                    NicClass = sim.CorundumVerilatorNIC
                    NcClass = node.CorundumDCTCPNode
                else:
                    raise NameError(nic)

                servers = create_dctcp_hosts(
                    e,
                    num_pairs,
                    'server',
                    net,
                    NicClass,
                    HostClass,
                    NcClass,
                    node.DctcpServer,
                    freq,
                    mtu
                )
                clients = create_dctcp_hosts(
                    e,
                    num_pairs,
                    'client',
                    net,
                    NicClass,
                    HostClass,
                    NcClass,
                    node.DctcpClient,
                    freq,
                    mtu,
                    ip_start=num_pairs + 1
                )

                i = 0
                for cl in clients:
                    cl.node_config.app.server_ip = servers[i].node_config.ip
                    i += 1

                # All the clients will not poweroff after finishing iperf test
                # except the last one This is to prevent the simulation gets
                # stuck when one of host exits.

                # The last client waits for the output printed in other hosts,
                # then cleanup
                clients[num_pairs - 1].node_config.app.is_last = True
                clients[num_pairs - 1].wait = True

                print(e.name)
                experiments.append(e)
