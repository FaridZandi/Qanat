#include "mig_manager_experimental.h"

MigrationManagerExperimental::MigrationManagerExperimental() {
}

MigrationManagerExperimental::~MigrationManagerExperimental() {
}

bool MigrationManagerExperimental::pre_classify(Packet* p, Handler* h, Node* n) {
    return false;
}

int MigrationManagerExperimental::activate_tunnel(Node* in, Node* out, Node* from, Node* to) {
    return -1;
}

void MigrationManagerExperimental::deactivate_tunnel(int uid) {
}
