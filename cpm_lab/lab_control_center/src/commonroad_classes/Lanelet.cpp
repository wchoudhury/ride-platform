#include "commonroad_classes/Lanelet.hpp"

/**
 * \file Lanelet.cpp
 * \ingroup lcc_commonroad
 */

Lanelet::Lanelet(
    const xmlpp::Node* node,
    std::map<int, std::pair<int, bool>>& traffic_sign_positions, 
    std::map<int, std::pair<int, bool>>& traffic_light_positions,
    std::shared_ptr<CommonroadDrawConfiguration> _draw_configuration
)
    :
    draw_configuration(_draw_configuration)
{
    //Check if node is of type lanelet
    assert(node->get_name() == "lanelet");

    commonroad_line = node->get_line();

    try
    {
        //Get the lanelet ID
        lanelet_id = xml_translation::get_attribute_int(node, "id", true).value();

        //2018 and 2020
        left_bound = translate_bound(node, "leftBound");
        right_bound = translate_bound(node, "rightBound");

        //Make sure that left and right bound are of equal size (points should be opposing pairs)
        if (left_bound.points.size() != right_bound.points.size())
        {
            std::stringstream error_msg_stream;
            error_msg_stream << "Left and right bounds of lanelet not of equal size (# of points), line: " << node->get_line();
            throw SpecificationError(error_msg_stream.str());
        }

        predecessors = translate_refs(node, "predecessor");
        successors = translate_refs(node, "successor");
        adjacent_left = translate_adjacent(node, "adjacentLeft");
        adjacent_right = translate_adjacent(node, "adjacentRight");

        //2018
        speed_limit = xml_translation::get_child_child_double(node, "speedLimit", false); //false bc. only in 2018 specs and only optional there

        //2020
        stop_line = translate_stopline(node, "stopLine");
        lanelet_type = translate_lanelet_type(node, "laneletType");
        user_one_way = translate_users(node, "userOneWay");
        user_bidirectional = translate_users(node, "userBidirectional");
        traffic_sign_refs = translate_refs(node, "trafficSignRef");
        traffic_light_refs = translate_refs(node, "trafficLightRef");

        //Add positional values for each traffic sign / light ref
        auto lanelet_id = xml_translation::get_attribute_int(node, "id", true).value();
        //First: Add positions stored for the lanelet itself
        for (const auto ref : traffic_sign_refs)
        {
            traffic_sign_positions[ref] = {lanelet_id, false};
        }
        for (const auto ref : traffic_light_refs)
        {
            traffic_light_positions[ref] = {lanelet_id, false};
        }
        //Then: Add or update positions defined for the stop_line
        if (stop_line.has_value())
        {
            for (const auto ref : stop_line->traffic_sign_refs)
            {
                traffic_sign_positions[ref] = {lanelet_id, true};
            }
            for (const auto ref : stop_line->traffic_light_ref)
            {
                traffic_light_positions[ref] = {lanelet_id, true};
            }
        }
    }
    catch(const SpecificationError& e)
    {
        throw SpecificationError(std::string("Could not translate Lanelet:\n") + e.what());
    }
    catch(...)
    {
        //Propagate error, if any subclass of CommonRoadScenario fails, then the whole translation should fail
        throw;
    }
    
    //According to the PDF Commonroad specs (not the actual XML .xsd specs), the size of the left and right bound
    //should be the same
    if (left_bound.points.size() != right_bound.points.size())
    {
        std::stringstream error_stream;
        error_stream << "Error in Lanelet: Left and right bound size does not match. Line: " << commonroad_line;
        throw SpecificationError(error_stream.str());
    }

    //Test output
    // std::cout << "Lanelet ------------------------------" << std::endl;
    // std::cout << "Left bound marking: (Not shown here)" << std::endl;
    // std::cout << "Right bound marking: (Not shown here)" << std::endl;

    // std::cout << "Predecessors refs: ";
    // for (int ref : predecessors)
    // {
    //     std:: cout << "| " << ref;
    // }
    // std::cout << std::endl;

    // std::cout << "Successor refs: ";
    // for (int ref : successors)
    // {
    //     std:: cout << "| " << ref;
    // }
    // std::cout << std::endl;

    // std::cout << "Adjacent left: " << adjacent_left.has_value() << "(exists)" << std::endl;
    // std::cout << "Adjacent right: " << adjacent_right.has_value() << "(exists)" << std::endl;

    // std::cout << "Speed limit (2018): " << speed_limit.value_or(-1.0) << std::endl;

    // std::cout << "Lanelet end --------------------------" << std::endl << std::endl;
}

