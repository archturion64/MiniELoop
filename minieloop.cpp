#include "minieloop.h"

namespace minieloop {

/**
 * @brief timeoutMs arbitrary op timeout value
 */
constexpr static const std::chrono::milliseconds TIMEOUT_MS{1000};

void MiniELoop::start() noexcept
{
    stop();
    shouldStop = false;
    thread = std::thread(std::bind(&MiniELoop::runThread, this));
}

void MiniELoop::stop() noexcept
{
    shouldStop = true;
    ScopeLock lock(mutex);
    condition.notify_all();
    lock.unlock();
    if(thread.joinable())
    {
        thread.join();
    }
}

TaskHandle MiniELoop::createEvent(const TaskPeriod& period,
                                  const TaskWork& work,
                                  const TaskData& data,
                                  const TaskPeriod& initialDelay) noexcept
{
    TaskHandle retVal(0);
    ScopeLock lock(mutex);
    auto iter = pool.emplace(currentHandle,
                             Task{currentHandle,
                                  MonoClock::now()+TaskDuration{initialDelay},
                                  TaskDuration{period},
                                  work,
                                  data});
    if (iter.second)
    {
        queue.insert(iter.first->second);
        retVal = currentHandle++;
    }

    return retVal;
}

bool MiniELoop::destroyEvent(const TaskHandle& handle) noexcept
{
    ScopeLock lock(mutex);
    // remove if scheduled to run
    try {
        queue.erase(std::ref(pool.at(handle)));
    }  catch (const std::out_of_range& oor) {
        (void)oor;
    }

    // remove stored data about it
    bool retVal = pool.erase(handle);

    condition.notify_all();

    return retVal;
}

bool MiniELoop::eventExists(const TaskHandle& handle) const noexcept
{
    ScopeLock lock(mutex);
    return pool.count(handle);
}


void MiniELoop::runThread() noexcept
{
    while (1)
    {
        ScopeLock lock(mutex);
        if(shouldStop)
        {
            break;
        } else if (queue.empty())
        {
            condition.wait_for(lock, TIMEOUT_MS);
        }else{
            const auto entry = queue.begin();
            Task& instance = entry->get();

            const auto waitSpan = std::chrono::duration_cast<std::chrono::milliseconds>
                    (instance.mTimestamp - MonoClock::now());

            if (waitSpan.count() > 0)
            {
                condition.wait_for(lock, std::min(waitSpan, TIMEOUT_MS));
            } else {
                queue.erase(entry);
                if (instance.mWork(instance.mData)) // trigger registered function
                {
                    instance.mTimestamp = instance.mTimestamp + instance.mDuration;
                    queue.insert(instance);
                } else {
                    lock.unlock();  // destroyEvent() uses same lock
                    destroyEvent(instance.mHandle);
                    lock.lock();
                }
            }
        }
    }
}

} // namespace minieloop
