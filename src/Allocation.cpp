#include "Allocation.h"

#include <algorithm>

#include "Exceptions.h"

namespace ReliefOps {
namespace Logistics {

namespace {
// Shared transfer step. Order matters for exception safety:
//   1. stock check (throws, nothing changed)
//   2. vehicle load (throws on overweight, nothing changed)
//   3. depot deduction (cannot fail after step 1)
int transfer(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
             const std::string& cargoId, int units) {
    const int available = depot.availableUnits(cargoId);
    if (units <= 0 || units > available)
        throw InsufficientStockException(cargoId, units, available);
    const Inventory::ReliefCargo parcel = depot.stockOf(cargoId).withQuantity(units);
    vehicle.loadCargo(parcel);
    depot -= parcel;
    return units;
}
}  // namespace

int allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                   const std::string& cargoId, double payloadKg) {
    const Inventory::ReliefCargo& line = depot.stockOf(cargoId);  // reference, no copy
    const double room = std::min(payloadKg, vehicle.maxPayloadKg() - vehicle.currentLoadKg());
    const int fitting = static_cast<int>(room / line.unitWeightKg() + 1e-9);
    const int units = std::min(fitting, depot.availableUnits(cargoId));
    return transfer(depot, vehicle, cargoId, units);
}

int allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                   const TriageRequest& request) {
    return transfer(depot, vehicle, request.cargoId, request.units);
}

}  // namespace Logistics
}  // namespace ReliefOps
