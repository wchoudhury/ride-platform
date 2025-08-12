#pragma once

#include <libxml++-2.6/libxml++/libxml++.h>

#include <optional>
//Optional is used for 3 reasons:
//1. Some values are optional according to the specification
//2. Some values might be missing if the file is not spec-conform, which is easy to handle when we do not require that they exist (though we still check for their existance)
//3. It is easier to set up an object piece by piece in the constructor, but that is not possible if the member object we want to set up does not have a default constructor (we would have to use the initializer list then)

#include <string>

#include "commonroad_classes/datatypes/IntervalOrExact.hpp"
#include "commonroad_classes/InterfaceDraw.hpp"
#include "commonroad_classes/XMLTranslation.hpp"

#include <sstream>
#include "commonroad_classes/SpecificationError.hpp"

#include <cassert> //To make sure that the translation is performed on the right node types, which should haven been made sure by the programming (thus not an error, but an assertion is used)

/**
 * \class SignalState
 * \brief This class, like all other classes in this folder, are heavily inspired by the current (2020) common road XML specification (https://gitlab.lrz.de/tum-cps/commonroad-scenarios/blob/master/documentation/XML_commonRoad_2020a.pdf)
 * It is used to store / represent a SignalState specified in an XML file
 * 
 * WARNING: Currently not used except for translation!
 * \ingroup lcc_commonroad
 */
class SignalState : public InterfaceDraw
{
private:
    //Commonroad data
    //! Optional time of signal, in commonroad time representation (not nanoseconds), defined w.r.t. global time step size
    std::optional<IntervalOrExact> time = std::nullopt;
    //! Optional, tells if the horn is active
    std::optional<bool> horn = std::nullopt;
    //! Optional, tells if the left indicator is active
    std::optional<bool> indicator_left = std::nullopt;
    //! Optional, tells if the right indicator is active
    std::optional<bool> indicator_right = std::nullopt;
    //! Optional, tells if the braking lights are active
    std::optional<bool> braking_lights = std::nullopt;
    //! Optional, tells if the hazard warning lights are active
    std::optional<bool> hazard_warning_lights = std::nullopt;
    //! Optional, tells if the flashing blue lights are active
    std::optional<bool> flashing_blue_lights = std::nullopt;

public:
    /**
     * \brief Constructor that creates a signal state from a commonroad xml signal state node
     */
    SignalState(const xmlpp::Node* node);

    /**
     * \brief Takes a boolean string node and translates its value to a boolean
     * \param node The parent node
     * \param child_name Name of the child node which contains the boolean value
     */
    std::optional<bool> get_child_bool(const xmlpp::Node* node, std::string child_name);

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

    /**
     * \brief Returns a DDS message created from the current scenario that contains all information relevant to the HLC
     * Due to the different return types for each class, no interface was defined for this function.
     * Still, it is required for all classes that are to be communicated via DDS to other members after the translation from XML
     */
    void to_dds_msg(); 

    //Getter
    /**
     * \brief Get the time of the signal or nullopt if undefined
     */
    const std::optional<IntervalOrExact>& get_time() const;
    /**
     * \brief See if the horn is active, nullopt if undefined
     */
    const std::optional<bool>& get_horn() const;
    /**
     * \brief See if the left indicator is active, nullopt if undefined
     */
    const std::optional<bool>& get_indicator_left() const;
    /**
     * \brief See if the right indicator is active, nullopt if undefined
     */
    const std::optional<bool>& get_indicator_right() const;
    /**
     * \brief See if the braking lights are active, nullopt if undefined
     */
    const std::optional<bool>& get_braking_lights() const;
    /**
     * \brief See if the hazard warning lights are active, nullopt if undefined
     */
    const std::optional<bool>& get_hazard_warning_lights() const;
    /**
     * \brief See if the flashing lights are active, nullopt if undefined
     */
    const std::optional<bool>& get_flashing_blue_lights() const;
};
