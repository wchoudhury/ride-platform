#pragma once

#include <string>
#include <functional>
#include <memory>

namespace cpm
{
    /**
     * \brief Stop symbol sent by the Timing Instance, e.g. the LCC, to indicate that the timer should be stopped (max uint64_t value)
     * \ingroup cpmlib
     */
    constexpr uint64_t TRIGGER_STOP_SYMBOL = (0xffffffffffffffffull);

    /**
     * \class Timer
     * \brief This class calls a callback function periodically 
     * based on either the system clock or a simulated 
     * clock. The calls are synchronized in both frequency 
     * and phase to the clock.
     * \ingroup cpmlib
     */
    class Timer
    {
    protected:
        Timer(){}

    public:
        /**
         * \brief Create a timer that can be used for function callback
         * \param node_id ID of the timer in the network
         * \param period_nanoseconds The timer is called periodically with a period of period_nanoseconds
         * \param offset_nanoseconds Initial offset (from timestamp 0)
         * \param wait_for_start For the real-time timer: Set whether the timer is started only if a start signal is sent via DDS
         * \param simulated_time_allowed Decide whether the timer can run with simulated time
         * \param simulated_time Set to true if simulated time should be used for the user, else false
         */
        static std::shared_ptr<Timer> create(
            std::string node_id,
            uint64_t period_nanoseconds, 
            uint64_t offset_nanoseconds, 
            bool wait_for_start,
            bool simulated_time_allowed,
            bool simulated_time
        );
        /**
         * Start the periodic callback of the callback function in the 
         * calling thread. The thread is blocked until stop() is 
         * called.
         * \param update_callback the callback function
         */
        virtual void start       (std::function<void(uint64_t t_now)> update_callback) = 0;

        /**
         * Start the periodic callback of the callback function in the 
         * calling thread. The thread is blocked until stop() is 
         * called. When a stop signal is received, the stop_callback function is called (and may also call stop(), if desired).
         * This allows to user to define a custom stop behaviour, e.g. that the vehicle that uses the timer only stops driving,
         * but does not stop the internal timer.
         * \param update_callback the callback function to call when the next timestep is reached
         * \param stop_callback the callback function to call when the timer is stopped
         */
        virtual void start       (std::function<void(uint64_t t_now)> update_callback, std::function<void()> stop_callback) = 0;

        /**
         * Start the periodic callback of the callback function 
         * in a new thread. The calling thread is not blocked.
         * \param update_callback the callback function
         */
        virtual void start_async (std::function<void(uint64_t t_now)> update_callback) = 0;

        /**
         * Start the periodic callback of the callback function 
         * in a new thread. The calling thread is not blocked.
         * When a stop signal is received, the stop_callback function is called (and may also call stop(), if desired).
         * This allows to user to define a custom stop behaviour, e.g. that the vehicle that uses the timer only stops driving,
         * but does not stop the internal timer.
         * \param update_callback the callback function to call when the next timestep is reached
         * \param stop_callback the callback function to call when the timer is stopped
         */
        virtual void start_async (std::function<void(uint64_t t_now)> update_callback, std::function<void()> stop_callback) = 0;
        
        /**
         * \brief Stops the periodic callback and kills the thread (if it was created using start_async).
         */
        virtual void stop() = 0;
        
        /**
         * \brief Can be used to obtain the current system time in nanoseconds.
         * \return the current system time in nanoseconds
         */
        virtual uint64_t get_time() = 0;

        /**
         * \brief Can be used to obtain the time the timer was started
         * \return The start time of the timer, either received as start signal or from internal start
         */
        virtual uint64_t get_start_time() = 0;
    };
}