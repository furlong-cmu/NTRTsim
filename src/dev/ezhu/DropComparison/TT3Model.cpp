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
 * @file TT3Model.cpp
 * @brief Contains the implementation of class TT3Model.
 * $Id$
 */

// This module
#include "TT3Model.h"
#include "TT3TensionController.h"
// This library
#include "core/tgBasicActuator.h"
#include "controllers/tgTensionController.h"
#include "core/tgRod.h"
#include "core/tgString.h"
#include "tgcreator/tgBuildSpec.h"
#include "tgcreator/tgBasicActuatorInfo.h"
#include "tgcreator/tgKinematicContactCableInfo.h"
#include "tgcreator/tgRodInfo.h"
#include "tgcreator/tgStructure.h"
#include "tgcreator/tgStructureInfo.h"
//#include "core/abstractMarker.h"
// The Bullet Physics library
#include "LinearMath/btScalar.h"
#include "LinearMath/btVector3.h"
// The C++ Standard Library
#include <stdexcept>

namespace
{
    // see tgBasicActuator and tgRod for a descripton of these rod parameters
    // (specifically, those related to the motor moving the strings.)
    // NOTE that any parameter that depends on units of length will scale
    // with the current gravity scaling. E.g., with gravity as 98.1,
    // the length units below are in decimeters.

    // Note: This current model of the SUPERball rod is 1.5m long by 3 cm radius,
    // which is 0.00424 m^3.
    // For SUPERball v1.5, mass = 3.5kg per strut, which comes out to 
    // 0.825 kg / (decimeter^3).

    // similarly, frictional parameters are for the tgRod objects.
    
    double sf = 10;

    const struct Config
    {
        double density;
        double density_capsule;
        double radius;
	    double radius_capsule;
        double stiffness;
        double damping;
        double rod_length;
        double rod_space;
        double capsule_length;  
        double friction;
        double rollFriction;
        double restitution;
        double pretension;
        bool   hist;
        double maxTens;
        double targetVelocity;
    } config =
        {
         688/pow(sf,3),     // density of rod (kg / length^3)
         751/pow(sf,3),     // density of capsule (kg/ length^3)
         0.0127/2*sf,       // radius (length) ** rod diameter / 2 **
         0.056/2*sf,        // radius (length) ** capsule diameter / 2 **
         200.0,             // stiffness (kg / sec^2) was 1500
         20.0,              // damping (kg / sec)
         0.66*sf,           // rod_length (length)
         0.325*sf,          // rod_space (length)
         0.19*sf,           // capsule_length (length)
         0.99,              // friction (unitless)
         0.01,              // rollFriction (unitless)
         0.0,               // restitution (?)
         17.5*sf,           // pretension -> scaled to 85.0 which is 10 times the actual scaling because gravity is 10 times higher
         0,			        // History logging (boolean)
         10000*sf,          // maxTens
         0.25*sf,           // targetVelocity   
        };
} // namespace

TT3Model::TT3Model() : tgModel() 
{
}

TT3Model::~TT3Model()
{
}

void TT3Model::addTT3(tgStructure& s)
{
    addNodes(s);
    addRods(s);
    addActuators(s);
}

