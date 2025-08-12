#include "TimerViewUI.hpp"

/**
 * \file TimerViewUI.cpp
 * \ingroup lcc_ui
 */

using namespace std::placeholders;
TimerViewUI::TimerViewUI(
    std::shared_ptr<TimerTrigger> timerTrigger
    ) :
    timer_trigger(timerTrigger),
    ui_dispatcher()
 {
    ui_builder = Gtk::Builder::create_from_file("ui/timer/timer.glade");

    ui_builder->get_widget("parent", parent);
    ui_builder->get_widget("button_start", button_start);
    ui_builder->get_widget("button_stop", button_stop);
    ui_builder->get_widget("button_reset", button_reset);
    ui_builder->get_widget("active_timers_treeview", active_timers_treeview);
    ui_builder->get_widget("current_timestep_label", current_timestep_label);

    assert(parent);
    assert(button_start);
    assert(button_stop);
    assert(button_reset);
    assert(active_timers_treeview);
    assert(current_timestep_label);

    //Create model for view
    timer_list_storage = Gtk::ListStore::create(timer_record);
    active_timers_treeview->set_model(timer_list_storage);

    //Use model_record, add it to the view
    active_timers_treeview->append_column("ID", timer_record.column_id);
    active_timers_treeview->append_column("Last message", timer_record.column_last_message);
    active_timers_treeview->append_column("Participant status", timer_record.column_participant_status);
    active_timers_treeview->append_column("Next timestep", timer_record.column_next_step);

    //Set equal width for all columns
    for (int i = 0; i < 4; ++i) {
        active_timers_treeview->get_column(i)->set_resizable(true);
        active_timers_treeview->get_column(i)->set_min_width(20);
        active_timers_treeview->get_column(i)->set_fixed_width(50);
        active_timers_treeview->get_column(i)->set_expand(true);
    }

    //Register callbacks for button presses
    button_start->signal_clicked().connect(sigc::mem_fun(this, &TimerViewUI::button_start_callback));
    button_stop->signal_clicked().connect(sigc::mem_fun(this, &TimerViewUI::button_stop_callback));
    button_reset->signal_clicked().connect(sigc::mem_fun(this, &TimerViewUI::button_reset_callback));

    //Register dispatcher callback (only do that ONCE!!)
    ui_dispatcher.connect(sigc::mem_fun(*this, &TimerViewUI::dispatcher_callback));
    //Create UI thread to trigger the dispatcher
    start_ui_thread();

    //Boolean variable to find out if the timer has been started
    participant_status_timer_started.store(false);
}

TimerViewUI::~TimerViewUI() {
    stop_ui_thread();
}

void TimerViewUI::button_reset_callback() {
    //Stop checking for program chrashes
    assert(crash_checker);
    crash_checker->stop_checking();

    //Kill current UI thread as it might rely on other values that need to be reset
    stop_ui_thread();

    reset_ui();

    timer_trigger->send_stop_signal();

    //Delete the old timer, replace it by a new one with the same settings as before
    bool use_simulated_time;
    uint64_t current_time;
    timer_trigger->get_current_simulated_time(use_simulated_time, current_time);
    std::atomic_store(&timer_trigger, std::make_shared<TimerTrigger>(use_simulated_time));

    //Recreate UI thread and register dispatcher callback
    start_ui_thread();
}

void TimerViewUI::reset(bool use_simulated_time, bool send_stop_signal) {
    //Kill current UI thread as it might rely on other values that need to be reset
    stop_ui_thread();

    reset_ui();

    //Delete the old timer, replace it by a new one with new settings
    if (send_stop_signal)
    {
        timer_trigger->send_stop_signal();
    }
    
    std::atomic_store(&timer_trigger, std::make_shared<TimerTrigger>(use_simulated_time));

    //Recreate UI thread and register dispatcher callback
    start_ui_thread();
}

void TimerViewUI::start_ui_thread() {
    run_thread.store(true);
    ui_thread = std::thread(&TimerViewUI::update_ui, this);
}

void TimerViewUI::stop_ui_thread() {
    run_thread.store(false);

    if(ui_thread.joinable()) {
        ui_thread.join();
    }
}

void TimerViewUI::reset_ui() {
    //Reset values that are connected to the timer
    timer_list_storage->clear();
    participant_status_timer_started.store(false);

    //Change UI button sensitivity
    button_start->set_sensitive(true);
}


