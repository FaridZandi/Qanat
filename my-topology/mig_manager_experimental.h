#ifndef mig_manager_experimental_h
#define mig_manager_experimental_h

#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>
#include "mig_manager.h"

class Node; 
class Packet; 
class Handler;


class MigrationManagerExperimental : public MigrationManager {
public:
    MigrationManagerExperimental(); 
    virtual ~MigrationManagerExperimental(); 

    virtual bool pre_classify(Packet* p, Handler* h, Node* n);
    
    virtual int activate_tunnel(Node* in, Node* out, 
                                Node* from, Node* to);

    virtual void deactivate_tunnel(int uid);
};

#endif
