#include "mig_manager_alltogether.h"

MigrationManagerAllTogether::MigrationManagerAllTogether() {
}

MigrationManagerAllTogether::~MigrationManagerAllTogether() {
}

bool MigrationManagerAllTogether::pre_classify(Packet* p, Handler* h, Node* n) {
    return false;
}

int MigrationManagerAllTogether::activate_tunnel(Node* in, Node* out, Node* from, Node* to) {
    return -1;
}

void MigrationManagerAllTogether::deactivate_tunnel(int uid) {
}
