#ifndef MINIELOOP_H
#define MINIELOOP_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <map>
#include <set>
#include <cstdint>
#include <atomic>
#include <functional>
#include <chrono>
#include <memory>

using TaskHandle = uint64_t;
using TaskDuration = std::chrono::milliseconds;
using TaskData = std::shared_ptr<void>;
using TaskWork = std::function<bool(TaskData)>;
using MonoClock = std::chrono::steady_clock;
using Timestamp = std::chrono::time_point<MonoClock>;

class SimpleTask
{

public:

    template<typename T>
    SimpleTask(TaskHandle handle, Timestamp startTime, TaskDuration dur, T&& work, TaskData data) noexcept
        : mHandle(handle)
        , mTimestamp(startTime)
        , mDuration(dur)
        , mWork(std::forward<T>(work))
        , mData(data)
        , mIsRunning(false)
    {}

    TaskHandle      mHandle;
    Timestamp       mTimestamp;
    TaskDuration    mDuration;
    TaskWork        mWork;
    TaskData        mData;
    bool            mIsRunning;

};

using TaskWrap = std::reference_wrapper<SimpleTask>;
using TaskPeriod = int64_t;
using ScopeLock = std::unique_lock<std::mutex>;

struct TaskWrapComp {
   bool operator()( const TaskWrap& a, const TaskWrap& b ) const
   {
       return a.get().mTimestamp < b.get().mTimestamp;
   }
};

/**
 * @brief The SimpleTimer class
 *
 * call timer with calling frequency in ms and function ptr
 *
 */
class MiniELoop
{
public:
    void start(void) noexcept;
    void stop(void) noexcept;

    TaskHandle createEvent(const TaskPeriod& period, const TaskWork& work, const TaskData data, const TaskPeriod &initialDelay = 0) noexcept;
    bool destroyEvent(const TaskHandle& handle) noexcept;
    bool eventExists(const TaskHandle& handle) const noexcept;

private:
    // queue for task execution
    using TimerQueue = std::multiset<TaskWrap, TaskWrapComp>;
    TimerQueue mQueue;

    // map for storing registered tasks
    using ActiveTaskPool = std::map<TaskHandle, SimpleTask>;
    ActiveTaskPool mActiveTasks;

    TaskHandle mNextTimerHandle{1};

    // locks and wakes
    mutable std::mutex mtx;
    std::condition_variable mWakeUp;
    std::thread mThread;

    // Thread and exit flag
    std::atomic_bool mShouldStop{false};

    void startThread(void);
};


#endif // MINIELOOP_H

