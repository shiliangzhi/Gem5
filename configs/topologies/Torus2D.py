from m5.params import *
from m5.objects import *

from common import FileSystemConfig

from topologies.BaseTopology import SimpleTopology

class Torus2D(SimpleTopology):
	description = "Torus2D"

	def __init__(self, controllers):
		self.nodes = controllers

	def makeTopology(self, options, network, IntLink, ExtLink, Router):
		nodes = self.nodes

		num_routers = options.num_cpus

		x_length = options.mesh_rows

		assert x_length != 0, "In 2D dim topology, row number can't be 0"

		y_length = int(num_routers / x_length)

		assert x_length * y_length == num_routers, "num_rows * num_cols must equal num_routers"

		network.x_length = x_length
		network.y_length = y_length

		link_latency = options.link_latency
		router_latency = options.router_latency

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

		# north outport
		for i in range(num_routers):
			north_i = (i + x_length) % num_routers

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[north_i],
					src_outport="North",
					dst_inport="South",
					latency=link_latency,
				)
			)
			link_count += 1

		# south outport
		for i in range(num_routers):
			south_i = (i - x_length + num_routers) % num_routers

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[south_i],
					src_outport="South",
					dst_inport="North",
					latency=link_latency,
				)
			)
			link_count += 1
		
		# east outport
		for i in range(num_routers):
			east_i = (i + 1) % x_length + (i // x_length) * x_length

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[east_i],
					src_outport="East",
					dst_inport="West",
					latency=link_latency,
				)
			)
			link_count += 1

		# west outport
		for i in range(num_routers):
			west_i = (i - 1 + x_length) % x_length + (i // x_length) * x_length

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[west_i],
					src_outport="West",
					dst_inport="East",
					latency=link_latency,
				)
			)
			link_count += 1

		network.int_links = int_links

	def registerTopology(self, options):
		for i in range(options.num_cpus):
			FileSystemConfig.register_node(
				[i], MemorySize(options.mem_size) // options.num_cpus, i
			)