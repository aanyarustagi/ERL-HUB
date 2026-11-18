// ReliefHub.h - the field hub: one depot, a polymorphic fleet, corridors and the transit log.
#ifndef ERLHUB_RELIEF_HUB_H
#define ERLHUB_RELIEF_HUB_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include "AuditLog.h"
#include "DistributionDepot.h"
#include "PriorityDispatchQueue.h"
#include "ReliefVehicle.h"

namespace ReliefOps {

class ReliefHub {
public:
    ReliefHub(const Inventory::DistributionDepot& depot, const std::string& logPath);
    ~ReliefHub();  // deletes the fleet through ReliefVehicle* (virtual destructors)

    Inventory::DistributionDepot& depot() { return depot_; }
    const Inventory::DistributionDepot& depot() const { return depot_; }
    Persistence::TransitLog& log() { return log_; }
    const std::vector<Logistics::ReliefVehicle*>& fleet() const { return fleet_; }

    void addVehicle(Logistics::ReliefVehicle* vehicle);  // takes ownership
    void addCorridor(const std::string& site, const Logistics::Corridor& corridor);
    const Logistics::Corridor& corridorTo(const std::string& site) const;
    std::vector<std::string> sites() const;

    // Serves one request: picks the fastest vehicle that can use the corridor, loads it,
    // dispatches it and records the trip. Narrates each decision to 'out'.
    // Throws InsufficientStockException or DamagedCorridorException; state is rolled back.
    Logistics::RouteReport serve(const Logistics::TriageRequest& request, std::ostream& out);

    void printFleet(std::ostream& out) const;
    void releaseFleet();  // deallocates every vehicle now

private:
    ReliefHub(const ReliefHub&);             // non-copyable: the hub owns raw pointers
    ReliefHub& operator=(const ReliefHub&);

    Inventory::DistributionDepot depot_;
    Persistence::TransitLog log_;
    std::vector<Logistics::ReliefVehicle*> fleet_;
    std::unordered_map<std::string, Logistics::Corridor> corridors_;
    std::unordered_map<std::string, std::string> assignments_;  // vehicle id -> site
};

}  // namespace ReliefOps

#endif
