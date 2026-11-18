// IncidentCommander.h - Unit I: friend class with override authority.
#ifndef ERLHUB_INCIDENT_COMMANDER_H
#define ERLHUB_INCIDENT_COMMANDER_H

#include <iosfwd>
#include <string>

#include "DistributionDepot.h"
#include "TransportVehicle.h"

namespace ReliefOps {

// DistributionDepot and TransportVehicle both declare this class a friend, so it
// alone can touch their private state during a critical triage override.
class IncidentCommander {
public:
    explicit IncidentCommander(const std::string& name) : name_(name), overrides_(0) {}

    const std::string& name() const { return name_; }
    int overridesIssued() const { return overrides_; }

    // Unlocks a depot's strategic reserve. Returns the units released.
    int releaseReserve(Inventory::DistributionDepot& depot, const std::string& cargoId);

    // Raises a vehicle's payload limit by a percentage (emergency overload clearance).
    void authoriseOverload(Logistics::TransportVehicle& vehicle, double extraPercent);

    // Reads private members directly: no public getter exposes the manifest array address.
    void audit(const Logistics::TransportVehicle& vehicle, std::ostream& out) const;

private:
    std::string name_;
    int overrides_;
};

}  // namespace ReliefOps

#endif