void TT3Model::addNodes(tgStructure& s)
{
    // const double third_length = (config.rod_length-config.capsule_length)/2; //226.155 mm
    double rodSpace = (-config.rod_length + sqrt(pow(config.rod_length,2)+4*pow(config.rod_length,2)))/2;

    // const size_t nNodes = 26;

    // Nodes in the x-z plane
    s.addNode(-rodSpace/2, 0, config.rod_length/2); // 0
    s.addNode(-rodSpace/2, 0, -config.rod_length/2); // 1
    s.addNode(rodSpace/2, 0, -config.rod_length/2); // 2
    s.addNode(rodSpace/2, 0, config.rod_length/2); // 3

    // Nodes in the y-z plane
    s.addNode(0, -config.rod_length/2, rodSpace/2); // 4
    s.addNode(0, config.rod_length/2, rodSpace/2); // 5
    s.addNode(0, config.rod_length/2, -rodSpace/2); // 6
    s.addNode(0, -config.rod_length/2, -rodSpace/2); // 7

    // Nodes in the x-y plane
    s.addNode(-config.rod_length/2, -rodSpace/2, 0); // 8
    s.addNode(-config.rod_length/2, rodSpace/2, 0); // 9
    s.addNode(config.rod_length/2, rodSpace/2, 0); // 10
    s.addNode(config.rod_length/2, -rodSpace/2, 0); // 11


    s.addNode(-rodSpace/2, 0, config.capsule_length/2); // 12
    s.addNode(-rodSpace/2, 0, -config.capsule_length/2); // 13
    s.addNode(rodSpace/2, 0, -config.capsule_length/2); //14
    s.addNode(rodSpace/2, 0, config.capsule_length/2); //15

    s.addNode(0, -config.capsule_length/2, rodSpace/2); // 16
    s.addNode(0, config.capsule_length/2, rodSpace/2); // 17
    s.addNode(0, config.capsule_length/2, -rodSpace/2); // 18
    s.addNode(0, -config.capsule_length/2, -rodSpace/2); // 19

    s.addNode(-config.capsule_length/2, -rodSpace/2, 0); // 20
    s.addNode(-config.capsule_length/2, rodSpace/2, 0); // 21
    s.addNode(config.capsule_length/2, rodSpace/2, 0); // 22
    s.addNode(config.capsule_length/2, -rodSpace/2, 0); // 23
}

void TT3Model::addRods(tgStructure& s)
{
    s.addPair(0, 12,  tgString("rod num", 0));
    s.addPair(12, 13, tgString("capsule num", 0));
    s.addPair(13, 1,  tgString("rod num", 1));

    s.addPair(3, 15,  tgString("rod num", 2));
    s.addPair(15, 14, tgString("capsule num", 1));
    s.addPair(14, 2,  tgString("rod num", 3));

    s.addPair(4, 16,  tgString("rod num", 4));
    s.addPair(16, 17, tgString("capsule num", 2));
    s.addPair(17, 5,  tgString("rod num", 5));

    s.addPair(7, 19,  tgString("rod num", 6));
    s.addPair(19, 18, tgString("capsule num", 3));
    s.addPair(18, 6,  tgString("rod num", 7));

    s.addPair(8, 20,  tgString("rod num", 8));
    s.addPair(20, 23, tgString("capsule num", 4));
    s.addPair(23, 11, tgString("rod num", 9));

    s.addPair(9, 21,  tgString("rod num", 10));
    s.addPair(21, 22, tgString("capsule num", 5));
    s.addPair(22, 10, tgString("rod num", 11));
}

void TT3Model::addActuators(tgStructure& s)
{

    s.addPair(0, 4,  tgString("actuator num", 0)); // 0
    s.addPair(0, 5,  tgString("actuator num", 1)); // 1
    s.addPair(0, 8,  tgString("actuator num", 2)); // 2
    s.addPair(0, 9,  tgString("actuator num", 3)); // 3

    s.addPair(1, 6,  tgString("actuator num", 4)); // 4
    s.addPair(1, 7,  tgString("actuator num", 5)); // 5
    s.addPair(1, 8,  tgString("actuator num", 6)); // 6
    s.addPair(1, 9,  tgString("actuator num", 7)); // 7

    s.addPair(2, 6,  tgString("actuator num", 8)); // 8
    s.addPair(2, 7,  tgString("actuator num", 9)); // 9
    s.addPair(2, 10, tgString("actuator num", 10)); // 10
    s.addPair(2, 11, tgString("actuator num", 11)); // 11

    s.addPair(3, 4,  tgString("actuator num", 12)); // 12
    s.addPair(3, 5,  tgString("actuator num", 13)); // 13
    s.addPair(3, 10, tgString("actuator num", 14)); // 14
    s.addPair(3, 11, tgString("actuator num", 15)); // 15

    s.addPair(4, 8,  tgString("actuator num", 16)); // 16
    s.addPair(4, 11, tgString("actuator num", 17)); // 17

    s.addPair(5, 9,  tgString("actuator num", 18)); // 18
    s.addPair(5, 10, tgString("actuator num", 19)); // 19

    s.addPair(6, 9,  tgString("actuator num", 20)); // 20
    s.addPair(6, 10, tgString("actuator num", 21)); // 21

    s.addPair(7, 8,  tgString("actuator num", 22)); // 22
    s.addPair(7, 11, tgString("actuator num", 23)); // 23
}

