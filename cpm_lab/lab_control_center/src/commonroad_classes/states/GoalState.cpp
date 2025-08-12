#include "commonroad_classes/states/GoalState.hpp"

/**
 * \file GoalState.cpp
 * \ingroup lcc_commonroad
 */

GoalState::GoalState(
    const xmlpp::Node* node,
    std::function<void (int, const DrawingContext&, double, double, double, double)> _draw_lanelet_refs,
    std::function<std::pair<double, double> (int)> _get_lanelet_center,
    std::shared_ptr<CommonroadDrawConfiguration> _draw_configuration
    ) :
    draw_configuration(_draw_configuration)
{
    //2018 and 2020 specs are the same
    //Check if node is of type goal state
    assert(node->get_name() == "goalState");

    try
    {
        const auto position_node = xml_translation::get_child_if_exists(node, "position", false);
        if (position_node)
        {
            position = std::optional<Position>{std::in_place, position_node};
        }
        //No position must be given for a goal state, so default values are NOT assumed to be used here!
        std::cerr << "WARNING: No position has been set for this goal state (line " << node->get_line() << "). This might be intended." << std::endl;
        std::stringstream err_stream;
        err_stream << "WARNING: No position has been set for this goal state (line " << node->get_line() << "). This might be intended.";
        LCCErrorLogger::Instance().log_error(err_stream.str());

        const auto velocity_node = xml_translation::get_child_if_exists(node, "velocity", false);
        if (velocity_node)
        {
            velocity = std::optional<Interval>(std::in_place, velocity_node);
        }

        const auto orientation_node = xml_translation::get_child_if_exists(node, "orientation", false);
        if (orientation_node)
        {
            orientation = std::optional<Interval>(std::in_place, orientation_node);
        }

        //Time is defined using intervals
        const auto time_node = xml_translation::get_child_if_exists(node, "time", true);
        if (time_node)
        {
            time = std::optional<IntervalOrExact>(std::in_place, time_node);

            //Time must have a value; make sure that, as specified, the value is greater than zero
            if (! time.value().is_greater_zero())
            {
                std::stringstream error_stream;
                error_stream << "Time must be greater than zero, in line: ";
                error_stream << time_node->get_line();
                throw SpecificationError(error_stream.str());
            }
        }
        else
        {
            //Time is the only actually required value
            std::stringstream error_msg_stream;
            error_msg_stream << "No time node in GoalState (required by specification) - line " << node->get_line();
            throw SpecificationError(error_msg_stream.str());
        }
    }
    catch(const SpecificationError& e)
    {
        throw SpecificationError(std::string("Could not translate GoalState:\n") + e.what());
    }
    catch(...)
    {
        //Propagate error, if any subclass of CommonRoadScenario fails, then the whole translation should fail
        throw;
    }
    
    //Set lanelet_ref functions
    if(position.has_value())
    {
        position->set_lanelet_ref_draw_function(_draw_lanelet_refs);
        position->set_lanelet_get_center_function(_get_lanelet_center);
    }

    //Test output
    // std::cout << "GoalState: " << std::endl;
    // std::cout << "\tPosition exists: " << position.has_value() << std::endl;
    // std::cout << "\tVelocity exists: " << velocity.has_value() << std::endl;
    // std::cout << "\tOrientation exists: " << orientation.has_value() << std::endl;
    // std::cout << "\tTime exists: " << time.has_value() << std::endl;
}

void GoalState::transform_coordinate_system(double scale, double angle, double translate_x, double translate_y)
{
    if (orientation.has_value())
    {
        orientation->rotate_orientation(angle);
    }

    if (position.has_value())
    {
        position->transform_coordinate_system(scale, angle, translate_x, translate_y);
    }

    //If all positional values are adjusted, the velocity must be adjusted as well
    if (velocity.has_value())
    {
        velocity->transform_coordinate_system(scale, angle, 0, 0);
    }

    if (scale > 0)
    {
        transform_scale *= scale;
    }
}

