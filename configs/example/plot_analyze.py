import os

os.system("echo > network_stats.txt")
cmd = "./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Mesh_XY --mesh-rows=8 --routing-algorithm 1 \
--inj-vnet=0 --synthetic=uniform_random \
--sim-cycles=20000 --vcs-per-vnet 1 --wormhole "

rate_list = [
    0.05,
    0.1,
    0.15,
    0.2,
    0.25,
    0.3,
    0.35,
    0.4,
    0.45,
    0.5,
    0.55,
    0.6,
    0.65,
    0.7,
    0.75,
    0.8,
    0.85,
    0.9,
    0.95,
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
