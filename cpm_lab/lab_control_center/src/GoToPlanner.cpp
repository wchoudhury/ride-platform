#include "GoToPlanner.hpp"

#include <algorithm> // std::max
#include <cmath>    // M_PI
#include <iostream>

#ifdef MATLAB
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "MatlabEngine.hpp"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "MatlabDataArray.hpp"
#pragma GCC diagnostic pop
#endif


GoToPlanner::GoToPlanner(
    std::function<std::vector<Pose2D>()> get_goal_poses
    ,std::function<VehicleData()> get_vehicle_data
    ,std::shared_ptr<TrajectoryCommand> trajectory_command
    ,std::string absolute_executable_path
)
:get_goal_poses(get_goal_poses)
,get_vehicle_data(get_vehicle_data)
,trajectory_command(trajectory_command)
{
    //Executable path: .../software/lab_control_center/build/lab_control_center
    //-> Remove everything up to the third-last slash
    matlab_functions_path = absolute_executable_path;
    for (int i = 0; i < 3; ++i)
    {
        auto last_slash = matlab_functions_path.find_last_of('/');
        if (last_slash != std::string::npos)
        {
            matlab_functions_path = matlab_functions_path.substr(0, last_slash);
        }
    }
    matlab_functions_path.append("/tools/go_to_formation/");
}


void GoToPlanner::go_to_start_poses()
{
    // get start poses from CommonRoadScenario
    std::vector<Pose2D> goal_poses = get_goal_poses();
    go_to_poses(goal_poses);
    return;
}

void GoToPlanner::go_to_poses(
    std::vector<Pose2D> goal_poses
)
{
#ifdef MATLAB
    if (planner_thread.joinable())
    {
        // end running thread
        planner_thread.join();
    }

    std::cout << "Going to poses ..." << std::endl;
    planner_thread = std::thread([=]()
    {

        // Start MATLAB engine synchronously
        std::unique_ptr<matlab::engine::MATLABEngine> matlabPtr = matlab::engine::connectMATLAB();

        // Create MATLAB data array factory
        matlab::data::ArrayFactory factory;

        // add matlab functions path
        matlab::data::Array mfunpath(
            factory.createCharArray(matlab_functions_path)
        );
        matlabPtr->feval(u"addpath", mfunpath);

        // locate all vehicles
        std::vector<double> vehicle_poses; // [m] [m] [deg]!
        std::vector<uint8_t> vehicle_ids;
        VehicleData vehicle_data = get_vehicle_data();
        for(const auto& entry : vehicle_data) {
            const auto vehicle_id = entry.first;
            const auto& vehicle_timeseries = entry.second;

            // if(!vehicle_timeseries.at("pose_x")->has_new_data(1.0)) continue;
            double x = vehicle_timeseries.at("pose_x")->get_latest_value();
            double y = vehicle_timeseries.at("pose_y")->get_latest_value();
            double yaw = vehicle_timeseries.at("pose_yaw")->get_latest_value();
            vehicle_poses.push_back(x);
            vehicle_poses.push_back(y);
            vehicle_poses.push_back(yaw*180.0/M_PI);

            vehicle_ids.push_back(vehicle_id);
        }
        // Plan path for every vehicle
        std::size_t nVeh_to_plan = std::min(vehicle_data.size(), goal_poses.size());
        std::size_t nVeh = vehicle_data.size();

        std::vector<int> veh_at_goal(nVeh_to_plan, 0);
        bool is_vehicle_moveable = true;
        uint64_t planning_delay = 1000000000ull;
        uint64_t total_trajectory_duration = 0ull;
        while ( is_vehicle_moveable )
        {
            is_vehicle_moveable = false;
            for (std::size_t iVeh = 0; iVeh < nVeh_to_plan; ++iVeh)
            {
                if (veh_at_goal[iVeh]) continue;

                // Plan path in MATLAB
                std::vector<matlab::data::Array> matlab_args({
                    // vehicle poses
                    factory.createArray<double>(
                        {3, nVeh},
                        vehicle_poses.data(),
                        vehicle_poses.data()+3*nVeh
                    ),
                    // vehicle index
                    factory.createScalar<int>(iVeh+1),
                    // goal pose
                    factory.createArray<double>(
                        {3, 1},
                        {
                            goal_poses[iVeh].x(),
                            goal_poses[iVeh].y(),
                            goal_poses[iVeh].yaw()*180.0/M_PI
                        }
                    )
                });
                
                std::vector<matlab::data::Array> result = matlabPtr->feval(
                    u"plan_path", 2, matlab_args
                );

                matlab::data::ArrayDimensions path_dims = result[0].getDimensions();
                matlab::data::Array is_valid = result[1];

                if (int16_t(is_valid[0]) == 0)
                {
                    std::cout   << "Found no valid path for vehicle "
                                << static_cast<int>(vehicle_ids[iVeh]) << std::endl;
                    continue;
                }
                is_vehicle_moveable = true;

                // pass path to trajectory_command
                std::vector<Pose2D> path_pts;
                for (std::size_t i_path_pt = 0; i_path_pt < path_dims[1]; ++i_path_pt)
                {
                    Pose2D path_point;
                    path_point.x(double(result[0][0][i_path_pt]));
                    path_point.y(double(result[0][1][i_path_pt]));
                    path_point.yaw(M_PI/180.0*double(result[0][2][i_path_pt]));
                    path_pts.push_back(path_point);
                }
                uint64_t new_traj_duration = trajectory_command->set_path(
                    vehicle_ids[iVeh],
                    path_pts,
                    std::max<uint64_t>(total_trajectory_duration, planning_delay)
                );
                total_trajectory_duration += new_traj_duration;
                veh_at_goal[iVeh] = 1;
                
                // assume vehicle at goal pose
                vehicle_poses.at(iVeh*3+0) = path_pts.back().x();
                vehicle_poses.at(iVeh*3+1) = path_pts.back().y();
                vehicle_poses.at(iVeh*3+2) = path_pts.back().yaw()*180.0/M_PI;
                std::cout   << "Planned trajectory for vehicle "
                            << static_cast<int>(vehicle_ids[iVeh]) << std::endl;
                break;
            }
        }
        std::cout   << "Going to poses done." << std::endl;
    }  // thread lambda function
    ); // thread call
#endif
#ifndef MATLAB
    std::cerr   << "Needs MATLAB installed with Automated Driving Toolbox."
                << std::endl;
#endif
}