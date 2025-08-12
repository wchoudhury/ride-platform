#pragma once

#include "defaults.hpp"
#include <atomic>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <glib.h>

#include "cpm/AsyncReader.hpp"
#include "cpm/get_topic.hpp"
#include "cpm/Timer.hpp"
#include "cpm/ParticipantSingleton.hpp"

#include "Log.hpp"

/**
 * \brief Used to receive and store Log messages (cpm::Logging) from all participants in the current domain
 * \ingroup lcc
 */
class LogStorage {
public:
    /**
     * \enum FilterType
     * \brief For searching through the log storage: Which values should be considered (only the content, only the ID...)
     */
    enum FilterType {ID, Content, Timestamp, All};

private:
    //Communication objects and callbacks
    /**
     * \brief Callback function for async. received log messages
     * \param samples The received log messages
     */
    void log_callback(std::vector<Log>& samples);
    //! Async. reader to receive log messages sent within the network
    cpm::AsyncReader<Log> log_reader;
    //! Only keeps the newest logs, used when not in search-mode
    std::vector<Log> log_buffer;
    //! Keeps all logs (might delete oldest ones if some limit is reached)
    std::vector<Log> log_storage;
    //! Mutex for accessing log_buffer
    std::mutex log_buffer_mutex;
    //! Mutex for accessing log_storage
    std::mutex log_storage_mutex;

    //! File for logging, to write all received logs to
    std::ofstream file;
    //! Filename for the logfile of all received logs
    std::string filename = "all_received_logs.csv"; 
    //! Mutex s.t. only one thread has access to the log file
    std::mutex file_mutex;

    /**
     * \brief Clear elements so that count last elements are kept
     * \param vector Data structure from which to clear elemetns
     * \param count Max. number of remaining elements
     */
    void keep_last_elements(std::vector<Log>& vector, size_t count);

    /**
     * \brief Gtk will complain about non-valid UTF-8 strings when added to the UI
     * Thus, this function appends a warning that the message is invalid to invalid log messages
     * The user thus finds out that some of his log messages of his program (-> node_id) are invalid
     * This function checks all parts of the message, but only changes the log message
     * Gtk will still show warnings for invalid messages (Pango), but the user, when looking at the logs,
     * should notice that his log messages are invalid, and where to find them (to be able to correct them)
     * \param log Log message to check
     */
    void assert_utf8_validity(Log& log);

public:
    /**
     * \brief Constructor
     */
    LogStorage();
    /**
     * \brief Destructor
     */
    ~LogStorage();

    /**
     * \brief Get all Log messages that have been received since the last time this function was called (up to 100 recent logs are remembered)
     * \param log_level Get all messages up to this level
     * \return Vector of log messages
     */
    std::vector<Log> get_new_logs(unsigned int log_level);
    
    /**
     * \brief Get all Log messages that have been received (remembers up to 10000 Log messages before old Log messages are deleted)
     * \param log_level Get all messages up to this level
     * \return Vector of log messages
     */
    std::vector<Log> get_all_logs(unsigned short log_level);

    /**
     * \brief Get the log_amount most recent Log messages of all that have been received
     * \param log_amount How many logs should be returned (max value)
     * \param log_level Get all messages up to this level
     * \return Vector of log messages
     */
    std::vector<Log> get_recent_logs(const long log_amount, unsigned short log_level);

    /**
     * \brief Performs a search that is supposed to be run asynchronously in a new thread - using a future is recommended to obtain the result. The search can be aborted by setting continue_search to false (should thus be false at start) - this is useful in case the user starts a new search before the old one is completed
     * \param filter_value the string to search for (TODO: Later REGEX)
     * \param filter_type where the filter should match (Log message, Log ID...)
     * \param log_level Get all messages up to this level
     * \param continue_search should be true initially, set to false to abort the search before it finished - the algorithm then returns immediately
     */
    std::vector<Log> perform_abortable_search(std::string filter_value, FilterType filter_type, unsigned short log_level, std::atomic_bool &continue_search);

    /**
    * \brief Reset all data structures / delete all log data. Is called from the UI element only -> if you want to reset the storage, just call reset on the UI!
    */
    void reset();
};