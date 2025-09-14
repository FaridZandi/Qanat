#include "mig_manager_bottom_up.h"
#include "my_topology.h"
#include "orchestrator.h"
#include "node.h"
#include "packet.h"
#include "tcp-full.h"
#include "utility.h"

#include <iostream>

int MigrationManagerBottomUp::tunnel_uid_counter = 0;

MigrationManagerBottomUp::MigrationManagerBottomUp() : MigrationManager() {
    tunnels = new tunnel_data[tunnel_count];
    for (int i = 0; i < tunnel_count; ++i) {
        tunnels[i].valid = false;
        tunnels[i].uid = -1;
    }
    verbose = MyTopology::verbose_mig;
    active_tunnels = 0; // informational only; loops do not rely on this
}

MigrationManagerBottomUp::~MigrationManagerBottomUp() {
    delete[] tunnels;
}

// ————————————————————————————————————————————————————————————————
// Basics: create/remove tunnels
// ————————————————————————————————————————————————————————————————

int MigrationManagerBottomUp::activate_tunnel(Node* in, Node* out, Node* from, Node* to) {
    tunnel_data td{};
    td.valid = true;
    td.in    = in;
    td.out   = out;
    td.from  = from;
    td.to    = to;
    td.uid   = MigrationManagerBottomUp::tunnel_uid_counter++;

    add_tunnel(td);
    return td.uid;
}

void MigrationManagerBottomUp::deactivate_tunnel(int uid) {
    for (int i = 0; i < MigrationManagerBottomUp::tunnel_count; ++i) {
        if (tunnels[i].valid && tunnels[i].uid == uid) {
            tunnels[i].valid = false;
            tunnels[i].uid   = -1;
            if (active_tunnels > 0) active_tunnels--; // keep a rough count
            break;
        }
    }
}

void MigrationManagerBottomUp::add_tunnel(tunnel_data tunnel) {
    for (int i = 0; i < MigrationManagerBottomUp::tunnel_count; ++i) {
        if (!tunnels[i].valid) {
            tunnels[i] = tunnel;
            active_tunnels++;
            return;
        }
    }
    std::cerr << "[MigrationManagerBottomUp] Not enough space to store tunnels. Exiting!\n";
    std::exit(1);
}

// ————————————————————————————————————————————————————————————————
// Lightweight helpers / logging
// ————————————————————————————————————————————————————————————————

bool MigrationManagerBottomUp::should_ignore(Packet* p) {
    // Ignore background traffic (traffic_class == 2); everything else is migration-sensitive.
    hdr_ip* iph = hdr_ip::access(p);
    return (iph->traffic_class == 2);
}

void MigrationManagerBottomUp::log_packet(Packet* p) {
    if (!verbose) return;
    hdr_ip* iph = hdr_ip::access(p);
    std::cout << "pre_classify "
              << iph->src_.addr_ << " -> " << get_packet_dst(p)
              << " prio=" << iph->prio_ << " class=" << iph->traffic_class
              << std::endl;
}

void MigrationManagerBottomUp::log_tunnel(tunnel_data td, Tunnel_Point tp, Packet* /*p*/) {
    if (!verbose || tp == Tunnel_Point::Tunnel_None) return;

    std::cout << "[tunnel " << td.uid << "] ";
    switch (tp) {
        case Tunnel_Point::Tunnel_From: std::cout << "From detected"; break;
        case Tunnel_Point::Tunnel_To:   std::cout << "To detected";   break;
        case Tunnel_Point::Tunnel_In:   std::cout << "In detected";   break;
        case Tunnel_Point::Tunnel_Out:  std::cout << "Out detected";  break;
        default:                        std::cout << "Unknown";       break;
    }
    std::cout << std::endl;
}

// ————————————————————————————————————————————————————————————————
// Core classification / matching
// ————————————————————————————————————————————————————————————————

