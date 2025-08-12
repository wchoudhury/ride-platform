#pragma once

#include <libxml++-2.6/libxml++/libxml++.h>

#include "commonroad_classes/InterfaceDraw.hpp"
#include "commonroad_classes/InterfaceTransform.hpp"
#include "commonroad_classes/XMLTranslation.hpp"

#include "CommonroadDDSShape.hpp"

#include <cassert> //To make sure that the translation is performed on the right node types, which should haven been made sure by the programming (thus not an error, but an assertion is used)

/**
 * \class Point
 * \brief Auxiliary class from the XML specification: https://gitlab.lrz.de/tum-cps/commonroad-scenarios/-/blob/master/documentation/XML_commonRoad_XSD_2020a.xsd
 * \ingroup lcc_commonroad
 */
class Point : public InterfaceTransform, public InterfaceDraw
{
private:
    //! x coordinate of the point
    double x;
    //! y coordinate of the point
    double y;
    //! optional z coordinate of the point
    std::optional<double> z = std::nullopt;
public:
    /**
     * \brief Constructor, set up a point object from a commonroad xml point node
     */
    Point(const xmlpp::Node* node);

    /**
     * \brief Second constructor, value is irrelevant - value given s.t. !!no default constructor exists!!
     * Call this if you need to use the specified default value from the specs because the value was not set explicitly
     */
    Point(int irrelevant_int);

    /**
     * \brief Third constructor, used whenever a point needs to be calculated somewhere (e.g. the centroid of a polygon) and then needs to be stored as a point
     */
    Point(double _x, double _y, double _z);

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
     * \brief This function is used to draw the data structure that imports this interface
     * If you want to set a color for drawing, perform this action on the context before using the draw function
     * To change local translation, just transform the coordinate system beforehand
     * As this does not always work with local orientation (where sometimes the translation in the object must be called before the rotation if performed, to rotate within the object's coordinate system),
     * local_orientation was added as a parameter
     * \param ctx A DrawingContext, used to draw on
     * \param scale - optional: The factor by which to transform all number values related to position - this is not permanent, only for drawing (else, use InterfaceTransform's functions)
     * \param global_orientation - optional: Rotation that needs to be applied before drawing - set as global transformation to the whole coordinate system
     * \param global_translate_x - optional: Translation in x-direction that needs to be applied before drawing - set as global transformation to the whole coordinate system
     * \param global_translate_y - optional: Translation in y-direction that needs to be applied before drawing - set as global transformation to the whole coordinate system
     * \param local_orientation - optional: Rotation that needs to be applied within the object's coordinate system
     */
    void draw(const DrawingContext& ctx, double scale = 1.0, double global_orientation = 0.0, double global_translate_x = 0.0, double global_translate_y = 0.0, double local_orientation = 0.0) override;

    /**
     * \brief Translates all relevant parts of the data structure to a DDS object, which is returned
     * No interface was created for this function because the return type depends on the class
     */
    CommonroadDDSPoint to_dds_msg();

    //Getter
    /**
     * \brief Get the x value of the point
     */
    double get_x();
    /**
     * \brief Get the y value of the point
     */
    double get_y();
    /**
     * \brief Get the z value of the point or nullopt, as this value is optional
     */
    const std::optional<double>& get_z() const;
};