Bound Lanelet::translate_bound(const xmlpp::Node* node, std::string name)
{
    const auto bound_node = xml_translation::get_child_if_exists(node, name, true); //True because this is a required part of both specs (2018 and 2020)

    //Same for both specs
    Bound bound;

    xml_translation::iterate_children(
        bound_node, 
        [&] (const xmlpp::Node* child) 
        {
            bound.points.push_back(Point(child));
        }, 
        "point"
    );

    //Warning if bound does not meet specification
    if (bound.points.size() < 2)
    {
        throw SpecificationError("Bound does not contain min amount of children");
    }

    const auto line_node = xml_translation::get_child_if_exists(bound_node, "lineMarking", false);
    if (line_node)
    {
        bound.line_marking = std::optional<LineMarking>(translate_line_marking(line_node));        
    }
    
    return bound;
}

std::vector<int> Lanelet::translate_refs(const xmlpp::Node* node, std::string name)
{
    //Refs are optional, so this element might not exist in the XML file
    std::vector<int> refs;
    
    //Get refs
    xml_translation::iterate_elements_with_attribute(
        node,
        [&] (std::string text) {
            auto optional_integer = xml_translation::string_to_int(text);
            if (optional_integer.has_value())
            {
                refs.push_back(optional_integer.value());
            }
            else
            {
                std::stringstream error_msg_stream;
                error_msg_stream << "At least one lanelet reference is not an integer - line " << node->get_line();
                throw SpecificationError(error_msg_stream.str());
            }
        },
        name,
        "ref"
    );

    return refs;
}

std::optional<Adjacent> Lanelet::translate_adjacent(const xmlpp::Node* node, std::string name)
{
    //Adjacents are optional, so this element might not exist
    const auto adjacent_node = xml_translation::get_child_if_exists(node, name, false);

    if (adjacent_node)
    {
        Adjacent adjacent_obj;
        auto adjacent = std::optional<Adjacent>(adjacent_obj);
        
        adjacent->ref_id = xml_translation::get_attribute_int(adjacent_node, "ref", true).value(); //As mentioned in other classes: Value must exist, else error is thrown, so .value() can be used safely here
    
        std::string direction_string = xml_translation::get_attribute_text(adjacent_node, "drivingDir", true).value(); //See comment above
        if(direction_string.compare("same") == 0)
        {
            adjacent->direction = DrivingDirection::Same;
        }
        else if(direction_string.compare("opposite") == 0)
        {
            adjacent->direction = DrivingDirection::Opposite;
        }
        else {
            std::stringstream error_msg_stream;
            error_msg_stream << "Specified driving direction not part of specs, in line " << adjacent_node->get_line();
            throw SpecificationError(error_msg_stream.str());
        }   

        return adjacent;
    }

    return std::nullopt;
}

std::optional<StopLine> Lanelet::translate_stopline(const xmlpp::Node* node, std::string name)
{
    //Stop lines are optional, so this element might not exist
    const auto line_node = xml_translation::get_child_if_exists(node, name, false);

    if (line_node)
    {
        StopLine line_obj;
        auto line = std::optional<StopLine>(line_obj);

        //Translate line points
        xml_translation::iterate_children(
            line_node, 
            [&] (const xmlpp::Node* child) 
            {
                line->points.push_back(Point(child));
            }, 
            "point"
        );

        if (line->points.size() > 2)
        {
            std::stringstream error_msg_stream;
            error_msg_stream << "Specified stop line has too many points, not part of specs, in line " << line_node->get_line();
            throw SpecificationError(error_msg_stream.str());
        }

        //If not enough points (2) are given, the spec says that the lanelet end points define the stop line
        if (line->points.size() < 2)
        {
            if (left_bound.points.size() < 1 || right_bound.points.size() < 1)
            {
                std::stringstream error_msg_stream;
                error_msg_stream << "Specified stop line has < 2 points, and lanelet has no bounds, in line " << line_node->get_line();
                throw SpecificationError(error_msg_stream.str());
            }

            //Set points to lanelet points
            line->points.clear();
            line->points.push_back(left_bound.points.back());
            line->points.push_back(right_bound.points.back());
        }

        //Translate line marking
        const auto line_marking = xml_translation::get_child_if_exists(line_node, "lineMarking", true);
        if (line_marking)
        {
            line->line_marking = translate_line_marking(line_marking);
        }

        //Translate refs to traffic signs and traffic lights
        line->traffic_sign_refs = translate_refs(line_node, "trafficSignRef");
        line->traffic_light_ref = translate_refs(line_node, "trafficLightRef");

        return line;
    }

    return std::nullopt;
}