Tunnel_Point MigrationManagerBottomUp::packet_match(tunnel_data td, Packet* p, Node* n) {
    if (!td.valid) return Tunnel_Point::Tunnel_None;

    hdr_ip* iph = hdr_ip::access(p);

    const auto t_in   = td.in->address();
    const auto t_out  = td.out->address();
    const auto t_from = td.from->address();
    const auto t_to   = td.to->address();

    const auto p_src  = iph->src_.addr_;
    const auto p_dst  = get_packet_dst(p);
    const auto n_addr = n->address();

    // NOTE: comparison logic mirrors original behavior but with clearer structure.
    if (n_addr == t_in) {
        // Packet at TUNNEL_IN node:
        if (p_dst == t_from || p_src == t_to) return Tunnel_Point::Tunnel_In;
    } else if (n_addr == t_out) {
        // Packet at TUNNEL_OUT node (used for early redirection):
        if (p_src == t_to || p_dst == t_to) return Tunnel_Point::Tunnel_Out;
    } else if (n_addr == t_from) {
        // Packet at original destination:
        if (p_dst == t_from || p_src == t_from) return Tunnel_Point::Tunnel_From;
    } else if (n_addr == t_to) {
        // Packet at new destination:
        if (p_dst == t_to) return Tunnel_Point::Tunnel_To;
    }

    return Tunnel_Point::Tunnel_None;
}

// This function is obsolete. Should not be used.
Direction MigrationManagerBottomUp::packet_dir(tunnel_data /*td*/, Packet* /*p*/) {
    return Direction::Dir_None;
}

// ————————————————————————————————————————————————————————————————
// pre_classify: main decision point
// ————————————————————————————————————————————————————————————————

bool MigrationManagerBottomUp::pre_classify(Packet* p, Handler* h, Node* n) {
    if (should_ignore(p)) return true;

    log_packet(p);

    // BUGFIX: original looped up to active_tunnels and could miss valid
    // entries if some earlier ones were deactivated. Now we scan full table.
    for (int i = 0; i < MigrationManagerBottomUp::tunnel_count; ++i) {
        if (!tunnels[i].valid) continue;

        auto& td = tunnels[i];
        Tunnel_Point tp = packet_match(td, p, n);

        log_tunnel(td, tp, p);

        switch (tp) {
            case Tunnel_Point::Tunnel_In:   return tunnel_packet_in(td, p, n);
            case Tunnel_Point::Tunnel_Out:  return tunnel_packet_out(td, p, n);
            case Tunnel_Point::Tunnel_From: return handle_packet_from(td, p, h, n);
            case Tunnel_Point::Tunnel_To:   return handle_packet_to  (td, p, h, n);
            case Tunnel_Point::Tunnel_None: default: break; // keep scanning
        }
    }

    // No tunnel handling required
    unset_high_prio(p);
    handle_non_ready_nodes(p, n);
    return true;
}

// ————————————————————————————————————————————————————————————————
// Non-ready nodes (topology/orchestrator integration)
// ————————————————————————————————————————————————————————————————

void MigrationManagerBottomUp::handle_non_ready_nodes(Packet* p, Node* node) {
    auto& topo = MyTopology::instance();
    auto& orch = BaseOrchestrator::instance();

    // top-down (2) or random (3) orchestration types
    if (topo.orch_type != 2 && topo.orch_type != 3) return;

    // Only act inside the destination tree after migration has started
    if (topo.get_data(node).which_tree != 1) return;
    if (!topo.is_migration_started) return;

    const auto node_state = orch.get_mig_state(node);
    if (node_state == MigState::Normal) return;

    // Route via the node's peer with high priority to avoid disruption
    convert_path(p);
    add_to_path(p, topo.get_peer(node)->address());
    set_high_prio(p);
    topo.inc_tunnelled_packets();
}

// ————————————————————————————————————————————————————————————————
// Packet header helpers
// ————————————————————————————————————————————————————————————————

int MigrationManagerBottomUp::get_packet_dst(Packet* p) {
    hdr_ip* iph = hdr_ip::access(p);
    int p_dst = iph->dst_.addr_;
    if (iph->gw_path_pointer != -1) {
        p_dst = iph->gw_path[0]; // keep original behavior
    }
    return p_dst;
}