void TT3Model::setup(tgWorld& world)
{
    // Calculate the space between two parallel rods based on the rod length from Config
    rodDist = (-config.rod_length + sqrt(pow(config.rod_length,2)+4*pow(config.rod_length,2)))/2;

    // Nodes in the x-z plane
    node0 = btVector3(-rodDist/2, 0, config.rod_length/2); // 0
    node1 = btVector3(-rodDist/2, 0, -config.rod_length/2); // 1
    node2 = btVector3(rodDist/2, 0, -config.rod_length/2); // 2
    node3 = btVector3(rodDist/2, 0, config.rod_length/2); // 3

    // Nodes in the y-z plane
    node4 = btVector3(0, -config.rod_length/2, rodDist/2); // 4
    node5 = btVector3(0, config.rod_length/2, rodDist/2); // 5
    node6 = btVector3(0, config.rod_length/2, -rodDist/2); // 6
    node7 = btVector3(0, -config.rod_length/2, -rodDist/2); // 7

    // Nodes in the x-y plane
    node8 = btVector3(-config.rod_length/2, -rodDist/2, 0); // 8
    node9 = btVector3(-config.rod_length/2, rodDist/2, 0); // 9
    node10 = btVector3(config.rod_length/2, rodDist/2, 0); // 10
    node11 = btVector3(config.rod_length/2, -rodDist/2, 0); // 11

    // Find all edge vectors of closed triangles    // Actuator #
    face0Edge0 = node8 - node4;     // 16
    face0Edge1 = node0 - node8;     // 2
    face0Edge2 = node4 - node0;     // 0

    face2Edge0 = node9 - node0;     // 3
    face2Edge1 = node5 - node9;     // 18
    face2Edge2 = node0 - node5;     // 1

    face5Edge0 = node3 - node4;     // 12
    face5Edge1 = node11 - node3;    // 15
    face5Edge2 = node4 - node11;    // 17

    face7Edge0 = node5 - node3;     // 13
    face7Edge1 = node10 - node5;    // 19
    face7Edge2 = node3 - node10;    // 14

    face8Edge0 = node11 - node7;    // 23
    face8Edge1 = node2 - node11;    // 11
    face8Edge2 = node7 - node2;     // 9

    face10Edge0 = node10 - node2;   // 10
    face10Edge1 = node6 - node10;   // 21
    face10Edge2 = node2 - node6;    // 8

    face13Edge0 = node1 - node7;    // 5
    face13Edge1 = node8 - node1;    // 6
    face13Edge2 = node7 - node8;    // 22

    face15Edge0 = node6 - node1;    // 4
    face15Edge1 = node9 - node6;    // 20
    face15Edge2 = node1 - node9;    // 7

    // Find normal vectors to all faces
    face0Norm = (face0Edge0.cross(face0Edge2)).normalize();
    face1Norm = (face0Edge1.cross(face2Edge0)).normalize();
    face2Norm = (face2Edge0.cross(face2Edge2)).normalize();
    face3Norm = (face7Edge0.cross(face2Edge2)).normalize();
    face4Norm = (face0Edge2.cross(face5Edge0)).normalize();
    face5Norm = (face5Edge0.cross(face5Edge2)).normalize();
    face6Norm = (face7Edge2.cross(face5Edge1)).normalize();
    face7Norm = (face7Edge0.cross(face7Edge2)).normalize();

    face8Norm = (face8Edge0.cross(face8Edge2)).normalize();
    face9Norm = (face8Edge1.cross(face10Edge0)).normalize();
    face10Norm = (face10Edge0.cross(face10Edge2)).normalize();
    face11Norm = (face15Edge0.cross(face10Edge2)).normalize();
    face12Norm = (face8Edge2.cross(face13Edge0)).normalize();
    face13Norm = (face13Edge0.cross(face13Edge2)).normalize();
    face14Norm = (face15Edge2.cross(face13Edge1)).normalize();
    face15Norm = (face15Edge0.cross(face15Edge2)).normalize();

    face16Norm = (face0Edge0.cross(face13Edge2)).normalize();
    face17Norm = (face15Edge1.cross(face2Edge1)).normalize();
    face18Norm = (face7Edge1.cross(face10Edge1)).normalize();
    face19Norm = (face8Edge0.cross(face5Edge2)).normalize();

    // Place all normal vectors into vector
    normalVectors.push_back(face0Norm);
    normalVectors.push_back(face1Norm);
    normalVectors.push_back(face2Norm);
    normalVectors.push_back(face3Norm);
    normalVectors.push_back(face4Norm);
    normalVectors.push_back(face5Norm);
    normalVectors.push_back(face6Norm);
    normalVectors.push_back(face7Norm);
    normalVectors.push_back(face8Norm);
    normalVectors.push_back(face9Norm);
    normalVectors.push_back(face10Norm);
    normalVectors.push_back(face11Norm);
    normalVectors.push_back(face12Norm);
    normalVectors.push_back(face13Norm);
    normalVectors.push_back(face14Norm);
    normalVectors.push_back(face15Norm);
    normalVectors.push_back(face16Norm);
    normalVectors.push_back(face17Norm);
    normalVectors.push_back(face18Norm);
    normalVectors.push_back(face19Norm);

    const tgRod::Config rodConfig(config.radius, config.density, config.friction, 
				config.rollFriction, config.restitution);
    const tgRod::Config capsuleConfig(config.radius_capsule, config.density_capsule, config.friction, 
				config.rollFriction, config.restitution);
  
    tgBasicActuator::Config actuatorConfig(config.stiffness, config.damping, config.pretension, config.hist, 
					    config.maxTens, config.targetVelocity);
 
    // Start creating the structure
    tgStructure s;
    
    addTT3(s);

    rotateToFace(s, 2);

    btVector3 offset (0, 10, 0);
    s.move(offset);

    // Create the build spec that uses tags to turn the structure into a real model
    tgBuildSpec spec;
    spec.addBuilder("rod", new tgRodInfo(rodConfig));
    spec.addBuilder("actuator", new tgBasicActuatorInfo(actuatorConfig));
    spec.addBuilder("capsule", new tgRodInfo(capsuleConfig));

    // Create your structureInfo
    tgStructureInfo structureInfo(s, spec);

    // Use the structureInfo to build ourselves
    structureInfo.buildInto(*this, world);

    // Get the rod rigid bodies for controller
    std::vector<tgRod*> rods = TT3Model::find<tgRod>("rod");
    for (int i = 0; i < rods.size(); i++) {
        allRods.push_back(TT3Model::find<tgRod>(tgString("rod num", i))[0]);
    }

    // Get the capsule rigid bodies for controller
    std::vector<tgRod*> capsules = TT3Model::find<tgRod>("capsule");
    for (int i = 0; i < capsules.size(); i++) {
        allCapsules.push_back(TT3Model::find<tgRod>(tgString("capsule num", i))[0]);
    }

    // Get the actuators for controller
    std::vector<tgBasicActuator*> actuators = TT3Model::find<tgBasicActuator>("actuator");
    for (int i = 0; i < actuators.size(); i++) {
        allActuators.push_back(TT3Model::find<tgBasicActuator>(tgString("actuator num", i))[0]);
    }

    // call the onSetup methods of all observed things e.g. controllers
    notifySetup();

    // Actually setup the children
    tgModel::setup(world);
}

void TT3Model::step(double dt)
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

void TT3Model::onVisit(tgModelVisitor& r)
{
    tgModel::onVisit(r);
}

const std::vector<tgBasicActuator*>& TT3Model::getAllActuators() const
{
    return allActuators;
}
   
const std::vector<tgRod*>& TT3Model::getAllRods() const 
{
    return allRods;
}

const std::vector<tgRod*>& TT3Model::getAllCapsules() const 
{
    return allCapsules;
}

void TT3Model::teardown()
{
    notifyTeardown();
    tgModel::teardown();
}

void TT3Model::rotateToFace(tgStructure& s, int face)
{
    btVector3 faceNorm = normalVectors[face];
    btVector3 goalDir = btVector3(0, -1, 0);
    double dotProd = faceNorm.dot(goalDir);
    double theta = acos(dotProd);
    btVector3 crossProd = faceNorm.cross(goalDir);

    s.addRotation(btVector3(0,0,0), crossProd, theta);
}