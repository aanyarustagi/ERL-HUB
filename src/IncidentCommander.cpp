#include "IncidentCommander.h"

#include <ostream>
#include <stdexcept>

#include "Utils.h"

namespace ReliefOps {

int IncidentCommander::releaseReserve(Inventory::DistributionDepot& depot,
                                      const std::string& cargoId) {
    const int released = depot.reservedUnits(cargoId);  // private member function
    depot.reserve_.erase(cargoId);                      // private data member
    ++overrides_;
    return released;
}

void IncidentCommander::authoriseOverload(Logistics::TransportVehicle& vehicle,
                                          double extraPercent) {
    if (extraPercent < 0.0 || extraPercent > 25.0)
        throw std::invalid_argument("overload clearance must be between 0 and 25 percent");
    vehicle.maxPayloadKg_ *= 1.0 + extraPercent / 100.0;  // private data member
    ++overrides_;
}

void IncidentCommander::audit(const Logistics::TransportVehicle& vehicle, std::ostream& out) const {
    out << "Audit by " << name_ << ": " << vehicle.id_ << " uses " << vehicle.count_ << " of "
        << vehicle.slots_ << " manifest slots, payload limit " << Util::fixed2
        << vehicle.maxPayloadKg_ << " kg\n";
}

}  // namespace ReliefOps
