#pragma once
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "cpm/AsyncReader.hpp"
#include "cpm/get_time_ns.hpp"
#include "cpm/get_topic.hpp"
#include "cpm/ParticipantSingleton.hpp"
#include "cpm/Writer.hpp"

#include "RoundTripTime.hpp"

namespace cpm
{
    /**
     * \brief This class (singleton) is used to create a thread that automatically runs in the background, 
     * which replies to every round trip time message received immediately with the current program's logging ID
     * 
     * It can also be used to measure the round trip time
     * \ingroup cpmlib
     */
    class RTTTool
    {
    private:
        //! DDS Writer to send an RTT request or an answer to a request
        cpm::Writer<RoundTripTime> rtt_writer;
        //! DDS Reader to receive an RTT request or answer
        std::shared_ptr<cpm::AsyncReader<RoundTripTime>> rtt_reader;
        //! ID of the program using or responding to an RTT request, e.g. "LCC", "middleware", ...
        std::string program_id = "no_prog_id_set";

        //Measure RTT request receive times, read async, requested by measure_rtt
        //! Used by activate, if not true than no measurement can be requested or answered to; enforces setting the program ID
        std::atomic_bool rtt_measurement_active;
        //! To find out if the current participant requested an RTT measurement (measure_rtt) or if it should only answer to measurements by others
        std::atomic_bool rtt_measure_requested;
        //! Counter to distinguish measurements. Cannot use atomic_uint8_t due to compatability to lower C++ standards for vehicles
        std::atomic<std::uint8_t> rtt_count; 
        //! Mutex for access to receive_times
        std::mutex receive_times_mutex;
        //! Multiple members may use the same ID (e.g. "vehicle") - then, multiple RTT times can exist. All RTT times are stored here during measurement.
        std::map<std::string, std::vector<uint64_t>> receive_times;

        /**
         * \brief Constructor - private bc singleton
         */
        RTTTool();

    public:
        RTTTool(RTTTool const&) = delete;
        RTTTool(RTTTool&&) = delete; 
        RTTTool& operator=(RTTTool const&) = delete;
        RTTTool& operator=(RTTTool &&) = delete;

        /**
         * \brief Interface to create / get access to the singleton
         */ 
        static RTTTool& Instance();

        /**
         * \brief Activate the RTT measurement for this participant
         * It will answer to RTT requests with the ID set in the parameter
         * If not activated, RTT requests are being ignored by the participant
         * Must also be called for the RTT measurer, to set a correct program id for RTT requests
         * \param _program_id ID set for the answers to round trip time messages, to identify the program (type) (e.g. vehicle (5), middleware, ...)
         */
        void activate(std::string _program_id);

        /**
         * \brief Use this function to measure the (best and worst) round trip time in your network in ns
         * WARNING: Make sure that this function is only used by one program in your whole network, 
         * or you might get wrong results due to bad timing of both functions!
         * \return empty map / missing entries in case of an error / missing answers, else the best and 'worst' (within 2200ms) measured RTT for each participant id
         */
        std::map<std::string, std::pair<uint64_t, uint64_t>> measure_rtt();
    };
};