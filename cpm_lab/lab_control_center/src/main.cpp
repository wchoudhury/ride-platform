#include "defaults.hpp"
#include "stdio.h"
#include <unistd.h>
#include "ObstacleAggregator.hpp"
#include "TimeSeriesAggregator.hpp"
#include "HLCReadyAggregator.hpp"
#include "ObstacleSimulationManager.hpp"
#include "VisualizationCommandsAggregator.hpp"
#include "VehicleManualControl.hpp"
#include "VehicleAutomatedControl.hpp"
#include "ui/commonroad/CommonroadViewUI.hpp"
#include "ui/monitoring/MonitoringUi.hpp"
#include "ui/manual_control/VehicleManualControlUi.hpp"
#include "ui/map_view/MapViewUi.hpp"
#include "ui/right_tabs/TabsViewUI.hpp"
#include "ui/params/ParamViewUI.hpp"
#include "ui/timer/TimerViewUI.hpp"
#include "ui/lcc_errors/LCCErrorViewUI.hpp"
#include "ui/logger/LoggerViewUI.hpp"
#include "ui/setup/SetupViewUI.hpp"
#include "LogStorage.hpp"
#include "ParameterServer.hpp"
#include "ParameterStorage.hpp"
#include "RTTAggregator.hpp"
#include "TrajectoryCommand.hpp"
#include "ui/MainWindow.hpp"
#include "cpm/RTTTool.hpp"
#include "cpm/Logging.hpp"
#include "cpm/CommandLineReader.hpp"
#include "TimerTrigger.hpp"
#include "cpm/init.hpp"

#include "commonroad_classes/CommonRoadScenario.hpp"

#include "cpm/Writer.hpp"
#include "CommonroadDDSGoalState.hpp"

#include "ProgramExecutor.hpp"

#include <gtkmm/builder.h>
#include <gtkmm.h>
#include <functional>
#include <sstream>

//For exit handlers
#include <signal.h>
#include <stdlib.h>
#include <cstdlib>

using namespace std::placeholders;

/**
 * \brief We need this to be a global variable, or else it cannot be used in the interrupt or exit handlers
 * to execute command line commands
 * \ingroup lcc
 */
std::shared_ptr<ProgramExecutor> program_executor;

/**
 * \brief Function to deploy cloud discovery (to help participants discover each other)
 * Will crash on purpose if program_executor is not set (we need this function to work)
 * \ingroup lcc
 */
void deploy_cloud_discovery() {
    std::string command = "tmux new-session -d -s \"rticlouddiscoveryservice\" \"rticlouddiscoveryservice -transport 25598\"";
    program_executor->execute_command(command.c_str());
}

/**
 * \brief Function to kill cloud discovery
 * \ingroup lcc
 */
void kill_cloud_discovery() {
    std::string command = "tmux kill-session -t \"rticlouddiscoveryservice\"";
    if (program_executor) program_executor->execute_command(command.c_str());
}

/**
 * \brief Sometimes, a tmux session can stay open if the LCC crashed. 
 * To make sure that no session is left over / everything is "clean" 
 * when a new LCC gets started, kill all old sessions
 */
void kill_all_tmux_sessions() {
    std::string command = "tmux kill-server >>/dev/null 2>>/dev/null";
    if (program_executor) program_executor->execute_command(command.c_str());
}

//Suppress warning for unused parameter (s)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * \brief Interrup handler of the LCC. Disallows interrupts.
 * \ingroup lcc
 */
void interrupt_handler(int s) {
    // Calling exit here would cause memory issues (destructors are not being called)
    // Also, setting a flag does not make much sense, as an infinite loop at the end of main
    // would mean that the program can ONLY be stopped with an interrupt. This solution should
    // suffice. In case of the program hanging, the user can still close the terminal, kill the LCC
    // in system monitor etc.
    std::cout << "---!!! Interrupts are not allowed, as they could lead to severe memory issues. Please exit the program normally!" << std::endl;
}

#pragma GCC diagnostic pop


/**
 * \brief Main function of the LCC.
 * Command line arguments:
 * 
 * --dds_domain
 * --logging_id
 * --dds_initial_peer
 * --simulated_time
 * --number_of_vehicles (default 20, set how many vehicles can max. be selected in the UI)
 * --config_file (default parameters.yaml)
 * \ingroup lcc
 */
