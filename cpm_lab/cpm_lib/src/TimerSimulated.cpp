#include "TimerSimulated.hpp"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/timerfd.h>
#include <unistd.h>
#include <cmath>

/**
 * \file TimerSimulated.cpp
 * \ingroup cpmlib
 */

namespace cpm {


    TimerSimulated::TimerSimulated(
        std::string _node_id, 
        uint64_t _period_nanoseconds, 
        uint64_t _offset_nanoseconds
    )
    :period_nanoseconds(_period_nanoseconds)
    ,offset_nanoseconds(_offset_nanoseconds)
    ,writer_ready_status("readyStatus", true)
    ,reader_system_trigger(dds::sub::Subscriber(cpm::ParticipantSingleton::Instance()), cpm::get_topic<SystemTrigger>("systemTrigger"), (dds::sub::qos::DataReaderQos() << dds::core::policy::Reliability::Reliable()))
    ,node_id(_node_id)
    ,current_time(0)
    {
        //Offset is allowed to be greater than period!

        dds::sub::cond::ReadCondition read_cond(reader_system_trigger, dds::sub::status::DataState::any());
        waitset += read_cond;

        active.store(false);
        cancelled.store(false);
    }

    TimerSimulated::Answer TimerSimulated::handle_system_trigger(uint64_t& deadline) {
        bool got_valid = false;
        bool got_new_deadline = false;

        for (auto sample : reader_system_trigger.take()) {
            if (sample.info().valid()) {
                got_valid = true;

                if (sample.data().next_start().nanoseconds() == deadline) {
                    current_time = deadline;

                    //Current deadline reached -> perform calculation, call callback, update deadline
                    if(m_update_callback) m_update_callback(deadline);
                    deadline += period_nanoseconds;

                    got_new_deadline = true;

                    // Current period finished -> Send next ready signal
                    ReadyStatus ready_status;
                    ready_status.next_start_stamp(TimeStamp(deadline));
                    ready_status.source_id(node_id);
                    writer_ready_status.write(ready_status);
                }
                else if (sample.data().next_start().nanoseconds() == TRIGGER_STOP_SYMBOL) {
                    //Received stop signal - use the user's stop function or stop the timer if none was registered
                    if (m_stop_callback)
                    {
                        m_stop_callback();
                    }
                    else
                    {
                        active.store(false);
                    }
                    
                    return Answer::STOP;
                }
            }
        }

        if (got_new_deadline) {
            return Answer::DEADLINE;
        }
        else if (got_valid) {
            return Answer::ANY;
        }
        else {
            return Answer::NONE;
        }
    }

    void TimerSimulated::start(std::function<void(uint64_t t_now)> update_callback)
    {
        if(active.load()) {
            Logging::Instance().write(
                2,
                "%s", 
                "TimerSimulated: The cpm::Timer can not be started twice."
            );
            throw cpm::ErrorTimerStart("The cpm::Timer can not be started twice.");
        }

        active.store(true);
        
        //In the rare case that active was set too early to false in stop / deconstructor: we must check that these have already been called
        if (cancelled.load())
        {
            return;
        }


        m_update_callback = update_callback;
        
        uint64_t deadline = offset_nanoseconds;
        current_time = deadline;

        //Send first ready signal until any signal has been received, process answer if necessary
        Answer system_trigger = Answer::NONE;
        while (system_trigger == Answer::NONE && active.load()) {
            ReadyStatus ready_status;
            ready_status.next_start_stamp(TimeStamp(deadline));
            ready_status.source_id(node_id);
            writer_ready_status.write(ready_status); 
            waitset.wait(dds::core::Duration::from_millisecs(2000));

            system_trigger = handle_system_trigger(deadline);
        }

        //Wait for the next relevant time step and call the callback function until the process has been stopped
        while(active.load()) {
            //Wait for the next signals until the next start signal has been received
            dds::core::cond::WaitSet::ConditionSeq active_conditions = waitset.wait();

            //Process new messages
            system_trigger = handle_system_trigger(deadline);
        }
    }

    void TimerSimulated::start(std::function<void(uint64_t t_now)> update_callback, std::function<void()> stop_callback)
    {
        m_stop_callback = stop_callback;
        start(update_callback);
    }

    void TimerSimulated::start_async(std::function<void(uint64_t t_now)> update_callback)
    {
        if(!runner_thread.joinable())
        {
            m_update_callback = update_callback;
            runner_thread = std::thread([this](){
                this->start(m_update_callback);
            });
        }
        else
        {
            Logging::Instance().write(
                2,
                "%s", 
                "TimerSimulated: The cpm::Timer can not be started twice."
            );
            throw cpm::ErrorTimerStart("The cpm::Timer can not be started twice.");
        }
    }

    void TimerSimulated::start_async(std::function<void(uint64_t t_now)> update_callback, std::function<void()> stop_callback)
    {
        m_stop_callback = stop_callback;
        start_async(update_callback);
    }

    void TimerSimulated::stop()
    {
        std::lock_guard<std::mutex> lock(join_mutex);

        cancelled.store(true);
        active.store(false);
        if(runner_thread.joinable())
        {
            runner_thread.join();
        }

        cancelled.store(false);
    }

    TimerSimulated::~TimerSimulated()
    {
        std::lock_guard<std::mutex> lock(join_mutex);

        cancelled.store(true);
        active.store(false);
        if(runner_thread.joinable())
        {
            runner_thread.join();
        }

        cancelled.store(false);
    }


    uint64_t TimerSimulated::get_time()
    {
        return current_time;
    }

    uint64_t TimerSimulated::get_start_time()
    {
        //For a simulated timer, 0 is always the starting point of the simulation
        return 0;
    }

}