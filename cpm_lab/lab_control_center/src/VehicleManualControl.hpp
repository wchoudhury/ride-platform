#pragma once
#include "defaults.hpp"
#include <dds/pub/ddspub.hpp>
#include "cpm/TimerFD.hpp"
#include "Joystick.hpp"
#include "VehicleCommandDirect.hpp"
#include "VehicleCommandSpeedCurvature.hpp"
#include "VehicleCommandTrajectory.hpp"
#include <functional>

/**
 * \brief Class for controlling a vehicle with a joystick
 * \ingroup lcc
 */
class VehicleManualControl
{
    //! TODO
    dds::domain::DomainParticipant& participant;
    //! TODO
    shared_ptr<Joystick> joystick = nullptr;
    //! TODO
    std::shared_ptr<cpm::TimerFD> update_loop = nullptr;
    //! TODO
    uint8_t vehicle_id = 0;
    
    //! TODO
    double ref_speed = 0;
    //! TODO
    uint64_t ref_trajectory_start_time = 0;
    //! TODO
    int ref_trajectory_index = 0;

    //! TODO
    dds::topic::Topic<VehicleCommandDirect> topic_vehicleCommandDirect;
    //! TODO
    dds::topic::Topic<VehicleCommandSpeedCurvature> topic_vehicleCommandSpeedCurvature;

    //! TODO
    shared_ptr<dds::pub::DataWriter<VehicleCommandDirect>> writer_vehicleCommandDirect = nullptr;
    //! TODO
    shared_ptr<dds::pub::DataWriter<VehicleCommandSpeedCurvature>> writer_vehicleCommandSpeedCurvature = nullptr;

    //! TODO
    std::function<void()> m_update_callback;

public:
    /**
     * \brief TODO Constructor
     */
    VehicleManualControl();

    /**
     * \brief TODO
     * \param vehicleId TODO
     * \param joystick_device_file TODO
     */
    void start(uint8_t vehicleId, string joystick_device_file);
    
    /**
     * \brief TODO
     */
    void stop();

    /**
     * \brief TODO
     * \param update_callback TODO
     */
    void set_callback(std::function<void()> update_callback) { m_update_callback = update_callback; }

    /**
     * \brief TODO
     * \param throttle TODO
     * \param steering TODO
     */
    void get_state(double& throttle, double& steering);
};