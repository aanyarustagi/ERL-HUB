// Scenario.h - the sample earthquake scenario used by the demo, the menu and the tests.
#ifndef ERLHUB_SCENARIO_H
#define ERLHUB_SCENARIO_H

#include <vector>

#include "PriorityDispatchQueue.h"
#include "ReliefHub.h"

namespace ReliefOps {
namespace Scenario {

// Field hub position (Dehradun) used as the start point for every vehicle.
const double kHubLatitude = 30.3165;
const double kHubLongitude = 78.0322;

Inventory::DistributionDepot makeDepot();           // stocked depot with a locked medical reserve
void populate(ReliefHub& hub);                      // adds the fleet and the corridors
std::vector<Logistics::TriageRequest> requests();   // the four opening requests

}  // namespace Scenario
}  // namespace ReliefOps

#endif
