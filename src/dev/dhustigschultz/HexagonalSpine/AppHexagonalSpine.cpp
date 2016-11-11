
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
 * @file AppHexagonalSpineTest.cpp
 * @brief Contains the definition function main() for the HexagonalSpineTest
 * application.
 * @author Lauren Sharo
 * @copyright Copyright (C) 2014 NASA Ames Research Center
 * $Id$
 */

// This application
#include "HexagonalSpineTestModel.h"
// This library
#include "core/tgModel.h"
#include "core/tgSimViewGraphics.h"
#include "core/tgSimulation.h"
#include "core/tgWorld.h"
// The C++ Standard Library
#include <iostream>

#include "models/obstacles/tgCraterShallow.cpp"
#include "models/obstacles/tgBlockField.cpp"
#include "models/obstacles/tgStairs.cpp"

int main(int argc, char** argv)
{
	std::cout << "AppHexagonalSpineTest" << std::endl;

	// First create the world
	const tgWorld::Config config(98.1); // gravity, dm/sec^2
	tgWorld world(config);

	// Second create the view
	const double stepSize = 1.0 / 2000.0; //Seconds
	tgSimViewGraphics view(world, stepSize);

	// Third create the simulation
	tgSimulation *simulation = new tgSimulation(view);

	// Fourth create the models and add to the simulation
	const int segmentsLeft = 2;
	const int segmentsRight = 2;
	HexagonalSpineTestModel* const myModel = new HexagonalSpineTestModel(
			segmentsLeft, segmentsRight);
	simulation->addModel(myModel);

	// Retrieves and prints the mass of the structure to the command line
	myModel->getMass();

	// Run until the user stops
	simulation->run();

	//Teardown is handled by delete, so that should be automatic
	return 0;
}
