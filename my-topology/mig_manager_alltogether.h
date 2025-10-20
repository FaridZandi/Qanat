#ifndef mig_manager_alltogether_h
#define mig_manager_alltogether_h

#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>
#include "mig_manager.h"

class Node; 
class Packet; 
class Handler;


class MigrationManagerAllTogether : public MigrationManager {
public:
    MigrationManagerAllTogether(); 
    virtual ~MigrationManagerAllTogether(); 

    virtual bool pre_classify(Packet* p, Handler* h, Node* n);
    
    virtual int activate_tunnel(Node* in, Node* out, 
                                Node* from, Node* to);

    virtual void deactivate_tunnel(int uid);
};

#endif