void TimerViewUI::dispatcher_callback() {
    //Update treeview
    for(const auto& entry : timer_trigger->get_participant_message_data()) {
        std::stringstream step_stream;
        step_stream << entry.second.next_timestep;

        Glib::ustring id_ustring(entry.first);
        Glib::ustring last_message_ustring(get_human_readable_time_diff(entry.second.last_message_receive_stamp));
        Glib::ustring participant_status_ustring(participant_status_to_ustring(entry.second.participant_status));
        Glib::ustring next_step_ustring(step_stream.str());

        Gtk::TreeModel::Row row;
        bool entry_exists = false;

        //Search if the entry already exists
        for (Gtk::TreeModel::iterator iter = timer_list_storage->children().begin(); iter != timer_list_storage->children().end(); ++iter) {
            row = *iter;
            if (row[timer_record.column_id] == id_ustring) {
                entry_exists = true;
                break;
            }
        }
        //Else create a new entry
        if (!entry_exists) {
            row = *(timer_list_storage->append());
        }

        row[timer_record.column_id] = id_ustring;
        row[timer_record.column_last_message] = last_message_ustring;
        row[timer_record.column_participant_status] = participant_status_ustring;
        row[timer_record.column_next_step] = next_step_ustring;
    }

    //Update current time in UI
    bool use_simulated_time;
    uint64_t current_simulated_time;
    timer_trigger->get_current_simulated_time(use_simulated_time, current_simulated_time);
    if (use_simulated_time) {
        std::stringstream time_stream;
        time_stream << current_simulated_time;
        Glib::ustring step_ustring(time_stream.str());
        current_timestep_label->set_label(step_ustring);
    }
}

void TimerViewUI::update_ui() {
    while (run_thread.load()) {
        ui_dispatcher.emit();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void TimerViewUI::button_start_callback() {
    timer_trigger->send_start_signal();

    button_start->set_sensitive(false);
    participant_status_timer_started.store(true);
}

void TimerViewUI::button_stop_callback() {
    //Stop checking for program chrashes
    assert(crash_checker);
    crash_checker->stop_checking();

    timer_trigger->send_stop_signal();

    //Stop obstacle simulation
    // obstacle_simulation_manager->stop();

    //We do not set participant_status_timer_started here, as it is only used to set the correct value for the timer labels
    //(Makes more sense to keep the last label while running instead of going back to READY)
    //The timer cannot be restarted without being reset, where participant_status_timer_started is reset as well, so 
    //this is not problematic

    std::string label_msg = "stopped";
    Glib::ustring label_msg_ustring(label_msg);
    current_timestep_label->set_label(label_msg_ustring);

    button_start->set_sensitive(false);
}

std::string TimerViewUI::participant_status_to_ustring(ParticipantStatus response) {
    if (participant_status_timer_started.load() == false) {
        return "READY";
    }
    else {
        if (response == ParticipantStatus::REALTIME) {
            return "(realtime)";
        }
        else if (response == ParticipantStatus::WAITING) {
            return "WAITING"; //TODO: For long waiting times with simulated time, one could log the responsible participant ID - on the other hand, this is already shown in the UI in the timer tab
        }
        else if (response == ParticipantStatus::WORKING) {
            return "WORKING";
        }
        else {
            return "OUT OF SYNC";
        }
    }
}

std::string TimerViewUI::get_human_readable_time_diff(uint64_t other_time) {
    uint64_t current_time = cpm::get_time_ns();
    if (current_time >= other_time) {
        uint64_t time_diff = current_time - other_time;
        std::stringstream time_stream;

        time_diff /= 1000000000ull;
        if (time_diff <= 59) {
            time_stream << time_diff << "s";
        }
        else {
            uint64_t time_diff_min = time_diff/60;
            uint64_t time_diff_sec = time_diff % 60;

            if (time_diff_min <= 59) {
                time_stream << time_diff_min << "min " << time_diff_sec << "s";
            }
            else {
                uint64_t time_diff_h = time_diff_min/60;
                time_diff_min = time_diff_min % 60;

                time_stream << time_diff_h << "h " << time_diff_min << "min " << time_diff_sec << "s";
            }
        }

        return time_stream.str();
    }
    else {
        return "-1";
    }
}

Gtk::Widget* TimerViewUI::get_parent() {
    return parent;
}

void TimerViewUI::register_crash_checker(std::shared_ptr<CrashChecker> _crash_checker)
{
    crash_checker = _crash_checker;
}