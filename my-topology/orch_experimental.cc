#include "orch_experimental.h"
#include <list>

// Definitions for OrchExperimental.
// Note: instance() is defined inline in the header.

OrchExperimental::OrchExperimental() {
    // constructor - empty for now
}

OrchExperimental::~OrchExperimental() {
    // destructor - empty for now
}

void OrchExperimental::start_migration() {
    // TODO: implement migration start logic
}

std::list<nf_spec> OrchExperimental::get_vm_nf_list() {
    // TODO: return list of NF specs for VMs
    return std::list<nf_spec>();
}

std::list<nf_spec> OrchExperimental::get_gw_nf_list() {
    // TODO: return list of NF specs for gateways
    return std::list<nf_spec>();
}