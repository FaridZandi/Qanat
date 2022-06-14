#ifndef my_utility_h
#define my_utility_h

class Packet; 

void print_time();
int get_random_transfer_size(int mean, int range_p);
void convert_path(Packet* p);
void add_to_path(Packet* p, int addr);
void set_high_prio(Packet* p);
void unset_high_prio(Packet* p);



#endif