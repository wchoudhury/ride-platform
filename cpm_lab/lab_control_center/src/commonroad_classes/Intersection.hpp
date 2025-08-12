#pragma once

#include <libxml++-2.6/libxml++/libxml++.h>

#include <optional>
//Optional is used for 3 reasons:
//1. Some values are optional according to the specification
//2. Some values might be missing if the file is not spec-conform, which is easy to handle when we do not require that they exist (though we still check for their existance)
//3. It is easier to set up an object piece by piece in the constructor, but that is not possible if the member object we want to set up does not have a default constructor (we would have to use the initializer list then)

#include <map>
#include <iostream>
#include <sstream>

#include <cassert> //To make sure that the translation is performed on the right node types, which should haven been made sure by the programming (thus not an error, but an assertion is used)

#include "commonroad_classes/InterfaceDraw.hpp"
#include "commonroad_classes/XMLTranslation.hpp"
#include "commonroad_classes/SpecificationError.hpp"

#include "LCCErrorLogger.hpp"

/**
 * \struct Incoming
 * \brief Specifies a part of the intersection, as in commonroad
 * \ingroup lcc_commonroad
 */
struct Incoming
{
    //! Lanelet ref for an incoming lanelet - at least one value must exist
    std::vector<int> incoming_lanelet;
    //! Lanelet ref for right successors
    std::vector<int> successors_right;
    //! Lanelet ref for straight successors
    std::vector<int> successors_straight;
    //! Lanelet ref for left successors
    std::vector<int> successors_left;
    //! Incoming ref
    std::vector<int> is_left_of;
};

/**
 * \struct Crossing
 * \brief Specifies a part of the intersection, as in commonroad
 * \ingroup lcc_commonroad
 */
struct Crossing
{
    //! Lanelet references for a crossing, at least one should exist
    std::vector<int> crossing_lanelets;
};

/**
 * \class Intersection
 * \brief This class, like all other classes in this folder, are heavily inspired by the current (2020) common road XML specification (https://gitlab.lrz.de/tum-cps/commonroad-scenarios/blob/master/documentation/XML_commonRoad_2020a.pdf)
 * It is used to store / represent an intersection specified in an XML file
 * \ingroup lcc_commonroad
 */
class Intersection : public InterfaceDraw
{
private:
    //! Map of incoming definitions, mapped by their ID, at least one entry should exist
    std::map<int, Incoming> incoming_map;

    //! Map of crossing definitions, mapped by their ID
    std::map<int, Crossing> crossing_map;

public:
     /**
     * \brief Constructor - we do not want the user to be able to set values after the class has been created
     */
    Intersection(const xmlpp::Node* node);

    /**
     * \brief Get attribute value(s) of child of node with name child_name
     * \param node The parent node
     * \param child_name Name of the child
     * \param warn Warn if the child or attribute does not exist
     */
    std::vector<int> get_child_attribute_ref(const xmlpp::Node* node, std::string child_name, bool warn);

    //Suppress warning for unused parameter (s)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

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
    void draw(const DrawingContext& ctx, double scale = 1.0, double global_orientation = 0.0, double global_translate_x = 0.0, double global_translate_y = 0.0, double local_orientation = 0.0) override {} //TODO

    #pragma GCC diagnostic pop

    //Getter
    /**
     * \brief Get all incoming definitions for this object
     */
    const std::map<int, Incoming>& get_incoming_map() const;
};