#include "commonroad_classes/geometry/Polygon.hpp"

/**
 * \file Polygon.cpp
 * \ingroup lcc_commonroad
 */

Polygon::Polygon(const xmlpp::Node* node)
{
    //Check if node is of type polygon
    assert(node->get_name() == "polygon");

    commonroad_line = node->get_line();

    try
    {
        xml_translation::iterate_children(
            node,
            [&] (const xmlpp::Node* child)
            {
                points.push_back(Point(child));
            },
            "point"
        );
    }
    catch(const SpecificationError& e)
    {
        throw SpecificationError(std::string("Could not translate Polygon:\n") + e.what());
    }
    catch(...)
    {
        //Propagate error, if any subclass of CommonRoadScenario fails, then the whole translation should fail
        throw;
    }

    if (points.size() < 3)
    {
        std::stringstream error_msg_stream;
        error_msg_stream << "Points missing in translated polygon (at least 3 required), line " << node->get_line();
        throw SpecificationError(error_msg_stream.str());
    }

    //Test output
    // std::cout << "Polygon:" << std::endl;
    // std::cout << "\tPoints: " << points.size() << std::endl;
}

void Polygon::transform_coordinate_system(double scale, double angle, double translate_x, double translate_y)
{
    for (auto& point : points)
    {
        point.transform_coordinate_system(scale, angle, translate_x, translate_y);
    }
}

void Polygon::draw(const DrawingContext& ctx, double scale, double global_orientation, double global_translate_x, double global_translate_y, double local_orientation)
{
    if (points.size() < 3)
    {
        std::stringstream error_stream;
        error_stream << "Points missing in translated polygon (at least 3 required) - will not be drawn, from line " << commonroad_line;
        LCCErrorLogger::Instance().log_error(error_stream.str());
    }
    else
    {
        ctx->save();

        //Perform required translation + rotation
        ctx->translate(global_translate_x, global_translate_y);
        ctx->rotate(global_orientation);

        //To allow for rotation in the local coordinate system, move to the center, then rotate, then use relative positions (to the center)
        auto center = get_center();
        ctx->translate(center.first * scale, center.second * scale);
        ctx->rotate(local_orientation);
        
        ctx->set_line_width(0.005);

        //Move to first point
        ctx->move_to((points.at(0).get_x() - center.first) * scale, (points.at(0).get_y() - center.second) * scale);

        //Draw lines to remaining points
        for (auto point : points)
        {
            ctx->line_to((point.get_x() - center.first) * scale, (point.get_y() - center.second) * scale);
        }
        ctx->line_to((points.at(0).get_x() - center.first) * scale, (points.at(0).get_y() - center.second) * scale); //Finish polygon by drawing a line to the starting point
        ctx->fill_preserve();
        ctx->stroke();

        ctx->restore();
    }
}

std::pair<double, double> Polygon::get_center()
{
    //Take the mean of min and max value for x and y coordinates, might not work for all shapes
    assert(std::numeric_limits<double>::has_infinity);
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = - std::numeric_limits<double>::infinity();
    double max_y = - std::numeric_limits<double>::infinity();

    for (auto point : points)
    {
        min_x = std::min(min_x, point.get_x());
        min_y = std::min(min_y, point.get_y());
        max_x = std::max(max_x, point.get_x());
        max_y = std::max(max_y, point.get_y());
    }

    return std::pair<double, double>(0.5 * min_x + 0.5 * max_x, 0.5 * min_y + 0.5 * max_y);
}

const std::vector<Point>& Polygon::get_points() const
{
    return points;
}

CommonroadDDSPolygon Polygon::to_dds_msg()
{
    CommonroadDDSPolygon polygon;

    std::vector<CommonroadDDSPoint> dds_points;
    for (auto point : points)
    {
        dds_points.push_back(point.to_dds_msg());
    }

    polygon.points(rti::core::vector<CommonroadDDSPoint>(dds_points));

    return polygon;
}