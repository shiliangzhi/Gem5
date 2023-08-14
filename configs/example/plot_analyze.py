import os

os.system("echo > network_stats.txt")
cmd = "./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Mesh_XY --mesh-rows=8 \
--inj-vnet=0 --synthetic=uniform_random \
--sim-cycles=20000 --link-width-bits 16 "

rate_list = [0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7]

for rate in rate_list:
    cmd_tmp = cmd + "--injectionrate {:.3f}".format(rate)
    os.system(cmd_tmp)
    cmd2 = "grep \"average_packet_latency\" m5out/stats.txt \
            | sed 's/system.ruby.network.average_packet_latency\s*//' \
            >> network_stats.txt"
    os.system(cmd2)
    rate = rate * 1.3
