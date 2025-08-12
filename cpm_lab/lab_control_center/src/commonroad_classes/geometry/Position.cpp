#include "commonroad_classes/geometry/Position.hpp"

/**
 * \file Position.cpp
 * \ingroup lcc_commonroad
 */

Position::Position(const xmlpp::Node* node)
{
    //Check if node is of type position
    assert(node->get_name() == "position");

    try
    {
        //2018 and 2020
        const auto point_node = xml_translation::get_child_if_exists(node, "point", false); //not mandatory
        if (point_node)
        {
            point = std::optional<Point>(std::in_place, point_node);
        }

        commonroad_line = node->get_line();
        
        //Optional parts (all unbounded -> lists)
        xml_translation::iterate_children(
            node,
            [&] (const xmlpp::Node* child)
            {
                circles.push_back(Circle(child));
            },
            "circle"
        );

        xml_translation::iterate_children(
            node,
            [&] (const xmlpp::Node* child)
            {
                lanelet_refs.push_back(xml_translation::get_attribute_int(child, "ref", true).value()); //As mentioned in other classes: Value must exist, else error is thrown, so .value() can be used safely here
            },
            "lanelet"
        );

        xml_translation::iterate_children(
            node,
            [&] (const xmlpp::Node* child)
            {
                polygons.push_back(Polygon(child));
            },
            "polygon"
        );

        xml_translation::iterate_children(
            node,
            [&] (const xmlpp::Node* child)
            {
                rectangles.push_back(Rectangle(child));
            },
            "rectangle"
        );
    }
    catch(const SpecificationError& e)
    {
        throw SpecificationError(std::string("Could not translate Position:\n") + e.what());
    }
    catch(...)
    {
        //Propagate error, if any subclass of CommonRoadScenario fails, then the whole translation should fail
        throw;
    }

    //According to the PDF Commonroad specs (not the actual XML .xsd specs), at least one of the entries should contain an element
    //This makes sense - an empty position is meaningless
    if (circles.size() == 0 && polygons.size() == 0 && rectangles.size() == 0 && lanelet_refs.size() == 0 && !(point.has_value()))
    {
        std::stringstream error_stream;
        error_stream << "Error in Position: Is empty. Line: " << commonroad_line;
        throw SpecificationError(error_stream.str());
    }

    //Also, point / rectangle etc. / lanelet ref are mutually exclusive, but the .xsd specs don't reflect this, thus this is not checked here

    //Test output
    // std::cout << "Position:" << std::endl;
    // std::cout << "\tPoint exists: " << point.has_value() << std::endl;
    // std::cout << "\tCircle size: " << circles.size() << std::endl;
    // std::cout << "\tLanelet ref size: " << lanelet_refs.size() << std::endl;
    // std::cout << "\tPolygon size: " << polygons.size() << std::endl;
    // std::cout << "\tRectangle size: " << rectangles.size() << std::endl;
}

//Suppress warning for unused parameter (s)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
Position::Position(int irrelevant_int)
{
    //If a position value is not given for a datatype where a position is necessary for its interpretation, we interpret this as a sign to use a 'default position' instead
    //We use this constructor because we do not want a default constructor to exist, but the parameter is actually pointless
    
    //As mentioned in the documentation, the default position is probably the origin
    point = std::optional<Point>(std::in_place, -1);
}
#pragma GCC diagnostic pop

void Position::set_lanelet_ref_draw_function(std::function<void (int, const DrawingContext&, double, double, double, double)> _draw_lanelet_refs)
{
    draw_lanelet_refs = _draw_lanelet_refs;
}

void Position::set_lanelet_get_center_function(std::function<std::pair<double, double> (int)> _get_lanelet_center)
{
    get_lanelet_center = _get_lanelet_center;
}

