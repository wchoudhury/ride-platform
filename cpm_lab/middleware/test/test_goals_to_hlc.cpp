#include "catch.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

#include "cpm/Timer.hpp"
#include "cpm/Parameter.hpp"
#include "cpm/ParticipantSingleton.hpp"
#include "cpm/Logging.hpp"
#include "CommonroadDDSGoalState.hpp"
#include "ReadyStatus.hpp"

#include "Communication.hpp"

/**
 * \test Tests communication of goal states up to virtual HLC
 * 
 * Tests if data sent by a virtual LCC, which sends a goal state, is received by a fake middleware, 
 * and then sent to the HLC
 * \ingroup middleware
 */
TEST_CASE( "GoalToHLCCommunication" ) {
    cpm::Logging::Instance().set_id("middleware_test_log");
    
    //Data for the tests
    CommonroadDDSGoalState state_1;
    CommonroadDDSGoalState state_2;
    state_1.planning_problem_id(1);
    state_2.planning_problem_id(2);

    //Communication parameters - mostly required just to set up the test
    int hlcDomainNumber = 1; 
    std::string goalStateTopicName = "commonroad_dds_goal_states";
    std::string vehicleStateListTopicName = "vehicleStateList"; 
    std::string vehicleTrajectoryTopicName = "vehicleCommandTrajectory"; 
    std::string vehiclePathTrackingTopicName = "vehicleCommandPathTracking"; 
    std::string vehicleSpeedCurvatureTopicName = "vehicleCommandSpeedCurvature"; 
    std::string vehicleDirectTopicName = "vehicleCommandDirect"; 

    //Ignore warning that vehicleID is unused
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"

    int vehicleID = 0; 
    
    #pragma GCC diagnostic pop
    
    std::vector<uint8_t> assigned_vehicle_ids = { 0 };
    std::vector<uint8_t> active_vehicle_ids = { 0, 1 };

    //Timer parameters
    std::string node_id = "middleware_test";
    uint64_t period_nanoseconds = 6000000; //6ms
    uint64_t offset_nanoseconds = 1;
    bool simulated_time_allowed = true;
    bool simulated_time = false;

    //Initialize the timer
    std::shared_ptr<cpm::Timer> timer = cpm::Timer::create(node_id, period_nanoseconds, offset_nanoseconds, false, simulated_time_allowed, simulated_time);

    //Initialize the communication 
    Communication communication(
        hlcDomainNumber,
        vehicleStateListTopicName,
        vehicleTrajectoryTopicName,
        vehiclePathTrackingTopicName,
        vehicleSpeedCurvatureTopicName,
        vehicleDirectTopicName,
        timer,
        assigned_vehicle_ids,
        active_vehicle_ids
    );

    //Initialize readers and writers to simulate HLC (ready messages) and LCC (goal state writer)
    dds::domain::DomainParticipant hlcParticipant = dds::domain::find(hlcDomainNumber);
    cpm::Writer<ReadyStatus> hlc_ready_status_writer(hlcParticipant, "readyStatus", true, true, true);
    cpm::Writer<CommonroadDDSGoalState> vehicleWriter(goalStateTopicName, true, true, true);
    cpm::ReaderAbstract<CommonroadDDSGoalState> hlc_goal_state_reader(hlcParticipant, goalStateTopicName, true, true, false);

    //Sleep for some milliseconds just to make sure that the readers and writers have been initialized properly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //Send test data from a virtual LCC
    vehicleWriter.write(state_1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    vehicleWriter.write(state_2);

    //Send ready message to the middleware from a virtual HLC
    ReadyStatus hlc_ready;
    hlc_ready.source_id("hlc_0");
    hlc_ready_status_writer.write(hlc_ready);

    //Wait for the HLC ready msgs as the middleware does; GoalStates received by the middleware before the ready messages are now sent to the HLC
    communication.wait_for_hlc_ready_msg({0});

    //Now the HLC should have received these messages; wait a bit to make sure this happened
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<int32_t> received_ids;
    for (auto& sample : hlc_goal_state_reader.take())
    {
        received_ids.push_back(sample.planning_problem_id());
    }

    //Perform checks
    CHECK(received_ids.size() == 2);
    CHECK(std::find(received_ids.begin(), received_ids.end(), 1) != received_ids.end());
    CHECK(std::find(received_ids.begin(), received_ids.end(), 2) != received_ids.end());
}