std::vector<LaneletType> Lanelet::translate_lanelet_type(const xmlpp::Node* node, std::string name)
{
    std::vector<LaneletType> lanelet_vector;

    //Fill vector with existing values
    xml_translation::iterate_children(
        node, 
        [&] (const xmlpp::Node* child) 
        {
            std::string lanelet_type_string = xml_translation::get_first_child_text(child);
            LaneletType type_out;

            if (lanelet_type_string.compare("urban") == 0)
            {
                type_out = LaneletType::Urban;
            }
            else if (lanelet_type_string.compare("interstate") == 0)
            {
                type_out = LaneletType::Interstate;
            }
            else if (lanelet_type_string.compare("country") == 0)
            {
                type_out = LaneletType::Country;
            }
            else if (lanelet_type_string.compare("highway") == 0)
            {
                type_out = LaneletType::Highway;
            }
            else if (lanelet_type_string.compare("sidewalk") == 0)
            {
                type_out = LaneletType::Sidewalk;
            }
            else if (lanelet_type_string.compare("crosswalk") == 0)
            {
                type_out = LaneletType::Crosswalk;
            }
            else if (lanelet_type_string.compare("busLane") == 0)
            {
                type_out = LaneletType::BusLane;
            }
            else if (lanelet_type_string.compare("bicycleLane") == 0)
            {
                type_out = LaneletType::BicycleLane;
            }
            else if (lanelet_type_string.compare("exitRamp") == 0)
            {
                type_out = LaneletType::ExitRamp;
            }
            else if (lanelet_type_string.compare("mainCarriageWay") == 0)
            {
                type_out = LaneletType::MainCarriageWay;
            }
            else if (lanelet_type_string.compare("accessRamp") == 0)
            {
                type_out = LaneletType::AccessRamp;
            }
            else if (lanelet_type_string.compare("shoulder") == 0)
            {
                type_out = LaneletType::Shoulder;
            }
            else if (lanelet_type_string.compare("driveWay") == 0)
            {
                type_out = LaneletType::DriveWay;
            }
            else if (lanelet_type_string.compare("busStop") == 0)
            {
                type_out = LaneletType::BusStop;
            }
            else if (lanelet_type_string.compare("unknown") == 0)
            {
                type_out = LaneletType::Unknown;
            }
            else 
            {
                std::stringstream error_msg_stream;
                error_msg_stream << "Specified lanelet type not part of specs, in line " << child->get_line();
                throw SpecificationError(error_msg_stream.str());
            }

            lanelet_vector.push_back(type_out);
        }, 
        name
    );

    return lanelet_vector;
}

std::vector<VehicleType> Lanelet::translate_users(const xmlpp::Node* node, std::string name)
{
    //Users are optional, so this element might not exist (also: 2020 specs only) 
    std::vector<VehicleType> vehicle_vector;

    //Fill vector with existing values
    xml_translation::iterate_children(
        node, 
        [&] (const xmlpp::Node* child) 
        {
            std::string user_string = xml_translation::get_first_child_text(child);
            VehicleType type_out;

            if (user_string.compare("vehicle") == 0)
            {
                type_out = VehicleType::Vehicle;
            }
            else if (user_string.compare("car") == 0)
            {
                type_out = VehicleType::Car;
            }
            else if (user_string.compare("truck") == 0)
            {
                type_out = VehicleType::Truck;
            }
            else if (user_string.compare("bus") == 0)
            {
                type_out = VehicleType::Bus;
            }
            else if (user_string.compare("motorcycle") == 0)
            {
                type_out = VehicleType::Motorcycle;
            }
            else if (user_string.compare("bicycle") == 0)
            {
                type_out = VehicleType::Bicycle;
            }
            else if (user_string.compare("pedestrian") == 0)
            {
                type_out = VehicleType::Pedestrian;
            }
            else if (user_string.compare("priorityVehicle") == 0)
            {
                type_out = VehicleType::PriorityVehicle;
            }
            else if (user_string.compare("train") == 0)
            {
                type_out = VehicleType::Train;
            }
            else if (user_string.compare("taxi") == 0)
            {
                type_out = VehicleType::Taxi;
            }
            else 
            {
                std::stringstream error_msg_stream;
                error_msg_stream << "Specified vehicle type not part of specs, in line " << child->get_line();
                throw SpecificationError(error_msg_stream.str());
            }

            vehicle_vector.push_back(type_out);
        }, 
        name
    );

    return vehicle_vector;
}

