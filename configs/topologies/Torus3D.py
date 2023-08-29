from m5.params import *
from m5.objects import *

from common import FileSystemConfig

from topologies.BaseTopology import SimpleTopology

class Torus3D(SimpleTopology):
	"""
	x: x_length, west to east
	y: y_length, south to north
	z: z_length, down to up
	"""
	description = "Torus2D"

	def __init__(self, controllers):
		self.nodes = controllers

	def makeTopology(self, options, network, IntLink, ExtLink, Router):
		nodes = self.nodes

		num_routers = options.num_cpus
		
		x_length = options.x_length
		y_length = options.y_length
		z_length = options.z_length


		assert x_length * y_length * z_length == num_routers, "x_length * y_length * z_length must equal num_routers"

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
		# up outport
		for i in range(num_routers):
			up_i = (i + x_length * y_length) % num_routers

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[up_i],
					src_outport="Up",
					dst_inport="Down",
				)
			)
			link_count += 1

		# down outport
		for i in range(num_routers):
			down_i = (i + num_routers - x_length * y_length) % num_routers
			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[down_i],
					src_outport="Down",
					dst_inport="Up",
				)
			)
			link_count += 1
		# north outport
		for i in range(num_routers):
			next_i = (i // (x_length * y_length)) * (x_length * y_length) + (i + x_length) % (x_length * y_length)

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[next_i],
					src_outport="North",
					dst_inport="South",
				)
			)
			link_count += 1

		# south outport
		for i in range(num_routers):
			next_i = (i // (x_length * y_length)) * (x_length * y_length) + (i - x_length + x_length * y_length) % (x_length * y_length)

			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[next_i],
					src_outport="South",
					dst_inport="North",
				)
			)
			link_count += 1
		
		# east outport
		for i in range(num_routers):
			next_i = (i // x_length) * x_length + (i + 1) % x_length
			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[next_i],
					src_outport="East",
					dst_inport="West",
				)
			)
			link_count += 1
		
		# west outport
		for i in range(num_routers):
			next_i = (i // x_length) * x_length + (i - 1 + x_length) % x_length
			int_links.append(
				IntLink(
					link_id=link_count,
					src_node=routers[i],
					dst_node=routers[next_i],
					src_outport="West",
					dst_inport="East",
				)
			)
			link_count += 1
		
		network.int_links = int_links

	def registerTopology(self, options):
		for i in range(options.num_cpus):
			FileSystemConfig.register_node(
				[i], MemorySize(options.mem_size) // options.num_cpus, i
			)

		
		
		