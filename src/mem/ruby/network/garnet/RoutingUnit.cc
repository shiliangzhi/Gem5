/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

#include <iostream>

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

std::vector<int>
RoutingUnit::outportComputeAll(RouteInfo route, int inport,
                               PortDirection inport_dirn)
{
    std::vector<int> outports;
    outports.clear();
    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        int outport = lookupRoutingTable(route.vnet, route.net_dest);
        outports.push_back(outport);
        return outports;
    }
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case ShortXY:
            return outportAll2D(route, inport, inport_dirn);
        case ShortXYZ:
            return outportAll3D(route, inport, inport_dirn);
        default:
            puts("Only ShortXY and ShortXYZ are supported in OBSERVE_ esapce algorithm.");
            assert(false);
    }
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn, 
                            bool get_espace)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); break;
        // any custom algorithm
        case CUSTOM_: outport =
            outportComputeCustom(route, inport, inport_dirn); break;
        case Ring_: outport =
            outportComputeRing(route, inport, inport_dirn); break;
        case Torus2DD_: outport =
            outportComputeTorus2DDeteministic(route, inport, inport_dirn); break;
        case Torus3DD_: outport =
            outportComputeTorus3DDeteministic(route, inport, inport_dirn); break;
        case ShortXY:
            if (get_espace) {
                outport = 
                outportComputeTorus2DDeteministic(route, inport, inport_dirn); break;
            }
            else {
                outport = 
                outportComputeTorus2DShortXY(route, inport, inport_dirn); break;
            }
        case ShortXYZ:
            if (get_espace) {
                outport =
                outportComputeTorus3DDeteministic(route, inport, inport_dirn); break;
            }
            else {
                outport = 
                outportComputeTorus3DShortXY(route, inport, inport_dirn); break;
            }
        case Torus2D2_: outport =
            outportComputeTorus2DTRY2(route, inport, inport_dirn); break;
        case Torus3D2_: outport =
            outportComputeTorus3DTRY2(route, inport, inport_dirn); break;
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    assert(outport != -1);
    return outport;
}

// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
int
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    panic("%s placeholder executed", __FUNCTION__);
}


// Routing algorithm for Ring
// To prevent deadlock, we use set weight 2 for link between
// router 0 and num - 1, other with weight 1
// Every path should eigher end with weight 2 or never go
// through weight 2
int
RoutingUnit::outportComputeRing(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int my_id = m_router->get_id();
    int dest_id = route.dest_router;
    int num_routers = m_router->get_net_ptr()->getNumRouters();

    int left_distance = (my_id - dest_id + num_routers) % num_routers;

    if (inport_dirn == "Right") {
        outport_dirn = "Left";
    }
    else if (inport_dirn == "Left") {
        outport_dirn = "Right";
    }
    else if (left_distance * 2 <= num_routers) {
        // prefer go to left
        if (dest_id < my_id || dest_id == num_routers - 1) {
            outport_dirn = "Left";
        }
        else outport_dirn = "Right";
    }
    else {
        // prefer go to right
        if (dest_id > my_id || dest_id == 0) {
            outport_dirn = "Right";
        }
        else outport_dirn = "Left";
    }

    return m_outports_dirn2idx[outport_dirn];

}

int
RoutingUnit::outportComputeTorus2DDeteministic(RouteInfo route,
                                                int inport,
                                                PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();


    int my_x = my_id % x_length;
    int my_y = my_id / x_length;

    int dest_x = dest_id % x_length;
    int dest_y = dest_id / x_length;

    if (dest_x != my_x){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            // prefer go to left
            if (dest_x < my_x || dest_x == x_length - 1) {
                outport_dirn = "West";
            }
            else outport_dirn = "East";
        }
        else {
            // prefer go to right
            if (dest_x > my_x || dest_x == 0) {
                outport_dirn = "East";
            }
            else outport_dirn = "West";
        }

        return m_outports_dirn2idx[outport_dirn];
    }

    if (dest_y != my_y) {
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            // prefer go to left
            if (dest_y < my_y || dest_y == y_length - 1) {
                outport_dirn = "South";
            }
            else outport_dirn = "North";
        }
        else {
            // prefer go to right
            if (dest_y > my_y || dest_y == 0) {
                outport_dirn = "North";
            }
            else outport_dirn = "South";
        }

        return m_outports_dirn2idx[outport_dirn];
    }
}

