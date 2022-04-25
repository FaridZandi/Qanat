#include <iostream>
#include <iomanip>
#include "simulator.h"

void print_time(){
    std::cout << "[";
    std::cout << std::setprecision(5);
    std::cout << std::setw(5);
    std::cout << Scheduler::instance().clock();
    std::cout << "] ";
}