void Position::transform_coordinate_system(double scale, double angle, double translate_x, double translate_y)
{
    if (point.has_value())
    {
        point->transform_coordinate_system(scale, angle, translate_x, translate_y);
    }

    for (auto& circle : circles)
    {
        circle.transform_coordinate_system(scale, angle, translate_x, translate_y);
    }

    for (auto& polygon : polygons)
    {
        polygon.transform_coordinate_system(scale, angle, translate_x, translate_y);
    }

    for (auto& rectangle : rectangles)
    {
        rectangle.transform_coordinate_system(scale, angle, translate_x, translate_y);
    }

    if (scale > 0)
    {
        transform_scale *= scale;
    }
}

void Position::draw(const DrawingContext& ctx, double scale, double global_orientation, double global_translate_x, double global_translate_y, double local_orientation)
{
    //Simple function that only draws the position (and orientation), but not the object itself
    ctx->save();

    //Perform required translation + rotation
    ctx->translate(global_translate_x, global_translate_y);
    ctx->rotate(global_orientation);
    
    //Rotate, if necessary
    if(point.has_value())
    {
        point->draw(ctx, scale);

        //Draw circle around point for better visibility
        double radius = 0.75 * scale * transform_scale;
        ctx->set_line_width(0.005);
        ctx->arc(point->get_x() * scale, point->get_y() * scale, radius * scale, 0.0, 2 * M_PI);
        ctx->stroke();
    }
    
    //Also draw "inexact" positional values, not only the point value
    {
        //Rotation implementation: Rotate around overall center of the shape, then translate back to original coordinate system (but rotated), then draw
        ctx->save();

        auto center = get_center();
        ctx->translate(center.first * scale, center.second * scale);
        ctx->rotate(local_orientation);
        ctx->translate(-1.0 * center.first * scale, -1.0 * center.second * scale);

        for (auto circle : circles)
        {
            circle.draw(ctx, scale, 0, 0, 0, 0);
        }
        for (auto polygon : polygons)
        {
            polygon.draw(ctx, scale, 0, 0, 0, 0);
        }
        for (auto rectangle : rectangles)
        {
            rectangle.draw(ctx, scale, 0, 0, 0, 0);
        }

        if (lanelet_refs.size() > 0)
        {
            if (draw_lanelet_refs)
            {
                for (auto lanelet_ref : lanelet_refs)
                {
                    draw_lanelet_refs(lanelet_ref, ctx, scale, 0, 0, 0);
                }
            }
            else
            {
                LCCErrorLogger::Instance().log_error("Cannot draw without lanelet ref function in Position, set function callback beforehand!");
            }
        }

        ctx->restore();
    }

    ctx->restore();
}

void Position::transform_context(const DrawingContext& ctx, double scale)
{
    //Rotate, if necessary
    if(point.has_value())
    {
        ctx->translate(point->get_x() * scale, point->get_y() * scale);        
    }
    else
    {
        //TODO: Is the center the best point to choose?
        auto center = get_center();
        ctx->translate(center.first * scale, center.second * scale);

        if (circles.size() == 0 && polygons.size() == 0 && rectangles.size() == 0 && lanelet_refs.size() == 0)
        {
            std::stringstream error_stream;
            error_stream << "Cannot transform context in Position when position is empty, from line " << commonroad_line;
            LCCErrorLogger::Instance().log_error(error_stream.str());
        }
    }
}