LineMarking Lanelet::translate_line_marking(const xmlpp::Node* line_node)
{
    std::string line_marking_text = xml_translation::get_first_child_text(line_node);
    if (line_marking_text.compare("dashed") == 0)
    {
        //2018 and 2020
        return LineMarking::Dashed;
    }
    else if (line_marking_text.compare("solid") == 0)
    {
        //2018 and 2020
        return LineMarking::Solid;
    }
    else if (line_marking_text.compare("broad_dashed") == 0)
    {
        //2020 only
        return LineMarking::BroadDashed;
    }
    else if (line_marking_text.compare("broad_solid") == 0)
    {
        //2020 only
        return LineMarking::BroadSolid;
    }
    else if (line_marking_text.compare("unknown") == 0)
    {
        //2020 only
        return LineMarking::Unknown;
    }
    else if (line_marking_text.compare("no_marking") == 0)
    {
        //2020 only
        return LineMarking::NoMarking;
    }
    else
    {
        std::stringstream error_msg_stream;
        error_msg_stream << "Specified line marking not part of specs, in line " << line_node->get_line();
        throw SpecificationError(error_msg_stream.str());
    }
}

double Lanelet::get_min_width()
{
    assert(left_bound.points.size() == right_bound.points.size());

    size_t min_vec_size = std::max(left_bound.points.size(), right_bound.points.size());

    double min_width = -1.0;
    for (size_t i = 0; i < min_vec_size; ++i)
    {
        //Width = sqrt(diff_x^2 + diff_y^2) -> Ignore third dimension for this (as we do not need it)
        double new_width = std::sqrt(std::pow((left_bound.points.at(i).get_x() - right_bound.points.at(i).get_x()), 2) + std::pow((left_bound.points.at(i).get_y() - right_bound.points.at(i).get_y()), 2));

        if (min_width < 0 || new_width < min_width)
        {
            min_width = new_width;
        }
    }

    return min_width;
}

void Lanelet::set_boundary_style(const DrawingContext& ctx, std::optional<LineMarking> line_marking, double dash_length)
{
    if (line_marking.has_value())
    {
        //Set or disable dashes
        if (line_marking.value() == LineMarking::BroadDashed || line_marking.value() == LineMarking::Dashed)
        {
            std::vector<double> dashes {dash_length};
            ctx->set_dash(dashes, 0.0);
        }
        else
        {
            //Disable dashes
            std::vector<double> dashes {};
            ctx->set_dash(dashes, 0.0);
        }
        
        //Set line width
        if (line_marking.value() == LineMarking::BroadSolid || line_marking.value() == LineMarking::BroadDashed)
        {
            ctx->set_line_width(0.03);
        }
        else
        {
            ctx->set_line_width(0.005);
        }

        //Set line color for unknown and no_marking (the latter should still be shown to the user, but almost-white)
        /* if (line_marking.value() == LineMarking::Unknown)
        {
            //More red
            ctx->set_source_rgb(0.9,0.4,0.4);
        }
        else*/ if (line_marking.value() == LineMarking::NoMarking)
        {
            //Almost-transparent
            ctx->set_source_rgba(0.5,0.5,0.5,0.1);
        }
        else
        {
            //Default lanelet color
            ctx->set_source_rgb(0.5,0.5,0.5);
        }
    }
    else
    {
        //Disable dashes
        std::vector<double> dashes {};
        ctx->set_dash(dashes, 0.0);

        ctx->set_line_width(0.005);
    }
    
}

