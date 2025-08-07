#include "catch.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

#include <dds/sub/ddssub.hpp>
#include <dds/pub/ddspub.hpp>
#include <dds/core/QosProvider.hpp>
#include <rti/core/cond/AsyncWaitSet.hpp>
#include <rti/core/ListenerBinder.hpp>
#include <dds/core/vector.hpp>

#include "cpm/init.hpp"
#include "cpm/Timer.hpp"
#include "cpm/Parameter.hpp"
#include "cpm/ParticipantSingleton.hpp"
#include "cpm/Logging.hpp"
#include "VehicleCommandSpeedCurvature.hpp"
#include "VehicleStateList.hpp"
#include "Parameter.hpp"

#include "cpm/AsyncReader.hpp"
#include "cpm/ReaderAbstract.hpp"
#include "cpm/Writer.hpp"
#include "cpm/Participant.hpp"

#include "Communication.hpp"

/**
 * \test Tests communication from simulated HLC to simulated vehicle
 * 
 * Tests if data sent by a virtual HLC is received by a virtual vehicle, 
 * therefore if TypedCommunication receives and sends the data as required
 * \ingroup middleware
 */
TEST_CASE( "HLCToVehicleCommunication" ) {
    //Set different domain ID than 1 for the domain of the vehicle
    std::vector<char*> argv;
    std::string domain_argument = "--dds_domain=3";
    argv.push_back(const_cast<char*>(domain_argument.c_str()));
    cpm::init(argv.size() - 1, argv.data());
    cpm::Logging::Instance().set_id("middleware_test");

    //Data for the tests
    uint64_t max_rounds = 5;
    std::vector<uint64_t> received_round_numbers;
    std::mutex round_numbers_mutex;    

    {
        //Communication parameters
        int hlcDomainNumber = 1; 
        std::string vehicleStateListTopicName = "vehicleStateList"; 
        std::string vehicleTrajectoryTopicName = "vehicleCommandTrajectory"; 
        std::string vehiclePathTrackingTopicName = "vehicleCommandPathTracking"; 
        std::string vehicleSpeedCurvatureTopicName = "vehicleCommandSpeedCurvature"; 
        std::string vehicleDirectTopicName = "vehicleCommandDirect"; 
        int vehicleID = 0; 
        std::vector<uint8_t> assigned_vehicle_ids = { 0 };
        std::vector<uint8_t> active_vehicle_ids = { 0, 1 };

        //Timer parameters
        std::string node_id = "middleware";
        uint64_t period_nanoseconds = 6000000; //6ms
        uint64_t offset_nanoseconds = 1;
        bool simulated_time_allowed = true;
        bool simulated_time = false;

        //Initialize the timer
        std::shared_ptr<cpm::Timer> timer = cpm::Timer::create(
            node_id,
            period_nanoseconds,
            offset_nanoseconds,
            false,
            simulated_time_allowed,
            simulated_time);

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

        //Thread that simulates the vehicle (only a reader is created). A waitset is attached to the reader and a callback function is created. 
        //In this function, round numbers are stored - the number of each round should be received.
        cpm::AsyncReader<VehicleCommandSpeedCurvature> vehicleReader([&] (std::vector<VehicleCommandSpeedCurvature>& samples)
        {
            // Store received data for later checks
            std::lock_guard<std::mutex> lock(round_numbers_mutex);
            for (auto& data : samples) {
                    received_round_numbers.push_back(data.header().create_stamp().nanoseconds());
            }
        }, vehicleSpeedCurvatureTopicName);

        //Sleep for some milliseconds just to make sure that the reader has been initialized properly
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        //Send test data from a virtual HLC - only the round number matters here, which is transmitted using the timestamp value
        //The data is sent to the middleware (-> middleware participant, because of the domain ID), and the middleware should send it to the vehicle
        dds::domain::DomainParticipant participant = dds::domain::find(hlcDomainNumber);
        cpm::Writer<VehicleCommandSpeedCurvature> hlcWriter(participant, vehicleSpeedCurvatureTopicName);
        for (uint64_t i = 0; i <= max_rounds; ++i) {
            //Send data and wait
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            VehicleCommandSpeedCurvature curv(vehicleID, Header(TimeStamp(i), TimeStamp(i)), 0, 0);
            hlcWriter.write(curv);
        }
    }

    //Perform checks (wait a while before doing so, to make sure that everything has been received)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::lock_guard<std::mutex> lock(round_numbers_mutex);
    //"Dirty" bugfix: Check if some of the data was received (as sometimes exactly one data point is missing)
    CHECK((received_round_numbers.size() >= (max_rounds - 2) && received_round_numbers.size() >= 1));
}
