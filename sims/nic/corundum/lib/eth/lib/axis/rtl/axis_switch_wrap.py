#!/usr/bin/env python
"""Generates an AXI Stream switch wrapper with the specified number of ports."""

from __future__ import print_function

import argparse
import math
from jinja2 import Template

def main():
    parser = argparse.ArgumentParser(description=__doc__.strip())
    parser.add_argument('-p', '--ports',  type=int, default=[4], nargs='+', help='number of ports')
    parser.add_argument('-n', '--name',   type=str, help='module name')
    parser.add_argument('-o', '--output', type=str, help='output file name')

    args = parser.parse_args()

    try:
        generate(**args.__dict__)
    except IOError as ex:
        print(ex)
        exit(1)

def generate(ports=4, name=None, output=None):
    if type(ports) is int:
        m = n = ports
    elif len(ports) == 1:
        m = n = ports[0]
    else:
        m, n = ports

    if name is None:
        name = 'axis_switch_wrap_{0}x{1}'.format(m, n)

    if output is None:
        output = name + '.v'

    print("Opening file '{0}'...".format(output))

    output_file = open(output, 'w')

    print('Generating {0}x{1} port AXI stream switch wrapper {2}...'.format(m, n, name))

    cm = int(math.ceil(math.log(m, 2)))
    cn = int(math.ceil(math.log(n, 2)))

    t = Template(u"""/*

Copyright (c) 2018 Alex Forencich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// Language: Verilog 2001

`timescale 1ns / 1ps

/*
 * AXI4-Stream {{m}}x{{n}} switch (wrapper)
 */
module {{name}} #
(
    // Width of AXI stream interfaces in bits
    parameter DATA_WIDTH = 8,
    // Propagate tkeep signal
    parameter KEEP_ENABLE = (DATA_WIDTH>8),
    // tkeep signal width (words per cycle)
    parameter KEEP_WIDTH = (DATA_WIDTH/8),
    // Propagate tid signal
    parameter ID_ENABLE = 0,
    // tid signal width
    parameter ID_WIDTH = 8,
    // tdest signal width
    // must be wide enough to uniquely address outputs
    parameter DEST_WIDTH = {{cm}},
    // Propagate tuser signal
    parameter USER_ENABLE = 1,
    // tuser signal width
    parameter USER_WIDTH = 1,
{%- for p in range(n) %}
    // Output interface routing base tdest selection
    // Port selected if M_BASE <= tdest <= M_TOP
    parameter M{{'%02d'%p}}_BASE = {{p}},
    // Output interface routing top tdest selection
    // Port selected if M_BASE <= tdest <= M_TOP
    parameter M{{'%02d'%p}}_TOP = {{p}},
    // Interface connection control
    parameter M{{'%02d'%p}}_CONNECT = {{m}}'b{% for p in range(m) %}1{% endfor %},
{%- endfor %}
    // Input interface register type
    // 0 to bypass, 1 for simple buffer, 2 for skid buffer
    parameter S_REG_TYPE = 0,
    // Output interface register type
    // 0 to bypass, 1 for simple buffer, 2 for skid buffer
    parameter M_REG_TYPE = 2,
    // arbitration type: "PRIORITY" or "ROUND_ROBIN"
    parameter ARB_TYPE = "ROUND_ROBIN",
    // LSB priority: "LOW", "HIGH"
    parameter LSB_PRIORITY = "HIGH"
)
(
    input  wire                  clk,
    input  wire                  rst,

    /*
     * AXI Stream inputs
     */
{%- for p in range(m) %}
    input  wire [DATA_WIDTH-1:0] s{{'%02d'%p}}_axis_tdata,
    input  wire [KEEP_WIDTH-1:0] s{{'%02d'%p}}_axis_tkeep,
    input  wire                  s{{'%02d'%p}}_axis_tvalid,
    output wire                  s{{'%02d'%p}}_axis_tready,
    input  wire                  s{{'%02d'%p}}_axis_tlast,
    input  wire [ID_WIDTH-1:0]   s{{'%02d'%p}}_axis_tid,
    input  wire [DEST_WIDTH-1:0] s{{'%02d'%p}}_axis_tdest,
    input  wire [USER_WIDTH-1:0] s{{'%02d'%p}}_axis_tuser,
{% endfor %}
    /*
     * AXI Stream outputs
     */
{%- for p in range(n) %}
    output wire [DATA_WIDTH-1:0] m{{'%02d'%p}}_axis_tdata,
    output wire [KEEP_WIDTH-1:0] m{{'%02d'%p}}_axis_tkeep,
    output wire                  m{{'%02d'%p}}_axis_tvalid,
    input  wire                  m{{'%02d'%p}}_axis_tready,
    output wire                  m{{'%02d'%p}}_axis_tlast,
    output wire [ID_WIDTH-1:0]   m{{'%02d'%p}}_axis_tid,
    output wire [DEST_WIDTH-1:0] m{{'%02d'%p}}_axis_tdest,
    output wire [USER_WIDTH-1:0] m{{'%02d'%p}}_axis_tuser{% if not loop.last %},{% endif %}
{% endfor -%}
);

// parameter sizing helpers
function [31:0] w_32(input [31:0] val);
    w_32 = val;
endfunction

function [S_COUNT-1:0] w_s(input [S_COUNT-1:0] val);
    w_s = val;
endfunction

axis_switch #(
    .S_COUNT({{m}}),
    .M_COUNT({{n}}),
    .DATA_WIDTH(DATA_WIDTH),
    .KEEP_ENABLE(KEEP_ENABLE),
    .KEEP_WIDTH(KEEP_WIDTH),
    .ID_ENABLE(ID_ENABLE),
    .ID_WIDTH(ID_WIDTH),
    .DEST_WIDTH(DEST_WIDTH),
    .USER_ENABLE(USER_ENABLE),
    .USER_WIDTH(USER_WIDTH),
    .M_BASE({ {% for p in range(n-1,-1,-1) %}w_32(M{{'%02d'%p}}_BASE){% if not loop.last %}, {% endif %}{% endfor %} }),
    .M_TOP({ {% for p in range(n-1,-1,-1) %}w_32(M{{'%02d'%p}}_TOP){% if not loop.last %}, {% endif %}{% endfor %} }),
    .M_CONNECT({ {% for p in range(n-1,-1,-1) %}w_s(M{{'%02d'%p}}_CONNECT){% if not loop.last %}, {% endif %}{% endfor %} }),
    .S_REG_TYPE(S_REG_TYPE),
    .M_REG_TYPE(M_REG_TYPE),
    .ARB_TYPE(ARB_TYPE),
    .LSB_PRIORITY(LSB_PRIORITY)
)
axis_switch_inst (
    .clk(clk),
    .rst(rst),
    // AXI inputs
    .s_axis_tdata({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tdata{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tkeep({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tkeep{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tvalid({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tvalid{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tready({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tready{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tlast({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tlast{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tid({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tid{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tdest({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tdest{% if not loop.last %}, {% endif %}{% endfor %} }),
    .s_axis_tuser({ {% for p in range(m-1,-1,-1) %}s{{'%02d'%p}}_axis_tuser{% if not loop.last %}, {% endif %}{% endfor %} }),
    // AXI outputs
    .m_axis_tdata({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tdata{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tkeep({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tkeep{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tvalid({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tvalid{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tready({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tready{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tlast({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tlast{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tid({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tid{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tdest({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tdest{% if not loop.last %}, {% endif %}{% endfor %} }),
    .m_axis_tuser({ {% for p in range(n-1,-1,-1) %}m{{'%02d'%p}}_axis_tuser{% if not loop.last %}, {% endif %}{% endfor %} })
);

endmodule

""")

    output_file.write(t.render(
        m=m,
        n=n,
        cm=cm,
        cn=cn,
        name=name
    ))

    print('Done')

if __name__ == '__main__':
    main()