std::string Lanelet::to_text(LaneletType lanelet_type)
{
    switch (lanelet_type)
    {
        case LaneletType::AccessRamp:
            return "AccessRamp";
            break;
        case LaneletType::BicycleLane:
            return "BicycleLane";
            break;
        case LaneletType::BusLane:
            return "BusLane";
            break;
        case LaneletType::BusStop:
            return "BusStop";
            break;
        case LaneletType::Country:
            return "Country";
            break;
        case LaneletType::Crosswalk:
            return "Crosswalk";
            break;
        case LaneletType::DriveWay:
            return "DriveWay";
            break;
        case LaneletType::ExitRamp:
            return "ExitRamp";
            break;
        case LaneletType::Highway:
            return "Highway";
            break;
        case LaneletType::Interstate:
            return "Interstate";
            break;
        case LaneletType::MainCarriageWay:
            return "MainCarriageWay";
            break;
        case LaneletType::Shoulder:
            return "Shoulder";
            break;
        case LaneletType::Sidewalk:
            return "Sidewalk";
            break;
        case LaneletType::Unknown:
            return "Unknown";
            break;
        case LaneletType::Urban:
            return "Urban";
            break;
    }

    return "Error";
}

std::string Lanelet::to_text(VehicleType vehicle_type)
{
    switch ((vehicle_type))
    {
    case VehicleType::Bicycle:
        return "Bicycle";
        break;
    case VehicleType::Bus:
        return "Bus";
        break;
    case VehicleType::Car:
        return "Car";
        break;
    case VehicleType::Motorcycle:
        return "Motorcycle";
        break;
    case VehicleType::Pedestrian:
        return "Pedestrian";
        break;
    case VehicleType::PriorityVehicle:
        return "PriorityVehicle";
        break;
    case VehicleType::Taxi:
        return "Taxi";
        break;
    case VehicleType::Train:
        return "Train";
        break;
    case VehicleType::Truck:
        return "Truck";
        break;
    case VehicleType::Vehicle:
        return "Vehicle";
        break;
    }

    return "Error";
}


/******************************Interface functions***********************************/

void Lanelet::transform_coordinate_system(double scale, double angle, double translate_x, double translate_y)
{
    if (stop_line.has_value())
    {
        for (auto& point : stop_line->points)
        {
            point.transform_coordinate_system(scale, angle, translate_x, translate_y);
        }
    }

    if (speed_limit.has_value())
    {
        auto new_speed_limit = speed_limit.value() * scale;
        speed_limit = std::optional<double>(new_speed_limit);
    }

    for (auto& point : right_bound.points)
    {
        point.transform_coordinate_system(scale, angle, translate_x, translate_y);
    }

    for (auto& point : left_bound.points)
    {
        point.transform_coordinate_system(scale, angle, translate_x, translate_y);
    }
}

