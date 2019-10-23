#include "minieloop.h"

void MiniELoop::start() noexcept
{
    stop();
    mShouldStop.store(false);
    mThread = std::thread(std::bind(&MiniELoop::startThread, this));
}

void MiniELoop::stop() noexcept
{
    mShouldStop.store(true);
    ScopeLock lock(mtx);
    mWakeUp.notify_all();
    lock.unlock();
    if(mThread.joinable())
    {
        mThread.join();
    }
}

TaskHandle MiniELoop::createEvent(const TaskPeriod& period, const TaskWork &work, const TaskData data, const TaskPeriod& initialDelay) noexcept
{
    TaskHandle retVal(0);
    SimpleTask newTask = SimpleTask(mNextTimerHandle, MonoClock::now()+TaskDuration{initialDelay}, TaskDuration{period}, work, data);

    {
        ScopeLock lock(mtx);
        auto iter = mActiveTasks.emplace(newTask.mHandle, std::move(newTask));

        if (iter.second)
        {
            mQueue.insert(iter.first->second);
            retVal = mNextTimerHandle++;
        }
    }

    return retVal;
}

bool MiniELoop::destroyEvent(const TaskHandle& handle) noexcept
{
    bool retVal(false);

    ScopeLock lock(mtx);
    try {
        mQueue.erase(std::ref(mActiveTasks.at(handle)));
    }  catch (const std::out_of_range& oor) {
        (void)oor;
    }

    mActiveTasks.erase(handle);

    mWakeUp.notify_all();

    return retVal;
}

bool MiniELoop::eventExists(const TaskHandle& handle) const noexcept
{
    ScopeLock lock(mtx);
    return mActiveTasks.count(handle);
}


void MiniELoop::startThread()
{
    while (!mShouldStop.load())
    {
        ScopeLock lock(mtx);
        if (mQueue.empty())
        {
            mWakeUp.wait_for(lock, std::chrono::seconds(1));
        }else{
            const auto entry = mQueue.begin();
            SimpleTask& instance = entry->get();

            // trigger now or wait until time to trigger
            if (MonoClock::now() < instance.mTimestamp)
            {
                mWakeUp.wait_until(lock, instance.mTimestamp);
            } else {
                mQueue.erase(entry);
                if (instance.mWork(instance.mData)) // trigger registered function
                {
                    instance.mTimestamp = instance.mTimestamp + instance.mDuration;
                    mQueue.insert(instance);
                } else {
                    lock.unlock();
                    destroyEvent(instance.mHandle);
                    lock.lock();
                }
            }
        }
    }
    return;
}
