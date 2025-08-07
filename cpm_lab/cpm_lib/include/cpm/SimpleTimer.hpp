#pragma once

#include "cpm/Timer.hpp"
#include "cpm/ParticipantSingleton.hpp"
#include "cpm/Logging.hpp"

#include "cpm/TimerFD.hpp"

#include <cmath>

namespace cpm {
    /**
     * \class SimpleTimer
     * \brief This class calls a callback function periodically based on TimerFD, is less exact but faster to quit.
     * The given period (in milliseconds) is broken down in intervals of 50ms, to be able
     * to stop the timer rather quickly when stop() is called. Else, stopping the timer takes up to one period in a worst case scenario.
     * The given period in milliseconds might thus be rounded up accordingly. 
     * This timer is neither intended to work with simulated time,
     * nor is it exact enough to be real-time capable. Use this e.g. for timing in the GUI or other non-critical
     * timing tasks only!
     * This timer listens to the stop signal if m_stop_callback is not set
     * \ingroup cpmlib
     */
    class SimpleTimer : public cpm::Timer
    {
    private:
        //! Internally, TimerFD is used, with a 50ms interval
        std::shared_ptr<cpm::TimerFD> internal_timer;
        //! A counter is used to call the actual callback function only every period_milliseconds (rounded up to 50ms)
        uint64_t internal_timer_counter = 0;
        //! Count in 50ms intervals up to this value, determined by period_milliseconds / fifty_ms
        uint64_t counter_max = 0;
        //! The internal timer works with 50ms as interval
        uint64_t fifty_ms = 50000000ull;

        //! Callback function for the timer 
        std::function<void(uint64_t t_now)> m_update_callback;
        //! Callback function when a stop signal is received, optional
        std::function<void()> m_stop_callback;

        //! Internal callback function for TimerFD, calls the callback if internal_timer_counter reached counter_max
        void simple_timer_callback(uint64_t t_now);

    public:
        /**
         * \brief Create a simple timer (not real-time capable) that can be used for function callback
         * \param _node_id ID of the timer in the network
         * \param period_milliseconds The timer is called periodically with a period of period_milliseconds (rounded up to 50ms)
         * \param wait_for_start Set whether the timer is started only if a start signal is sent via DDS (true), or if it should should start immediately (false)
         * \param react_to_stop_signal Set whether the timer should be stopped if a stop signal is sent within the network (optional, default is true); 
         * \param stop_signal Set your own custom stop signal - unrecommended, unless you know exactly what you are doing
         * if set, stop_callback is set to be an empty lambda if start is called without stop_callback
         */
        SimpleTimer(std::string _node_id, uint64_t period_milliseconds, bool wait_for_start, bool react_to_stop_signal = true, uint64_t stop_signal = TRIGGER_STOP_SYMBOL);
        
        /**
         * \brief Destructor due to internal mutex, timer...
         */
        ~SimpleTimer();

        /**
         * Start the periodic callback of the callback function in the 
         * calling thread. The thread is blocked until stop() is 
         * called.
         * \param update_callback the callback function, which in this case just gets the current time
         */
        void start       (std::function<void(uint64_t t_now)> update_callback) override;

        /**
         * Start the periodic callback of the callback function in the 
         * calling thread. The thread is blocked until stop() is 
         * called. When a stop signal is received, the stop_callback function is called (and may also call stop(), if desired).
         * This allows to user to define a custom stop behaviour, e.g. that the vehicle that uses the timer only stops driving,
         * but does not stop the internal timer.
         * \param update_callback the callback function to call when the next timestep is reached, which in this case just gets the current time
         * \param stop_callback the callback function to call when the timer is stopped
         */
        void start       (std::function<void(uint64_t t_now)> update_callback, std::function<void()> stop_callback) override;

        /**
         * Start the periodic callback of the callback function 
         * in a new thread. The calling thread is not blocked.
         * \param update_callback the callback function, which in this case just gets the current time
         */
        void start_async (std::function<void(uint64_t t_now)> update_callback) override;

        /**
         * Start the periodic callback of the callback function 
         * in a new thread. The calling thread is not blocked.
         * When a stop signal is received, the stop_callback function is called (and may also call stop(), if desired).
         * This allows to user to define a custom stop behaviour, e.g. that the vehicle that uses the timer only stops driving,
         * but does not stop the internal timer.
         * \param update_callback the callback function to call when the next timestep is reached, which in this case just gets the current time
         * \param stop_callback the callback function to call when the timer is stopped
         */
        void start_async (std::function<void(uint64_t t_now)> update_callback, std::function<void()> stop_callback) override;

        /**
         * \brief Stops the periodic callback and kills the thread (if it was created using start_async).
         */
        void stop() override;

        /**
         * \brief Can be used to obtain the current system time in milliseconds.
         * \return the current system time in milliseconds
         */
        uint64_t get_time() override;

        /**
         * \brief Can be used to obtain the time the timer was started in nanoseconds
         * \return The start time of the timer, either received as start signal or from internal start, in nanoseconds OR
         * 0 if not yet started or stopped before started
         */
        uint64_t get_start_time() override;
    };

}