//Suppress warning for unused parameter (s)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Lanelet::draw(const DrawingContext& ctx, double scale, double global_orientation, double global_translate_x, double global_translate_y, double local_orientation)
{
    assert(draw_configuration);

    //Local orientation does not really make sense here, so it is ignored
    ctx->save();

    //Perform required translation + rotation
    //Local orientation is irrelevant here (we do not want to rotate the lanelet around its center)
    ctx->translate(global_translate_x, global_translate_y);
    ctx->rotate(global_orientation);

    ctx->set_line_width(0.005);

    //Draw lines between points
    if (left_bound.points.size() > 0 && right_bound.points.size() > 0)
    {
        //From the Commonroad specs: All laterally adjacent lanes have the same length -> 
        //if we want to modify the boundary based on adjacent lanes, we just need to check
        //the content of adjacent

        //Draw left bound with set line marking / boundary style
        ctx->save();
        ctx->begin_new_path();
        set_boundary_style(ctx, left_bound.line_marking, 0.03);
        if (adjacent_left.has_value())
        {
            //Adjust boundary style to make adjacency to other lanelet more visible
            if (adjacent_left->direction == DrivingDirection::Same)
            {
                //Adjacent lanelet boundaries are drawn in blue
                ctx->set_source_rgb(0.03, 0.65, 0.74);
            }
        }
        ctx->move_to(left_bound.points.at(0).get_x() * scale, left_bound.points.at(0).get_y() * scale);
        for (auto point : left_bound.points)
        {
            //Draw lines between points
            ctx->line_to(point.get_x() * scale, point.get_y() * scale);
        }
        ctx->stroke();
        ctx->restore();

        //Draw right bound with set line marking / boundary style
        ctx->save();
        ctx->begin_new_path();
        set_boundary_style(ctx, right_bound.line_marking, 0.03);
        if (adjacent_right.has_value())
        {
            //Adjust boundary style to make adjacency to other lanelet more visible
            if (adjacent_right->direction == DrivingDirection::Same)
            {
                //Adjacent lanelet boundaries are drawn in blue
                ctx->set_source_rgb(0.03, 0.65, 0.74);
            }
        }
        ctx->move_to(right_bound.points.at(0).get_x() * scale, right_bound.points.at(0).get_y() * scale);
        for (auto point : right_bound.points)
        {
            //Draw lines between points
            ctx->line_to(point.get_x() * scale, point.get_y() * scale);
        }
        ctx->stroke();
        ctx->restore();
    }


    //Draw arrows for lanelet orientation
   if (draw_configuration->draw_lanelet_orientation.load())
    {
        //These things must be true, or else the program should already have thrown an error before / the calculation above is wrong
        assert(left_bound.points.size() == right_bound.points.size());
        size_t arrow_start_pos = 0;
        size_t arrow_end_pos = 1;

        //Set arrow color
        ctx->set_source_rgba(0.0, 0.0, 0.0, 0.1);

        //Use another form for bidirectional lanelets - TODO @Max: Filter by User?
        bool is_bidirectional = user_bidirectional.size() > 0;

        //Draw arrows
        while (arrow_end_pos < left_bound.points.size())
        {
            double x_1 = (0.5 * left_bound.points.at(arrow_start_pos).get_x() + 0.5 * right_bound.points.at(arrow_start_pos).get_x());
            double y_1 = (0.5 * left_bound.points.at(arrow_start_pos).get_y() + 0.5 * right_bound.points.at(arrow_start_pos).get_y());
            double x_2 = (0.5 * left_bound.points.at(arrow_end_pos).get_x() + 0.5 * right_bound.points.at(arrow_end_pos).get_x());
            double y_2 = (0.5 * left_bound.points.at(arrow_end_pos).get_y() + 0.5 * right_bound.points.at(arrow_end_pos).get_y());

            //Don't draw an arrow for bidirectional users, but another form
            //Drawing nothing would be confusing (arrows don't always have the same distance, so this could be misinterpreted)
            if (!is_bidirectional)
            {
                draw_arrow(ctx, x_1, y_1, x_2, y_2, scale * 0.33); //0.33 because they are a bit too large
            }
            else
            {
                ctx->save();

                ctx->set_source_rgba(0.0, 0.0, 0.0, 0.2);

                double center_x = 0.5 * x_1 + 0.5 * x_2;
                double center_y = 0.5 * y_1 + 0.5 * y_2;
                double radius = 0.1 * std::max(abs(x_2 - x_1), abs(y_2 - y_1));

                //Move to center
                ctx->move_to(center_x, center_y);

                //Draw circle
                ctx->arc(center_x, center_y, radius * scale, 0.0, 2 * M_PI);
                ctx->stroke();

                ctx->restore();
            }

            ++arrow_start_pos;
            ++arrow_end_pos;
        }
    }

    //Draw stop lines - we already made sure that it consists of two points in translation
    if (stop_line.has_value())
    {
        ctx->begin_new_path();
        ctx->set_source_rgb(.9,.3,.3);
        set_boundary_style(ctx, std::optional<LineMarking>{stop_line->line_marking}, 0.03);

        
        ctx->move_to(stop_line->points.at(0).get_x() * scale, stop_line->points.at(0).get_y() * scale);
        ctx->line_to(stop_line->points.at(1).get_x() * scale, stop_line->points.at(1).get_y() * scale);
        ctx->stroke();
    }

    //Draw lanelet ID
    if (draw_configuration->draw_lanelet_id.load())
    {
        ctx->save();

        std::stringstream descr_stream;
        descr_stream << "ID: " << lanelet_id;
        
        //Move to lanelet center for text, draw centered around it, rotate by angle of lanelet
        auto center = get_center();
        ctx->translate(center.first, center.second);
        //Calculate lanelet angle (trivial solution used here will not work properly for arcs etc)
        auto alpha = atan((left_bound.points.rbegin()->get_y() - left_bound.points.at(0).get_y()) / (left_bound.points.rbegin()->get_x() - left_bound.points.at(0).get_x())); //alpha = arctan(dy / dx)
        
        //Draw set text. Re-scale text based on current zoom factor
        draw_text_centered(ctx, 0, 0, alpha, 1200.0 / draw_configuration->zoom_factor.load(), descr_stream.str());

        ctx->restore();
    }

    //Type, speed, user restriction etc. are all shown in a table within the UI, if present, w.r.t. the lanelet ID

    ctx->restore();
}
#pragma GCC diagnostic pop

