#include <iostream>
#include <unistd.h>
#include "minieloop.h"

int main()
{
    MiniELoop t;
    t.start();

    // Timer fires every second, starting five seconds from now
    t.createEvent( 1000,
             [](TaskData) {
                 std::cout << "Timer fired 0" << std::endl;
                 return true;
             }, nullptr);
    // Timer fires every second, starting now
    t.createEvent( 500,
             [](TaskData) {
                 std::cout << "Timer fired 1" << std::endl;
                 return true;
             }, nullptr);
    // Timer fires every 100ms, starting now
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

