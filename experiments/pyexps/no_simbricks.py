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

app_types = ['sleep', 'busy']

experiments = []

for app_type in app_types:

    e = exp.Experiment('no_simb-gt-' + app_type)

    HostClass = sim.Gem5Host
    e.checkpoint = True
    e.no_simbricks = True

    NcClass = node.I40eLinuxNode
    # create servers and clients

    node_config = NcClass()
    node_config.ip = '10.0.0.1'
    node_config.app = node.NoTraffic()
    node_config.cores = 1

    # is busy
    if app_type == 'sleep':
        node_config.app.is_sleep = 1
    else:
        node_config.app.is_sleep = 0

    host = HostClass(node_config)
    host.name = 'host.0'

    e.add_host(host)
    host.wait = True

    print(e.name)

    # add to experiments
    experiments.append(e)