int
RoutingUnit::outportComputeTorus3DDeteministic(RouteInfo route,
                                                int inport,
                                                PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();
    int z_length = m_router->get_net_ptr()->getZLength();

    int my_x = my_id % x_length;
    int my_y = (my_id / x_length) % y_length;
    int my_z = my_id / (x_length * y_length);

    int dest_x = dest_id % x_length;
    int dest_y = (dest_id / x_length) % y_length;
    int dest_z = dest_id / (x_length * y_length);

    if (dest_x != my_x){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            // prefer go to left
            if (dest_x < my_x || dest_x == x_length - 1) {
                outport_dirn = "West";
            }
            else outport_dirn = "East";
        }
        else {
            // prefer go to right
            if (dest_x > my_x || dest_x == 0) {
                outport_dirn = "East";
            }
            else outport_dirn = "West";
        }
        return m_outports_dirn2idx[outport_dirn];
    }

    if (dest_y != my_y) {
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            // prefer go to left
            if (dest_y < my_y || dest_y == y_length - 1) {
                outport_dirn = "South";
            }
            else outport_dirn = "North";
        }
        else {
            // prefer go to right
            if (dest_y > my_y || dest_y == 0) {
                outport_dirn = "North";
            }
            else outport_dirn = "South";
        }
        return m_outports_dirn2idx[outport_dirn];
    }

    if (dest_z != my_z) {
        int left_distance = (my_z - dest_z + z_length) % z_length;
        if (left_distance * 2 <= z_length) {
            // prefer go to left
            if (dest_z < my_z || dest_z == z_length - 1) {
                outport_dirn = "Down";
            }
            else outport_dirn = "Up";
        }
        else {
            // prefer go to right
            if (dest_z > my_z || dest_z == 0) {
                outport_dirn = "Up";
            }
            else outport_dirn = "Down";
        }
        return m_outports_dirn2idx[outport_dirn];
    }
}

int 
RoutingUnit::outportComputeTorus2DShortXY(RouteInfo route,
                                         int inport,
                                         PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();


    int my_x = my_id % x_length;
    int my_y = my_id / x_length;

    int dest_x = dest_id % x_length;
    int dest_y = dest_id / x_length;

    if (dest_x != my_x){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            outport_dirn = "West";
        }
        else {
            outport_dirn = "East";
        }

        return m_outports_dirn2idx[outport_dirn];
    }

    if (dest_y != my_y) {
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            outport_dirn = "South";
        }
        else {
            outport_dirn = "North";
        }

        return m_outports_dirn2idx[outport_dirn];
    }
}

int 
RoutingUnit::outportComputeTorus3DShortXY(RouteInfo route,
                                         int inport,
                                         PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();
    int z_length = m_router->get_net_ptr()->getZLength();

    int my_x = my_id % x_length;
    int my_y = (my_id / x_length) % y_length;
    int my_z = my_id / (x_length * y_length);

    int dest_x = dest_id % x_length;
    int dest_y = (dest_id / x_length) % y_length;
    int dest_z = dest_id / (x_length * y_length);

    if (dest_x != my_x){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            outport_dirn = "West";
        }
        else {
            outport_dirn = "East";
        }
        return m_outports_dirn2idx[outport_dirn];
    }

    if (dest_y != my_y) {
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            outport_dirn = "South";
        }
        else {
            outport_dirn = "North";
        }
        return m_outports_dirn2idx[outport_dirn];
    }

    if (dest_z != my_z) {
        int left_distance = (my_z - dest_z + z_length) % z_length;
        if (left_distance * 2 <= z_length) {
            outport_dirn = "Down";
        }
        else {
            outport_dirn = "Up";
        }
        return m_outports_dirn2idx[outport_dirn];
    }
}

std::vector<int> 
RoutingUnit::outportAll2D(RouteInfo route,
                          int inport,
                          PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();


    int my_x = my_id % x_length;
    int my_y = my_id / x_length;

    int dest_x = dest_id % x_length;
    int dest_y = dest_id / x_length;

    std::vector<int> outports;
    outports.clear();

    if (dest_x != my_x){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            outport_dirn = "West";
        }
        else {
            outport_dirn = "East";
        }

        outports.push_back(m_outports_dirn2idx[outport_dirn]);
    }

    if (dest_y != my_y) {
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            outport_dirn = "South";
        }
        else {
            outport_dirn = "North";
        }

        outports.push_back(m_outports_dirn2idx[outport_dirn]);
    }
    return outports;
}

