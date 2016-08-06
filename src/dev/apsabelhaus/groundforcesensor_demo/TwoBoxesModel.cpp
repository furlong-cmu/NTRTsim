/*
 * Copyright © 2012, United States Government, as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All rights reserved.
 * 
 * The NASA Tensegrity Robotics Toolkit (NTRT) v1 platform is licensed
 * under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
*/

/**
 * @file TwoBoxesModel.cpp
 * @brief Contains the implementation of class TwoBoxesModel.
 * @author Drew Sabelhaus
 * @copyright Copyright (C) 2016 NASA Ames Research Center
 * $Id$
 */

// This module
#include "TwoBoxesModel.h"
// This library
#include "core/tgCompressionSpringActuator.h"
#include "core/tgUnidirectionalCompressionSpringActuator.h"
#include "core/tgRod.h"
#include "core/tgBox.h"
#include "tgcreator/tgBuildSpec.h"
#include "tgcreator/tgCompressionSpringActuatorInfo.h"
#include "tgcreator/tgUnidirectionalCompressionSpringActuatorInfo.h"
#include "tgcreator/tgRodInfo.h"
#include "tgcreator/tgBoxInfo.h"
#include "tgcreator/tgStructure.h"
#include "tgcreator/tgStructureInfo.h"
// The Bullet Physics library
#include "LinearMath/btVector3.h"
// The C++ Standard Library
#include <stdexcept>

namespace
{
    // using tgCompressionSpringActuator here.
    // frictional parameters are for the tgBox objects.
    const struct Config
    {
        double density;
        double radius;
        bool isFreeEndAttached;
        double stiffness;
        double damping;
        btVector3 direction;
        double boxLength;
        double boxWidth;
        double boxHeight;
        double friction;
        double rollFriction;
        double restitution;
        double springRestLength;
    } c =
   {
     0.1,    // density (kg / length^3)
     0.31,     // radius (length)
     false,   // isFreeEndAttached
     200.0,   // stiffness (kg / sec^2) was 1500
     20.0,    // damping (kg / sec)
     *(new btVector3(0, 1, 0) ),  // direction
     3.0,   // boxLength (length)
     3.0,   // boxWidth (length)
     3.0,   // boxHeight (length)
     1.0,      // friction (unitless)
     1.0,     // rollFriction (unitless)
     0.2,      // restitution (?)
     2.0,   // springRestLength (length)
  };
} // namespace

// Constructor: does nothing. All the good stuff happens when the 'Info' classes
// are passed to the tgBuilders and whatnot.
TwoBoxesModel::TwoBoxesModel() : tgModel() 
{
}

// Destructor: does nothing
TwoBoxesModel::~TwoBoxesModel()
{
}

// a helper function to add a bunch of nodes
void TwoBoxesModel::addNodes(tgStructure& s)
{
  s.addNode(0, 0, 0);              // 0, origin
  s.addNode(0, c.boxLength, 0);      // 1, top of box 1
  s.addNode(0, 3 * c.boxLength, 0);  // 2, bottom of box 2
  s.addNode(0, 4 * c.boxLength, 0);  // 3, top of box 3
}

// helper function to tag two sets of nodes as boxes
void TwoBoxesModel::addBoxes(tgStructure& s)
{
    s.addPair( 0,  1, "box");
    s.addPair( 2,  3, "box");
}

// helper function to add our single compression spring actuator
void TwoBoxesModel::addActuators(tgStructure& s)
{
  // spring is vertical between top of box 1 and bottom of box 2.
  s.addPair(1, 2,  "compressionSpring");
}

// Finally, create the model!
void TwoBoxesModel::setup(tgWorld& world)
{
    // config struct for the rods
    const tgBox::Config boxConfig(c.boxWidth, c.boxHeight, c.density, 
				  c.friction, c.rollFriction, c.restitution);

    // config struct for the compression spring
    //tgCompressionSpringActuator::Config compressionSpringConfig(c.isFreeEndAttached,
    //				       c.stiffness, c.damping, c.springRestLength);
    tgUnidirectionalCompressionSpringActuator::Config compressionSpringConfig(
				       c.isFreeEndAttached,
				       c.stiffness, c.damping, c.springRestLength,
				       c.direction);

    
    // Start creating the structure
    tgStructure s;
    addNodes(s);
    addBoxes(s);
    addActuators(s);
    //s.move(btVector3(0, 10, 0));

    // Add a rotation. This is needed if the ground slopes too much,
    // otherwise  glitches put a rod below the ground.
    /*btVector3 rotationPoint = btVector3(0, 0, 0); // origin
    btVector3 rotationAxis = btVector3(0, 1, 0);  // y-axis
    double rotationAngle = M_PI/2;
    s.addRotation(rotationPoint, rotationAxis, rotationAngle);
    */

    // Create the build spec that uses tags to turn the structure into a real model
    tgBuildSpec spec;
    spec.addBuilder("box", new tgBoxInfo(boxConfig));
    //spec.addBuilder("compressionSpring", new tgCompressionSpringActuatorInfo(compressionSpringConfig));
    spec.addBuilder("compressionSpring", new tgUnidirectionalCompressionSpringActuatorInfo(compressionSpringConfig));

    
    // Create your structureInfo
    tgStructureInfo structureInfo(s, spec);

    // Use the structureInfo to build ourselves
    structureInfo.buildInto(*this, world);

    // We could now use tgCast::filter or similar to pull out the
    // models (e.g. muscles) that we want to control. 
    allActuators = tgCast::filter<tgModel, tgCompressionSpringActuator> (getDescendants());

    // call the onSetup methods of all observed things e.g. controllers
    notifySetup();

    // Actually setup the children
    tgModel::setup(world);
}

void TwoBoxesModel::step(double dt)
{
    // Precondition
    if (dt <= 0.0)
    {
        throw std::invalid_argument("dt is not positive");
    }
    else
    {
        // Notify observers (controllers) of the step so that they can take action
        notifyStep(dt);
        tgModel::step(dt);  // Step any children
    }
}

void TwoBoxesModel::onVisit(tgModelVisitor& r)
{
    tgModel::onVisit(r);
}

const std::vector<tgCompressionSpringActuator*>& TwoBoxesModel::getAllActuators() const
{
    return allActuators;
}
    
void TwoBoxesModel::teardown()
{
    notifyTeardown();
    tgModel::teardown();
}