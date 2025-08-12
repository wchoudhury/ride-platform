#pragma once

#include <libxml++-2.6/libxml++/libxml++.h>

#include <optional>
//Optional is used for 3 reasons:
//1. Some values are optional according to the specification
//2. Some values might be missing if the file is not spec-conform, which is easy to handle when we do not require that they exist (though we still check for their existance)
//3. It is easier to set up an object piece by piece in the constructor, but that is not possible if the member object we want to set up does not have a default constructor (we would have to use the initializer list then)

#include <vector>

#include "commonroad_classes/geometry/Shape.hpp"

#include "commonroad_classes/states/Occupancy.hpp"
#include "commonroad_classes/states/SignalState.hpp"
#include "commonroad_classes/states/State.hpp"

#include "commonroad_classes/InterfaceDraw.hpp"
#include "commonroad_classes/InterfaceTransform.hpp"
#include "commonroad_classes/InterfaceTransformTime.hpp"
#include "commonroad_classes/XMLTranslation.hpp"

#include <sstream>
#include "commonroad_classes/SpecificationError.hpp"

#include "ObstacleSimulationData.hpp"

#include "LCCErrorLogger.hpp"

#include <cassert> //To make sure that the translation is performed on the right node types, which should haven been made sure by the programming (thus not an error, but an assertion is used)

/**
 * \enum ObstacleTypeDynamic
 * \brief Specifies dynamic obstacle types, as in commonroad, NotInSpec for types that should not exist
 * 2018 and 2020 differ because in 2020, we have static and dynamic obstacles, whereas in 2018, we only have obstacles of type static or dynamic
 * Nonetheless, we should only use the dynamic types here, also according to 2018 specs (We throw an error if a wrong (static) type is used)
 * We do not need to store "role", as we already have two different classes for that
 * \ingroup lcc_commonroad
 */
enum class ObstacleTypeDynamic {Unknown, Car, Truck, Bus, Motorcycle, Bicycle, Pedestrian, PriorityVehicle, Train, Taxi};

/**
 * \class DynamicObstacle
 * \brief This class, like all other classes in this folder, are heavily inspired by the current (2020) common road XML specification (https://gitlab.lrz.de/tum-cps/commonroad-scenarios/blob/master/documentation/XML_commonRoad_2020a.pdf)
 * It is used to store / represent a DynamicObstacle specified in an XML file
 * \ingroup lcc_commonroad
 */
class DynamicObstacle : public InterfaceTransform,public InterfaceTransformTime
{
private:
    //! Commonroad obstacle type
    ObstacleTypeDynamic type;
    //! Obstacle type as string
    std::string obstacle_type_text;
    //! Obstacle shape, which must exist
    std::optional<Shape> shape = std::nullopt;
    //! Obstacle initial state, which must exist
    std::optional<State> initial_state = std::nullopt;
    //! Initial signal state of the obstacle, optional
    std::optional<SignalState> initial_signal_state = std::nullopt;

    //Choice in specification - thus, only one of these two value will be valid for each object
    //! Trajectory (list of states) of the object; alternative specification: occupancy_set
    std::vector<State> trajectory;
    //! Occupancy set (list of occupancies) of the object; alternative specification: trajectory
    std::vector<Occupancy> occupancy_set;

    //! List of signal states, might not exist
    std::vector<SignalState> signal_series;

    //! Transformation scale of transform_coordinate_system is remembered to draw text correctly scaled
    double transform_scale = 1.0;

    //! Remember line in commonroad file for logging
    int commonroad_line = 0;

public:
    /**
     * \brief The constructor gets an XML node and parses it once, translating it to the C++ dynamic obstacle representation
     * An error is thrown in case the node is invalid / does not match the expected CommonRoad specs
     * \param node A (dynamic) obstacle node
     * \param _draw_lanelet_refs Function that, given an lanelet reference and the typical drawing arguments, draws a lanelet reference
     */
    DynamicObstacle(
        const xmlpp::Node* node,
        std::function<void (int, const DrawingContext&, double, double, double, double)> _draw_lanelet_refs
    );

    /**
     * \brief This function is used to fit the imported XML scenario to a given min. lane width
     * The lane with min width gets assigned min. width by scaling the whole scenario up until it fits
     * This scale value is used for the whole coordinate system
     * \param scale The factor by which to transform all number values related to position, or the min lane width (for commonroadscenario) - 0 means: No transformation desired
     * \param angle Rotation of the coordinate system, around the origin, w.r.t. right-handed coordinate system (according to commonroad specs), in radians
     * \param translate_x Move the coordinate system's origin along the x axis by this value
     * \param translate_y Move the coordinate system's origin along the y axis by this value
     */
    void transform_coordinate_system(double scale, double angle, double translate_x, double translate_y) override;

    /**
     * \brief This function is used to change timing-related values, like velocity, where needed
     * \param time_scale The factor with which time step size was changed (e.g. 1.0->2.0 results in a factor of 0.5)
     */
    void transform_timing(double time_scale) override;

    //No draw function, obstacles are handled by LCC directly via simulation

    //Helper function for draw, because this is done multiple times
    /**
     * \brief Helper function to draw the shape of the obstacle with text
     */
    void draw_shape_with_text(const DrawingContext& ctx, double scale = 1.0, double local_orientation = 0.0);
    /**
     * \brief Helper function to draw the text / description of the obstacle
     */
    void draw_text(const DrawingContext& ctx, double scale, double local_orientation, std::pair<double, double> center);

    //Getter
    /**
     * \brief Returns a trajectory constructed from occupancy or trajectory data, and further dynamic obstacle information
     * Throws errors if expected types are missing
     */
    ObstacleSimulationData get_obstacle_simulation_data();

    /**
     * \brief Get the obstacle type as text
     */
    std::string get_obstacle_type_text();
    /**
     * \brief Get the obstacle type
     */
    ObstacleTypeDynamic get_type();
    /**
     * \brief Get the shape of the obstacle, this value must exist, still due to a translation error a nullopt is possible
     */
    const std::optional<Shape>& get_shape() const;
    /**
     * \brief Get the initial state of the obstacle, this value must exist, still due to a translation error a nullopt is possible
     */
    const std::optional<State>& get_initial_state() const;
    /**
     * \brief Get the initial signal state or nullopt if that was not set
     */
    const std::optional<SignalState>& get_initial_signal_state() const;
    /**
     * \brief Get the obstacle's trajectory or nullopt if that was not set. In that case, an occupancy set should have been set.
     */
    const std::vector<State>& get_trajectory() const;
    /**
     * \brief Get the obstacle's occupancy set or nullopt if that was not set. In that case, an trajectory should have been set.
     */
    const std::vector<Occupancy>& get_occupancy_set() const;
    /**
     * \brief Get the signal series or nullopt if that was not set
     */
    const std::vector<SignalState>& get_signal_series() const;
};