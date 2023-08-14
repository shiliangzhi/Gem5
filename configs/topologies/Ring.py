from m5.params import *
from m5.objects import *

from common import FileSystemConfig

from topologies.BaseTopology import SimpleTopology


class Ring(SimpleTopology):
    description = "Ring"

    def __init__(self, controllers):
        self.nodes = controllers

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes

        num_routers = options.num_cpus

        link_latency = options.link_latency  # used by simple and garnet
        router_latency = options.router_latency  # only used by garnet

        routers = [
            Router(router_id=i, latency=router_latency)
            for i in range(num_routers)
        ]
        network.routers = routers

        link_count = 0
        ext_links = []

        for (i, n) in enumerate(nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            ext_links.append(
                ExtLink(
                    link_id=link_count,
                    ext_node=n,
                    int_node=routers[router_id],
                    latency=link_latency,
                )
            )
            link_count += 1

        network.ext_links = ext_links

        int_links = []

        for i in range(num_routers):
            next_i = (i + 1) % num_routers

            int_links.append(
                IntLink(
                    link_id=link_count,
                    src_node=routers[i],
                    dst_node=routers[next_i],
                    src_outport="Right",
                    dst_inport="Left"
                )
            )
            link_count += 1

            int_links.append(
                IntLink(
                    link_id=link_count,
                    src_node=routers[next_i],
                    dst_node=routers[i],
                    src_outport="Left",
                    dst_inport="Right"
                )
            )
            link_count += 1

        network.int_links = int_links

    def registerTopology(self, options):
        for i in range(options.num_cpus):
            FileSystemConfig.register_node(
                [i], MemorySize(options.mem_size) // options.num_cpus, i
            )