void Lanelet::draw_ref(const DrawingContext& ctx, double scale, double global_orientation, double global_translate_x, double global_translate_y)
{
    //Local orientation does not really make sense here, so it is ignored
    ctx->save();

    //Perform required translation + rotation
    //Local orientation is irrelevant here
    ctx->translate(global_translate_x, global_translate_y);
    ctx->rotate(global_orientation);

    ctx->set_line_width(0.005);

    //Draw lines between points
    if (left_bound.points.size() > 0 && right_bound.points.size() > 0)
    {
        ctx->begin_new_path();
        ctx->move_to(left_bound.points.at(0).get_x() * scale, left_bound.points.at(0).get_y() * scale);

        //Draw lines on left side, then switch to right side & draw in backwards order (-> draw rectangle)
        for (auto point : left_bound.points)
        {
            ctx->line_to(point.get_x() * scale, point.get_y() * scale);
        }

        for(auto reverse_it_point = right_bound.points.rbegin(); reverse_it_point != right_bound.points.rend(); ++reverse_it_point)
        {
            ctx->line_to(reverse_it_point->get_x() * scale, reverse_it_point->get_y() * scale);
        }

        //Close rectangle
        ctx->line_to(left_bound.points.at(0).get_x() * scale, left_bound.points.at(0).get_y() * scale);

        ctx->fill_preserve();
        ctx->stroke();
    }

    ctx->restore();
}

std::pair<double, double> Lanelet::get_center()
{
    //The calculation of the center follows a simple assumption: Center = Mean of middle segment (middle value of all points might not be within the lanelet boundaries)
    assert(left_bound.points.size() == right_bound.points.size());

    size_t vec_size = left_bound.points.size();
    size_t middle_index = static_cast<size_t>(static_cast<double>(vec_size) / 2.0);

    double x = (0.5 * left_bound.points.at(middle_index).get_x() + 0.5 * right_bound.points.at(middle_index).get_x());
    double y = (0.5 * left_bound.points.at(middle_index).get_y() + 0.5 * right_bound.points.at(middle_index).get_y());

    return std::pair<double, double>(x, y);
}

std::pair<double, double> Lanelet::get_center_of_all_points()
{
    //The calculation of the center follows a simple assumption: Center = Mean of all points
    assert(left_bound.points.size() == right_bound.points.size());

    size_t vec_size = left_bound.points.size();
    
    double x = 0.0;
    double y = 0.0;
    for (size_t index = 0; index < vec_size; ++index)
    {
        //Calculate avg iteratively to avoid overflow
        x += ((left_bound.points.at(index).get_x() + right_bound.points.at(index).get_x()) / 2.0 - x) / (index + 1);
        y += ((left_bound.points.at(index).get_y() + right_bound.points.at(index).get_y()) / 2.0 - y) / (index + 1);
    }

    return std::pair<double, double>(x, y);
}

std::optional<std::pair<double, double>> Lanelet::get_stopline_center()
{
    if (stop_line.has_value())
    {
        //We already made sure during translation that Point actually consists of two points
        return std::optional<std::pair<double, double>>({
            stop_line->points.at(0).get_x() * 0.5 + stop_line->points.at(1).get_x() * 0.5,
            stop_line->points.at(0).get_y() * 0.5 + stop_line->points.at(1).get_y() * 0.5
        });
    }
    else
    {
        return std::nullopt;
    }
}

