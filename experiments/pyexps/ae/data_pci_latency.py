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

import glob
import json
import sys

if len(sys.argv) != 2:
    print('Usage: data_pci_latency.py OUTDIR')
    sys.exit(1)

basedir = sys.argv[1]

types_of_pci = [10, 50, 100, 500, 1000]


# pylint: disable=redefined-outer-name
def time_diff_min(data):
    start_time = data['start_time']
    end_time = data['end_time']

    time_diff_in_min = (end_time - start_time) / 60
    return time_diff_in_min


print('# PCI latency (ns)' + '\t' + 'Sim.Time')

for workload in types_of_pci:

    line = [str(workload)]
    path_pat = f'{basedir}pci-gt-ib-sw-{workload}'

    runs = []
    for path in glob.glob(path_pat + '-*.json'):
        if path == path_pat + '-0.json':
            # skip checkpoints
            continue

        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        res = time_diff_min(data)
        if res is not None:
            runs.append(res)

    if not runs:
        line.append(' ')
    else:
        line.append(f'{sum(runs) / len(runs)}')

    print('\t'.join(line))
