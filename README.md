#### Project Readme

Our project implement an adaptive routing-algorithms on Torus topologies, call Multi-Selectable Shortest Path (MSSP). To avoid deadlock happend, we implement Adaptive Escape Virtual Channel (AEVC) algorithm.

In our codes, we have implemented :
```
New topologies:
   - 2D-Torus
   - 3D-Torus
New routing-algorithms:
   - Deterministic deadlock free algorithm on Torus (Our algorithm)
   - Deterministic deadlock free algorithm on Torus (Dally's paper)
   - Shortest Path algorithm
   - Random Shortest Path algorithm
   - Multi-Selectable Shortest Path algorithm
New flow-controls:
   - Escape Virtual Channel
   - Adaptive Escape Virtual Channel 
```

Here, we provide the way to run these algorithm in 2D-Torus.

In `another` branch, you can run: 

```
# MeshXY
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Mesh_XY --mesh-rows=8 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={}

# Deterministic deadlock free algorithm on Torus (Our algorithm)
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --torus-cols=8 --mesh-rows=8 --routing-algorithm=4 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={}

# Shortest Path (SP) algorithm
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --torus-cols=8 --mesh-rows=8 --routing-algorithm=6 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={}

# Random Shortest Path algorithm
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --torus-cols=8 --mesh-rows=8 --routing-algorithm=8 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={}

# Multi-Selectable Shortest Path (MSSP) algorithm
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --torus-cols=8 --mesh-rows=8 --routing-algorithm=6 --espace-algorithm=2 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={}
```

In `main` branch, you can run:
```
# Deterministic deadlock free algorithm on Torus (Dally's paper)
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --mesh-rows=8 --routing-algorithm=6 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={} --espace-algorithm=4

# MSSP with one espace VC
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --mesh-rows=8 --routing-algorithm=6 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={} --espace-algorithm=3

# MSSP with Adaptive Escape Virtual Channel (AEVC) algorithm
./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --mesh-rows=8 --routing-algorithm=6 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={} --espace-algorithm=3 --aevc
```
#### For data prepare

##### Experiment 2

Here we prepare the code for getting `latency-injectionrate` data in `uniform_random` synthetic for `Deterministic deadlock free algorithm on Torus (Our algorithm)`. You should run this code in `another` branch.

By changing the instruction in the code, we can get other data.

```python
import os
import numpy as np
import matplotlib.pyplot as plt

synthetic = "uniform_random"
begin = 0.1
end = 0.4

inj_rate_list = np.linspace(begin, end, 60)

cut = True
cut_threshold = 50

os.system("echo > latency_{}.txt".format(synthetic))
for inj_rate in inj_rate_list:
    os.system(
        "echo inject rate: {} >> latency_{}.txt".format(inj_rate, synthetic)
    )
    os.system(
        "./build/NULL/gem5.opt configs/example/garnet_synth_traffic.py --network=garnet --vcs-per-vnet=4 --num-cpus=64 --num-dirs=64 --topology=Torus2D --torus-cols=8 --mesh-rows=8 --routing-algorithm=4 --inj-vnet=0 --synthetic={} --sim-cycles=20000 --injectionrate={}".format(
            synthetic, inj_rate
        )
    )
    os.system(
        "grep 'average_packet_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> latency_{}.txt".format(
            synthetic
        )
    )

injection_rate_list = []
torus_latency_1 = []

test_txt = open("latency_{}.txt".format(synthetic), "r")
lines = test_txt.readlines()
test_txt.close()

for line in lines:
    if "inject rate" in line:
        injection_rate_list.append(float(line.split()[2]))
    elif "average_packet_latency" in line:
        if cut and float(line.split()[2]) > cut_threshold:
            torus_latency_1.append(cut_threshold)
        else:
            torus_latency_1.append(float(line.split()[2]))

plt.figure()
plt.title("Average Packet Latency vs. Injection Rate")
plt.plot(
    injection_rate_list,
    torus_latency_1,
    label="Deadlock-Free",
    color="blue",
    linestyle="--",
)
plt.xlabel("Injection Rate")
plt.ylabel("Average Packet Latency")
```

##### Experiment 4

Here we prepare the way to get `latency-injectionrate` data for MSSP with AEVC algorithm in 2D-Trous topology.  In `main` branch, run

```
python ./configs/example/plot_analyze.py
```

You can get the latency data in `network_stats.txt`.

By changing the `cmd` in `plot_analyze.py`, you can get the latency data for other algorithm and other synthetic.

#### Gem5 Readme

This is the gem5 simulator.

The main website can be found at http://www.gem5.org

A good starting point is http://www.gem5.org/about, and for
more information about building the simulator and getting started
please see http://www.gem5.org/documentation and
http://www.gem5.org/documentation/learning_gem5/introduction.

To build gem5, you will need the following software: g++ or clang,
Python (gem5 links in the Python interpreter), SCons, zlib, m4, and lastly
protobuf if you want trace capture and playback support. Please see
http://www.gem5.org/documentation/general_docs/building for more details
concerning the minimum versions of these tools.

Once you have all dependencies resolved, type 'scons
build/<CONFIG>/gem5.opt' where CONFIG is one of the options in build_opts like
ARM, NULL, MIPS, POWER, SPARC, X86, Garnet_standalone, etc. This will build an
optimized version of the gem5 binary (gem5.opt) with the the specified
configuration. See http://www.gem5.org/documentation/general_docs/building for
more details and options.

The main source tree includes these subdirectories:
   - build_opts: pre-made default configurations for gem5
   - build_tools: tools used internally by gem5's build process.
   - configs: example simulation configuration scripts
   - ext: less-common external packages needed to build gem5
   - include: include files for use in other programs
   - site_scons: modular components of the build system
   - src: source code of the gem5 simulator
   - system: source for some optional system software for simulated systems
   - tests: regression tests
   - util: useful utility programs and files

To run full-system simulations, you may need compiled system firmware, kernel
binaries and one or more disk images, depending on gem5's configuration and
what type of workload you're trying to run. Many of those resources can be
downloaded from http://resources.gem5.org, and/or from the git repository here:
https://gem5.googlesource.com/public/gem5-resources/

If you have questions, please send mail to gem5-users@gem5.org

Enjoy using gem5 and please share your modifications and extensions.
