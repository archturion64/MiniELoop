#include <iostream>
#include <unistd.h>
#include "minieloop.h"

int main()
{
    MiniELoop t;
    t.start();

    // Cyclic timer fires every 500ms, starting now
    t.createEvent( 500,
             [](TaskData) {
                 std::cout << "Timer fired 1" << std::endl;
                 return true;
             }, nullptr);
    // Oneshot timer fires every 100ms, starting after 10s
    t.createEvent(100,
             [](TaskData) {
                 std::cout << "Timer fired 2" << std::endl;
                 return false;
             }, nullptr, 10000);

    while (1)
    {
        sleep(1);
    }
}