std::pair<double, double> Position::get_center()
{
    //Return either exact point value or center value of inexact position
    if (point.has_value())
    {
        return std::pair<double, double>(point->get_x(), point->get_y());
    }

    assert(std::numeric_limits<double>::has_infinity);
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = - std::numeric_limits<double>::infinity();
    double max_y = - std::numeric_limits<double>::infinity();

    for (auto circle : circles)
    {
        auto center = circle.get_center();
        min_x = std::min(min_x, center.first);
        min_y = std::min(min_y, center.second);
        max_x = std::max(max_x, center.first);
        max_y = std::max(max_y, center.second);
    }

    for (auto polygon : polygons)
    {
        auto center = polygon.get_center();
        min_x = std::min(min_x, center.first);
        min_y = std::min(min_y, center.second);
        max_x = std::max(max_x, center.first);
        max_y = std::max(max_y, center.second);
    }

    for (auto rectangle : rectangles)
    {
        auto center = rectangle.get_center();
        min_x = std::min(min_x, center.first);
        min_y = std::min(min_y, center.second);
        max_x = std::max(max_x, center.first);
        max_y = std::max(max_y, center.second);
    }

    if (get_lanelet_center)
    {
        for (auto lanelet_ref : lanelet_refs)
        {
            auto center = get_lanelet_center(lanelet_ref);
            min_x = std::min(min_x, center.first);
            min_y = std::min(min_y, center.second);
            max_x = std::max(max_x, center.first);
            max_y = std::max(max_y, center.second);
        }
    }
    else if (lanelet_refs.size() > 0)
    {
        LCCErrorLogger::Instance().log_error("Cannot compute center properly without lanelet center function in Position, set function callback beforehand!");
    }

    return std::pair<double, double>(0.5 * min_x + 0.5 * max_x, 0.5 * min_y + 0.5 * max_y);
}

std::optional<int> Position::get_lanelet_ref()
{
    if (lanelet_refs.size() > 1)
    {
        std::stringstream error_stream;
        error_stream << "In Position: Cannot handle more than one lanelet ref, from line " << commonroad_line;
        LCCErrorLogger::Instance().log_error(error_stream.str());

        return std::optional<int>(lanelet_refs.at(0));
    }
    else if (lanelet_refs.size() == 1)
    {
        return std::optional<int>(lanelet_refs.at(0));
    }
    else 
    {
        return std::nullopt;
    }
}

bool Position::is_exact()
{
    if (point.has_value())
        return true;

    return false;
}

bool Position::position_is_lanelet_ref()
{
    return (lanelet_refs.size() > 0) && !(point.has_value()) && (circles.size() == 0) && (polygons.size() == 0) && (rectangles.size() == 0);
}

std::optional<Point> Position::get_point()
{
    return point;
}

const std::vector<Circle>& Position::get_circles() const
{
    return circles;
}

const std::vector<int>& Position::get_lanelet_refs() const
{
    return lanelet_refs;
}

const std::vector<Polygon>& Position::get_polygons() const
{
    return polygons;
}

const std::vector<Rectangle>& Position::get_rectangles() const
{
    return rectangles;
}

CommonroadDDSPositionInterval Position::to_dds_position_interval()
{
    //Throw error if conversion is invalid because of position type
    if (is_exact())
    {
        throw std::runtime_error("Position cannot be translated to DDS Interval, is exact");
    }

    CommonroadDDSPositionInterval position_interval;

    std::vector<CommonroadDDSCircle> vector_circles;
    std::vector<CommonroadDDSPolygon> vector_polygons;
    std::vector<CommonroadDDSRectangle> vector_rectangles;
    std::vector<int32_t> vector_lanelet_refs;

    for (auto& circle : circles)
    {
        vector_circles.push_back(circle.to_dds_msg());
    }
    for (auto& polygon : polygons)
    {
        vector_polygons.push_back(polygon.to_dds_msg());
    }
    for (auto& rectangle : rectangles)
    {
        vector_rectangles.push_back(rectangle.to_dds_msg());
    }
    for (auto& lanelet_ref : lanelet_refs)
    {
        vector_lanelet_refs.push_back(static_cast<int32_t>(lanelet_ref));
    }

    position_interval.circles(rti::core::vector<CommonroadDDSCircle>(vector_circles));
    position_interval.polygons(rti::core::vector<CommonroadDDSPolygon>(vector_polygons));
    position_interval.rectangles(rti::core::vector<CommonroadDDSRectangle>(vector_rectangles));
    position_interval.lanelet_refs(rti::core::vector<int32_t>(vector_lanelet_refs));

    return position_interval;
}

CommonroadDDSPoint Position::to_dds_point()
{
    //Throw error if conversion is invalid because of position type
    if (!is_exact())
    {
        throw std::runtime_error("Position cannot be translated to DDS Point, is interval");
    }

    return point->to_dds_msg();
}