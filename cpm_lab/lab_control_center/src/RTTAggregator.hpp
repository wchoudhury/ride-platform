#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>

#include "ReadyStatus.hpp"

#include "cpm/Logging.hpp"
#include "cpm/RTTTool.hpp"
#include "cpm/get_time_ns.hpp"

/**
 * \brief This class can be used to get RTT measurements and stop (/restart) them during simulation (/afterwards).
 * Measurements also include the number of missing replies (if no reply was received by any sender with the same ID (e.g. 'vehicle')).
 * After a timeout and no replies, measurements are reset / deleted for that ID (they are assumed to be turned off on purpose).
 * 
 * The identifier can be used by multiple participants, e.g. vehicle by different vehicle instances, to get the best / worst RTT for the vehicles etc.
 * To find out more: https://cpm.embedded.rwth-aachen.de/doc/display/CLD/RoundTripTime
 * \ingroup lcc
 */
class RTTAggregator
{
private:
    //! One mutex for all calculated values / maps, because we read / write on all at the same time
    std::mutex rtt_values_mutex;
    //! Map that, for string identifiers (e.g. vehicle), stores the best RTT value for that identifier for the current measurement (e.g. the fastest response of all vehicles)
    std::map<std::string, uint64_t> current_best_rtt;
    //! Map that, for string identifiers (e.g. vehicle), stores the worst RTT value for that identifier for the current measurement  (e.g. the slowest response of all vehicles)
    std::map<std::string, uint64_t> current_worst_rtt;
    //! Map that, for string identifiers (e.g. vehicle), stores the worst RTT value of all measurements taken so far for that identifier (e.g. the slowest response of all vehicles)
    std::map<std::string, uint64_t> all_time_worst_rtt;
    //! Taken measures so far for a string identifier, starts counting as soon as the first answer was received, also counts missing ones
    std::map<std::string, double> measure_count;
    //! Total of missed answers for a string identifier (e.g. if no vehicle answered), counting starts as for measure_count
    std::map<std::string, double> missed_answers;
    //! Map for each ID to remember when the last message was received, calls delete_entry if delete_entry_timeout_ns was reached
    std::map<std::string, uint64_t> last_msg_timestamp;
    //! Remember IDs to notice missing ones
    std::set<std::string> received_ids;

    //! Timeout before entries are deleted again, because no answer was received for a longer time. 10 seconds, because the RTT Tool already takes a while for one missing message.
    const uint64_t delete_entry_timeout_ns = 10e9;

    //! Thread for measuring the RTT regularly
    std::thread check_rtt_thread;
    //! Tells if the thread is currently running, set to false to interrupt it
    std::atomic_bool run_rtt_thread;
    /**
     * \brief Create the rtt measurement thread
     */
    void create_rtt_thread();
    /**
     * \brief Destroy the rtt measurement thread
     */
    void destroy_rtt_thread();

    /**
     * \brief Used to delete an entry from all data structures
     * \param id Key of the entry
     */
    void delete_entry(std::string& id);

public:
    /**
     * \brief Constructor. Measurement is started after object construction.
     */
    RTTAggregator();
    /**
     * \brief Destructor, destroys the measurement thread.
     */
    ~RTTAggregator();

    /**
     * \brief Restart RTT measurement after it was stopped.
     */
    void restart_measurement();

    /**
     * \brief Stop measurement of RTT (to reduce load on the network)
     */
    void stop_measurement();
    
    /**
     * \brief Get current measurements for RTT
     * \param participant_id ID of the participant of which the RTT was supposed to be measured (entry might not exist), e.g. 'vehicle'
     * \param _current_best_rtt Best RTT of the current measurement (in ns)
     * \param _current_worst_rtt Worst RTT of the current measurement (in ns)
     * \param _all_time_worst_rtt Worst RTT of all measurements (in ns)
     * \param _missed_answers Percentage of missed answers for this participant (Measured from first received answer)
     * \returns False if no RTT has yet been measured, else true
     */
    bool get_rtt(std::string participant_id, uint64_t &_current_best_rtt, uint64_t &_current_worst_rtt, uint64_t &_all_time_worst_rtt, double &_missed_answers);
};