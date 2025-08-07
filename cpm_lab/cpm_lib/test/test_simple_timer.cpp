#include "catch.hpp"
#include "cpm/SimpleTimer.hpp"
#include <unistd.h>

#include <thread>

#include "cpm/Writer.hpp"
#include "cpm/ParticipantSingleton.hpp"
#include "cpm/get_topic.hpp"

#include <dds/sub/ddssub.hpp>
#include <dds/core/ddscore.hpp>
#include <dds/topic/ddstopic.hpp>
#include "ReadyStatus.hpp"
#include "SystemTrigger.hpp"

/**
 * \test Tests SimpleTimer
 * 
 * - Is the timer started after the initial starting time
 * - Does t_now match the expectation regarding offset, period and start values
 * - Is the callback function called shortly after t_now
 * - Is the timer actually stopped when it should be stopped
 * - If the callback function takes longer than period to finish, is this handled correctly
 * \ingroup cpmlib
 */
TEST_CASE( "SimpleTimer functionality" ) {
    //Set the Logger ID
    cpm::Logging::Instance().set_id("test_simple_timer");

    const uint64_t period = 100; //every 100ms

    const std::string time_name = "asdfg";


    cpm::SimpleTimer timer(time_name, period, true);

    //Starting time to check for:
    uint64_t starting_time = timer.get_time() + 2000000000;

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

    //Variables for CHECKs - only to identify the timer by its id
    std::string source_id;

    //It usually takes some time for all instances to see each other - wait until then
    std::cout << "Waiting for DDS entity match in Simple Timer test" << std::endl << "\t";
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

    uint64_t period_ns = period * 1000000;

    timer.start([&](uint64_t t_start){
        uint64_t now = timer.get_time();

        //Curent timer should match the expectation regarding starting time and period
        CHECK( now >= starting_time + period_ns * timer_loop_count); 

        if (timer_loop_count == 0) {
            // actual start time is within 2 ms of initial start time
            CHECK( t_start <= starting_time + period_ns + 2000000); 
        }

        timer_loop_count++;
        if(timer_loop_count > 15) {
            timer.stop();
        }

        t_start_prev = t_start;

        // simluate variable runtime that can be greater than period
        usleep( ((timer_loop_count%3)*period_ns + period_ns/3) / 1000 ); 
    });

    if (signal_thread.joinable()) {
        signal_thread.join();
    }

    // Check that the ready signal matches the expected ready signal
    CHECK(source_id == time_name);
}
