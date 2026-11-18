// Allocation.h - Unit I: overloaded supply-allocation routines.
#ifndef ERLHUB_ALLOCATION_H
#define ERLHUB_ALLOCATION_H

#include <string>

#include "DistributionDepot.h"
#include "PriorityDispatchQueue.h"
#include "ReliefVehicle.h"

namespace ReliefOps {
namespace Logistics {

// Overload 1 - volume-based payload: load as many whole units of 'cargoId' as fit
// inside 'payloadKg'. Returns the number of units moved from depot to vehicle.
int allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                   const std::string& cargoId, double payloadKg);

// Overload 2 - priority medical triage: load exactly the units the request names.
// Returns the number of units moved.
int allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                   const TriageRequest& request);

}  // namespace Logistics
}  // namespace ReliefOps

#endif
