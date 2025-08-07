#include "catch.hpp"
#include "cpm/TimerFD.hpp"
#include <unistd.h>

#include <thread>

#include "cpm/ParticipantSingleton.hpp"
#include "cpm/Writer.hpp"
#include "cpm/get_topic.hpp"

#include <dds/sub/ddssub.hpp>
#include <dds/core/ddscore.hpp>
#include <dds/topic/ddstopic.hpp>
#include "ReadyStatus.hpp"
#include "SystemTrigger.hpp"

/**
 * \test Tests TimerFD accuracy
 * 
 * - Is the timer started after the initial starting time
 * - Does t_now match the expectation regarding offset, period and start values
 * - Is the callback function called shortly after t_now
 * - Is the timer actually stopped when it should be stopped
 * - If the callback function takes longer than period to finish, is this handled correctly
 * \ingroup cpmlib
 */
TEST_CASE( "TimerFD_accuracy" ) {
    //Set the Logger ID
    cpm::Logging::Instance().set_id("test_timerfd_accuracy");

    const uint64_t period = 200000000;
    const uint64_t offset =  5000000;

    const std::string time_name = "asdfg";


    cpm::TimerFD timer(time_name, period, offset, true);

    //Starting time to check for:
    uint64_t starting_time = timer.get_time() + 3000000000;

    //Writer to send system triggers to the timer 
    cpm::Writer<SystemTrigger> timer_system_trigger_writer("systemTrigger", true);
    //Reader to receive ready signals from the timer
    dds::sub::DataReader<ReadyStatus> timer_ready_signal_ready(dds::sub::Subscriber(cpm::ParticipantSingleton::Instance()), 
        cpm::get_topic<ReadyStatus>("readyStatus"),
        (dds::sub::qos::DataReaderQos() << dds::core::policy::Reliability::Reliable()));
    
    //Waitset to wait for any data
    dds::core::cond::WaitSet waitset;
    dds::sub::cond::ReadCondition read_cond(timer_ready_signal_ready, dds::sub::status::DataState::any());
    waitset += read_cond;

    //It usually takes some time for all instances to see each other - wait until then
    std::cout << "Waiting for DDS entity match in Timer Accuracy test" << std::endl << "\t";
    bool wait = true;
    while (wait)
    {
        usleep(100000); //Wait 100ms
        std::cout << "." << std::flush;

        auto matched_pub = dds::sub::matched_publications(timer_ready_signal_ready);

        if (timer_system_trigger_writer.matched_subscriptions_size() >= 1 && matched_pub.size() >= 1)
            wait = false;
    }
    std::cout << std::endl;

    //Variables for CHECKs - only to identify the timer by its id
    std::string source_id;

    //Thread to receive the ready signal and send a start signal afterwards
    std::thread signal_thread = std::thread([&](){

        //Wait for ready signal
        waitset.wait();
        for (auto sample : timer_ready_signal_ready.take()) {
            if (sample.info().valid()) {
                source_id = sample.data().source_id();
                break;
            }
        }

        //Send start signal
        SystemTrigger trigger;
        trigger.next_start(TimeStamp(starting_time));
        timer_system_trigger_writer.write(trigger);
    });


    //Variables for the callback function
    int timer_loop_count = 0;
    uint64_t t_start_prev = 0;

    timer.start([&](uint64_t t_start){
        uint64_t now = timer.get_time();

        //Curent timer should match the expectation regarding starting time and period
        CHECK( now >= starting_time + period * timer_loop_count); 

        if (timer_loop_count == 0) {
            // actual start time is within 3 ms of initial start time
            CHECK( t_start <= starting_time + period + 3000000); 
        }
        CHECK( t_start <= now ); //Callback should not be called before t_start
        CHECK( now <= t_start + 3000000 ); // actual start time is within 3 ms of declared start time
        CHECK( t_start % period == offset ); // start time corresponds to timer definition

        if(timer_loop_count > 0)
        {
            //Fitting to the sleep behaviour, the difference between the periods should match this expression
            CHECK( ((timer_loop_count%3)+1)*period == t_start - t_start_prev); 
        }

        timer_loop_count++;
        if(timer_loop_count > 15) {
            timer.stop();
        }

        t_start_prev = t_start;

        // simluate variable runtime that can be greater than period
        usleep( ((timer_loop_count%3)*period + period/3) / 1000 ); 
    });

    if (signal_thread.joinable()) {
        signal_thread.join();
    }

    // Check that the ready signal matches the expected ready signal
    CHECK(source_id == time_name);
}