int main(int argc, char *argv[])
{
    //To avoid exit() statements, which can be harmful regarding memory handling and 
    //destructors, the next most simple solution was implemented: Throw instead,
    //and try to handle these errors in main (print the output, if possible) and end the program normally afterwards
    auto return_code = 0;

    try {
        //Do this even before creating the process-spawning child
        //We need to get the path to the executable, and argv[0] is not
        //reliable enough for that (and sometimes also only returns a relative path)
        std::array<char, 128> buffer;
        std::string absolute_executable_path;
        ssize_t len = ::readlink("/proc/self/exe", buffer.data(), buffer.size()-1);
        if (len >= 0) {
        buffer[len] = '\0';
        std::string temp(buffer.data());
        absolute_executable_path = temp;
        }
        else
        {
            throw std::runtime_error("ERROR: Could not obtain executable path, thus deploying functions would not work. Shutting down...");
        }

        //-----------------------------------------------------------------------------------------------------
        //It is vital to call this function before any threads or objects have been set up that are not
        //required in child processes
        //DO NOT move this further done unless you know what you do! Especially DO NOT put this below the cpm
        //initialization, unless you want to risk memory leaks
        std::string exec_path = argv[0];
        
        //Get absolute path to main.cpp 
        auto pos = exec_path.rfind("/");
        std::string main_cpp_path = "../src/main.cpp"; //Does not seem to work, thus we obtain the absolute path next
        if (pos != std::string::npos)
        {
            auto build_path = exec_path.substr(0, pos);
            pos = build_path.rfind("/");
            if (pos != std::string::npos)
            {
                std::stringstream main_cpp_stream;
                main_cpp_stream << build_path.substr(0, pos) << "/src/main.cpp";
                main_cpp_path = main_cpp_stream.str();
            }
        }

        program_executor = std::make_shared<ProgramExecutor>();
        bool program_execution_possible = program_executor->setup_child_process(exec_path, main_cpp_path);
        if (! program_execution_possible)
        {
            throw std::runtime_error("Killing LCC because no child process for program execution could be created!");
        }
        //-----------------------------------------------------------------------------------------------------

        //Kill remaining tmux sessions
        kill_all_tmux_sessions();

        //Must be done as soon as possible, s.t. no class using the logger produces an error
        cpm::init(argc, argv);
        cpm::Logging::Instance().set_id("lab_control_center");
        cpm::RTTTool::Instance().activate("lab_control_center");

        //To receive logs as early as possible, and for Logging in main
        auto logStorage = make_shared<LogStorage>();

        //Create regular and irregular (interrupt) exit handlers
        //Disallow interrupts (Strg + C), as they could lead to memory issues
        struct sigaction interruptHandler;
        interruptHandler.sa_handler = interrupt_handler;
        sigemptyset(&interruptHandler.sa_mask);
        interruptHandler.sa_flags = 0;
        sigaction(SIGINT, &interruptHandler, NULL);

        //Start IPS and Cloud Discovery Service which are always required to communicate with real vehicles
        deploy_cloud_discovery();

        //Pre define setup view UI
        std::shared_ptr<SetupViewUI> setupViewUi;

        //Read command line parameters (current params: auto_start and config_file)
        //TODO auto_start: User does not need to trigger the process manually / does not need to press 'start' when all participants are ready

        std::string config_file = cpm::cmd_parameter_string("config_file", "parameters.yaml", argc, argv);

        //Load commonroad scenario (TODO: Implement load by user, this is just a test load)
        std::string filepath_lab_map = "./ui/map_view/LabMapCommonRoad.xml";
        auto commonroad_scenario = std::make_shared<CommonRoadScenario>();
        try
        {
            commonroad_scenario->load_file(filepath_lab_map);
        }
        catch(const std::exception& e)
        {
            cpm::Logging::Instance().write(1, "Could not load initial commonroad scenario, error is: %s", e.what());
        }

        auto storage = make_shared<ParameterStorage>(config_file, 32);
        ParameterServer server(storage);
        storage->register_on_param_changed_callback(std::bind(&ParameterServer::resend_param_callback, &server, _1));

        Glib::RefPtr<Gtk::Application> app = Gtk::Application::create();
        Glib::RefPtr<Gtk::CssProvider> cssProvider = Gtk::CssProvider::create();
        cssProvider->load_from_path("ui/style.css");
        Gtk::StyleContext::create()->add_provider_for_screen (Gdk::Display::get_default()->get_default_screen(),cssProvider,500);

        bool use_simulated_time = cpm::cmd_parameter_bool("simulated_time", false, argc, argv);

        auto obstacle_simulation_manager = std::make_shared<ObstacleSimulationManager>(commonroad_scenario, use_simulated_time);

        auto timerTrigger = make_shared<TimerTrigger>(use_simulated_time);
        auto timerViewUi = make_shared<TimerViewUI>(timerTrigger);
        auto loggerViewUi = make_shared<LoggerViewUI>(logStorage);
        auto vehicleManualControl = make_shared<VehicleManualControl>();
        auto vehicleAutomatedControl = make_shared<VehicleAutomatedControl>();
        auto trajectoryCommand = make_shared<TrajectoryCommand>();
        auto timeSeriesAggregator = make_shared<TimeSeriesAggregator>(255); //LISTEN FOR VEHICLE DATA UP TO ID 255 (for Commonroad Obstacles; is max. uint8_t value)
        auto obstacleAggregator = make_shared<ObstacleAggregator>(commonroad_scenario); //Use scenario to register reset callback if scenario is reloaded
        auto hlcReadyAggregator = make_shared<HLCReadyAggregator>();
        auto visualizationCommandsAggregator = make_shared<VisualizationCommandsAggregator>();
        unsigned int cmd_domain_id = cpm::cmd_parameter_int("dds_domain", 0, argc, argv);
        std::string cmd_dds_initial_peer = cpm::cmd_parameter_string("dds_initial_peer", "", argc, argv);

        auto goToPlanner = make_shared<GoToPlanner>(
            std::bind(&CommonRoadScenario::get_start_poses, commonroad_scenario),
            [=](){return timeSeriesAggregator->get_vehicle_data();},
            trajectoryCommand,
            absolute_executable_path
        );

        //Create deploy class
        std::shared_ptr<Deploy> deploy_functions = std::make_shared<Deploy>(
            cmd_domain_id, 
            cmd_dds_initial_peer, 
            [&](uint8_t id){vehicleAutomatedControl->stop_vehicle(id);},
            program_executor,
            absolute_executable_path
        );

        //UI classes
        auto mapViewUi = make_shared<MapViewUi>(
            trajectoryCommand, 
            commonroad_scenario,
            [&](){return timeSeriesAggregator->get_vehicle_data();},
            [&](){return timeSeriesAggregator->get_vehicle_trajectory_commands();},
            [&](){return timeSeriesAggregator->get_vehicle_path_tracking_commands();},
            [&](){return obstacleAggregator->get_obstacle_data();}, 
            [&](){return visualizationCommandsAggregator->get_all_visualization_messages();}
        );
        auto rtt_aggregator = make_shared<RTTAggregator>();
        auto monitoringUi = make_shared<MonitoringUi>(
            deploy_functions, 
            [&](){return timeSeriesAggregator->get_vehicle_data();}, 
            [&](){return hlcReadyAggregator->get_hlc_ids_uint8_t();},
            [&](){return timeSeriesAggregator->get_vehicle_trajectory_commands();},
            [&](){return timeSeriesAggregator->reset_all_data();},
            [&](std::string id, uint64_t& c_best_rtt, uint64_t&  c_worst_rtt, uint64_t&  a_worst_rtt, double& missed_msgs)
                {
                    return rtt_aggregator->get_rtt(id, c_best_rtt, c_worst_rtt, a_worst_rtt, missed_msgs);
                },
            [&](){return setupViewUi->kill_deployed_applications();}
        );
        auto vehicleManualControlUi = make_shared<VehicleManualControlUi>(vehicleManualControl);
        auto paramViewUi = make_shared<ParamViewUI>(storage, 5);
        auto commonroadViewUi = make_shared<CommonroadViewUI>(
            commonroad_scenario,
            obstacle_simulation_manager 
        );

        //Writer to send planning problems translated from commonroad to HLCs
        //As it is transient local, we need to reset the writer before each simulation start
        auto writer_planning_problems = std::make_shared<cpm::Writer<CommonroadDDSGoalState>>("commonroad_dds_goal_states", true, true, true);

        setupViewUi = make_shared<SetupViewUI>(
            deploy_functions,
            vehicleAutomatedControl, 
            hlcReadyAggregator, 
            goToPlanner,
            [&](){return timeSeriesAggregator->get_vehicle_data();},
            [&](bool simulated_time, bool reset_timer){return timerViewUi->reset(simulated_time, reset_timer);}, 
            [&](){return monitoringUi->reset_vehicle_view();},
            [&](){
                //Things to do when the simulation is started

                //Reset writer for planning problems (used down below), as it is transient local and we do not want to pollute the net with outdated data
                writer_planning_problems.reset();
                writer_planning_problems = std::make_shared<cpm::Writer<CommonroadDDSGoalState>>("commonroad_dds_goal_states", true, true, true);

                //Stop RTT measurement
                rtt_aggregator->stop_measurement();

                //Reset old UI elements (difference to kill: Also reset the Logs)
                //Kill timer in UI as well, as it should not show invalid information
                //Reset all relevant UI parts
                timeSeriesAggregator->reset_all_data();
                obstacleAggregator->reset_all_data();
                trajectoryCommand->stop_all();
                monitoringUi->notify_sim_start();
                visualizationCommandsAggregator->reset_visualization_commands();
                
                //We also reset the log file here - if you want to use it, make sure to rename it before you start a new simulation!
                loggerViewUi->reset();

                //Send commonroad planning problems to the HLCs (we use transient settings, so that the readers do not need to have joined)
                if(commonroad_scenario) commonroad_scenario->send_planning_problems(writer_planning_problems);

                //Reset preview, must be done before starting the obstacle simulation manager because this stops the manager running for the preview
                commonroadViewUi->reset_preview();

                //Start simulated obstacles - they will also wait for a start signal, so they are just activated to do so at this point
                obstacle_simulation_manager->stop(); //In case the preview has been used
                obstacle_simulation_manager->start();

            }, 
            [&](){
                //Things to do when the simulation is stopped

                //Stop obstacle simulation
                obstacle_simulation_manager->stop();
                
                //Kill timer in UI as well, as it should not show invalid information
                //TODO: Reset Logs? They might be interesting even after the simulation was stopped, so that should be done separately/never (there's a log limit)/at start?
                //Reset all relevant UI parts
                timeSeriesAggregator->reset_all_data();
                obstacleAggregator->reset_all_data();
                trajectoryCommand->stop_all();
                monitoringUi->notify_sim_stop();
                visualizationCommandsAggregator->reset_visualization_commands();

                //Restart RTT measurement
                rtt_aggregator->restart_measurement();
            },
            [&](bool set_sensitive){return commonroadViewUi->set_sensitive(set_sensitive);}, 
            [&](std::vector<int32_t> active_vehicle_ids){
                storage->set_parameter_ints("active_vehicle_ids", active_vehicle_ids, "Currently active vehicle ids");
                paramViewUi->read_storage_data();
            },
            absolute_executable_path,
            argc, 
            argv
        );
        monitoringUi->register_vehicle_to_hlc_mapping(
            [&](){return setupViewUi->get_vehicle_to_hlc_matching();}
        );
        monitoringUi->register_crash_checker(setupViewUi->get_crash_checker());
        timerViewUi->register_crash_checker(setupViewUi->get_crash_checker());
        auto lccErrorViewUi = make_shared<LCCErrorViewUI>();
        auto tabsViewUi = make_shared<TabsViewUI>(
            setupViewUi, 
            vehicleManualControlUi, 
            paramViewUi, 
            timerViewUi, 
            lccErrorViewUi,
            loggerViewUi, 
            commonroadViewUi);
        auto mainWindow = make_shared<MainWindow>(tabsViewUi, monitoringUi, mapViewUi, paramViewUi);

        //To create a window without Gtk complaining that no parent has been set, we need to pass the main window after mainWindow has been created
        //(Wherever we want to create windows)
        setupViewUi->set_main_window_callback(std::bind(&MainWindow::get_window, mainWindow));
        paramViewUi->set_main_window_callback(std::bind(&MainWindow::get_window, mainWindow));
        commonroadViewUi->set_main_window_callback(std::bind(&MainWindow::get_window, mainWindow));

        vehicleManualControl->set_callback([&](){vehicleManualControlUi->update();});

        return_code = app->run(mainWindow->get_window());

        // Clean up on exit
        kill_cloud_discovery();
        //Kill remaining programs opened by setup view ui
        if (setupViewUi)
        {
            setupViewUi->on_lcc_close();
        }
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << "THE LCC IS NOW BEING SHUT DOWN" << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "!!!An unknown error was thrown" << std::endl;
        std::cerr << "THE LCC IS NOW BEING SHUT DOWN" << std::endl;
        return 1;
    }

    return return_code;
}
