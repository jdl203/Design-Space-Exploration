#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>
#include <vector>
#include <iterator>

#include "431project.h"

#include <iostream>

using namespace std;

/*
 * Enter your PSU IDs here to select the appropriate scanning order.
 * Tamara's ID: 954153742
 * James's ID: 941006691
 */
#define PSU_ID_SUM (954153742+941006691)
// LETS GOOOOOOOO :)

/*
 * Some global variables to track heuristic progress.
 * 
 * Feel free to create more global variables to track progress of your
 * heuristic.
 */
unsigned int currentlyExploringDim = 12;
bool currentDimDone = false;
bool isDSEComplete = false;

bool firstConfig = true;
bool dimsComplete[NUM_DIMS] = {false, false, false, false, false, false, false, false, false, 
								false, false, false, false, false, false, false ,false, false};

/*
 * Given a half-baked configuration containing cache properties, generate
 * latency parameters in configuration string. You will need information about
 * how different cache paramters affect access latency.
 * 
 * Returns a string similar to "1 1 1"
 */
std::string generateCacheLatencyParams(string halfBackedConfig) {

	string latencySettings;

	//
	//YOUR CODE BEGINS HERE
	//

	// This is a dumb implementation.
	latencySettings = "1 1 1";

	//
	//YOUR CODE ENDS HERE
	//

	return latencySettings;
}

/*
 * Returns 1 if configuration is valid, else 0
 */
int validateConfiguration(std::string configuration) {

	int il1 = extractConfigPararm(configuration, 2);
	int ifq = extractConfigPararm(configuration, 0);
	int ul2 = extractConfigPararm(configuration, 8);

	// Checks if il1 less than ifq (il1 in B, ifq * 8 = B)
	if (il1 < ifq) {
		cout << "NOT VALID\n";
		return 0;
	}

	// Checks if ul2 less than twice il1
	if (ul2 < il1) {
		cout << "NOT VALID ul2\n";
		return 0;
	}
	
	// Checks il1 and dl1 size Min: 2 KB, Max: 64 KB
	if (getil1size(configuration) < 2000 || getil1size(configuration) > 64000) {
		cout << "NOT VALID IL1 SIZE\n";
		return 0;
	}
	if (getdl1size(configuration) < 2000 || getdl1size(configuration) > 64000) {
		cout << "NOT VALID DL1 SIZE\n";
		return 0;
	}

	// Checks ul2 size Min: 32 KB, Max: 1 MB
	if (getl2size(configuration) < 32000 || getl2size(configuration) > 1000000) {
		cout << "NOT VALID L2 SIZE\n";
		return 0;
	}


	// The below is a necessary, but insufficient condition for validating a
	// configuration.
	return isNumDimConfiguration(configuration);
}

/*
 * Given the current best known configuration, the current configuration,
 * and the globally visible map of all previously investigated configurations,
 * suggest a previously unexplored design point. You will only be allowed to
 * investigate 1000 design points in a particular run, so choose wisely.
 *
 * In the current implementation, we start from the leftmost dimension and
 * explore all possible options for this dimension and then go to the next
 * dimension until the rightmost dimension.
 */
std::string generateNextConfigurationProposal(std::string currentconfiguration,
		std::string bestEXECconfiguration, std::string bestEDPconfiguration,
		int optimizeforEXEC, int optimizeforEDP) {

	//
	// Some interesting variables in 431project.h include:
	//
	// 1. GLOB_dimensioncardinality
	// 2. GLOB_baseline
	// 3. NUM_DIMS
	// 4. NUM_DIMS_DEPENDENT
	// 5. GLOB_seen_configurations

	std::string nextconfiguration = currentconfiguration;
	// Continue if proposed configuration is invalid or has been seen/checked before.

	// Variable to store paramter of dimension from baseline and if its the first time 
	// searching through this dimension

	while (!validateConfiguration(nextconfiguration) ||
		GLOB_seen_configurations[nextconfiguration]) {

		// Check if DSE has been completed before and return current
		// configuration.
		if(isDSEComplete) {
			return currentconfiguration;
		}

		std::stringstream ss;

		string bestConfig;
		if (optimizeforEXEC == 1)
			bestConfig = bestEXECconfiguration;

		if (optimizeforEDP == 1)
			bestConfig = bestEDPconfiguration;


		// Handling for currently exploring dimension. This is a very dumb
		// implementation.
		int nextValue = extractConfigPararm(nextconfiguration, currentlyExploringDim) + 1;

		// Checks if this is first time searching in current dimension
		if (firstConfig) {
			// Sets start value as 0 and marks no longer first param at current dimen
			nextValue = 0;
			firstConfig = false;
		}

		// Checks if nextValue is larger than the cardinality
		if (nextValue >= GLOB_dimensioncardinality[currentlyExploringDim]) {
			// Decrements if it is
			nextValue = GLOB_dimensioncardinality[currentlyExploringDim] - 1;

			// Marks current dimension as done
			currentDimDone = true;
			dimsComplete[currentlyExploringDim] = true;
		}

		// ---------BUILDS THE SS------------
		
		for (int dim = 0; dim < NUM_DIMS - NUM_DIMS_DEPENDENT; ++dim) {
			if (dimsComplete[dim] == 1) {
				// Fill in the dimensions already-scanned with the already-selected best value.
				ss << extractConfigPararm(bestConfig, dim) << " ";
			}
			else if (dim == currentlyExploringDim) {
				// Fill in the nextValue to be explored
				ss << nextValue << " ";
			}
			else {
				// Adds remainder of baseline to the current best searches and current value
				//std::string restOfBaseline;
				//restOfBaseline.append(GLOB_baseline, currentlyExploringDim*2 + 2, NUM_DIMS*2 - (currentlyExploringDim+1)*2 - NUM_DIMS_DEPENDENT*2);
				//ss << restOfBaseline;

				std::string baseLineParam;
				baseLineParam.append(GLOB_baseline, dim*2, 2);
				ss << baseLineParam;
			}
			
		}
		//
		// Last NUM_DIMS_DEPENDENT3 configuration parameters are not independent.
		// They depend on one or more parameters already set. Determine the
		// remaining parameters based on already decided independent ones.
		//
		string configSoFar = ss.str();

		// Populate this object using corresponding parameters from config.
		ss << generateCacheLatencyParams(configSoFar);

		// Configuration is ready now.
		nextconfiguration = ss.str();


		// Handles the order of dimensions 
		// Mod 1: BP -> Cache -> FPU -> Core
		// 12->13->14->2->3->4->5->6->7->8->9->10->11->0->1

		if (currentDimDone) {
			
			// Signal that DSE is complete after this configuration.
			if (currentlyExploringDim == 1){
				isDSEComplete = true;
			}
			else if (currentlyExploringDim == 14) {
				currentlyExploringDim = 2;
			}
			else if (currentlyExploringDim == 11) {
				currentlyExploringDim = 0;
			}
			else {
				currentlyExploringDim++;
			}
			currentDimDone = false;

			//Resets bool to store the inital parameter of the next dimension
			firstConfig = true;	
		}
	}
	return nextconfiguration;
}