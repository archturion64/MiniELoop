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

namespace minieloop {

using TaskHandle = uint64_t;
using TaskDuration = std::chrono::milliseconds;
using TaskData = std::shared_ptr<void>;
using TaskWork = std::function<bool(TaskData)>;
using MonoClock = std::chrono::steady_clock;
using Timestamp = std::chrono::time_point<MonoClock>;

/**
 * @brief The Task struct is all the information the timer needs in order to function
 */
struct Task
{
    Task(const TaskHandle& handle,
         const Timestamp& startTime,
         const TaskDuration& dur,
         const TaskWork& work,
         const TaskData& data) noexcept
        : mHandle{handle}
        , mTimestamp{startTime}
        , mDuration{dur}
        , mWork{work}
        , mData{data}
    {}

    TaskHandle      mHandle;
    Timestamp       mTimestamp;
    TaskDuration    mDuration;
    TaskWork        mWork;
    TaskData        mData;
};

using TaskWrap = std::reference_wrapper<Task>;
using TaskPeriod = int64_t;
using ScopeLock = std::unique_lock<std::mutex>;
using ActiveTaskPool = std::map<TaskHandle, Task>;

/**
 * @brief The TaskWrapComp struct is a functor that helps sort the timers in the event queue
 */
struct TaskWrapComp {
    bool operator()( const TaskWrap& a, const TaskWrap& b ) const
    {
        return a.get().mTimestamp < b.get().mTimestamp;
    }
};

using TimerQueue = std::multiset<TaskWrap, TaskWrapComp>;

/**
 * @brief The MiniELoop class
 * a glib inspired event loop implementation written in c++
 */
class MiniELoop
{
public:
    /**
     * @brief start - run the loop, timers trigger only on a running event loop
     */
    void start(void) noexcept;

    /**
     * @brief stop - cancle timer execution, will not clear pending tasks
     */
    void stop(void) noexcept;

    /**
     * @brief createEvent creates a timer
     * @param period after which work shall be invoked
     * @param work a function poiner to the work that it to be done.
     * if afterwards the work function returns false, the timer will not reschedule.
     * @param data non trivial destructables must supply own destructor
     * @param initialDelay delay in ms after which timer will start for the first time
     * @return a timer handle which can be used to address / modify the timer or
     * 0 if timer creation failed
     */
    TaskHandle createEvent(const TaskPeriod& period,
                           const TaskWork& work,
                           const TaskData& data,
                           const TaskPeriod& initialDelay = 0) noexcept;

    /**
     * @brief destroyEvent
     * @param handle
     * @return
     */
    bool destroyEvent(const TaskHandle& handle) noexcept;

    /**
     * @brief eventExists
     * @param handle unique id returned on timer creation
     * @return true if timer stil exists
     */
    bool eventExists(const TaskHandle& handle) const noexcept;


private:
    /**
     * @brief shoudlStop - break condition for the loop
     */
    bool shouldStop{false};

    /**
     * @brief queue contains refs to the pool of all the tasks that
     * actualy / eventualy will execute
     */
    TimerQueue queue;

    /**
     * @brief pool storage of all created tasks
     */
    ActiveTaskPool pool;

    /**
     * currentHandle keeps track of unique handles generated
     */
    TaskHandle currentHandle{1};

    mutable std::mutex mutex;
    std::condition_variable condition;
    std::thread thread;

    void runThread(void) noexcept;
};

} // namespace minieloop

#endif // MINIELOOP_H

