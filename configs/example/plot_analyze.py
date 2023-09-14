import os

os.system("echo > network_stats.txt")
cmd = "./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Torus2D --mesh-rows=8 --routing-algorithm=6 \
--inj-vnet=0 --synthetic=uniform_random \
--sim-cycles=20000 --espace-algorithm=3 --aevc "

rate_list = [
    0.02,
    0.04,
    0.06,
    0.08,
    0.1,
    0.12,
    0.14,
    0.16,
    0.18,
    0.2,
    0.22,
    0.24,
    0.26,
    0.28,
    0.3,
    0.32,
    0.34,
    0.36,
    0.38,
    0.4,
    0.42,
    0.44,
    0.46,
    0.48,
    0.5,
    0.52,
    0.54,
    0.56,
    0.58,
    0.6,
    0.62,
    0.64,
    0.66,
    0.68,
    0.7,
    0.72,
    0.74,
    0.76,
    0.78,
    0.8
]

for rate in rate_list:
    cmd_tmp = cmd + "--injectionrate {:.3f}".format(rate)
    os.system(cmd_tmp)
    cmd2 = "grep \"average_packet_latency\" m5out/stats.txt \
            | sed 's/system.ruby.network.average_packet_latency\s*//' \
            >> network_stats.txt"
    # cmd2 = "grep \"average_hops\" m5out/stats.txt \
    #         | sed 's/system.ruby.network.average_hops\s*//' \
    #         >> network_stats.txt"
    # cmd2 = "grep \"packets_received::total\" m5out/stats.txt \
    #         | sed 's/system.ruby.network.packets_received::total\s*//' \
    #         >> network_stats.txt"
    os.system(cmd2)