std::pair<double, double> Lanelet::get_end_center()
{
    auto left_end = left_bound.points.back();
    auto right_end = right_bound.points.back();

    return std::pair<double, double>(
        left_end.get_x() * 0.5 + right_end.get_x() * 0.5, 
        left_end.get_y() * 0.5 + right_end.get_y() * 0.5
    );
}

std::optional<std::array<std::array<double, 2>, 2>> Lanelet::get_range_x_y()
{
    //The calculation of the center follows a simple assumption: Center = Middle of middle segment (middle value of all points might not be within the lanelet boundaries)
    assert(left_bound.points.size() == right_bound.points.size());

    size_t vec_size = left_bound.points.size();

    //Return empty optional if points are missing
    if (vec_size == 0)
    {
        return std::nullopt;
    }
    
    //Working with numeric limits at start lead to unforseeable behaviour with min and max, thus we now use this approach instead
    bool uninitialized = true;
    double x_min, x_max, y_min, y_max;
    for (size_t index = 0; index < vec_size; ++index)
    {
        if (uninitialized)
        {
            x_min = left_bound.points.at(index).get_x();
            y_min = left_bound.points.at(index).get_y();
            x_max = left_bound.points.at(index).get_x();
            y_max = left_bound.points.at(index).get_y();
            uninitialized = false;
        }

        x_min = std::min(left_bound.points.at(index).get_x(), x_min);
        y_min = std::min(left_bound.points.at(index).get_y(), y_min);
        x_max = std::max(left_bound.points.at(index).get_x(), x_max);
        y_max = std::max(left_bound.points.at(index).get_y(), y_max);

        x_min = std::min(right_bound.points.at(index).get_x(), x_min);
        y_min = std::min(right_bound.points.at(index).get_y(), y_min);
        x_max = std::max(right_bound.points.at(index).get_x(), x_max);
        y_max = std::max(right_bound.points.at(index).get_y(), y_max);
    }

    std::array<std::array<double, 2>, 2> result;
    result[0][0] = x_min;
    result[0][1] = x_max;
    result[1][0] = y_min;
    result[1][1] = y_max;

    return result;
}


std::vector<Point> Lanelet::get_shape()
{
    std::vector<Point> shape; 

    for (auto point : left_bound.points)
    {
        shape.push_back(point);
    }

    for(auto reverse_it_point = right_bound.points.rbegin(); reverse_it_point != right_bound.points.rend(); ++reverse_it_point)
    {
        shape.push_back(*reverse_it_point);
    }

    return shape;
}

std::string Lanelet::get_speed_limit()
{
    std::stringstream speed_limit_stream;
    if (speed_limit.has_value())
    {
        speed_limit_stream << speed_limit.value();
    }

    return speed_limit_stream.str();
}

std::string Lanelet::get_lanelet_type()
{
    std::stringstream lanelet_stream;
    for (size_t i = 0; i < lanelet_type.size(); ++i)
    {
        lanelet_stream << to_text(lanelet_type[i]);

        if (i < lanelet_type.size() - 1)
        {
            lanelet_stream << ", ";
        }
    }

    return lanelet_stream.str();
}

std::string Lanelet::get_user_one_way()
{
    std::stringstream user_stream;
    for (size_t i = 0; i < user_one_way.size(); ++i)
    {
        user_stream << to_text(user_one_way[i]);

        if (i < user_one_way.size() - 1)
        {
            user_stream << ", ";
        }
    }

    return user_stream.str();
}

std::string Lanelet::get_user_bidirectional()
{
    std::stringstream user_stream;
    for (size_t i = 0; i < user_bidirectional.size(); ++i)
    {
        user_stream << to_text(user_bidirectional[i]);

        if (i < user_bidirectional.size() - 1)
        {
            user_stream << ", ";
        }
    }

    return user_stream.str();
}

bool Lanelet::has_relevant_table_info()
{
    return user_bidirectional.size() > 0
        || user_one_way.size() > 0
        || speed_limit.has_value()
        ||
            (! (std::find(lanelet_type.begin(), lanelet_type.end(), LaneletType::Unknown) != lanelet_type.end() &&
            lanelet_type.size() == 1));
}