std::vector<int> 
RoutingUnit::outportAll3D(RouteInfo route,
                          int inport,
                          PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();
    int z_length = m_router->get_net_ptr()->getZLength();

    int my_x = my_id % x_length;
    int my_y = (my_id / x_length) % y_length;
    int my_z = my_id / (x_length * y_length);

    int dest_x = dest_id % x_length;
    int dest_y = (dest_id / x_length) % y_length;
    int dest_z = dest_id / (x_length * y_length);

    std::vector<int> outports;
    outports.clear();

    if (dest_x != my_x){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            outport_dirn = "West";
        }
        else {
            outport_dirn = "East";
        }
        outports.push_back(m_outports_dirn2idx[outport_dirn]);
    }

    if (dest_y != my_y) {
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            outport_dirn = "South";
        }
        else {
            outport_dirn = "North";
        }
        outports.push_back(m_outports_dirn2idx[outport_dirn]);
    }

    if (dest_z != my_z) {
        int left_distance = (my_z - dest_z + z_length) % z_length;
        if (left_distance * 2 <= z_length) {
            outport_dirn = "Down";
        }
        else {
            outport_dirn = "Up";
        }
        outports.push_back(m_outports_dirn2idx[outport_dirn]);
    }
    return outports;
}


long long C(int n, int m)
{
	if (m < n - m) m = n - m;
	long long ans = 1;
	for (int i = m + 1; i <= n; i++) ans *= i;
	for (int i = 1; i <= n - m; i++) ans /= i;
	return ans;
}


int
RoutingUnit::outportComputeTorus3DTRY2(RouteInfo route,
                                        int inport,
                                        PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();
    int z_length = m_router->get_net_ptr()->getZLength();

    int my_x = my_id % x_length;
    int my_y = (my_id / x_length) % y_length;
    int my_z = my_id / (x_length * y_length);

    int dest_x = dest_id % x_length;
    int dest_y = (dest_id / x_length) % y_length;
    int dest_z = dest_id / (x_length * y_length);

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    int z_hops = abs(dest_z - my_z);

    int total_hops = x_hops + y_hops + z_hops;

    double go_x_prob = 0.;
    double go_y_prob = 0.;
    double go_z_prob = 0.;

    if (x_hops !=0){
        go_x_prob = 1.0 * C((total_hops-1), (x_hops-1)) / C(total_hops, x_hops);
    }
    if (y_hops !=0){
        go_y_prob = 1.0 * C((total_hops-1), (y_hops-1)) / C(total_hops, y_hops);
    }
    if (z_hops !=0){
        go_z_prob = 1.0 * C((total_hops-1), (z_hops-1)) / C(total_hops, z_hops);
    }

    double rand_num = (double)rand() / RAND_MAX;

    if (rand_num < go_x_prob){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            outport_dirn = "West";
        }
        else {
            outport_dirn = "East";
        }
    }
    else if (rand_num < go_x_prob + go_y_prob){
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            outport_dirn = "South";
        }
        else {
            outport_dirn = "North";
        }
    }
    else {
        int left_distance = (my_z - dest_z + z_length) % z_length;
        if (left_distance * 2 <= z_length) {
            outport_dirn = "Down";
        }
        else {
            outport_dirn = "Up";
        }
    }
    return m_outports_dirn2idx[outport_dirn];

}

int
RoutingUnit::outportComputeTorus2DTRY2(RouteInfo route,
                                        int inport,
                                        PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_id = m_router->get_id();
    int dest_id = route.dest_router;

    int x_length = m_router->get_net_ptr()->getXLength();
    int y_length = m_router->get_net_ptr()->getYLength();

    int my_x = my_id % x_length;
    int my_y = (my_id / x_length) % y_length;

    int dest_x = dest_id % x_length;
    int dest_y = (dest_id / x_length) % y_length;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    int total_hops = x_hops + y_hops;

    double go_x_prob = 0.;
    double go_y_prob = 0.;

    if (x_hops !=0){
        go_x_prob = 1.0 * C((total_hops-1), (x_hops-1)) / C(total_hops, x_hops);
    }
    if (y_hops !=0){
        go_y_prob = 1.0 * C((total_hops-1), (y_hops-1)) / C(total_hops, y_hops);
    }

    double rand_num = (double)rand() / RAND_MAX;

    if (rand_num < go_x_prob){
        int left_distance = (my_x - dest_x + x_length) % x_length;
        if (left_distance * 2 <= x_length) {
            outport_dirn = "West";
        }
        else {
            outport_dirn = "East";
        }
    }
    else if (rand_num < go_x_prob + go_y_prob){
        int left_distance = (my_y - dest_y + y_length) % y_length;
        if (left_distance * 2 <= y_length) {
            outport_dirn = "South";
        }
        else {
            outport_dirn = "North";
        }
    }
    return m_outports_dirn2idx[outport_dirn];

}


} // namespace garnet
} // namespace ruby
} // namespace gem5