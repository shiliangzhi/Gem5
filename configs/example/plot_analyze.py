import os

os.system("echo > network_stats.txt")
cmd = "./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Torus2D --mesh-rows=8 --routing-algorithm=6 \
--inj-vnet=0 --synthetic=uniform_random \
--sim-cycles=20000 --espace-algorithm=2 "

rate_list = [
    0.62,
    0.63,
    0.64,
    0.65,
    0.66,
    0.67,
    0.68,
    0.69,
    0.7,
    0.71,
    0.72,
    0.73,
    0.74,
    0.75,
    0.76,
    0.77,
    0.78,
    0.79,
    0.8,
    0.81,
    0.82,
    0.83,
    0.84,
    0.85
]

for rate in rate_list:
    cmd_tmp = cmd + "--injectionrate {:.3f}".format(rate)
    os.system(cmd_tmp)
    #     cmd2 = "grep \"average_packet_latency\" m5out/stats.txt \
    #             | sed 's/system.ruby.network.average_packet_latency\s*//' \
    #             >> network_stats.txt"
    cmd2 = "grep \"packets_received::total\" m5out/stats.txt \
            | sed 's/system.ruby.network.packets_received::total\s*//' \
            >> network_stats.txt"
    os.system(cmd2)
