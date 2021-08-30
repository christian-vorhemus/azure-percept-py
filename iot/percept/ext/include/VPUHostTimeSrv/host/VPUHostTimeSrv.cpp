/*
 * VPUHostTimeSrv.cpp
 *
 *  Created on: Sep 2, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#include <mutex>
#include <thread>
#include <chrono>
#include <cassert>
#include <condition_variable>

#include <VpualDispatcher.h>
#include <VpualMessage.h>

#include "VPUHostTimeSrv.hpp"
#include "VPUHostTimeSrvDefs.hpp"

#define MVLOG_UNIT_NAME VPUHostTime
#include <mvLog.h>

// Defines
// ----------------------------------------------------------------------------
#ifndef VPU_HOST_TIME_UPDATE_INTERVAL
#define VPU_HOST_TIME_UPDATE_INTERVAL (60) // 1 minute
#endif // VPU_HOST_TIME_UPDATE_INTERVAL

namespace vpual
{
namespace hosttimesrv
{

// Local data
// ----------------------------------------------------------------------------
static vpual::core::Stub *p_stub;

// Update thread synchronization
static bool stopped = true;
static std::mutex mutex;
static std::condition_variable cv;
static std::thread update_thread_id;


// Function Implementations
// ----------------------------------------------------------------------------
static int get_clock(struct timespec * p_ts)
{
    int ret = 0;
    ret = clock_gettime(CLOCK_REALTIME, p_ts);
    if (0 != ret) return errno;
    return 0;
}

static int get_time_ns(std::int64_t *p_time_ns)
{
    int ret = 0;
    struct timespec ts = {0, 0};

    ret = get_clock(&ts); if (ret != 0) return ret;

    *p_time_ns = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    mvLog(MVLOG_DEBUG, "current time is %lld (ns)", *p_time_ns);

    return 0;
}

static Ret update_time()
{
    std::int64_t start_time_ns, end_time_ns, exchange_time_ns;
    while(1) {
        core::Message req_msg, resp_msg;

        // Propose the Host time to VPU
        Command cmd = Command::PROPOSE;
        req_msg.serialize(&cmd, sizeof(Command));
        // FIXME: add error handling
        (void)get_time_ns(&start_time_ns);
        req_msg.serialize(&start_time_ns, sizeof(std::int64_t));
        p_stub->dispatch(req_msg, resp_msg);
        // FIXME: add response parsing

        // Check total exchange time
        (void)get_time_ns(&end_time_ns);
        exchange_time_ns = end_time_ns-start_time_ns;
        if (exchange_time_ns > 10 * 1000 * 1000) {
            mvLog(MVLOG_WARN, "Updating host time took %lld (ns); Retrying...",
                    exchange_time_ns);
        }
        else {
            mvLog(MVLOG_DEBUG, "Updating host time took %lld (ns)",
                    exchange_time_ns);
            break;
        }
    }

    // Update the Host time on VPU
    core::Message req_msg, resp_msg;
    Command cmd = Command::UPDATE;
    req_msg.serialize(&cmd, sizeof(Command));
    // Adjust the proposed time with half the total exchange time
    std::int64_t adjust_time_ns = exchange_time_ns / 2;
    req_msg.serialize(&adjust_time_ns, sizeof(std::int64_t));
    p_stub->dispatch(req_msg, resp_msg);

    return Ret::SUCCESS;
}

static void update_thread()
{
    while (1) {
        // Get current time
        auto now = std::chrono::system_clock::now();

        std::unique_lock<std::mutex> lock(mutex);

        // Wait for timeout or for stop
        auto stop_cond = cv.wait_until(lock,now +
                std::chrono::seconds(VPU_HOST_TIME_UPDATE_INTERVAL),
                [](){ return stopped == true; });

        // Check for stop condition
        if (stop_cond) {
            break;
        }

        // Update time
        (void)update_time();
    }
}

Ret start()
{
    mvLogLevelSet(MVLOG_WARN);
    mvLog(MVLOG_INFO, "Starting VPUHostTimeSrv");

    // Create stub
    p_stub = new core::Stub("HostTimeSrv");
    assert(nullptr != p_stub);

    (void)update_time();

    stopped = false;
    update_thread_id = std::thread(update_thread);

    return Ret::SUCCESS;
}

Ret stop()
{
    // Notify update thread to stop
    stopped = true;
    cv.notify_all();

    update_thread_id.join();

    return Ret::SUCCESS;
}

} // namespace hosttimesrv
} // namespace vpual
