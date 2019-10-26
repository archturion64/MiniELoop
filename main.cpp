#include <iostream>
#include <unistd.h>
#include "minieloop.h"

/**
 * @brief The DataForTask struct some data that is needed during timer execution
 */
struct DataForTask
{
    DataForTask() noexcept
    {
        std::cout << "Created new DataForTask" << std::endl;
    }

    ~DataForTask() noexcept
    {
        std::cout << "Destroying DataForTask" << std::endl;
    }

};

using namespace minieloop;
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
             },  std::make_shared<DataForTask>(), 10000);

    while (1)
    {
        sleep(1);
    }
}