void GoalState::transform_timing(double time_scale)
{
    //time_scale: t_step_size_prev / t_step_size_new
    //Thus e.g. for velocity:
    // v_old * time_scale = distance / (t * t_step_size_prev) * time_scale = distance / (t * t_step_size_new)

    if (velocity.has_value())
    {
        velocity->transform_coordinate_system(time_scale, 0, 0, 0);
    }
}

void GoalState::draw(const DrawingContext& ctx, double scale, double global_orientation, double global_translate_x, double global_translate_y, double local_orientation)
{
    assert(draw_configuration);
    assert(unique_id != "ERR");

    //Simple function that only draws the position (and orientation), but not the object itself
    ctx->save();

    //Perform required translation + rotation
    ctx->translate(global_translate_x, global_translate_y);
    ctx->rotate(global_orientation);

    //Draw goal position
    if(position.has_value())
    {
        position->draw(ctx, scale, 0, 0, 0, local_orientation);
    }

    //Rotation is an interval - draw position for orientation middle value
    if(orientation.has_value())
    {
        auto middle = orientation->get_interval_avg();

        ctx->save();
        ctx->set_source_rgba(.9,.2,.7,.5); //Color used for inexact values

        if(position.has_value())
        {
            //Try to draw in the middle of the shape of the goal
            position->transform_context(ctx, scale);
        }
        ctx->rotate(middle + local_orientation);

        //Draw arrow
        double arrow_scale = scale * transform_scale; //To quickly change the scale to your liking
        draw_arrow(ctx, 0.0, 0.0, 3.0 * arrow_scale, 0.0, 3.0 * arrow_scale);
        
        ctx->restore();
    }

    //Draw time, velocity description
    if (draw_configuration->draw_goal_description.load())
    {
        std::stringstream descr_stream;
        descr_stream << "ID (" << unique_id << "): ";
        if (time.has_value())
        {
            descr_stream << "t (mean): " << time.value().get_mean();
        }
        if (velocity.has_value())
        {
            auto avg = velocity.value().get_interval_avg();

            if (time.has_value())
            {
                descr_stream << ", v (mean): " << avg;
            }
            else
            {
                descr_stream << "v (mean): " << avg;
            }
        }
        //Goal information is only shown if a position has been set for the goal
        if (position.has_value())
        {
            position->transform_context(ctx, scale);

            //Draw set text. Re-scale text based on current zoom factor
            draw_text_centered(ctx, 0, 0, 0, 1200.0 / draw_configuration->zoom_factor.load(), descr_stream.str());
        }
    }

    ctx->restore();
}

const std::optional<IntervalOrExact>& GoalState::get_time() const
{
    return time;
}

const std::optional<Position>& GoalState::get_position() const
{
    return position;
}

const std::optional<Interval>& GoalState::get_orientation() const
{
    return orientation;
}

const std::optional<Interval>& GoalState::get_velocity() const
{
    return velocity;
}

CommonroadDDSGoalState GoalState::to_dds_msg(double time_step_size)
{
    CommonroadDDSGoalState goal_state;

    goal_state.time_set(time.has_value());
    if(time.has_value())
    {
        goal_state.time(time->to_dds_interval(time_step_size));
    }

    if (position.has_value())
    {
        goal_state.has_exact_position(position->is_exact());

        //The original commonroad specification only uses position intervals here, but we also support exact positions
        if (position->is_exact())
        {
            goal_state.exact_position(position->to_dds_point());
        }
        else
        {
            goal_state.position(position->to_dds_position_interval());
        }
    }
    if (orientation.has_value())
    {
        goal_state.orientation(orientation->to_dds_msg());
    }
    if (velocity.has_value())
    {
        goal_state.velocity(velocity->to_dds_msg());
    }

    return goal_state;
}

void GoalState::set_unique_id(std::string _unique_id)
{
    unique_id = _unique_id;
}

const std::string& GoalState::get_unique_id() const
{
    return unique_id;
}