#include "commonroad_classes/geometry/Circle.hpp"

/**
 * \file Circle.cpp
 * \ingroup lcc_commonroad
 */

Circle::Circle(const xmlpp::Node* node)
{
    //Check if node is of type circle
    assert(node->get_name() == "circle");

    try
    {
        radius = xml_translation::get_child_child_double(node, "radius", true).value(); //mandatory, error thrown if nonexistant, so we can use .value() here

        if (radius < 0)
        {
            throw SpecificationError(std::string("Could not translate Circle - radius is smaller than zero"));
        }

        //Get point value, which must not be specified
        const auto point_node = xml_translation::get_child_if_exists(node, "center", false);
        if (point_node)
        {
            center = std::optional<Point>{std::in_place, point_node};
        }
        else
        {
            //Use default-value constructor (parameter is irrelevant)
            center = std::optional<Point>{std::in_place, 0};
        }
    }
    catch(const SpecificationError& e)
    {
        throw SpecificationError(std::string("Could not translate Circle:\n") + e.what());
    }
    catch(...)
    {
        //Propagate error, if any subclass of CommonRoadScenario fails, then the whole translation should fail
        throw;
    }

    //Test output
    // std::cout << "Circle:" << std::endl;
    // std::cout << "\tRadius: " << radius << std::endl;
    // std::cout << "\tCenter set: " << center.has_value() << std::endl;
}

void Circle::transform_coordinate_system(double scale, double angle, double translate_x, double translate_y)
{
    if (scale > 0)
    {
        radius *= scale;
    }
    center->transform_coordinate_system(scale, angle, translate_x, translate_y);
}

//Suppress warning for unused parameter (s)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Circle::draw(const DrawingContext& ctx, double scale, double global_orientation, double global_translate_x, double global_translate_y, double local_orientation)
{
    ctx->save();

    //Perform required translation + rotation
    //Local rotation does not really make sense here and is thus ignored (rotating a circle in its own coordinate system is pointless)
    ctx->translate(global_translate_x, global_translate_y);
    ctx->rotate(global_orientation);

    ctx->set_line_width(0.005);

    //Move to center
    ctx->move_to(center->get_x() * scale, center->get_y() * scale);

    //Draw circle
    ctx->arc(center->get_x() * scale, center->get_y() * scale, radius * scale, 0.0, 2 * M_PI);
    ctx->stroke();

    ctx->restore();
}
#pragma GCC diagnostic pop

std::pair<double, double> Circle::get_center()
{
    if (center.has_value())
    {
        return std::pair<double, double>(center->get_x(), center->get_y());
    }
    else
    {
        //Return "default value", though this should never be reached if the constructor worked
        auto default_center = Point(-1);
        return std::pair<double, double>(default_center.get_x(), default_center.get_y());
    }
}

CommonroadDDSCircle Circle::to_dds_msg()
{
    CommonroadDDSCircle circle;

    if (center.has_value())
    {
        circle.center(center->to_dds_msg());
    }

    circle.radius(radius);

    return circle;
}

const std::optional<Point>& Circle::get_center() const
{
    return center;
}

double Circle::get_radius()
{
    return radius;
}