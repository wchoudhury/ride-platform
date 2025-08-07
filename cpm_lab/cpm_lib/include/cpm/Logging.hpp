#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <time.h>
#include <stdio.h>
#include <mutex>
#include <algorithm>
#include <atomic>

#include "Log.hpp"
#include "LogLevel.hpp"

#include "cpm/AsyncReader.hpp"
#include "cpm/ParticipantSingleton.hpp"
#include "cpm/get_topic.hpp"
#include "cpm/get_time_ns.hpp"
#include "cpm/Parameter.hpp"
#include "cpm/Writer.hpp"

namespace cpm {
    /**
     * \class Logging
     * \brief This class can be used to log all relevant information or errors during runtime. These information are transmitted to the lab_control_center.
     * \ingroup cpmlib
     */
    class Logging {
        private:
            //! DDS Writer for Logging
            cpm::Writer<Log> logger;

            //! File for logging
            std::ofstream file;
            //! Filename for logging
            std::string filename = ""; //Is changed in Instance creation: Current timestamp added
            //! Logging identifier, e.g. "middleware", "LCC", ...
            std::string id = "uninitialized";

            //! Mutex s.t. only one thread has access to the file and the writer
            std::mutex log_mutex;

            /**
             * \brief Log-level (default value is 1) -> Determine verbosity, user messages are only printed if their log level is <= current log level.
             * This value is set by the LCC - else, the default value is used (1: only most important messages).
             * From 0 (none) to e.g. 3 (verbose).
             */
            std::atomic_ushort log_level;
            //! Reader to receive the currently set log level in the system
            std::shared_ptr<cpm::AsyncReader<LogLevel>> log_level_reader;

            /**
             * \brief Private Logging constructor to set up the Logging Singleton
             */
            Logging();

            /**
             * \brief Private function to get the current time in ns, just uses get_time_ns
             */
            uint64_t get_time();

            /**
             * \brief Check if the logging ID of the Logger Instance was set, else report an error and stop the program
             */
            void check_id();

        public:
            Logging(Logging const&) = delete;
            Logging(Logging&&) = delete; 
            Logging& operator=(Logging const&) = delete;
            Logging& operator=(Logging &&) = delete;
            
            /**
             * \brief Singleton constructor / method to access the Logging Singleton Instance; from there, the Logging functionality can be accessed
             */
            static Logging& Instance();

            /**
             * \brief Function to set the Logging ID - must be called at the start of your program! 
             * Is also called by internal configuration, so the init function initializes this automatically if
             * --logging_id was set in the command line as parameter (else, the ID is "uninitialized")
             * If you want to override the init function logging ID, call set_id explicitly after the cpm init
             * \param id ID of the Logging program, with which the Logs can be identified (e.g. middleware, hlc, lcc...)
             */
            void set_id(std::string id);

            /**
             * \brief Get the file name of the Logging file
             */
            std::string get_filename();

            /**
             * \brief Allows for a C-style use of the logger, like printf, using snprintf
             * \param message_log_level Determines the relevance of the message (1: critical system failure, 2: typical error message, 3: any other message (verbose) - 0 means 'never log anything')
             * \param f String of a form like in fprintf
             * \param args Optional parameters as given to fprintf after the format string
             */
            template<class ...Args> void write(unsigned short message_log_level, const char* f, Args&& ...args) {
                //Only log the message if the log_level of the message is <= the current level - else, it is not relevant enough
                if (message_log_level <= log_level.load())
                {
                    int size = snprintf(nullptr, 0, f, args...); //Determine the size of the resulting string without actually writing it
                    std::string str(size, ' ');
                    snprintf(& str[0], size + 1, f, args...);

                    //Before flushing make sure that the Logger was initialized properly / that its ID was set
                    check_id();
            
                    //Get the current time, use this timestamp for logging purposes
                    uint64_t time_now = get_time();

                    //For the log file: csv, so escape '"'
                    std::string log_string = std::string(str);
                    std::string escaped_quote = std::string("\"\"");
                    size_t pos = 0;
                    while ((pos = log_string.find('"', pos)) != std::string::npos) {
                        log_string.replace(pos, 1, escaped_quote);
                        pos += escaped_quote.size();
                    }
                    //Also put the whole string in quotes
                    log_string.insert(0, "\"");
                    log_string += "\"";

                    //Mutex for writing the message (file, writer) - is released when going out of scope
                    std::lock_guard<std::mutex> lock(log_mutex);

                    //Add the message to the log file - cast for log level is necessary to not create garbage symbols
                    file.open(filename, std::ios::app);
                    file << id << "," << static_cast<int>(message_log_level) << "," << time_now << "," << log_string << std::endl;
                    file.close();

                    //Send the log message via RTI
                    Log log(id, str, TimeStamp(time_now), message_log_level);
                    logger.write(log);

                    //Show the log message on the console
                    std::cerr << "Log at time " << time_now << ", level " << static_cast<int>(message_log_level) << ": " << str << std::endl;
                }
            }

            /**
             * \brief Allows for a C-style use of the logger, like printf, using snprintf
             * We do not set any log-level here, the default value then is just one (as can be seen in the if clause)
             * \param f String of a form like in fprintf
             * \param args Optional parameters as given to fprintf after the format string
             */
            template<class ...Args> void write(const char* f, Args&& ...args) {
                //Only log the message if the log_level of the message is <= the current level - else, it is not relevant enough
                //The default log-level, if none is specified, is 1 (highest priority)
                write(1, f, args...);
            }
    };
}