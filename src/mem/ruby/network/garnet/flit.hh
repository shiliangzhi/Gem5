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


#ifndef __MEM_RUBY_NETWORK_GARNET_0_FLIT_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FLIT_HH__

#include <cassert>
#include <iostream>

#include "base/types.hh"
#include "mem/ruby/network/garnet/CommonTypes.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

class flit
{
  public:
    flit() {}
    flit(int packet_id, int id, int vc, int vnet, RouteInfo route, int size,
         MsgPtr msg_ptr, int MsgSize, uint32_t bWidth, Tick curTime);

    virtual ~flit(){};

    int get_outport() {return m_outport; }
    int get_size() { return m_size; }
    Tick get_enqueue_time() { return m_enqueue_time; }
    Tick get_dequeue_time() { return m_dequeue_time; }
    int getPacketID() { return m_packet_id; }
    int get_id() { return m_id; }
    Tick get_time() { return m_time; }
    int get_vnet() { return m_vnet; }
    int get_vc() { return m_vc; }
    int get_must_vc() { return m_must_vc; }
    int get_common_outport() {return m_common_outport;}
    int get_espace_outport() {return m_espace_outport;}
    int get_state() {return m_state;}
    int get_num_escape_vc() { return num_escape_vc; }
    RouteInfo get_route() { return m_route; }
    MsgPtr& get_msg_ptr() { return m_msg_ptr; }
    flit_type get_type() { return m_type; }
    std::pair<flit_stage, Tick> get_stage() { return m_stage; }
    Tick get_src_delay() { return src_delay; }

    void set_outport(int port) { m_outport = port; }
    void set_time(Tick time) { m_time = time; }
    void set_vc(int vc) { m_vc = vc; }
    void set_must_vc(int vc) { m_must_vc = vc; }
    void set_route(RouteInfo route) { m_route = route; }
    void set_src_delay(Tick delay) { src_delay = delay; }
    void set_dequeue_time(Tick time) { m_dequeue_time = time; }
    void set_enqueue_time(Tick time) { m_enqueue_time = time; }
    void set_common_outport(int outport) { m_common_outport = outport; }
    void set_espace_outport(int outport) { m_espace_outport = outport;}
    void set_state(int state) { m_state = state; }
    void set_num_escape_vc(int num_vc) { num_escape_vc = num_vc; }
    void clear_possible_outport() { possible_port.clear(); }
    void insert_port(int port) { possible_port.push_back(port); }
    std::vector<int> get_all_outport() { return possible_port; }

    void increment_hops() { m_route.hops_traversed++; }
    virtual void print(std::ostream& out) const;

    bool
    is_stage(flit_stage stage, Tick time)
    {
        return (stage == m_stage.first &&
                time >= m_stage.second);
    }

    void
    advance_stage(flit_stage t_stage, Tick newTime)
    {
        m_stage.first = t_stage;
        m_stage.second = newTime;
    }

    static bool
    greater(flit* n1, flit* n2)
    {
        if (n1->get_time() == n2->get_time()) {
            //assert(n1->flit_id != n2->flit_id);
            return (n1->get_id() > n2->get_id());
        } else {
            return (n1->get_time() > n2->get_time());
        }
    }

    bool functionalRead(Packet *pkt, WriteMask &mask);
    bool functionalWrite(Packet *pkt);

    virtual flit* serialize(int ser_id, int parts, uint32_t bWidth);
    virtual flit* deserialize(int des_id, int num_flits, uint32_t bWidth);

    uint32_t m_width;
    int msgSize;
  protected:
    int m_packet_id;
    int m_id;
    int m_vnet;
    int m_vc;

    int m_state; // 0: in common vc, 1: in espace vc
    int m_common_outport, m_espace_outport, num_escape_vc;
    int m_must_vc;

    std::vector<int> possible_port;

    RouteInfo m_route;
    int m_size;
    Tick m_enqueue_time, m_dequeue_time;
    Tick m_time;
    flit_type m_type;
    MsgPtr m_msg_ptr;
    int m_outport;
    Tick src_delay;
    std::pair<flit_stage, Tick> m_stage;
};

inline std::ostream&
operator<<(std::ostream& out, const flit& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_FLIT_HH__