void MigrationManagerBottomUp::set_packet_src(Packet* p, int src) {
    hdr_ip* iph = hdr_ip::access(p);
    iph->src_.addr_ = src;
}

// ————————————————————————————————————————————————————————————————
// Tunnel actions
// ————————————————————————————————————————————————————————————————

bool MigrationManagerBottomUp::tunnel_packet_in(tunnel_data td, Packet* p, Node* /*n*/) {
    // Send to tunnel_out with high priority; record original path.
    convert_path(p);
    add_to_path(p, td.out->address());
    set_high_prio(p);
    MyTopology::instance().inc_tunnelled_packets();
    return true;
}

bool MigrationManagerBottomUp::tunnel_packet_out(tunnel_data /*td*/, Packet* p, Node* n) {
    // Arrived at tunnel end; restore normal handling.
    unset_high_prio(p);
    handle_non_ready_nodes(p, n);
    return true;
}

// ————————————————————————————————————————————————————————————————
// Handling at old (from) / new (to) destinations
// ————————————————————————————————————————————————————————————————

bool MigrationManagerBottomUp::handle_packet_from(tunnel_data td, Packet* p, Handler* h, Node* n) {
    // Packets at the old destination 'from'
    hdr_ip* iph = hdr_ip::access(p);

    const int p_src  = iph->src_.addr_;
    const int p_dst  = get_packet_dst(p);
    const int n_addr = n->address();

    // Infer direction relative to 'n'
    Direction dir = Direction::Dir_None;
    if (p_dst == n_addr)      dir = Direction::Incoming;
    else if (p_src == n_addr) dir = Direction::Outgoing;

    auto& orch       = BaseOrchestrator::instance();
    auto  node_state = orch.get_mig_state(n);

    if (dir == Direction::Outgoing) {
        // Traffic generated at 'from'
        if (node_state == MigState::Migrated || node_state == MigState::InMig) {
            // Rewrite source to 'to' and hand to its classifier
            iph->src_.addr_ = td.to->address();
            td.to->get_classifier()->recv(p, h);
            return false; // consumed
        }
        return true; // normal egress
    }

    if (dir == Direction::Incoming) {
        // Traffic destined to 'from'
        if (node_state == MigState::Migrated || node_state == MigState::InMig) {
            // Reroute to 'to' with high priority
            iph->dst_.addr_ = td.to->address();
            set_high_prio(p);
            MyTopology::instance().inc_tunnelled_packets();
            return true; // continue
        } else {
            // First hop at manager should pass through once, then use recv2
            if (!iph->skip_first_mngr_flag) {
                iph->skip_first_mngr_flag = true;
                return true;
            }
            n->get_classifier()->recv2(p, h);
            return false; // consumed
        }
    }

    // Should not happen
    if (verbose) {
        std::cout << "[MigrationManagerBottomUp] Unexpected path in handle_packet_from at node "
                  << n->address() << std::endl;
    }
    return true;
}

bool MigrationManagerBottomUp::handle_packet_to(tunnel_data td, Packet* p, Handler* h, Node* n) {
    // Packets at the new destination 'to'
    hdr_ip* iph = hdr_ip::access(p);

    // First encounter: let it pass once as-is (compat with original flag dance)
    if (!iph->skip_first_mngr_flag) {
        iph->skip_first_mngr_flag = true;
        return true;
    }
    iph->skip_first_mngr_flag = false; // reset

    auto& orch       = BaseOrchestrator::instance();
    auto  node_state = orch.get_mig_state(n);

    if (node_state == MigState::Normal) {
        // Not migrated yet: send back to old destination for delivery
        iph->dst_.addr_ = td.from->address();
        td.from->get_classifier()->recv2(p, h);
        return false; // consumed
    } else {
        // During/after migration: use standard handling (with tunnel assist if needed)
        handle_non_ready_nodes(p, n);
        return true;
    }
}
