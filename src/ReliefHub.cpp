#include "ReliefHub.h"

#include <algorithm>
#include <ostream>

#include "Allocation.h"
#include "Exceptions.h"
#include "Utils.h"

namespace ReliefOps {

using Logistics::Corridor;
using Logistics::ReliefVehicle;
using Logistics::RouteReport;
using Logistics::TriageRequest;

ReliefHub::ReliefHub(const Inventory::DistributionDepot& depot, const std::string& logPath)
    : depot_(depot), log_(logPath) {}

ReliefHub::~ReliefHub() { releaseFleet(); }

void ReliefHub::releaseFleet() {
    for (std::size_t i = 0; i < fleet_.size(); ++i) {
        // Static type is ReliefVehicle*; the virtual destructor runs the full chain
        // for the real object type, so derived parts and the manifest are all released.
        delete fleet_[i];
    }
    fleet_.clear();
    assignments_.clear();
}

void ReliefHub::addVehicle(ReliefVehicle* vehicle) { fleet_.push_back(vehicle); }

void ReliefHub::addCorridor(const std::string& site, const Corridor& corridor) {
    corridors_[site] = corridor;
}

const Corridor& ReliefHub::corridorTo(const std::string& site) const {
    std::unordered_map<std::string, Corridor>::const_iterator it = corridors_.find(site);
    if (it == corridors_.end()) throw DamagedCorridorException(site, "no corridor mapped to this site");
    return it->second;
}

std::vector<std::string> ReliefHub::sites() const {
    std::vector<std::string> names;
    for (std::unordered_map<std::string, Corridor>::const_iterator it = corridors_.begin();
         it != corridors_.end(); ++it)
        names.push_back(it->first);
    std::sort(names.begin(), names.end());
    return names;
}

RouteReport ReliefHub::serve(const TriageRequest& request, std::ostream& out) {
    using Util::fixed2;
    const Corridor& corridor = corridorTo(request.site);
    const double neededKg =
        Util::payloadKg(depot_.stockOf(request.cargoId).unitWeightKg(), request.units);

    ReliefVehicle* best = nullptr;
    double bestEta = 0.0;
    for (std::size_t i = 0; i < fleet_.size(); ++i) {
        ReliefVehicle* candidate = fleet_[i];
        if (assignments_.count(candidate->id())) continue;  // already out on a mission
        try {
            const RouteReport estimate = candidate->evaluateRoute(corridor);  // dynamic dispatch
            if (neededKg > candidate->maxPayloadKg() - candidate->currentLoadKg()) {
                out << "    skip " << candidate->id() << ": payload too heavy for this vehicle\n";
            } else if (estimate.fuelLitres > candidate->fuelLitres()) {
                out << "    skip " << candidate->id() << ": not enough fuel for the round\n";
            } else if (best == nullptr || estimate.etaHours < bestEta) {
                best = candidate;
                bestEta = estimate.etaHours;
            }
        } catch (const DamagedCorridorException& error) {
            out << "    skip " << candidate->id() << ": " << error.what() << '\n';
        }
    }
    if (best == nullptr)
        throw DamagedCorridorException(corridor.name, "no free vehicle can reach " + request.site);

    Logistics::allocateSupply(depot_, *best, request);  // may throw InsufficientStockException

    try {
        const RouteReport report = best->dispatch(corridor);
        assignments_[best->id()] = request.site;
        log_.append(Persistence::makeRecord(best->id(), request.site, corridor.name,
                                            report.payloadKg, report.effectiveKm, report.etaHours,
                                            Persistence::TransitStatus::Delivered));
        out << "    DISPATCH " << best->id() << " [" << best->kind() << "] -> " << request.site
            << " via " << corridor.name << ": " << fixed2 << report.payloadKg << " kg, "
            << report.effectiveKm << " km, ETA " << report.etaHours << " h, fuel "
            << report.fuelLitres << " L\n";
        return report;
    } catch (const ReliefException&) {
        // Roll back: the vehicle never left, so its cargo returns to the depot shelves.
        for (std::size_t i = 0; i < best->manifestSize(); ++i) depot_ += best->manifestAt(i);
        best->clearManifest();
        throw;
    }
}

void ReliefHub::printFleet(std::ostream& out) const {
    using namespace Util;
    out << tableRule << col(7) << "ID" << col(12) << "Type" << col(18) << "Call sign"
        << num(10) << "Limit kg" << num(9) << "Fuel L" << "  " << "Assignment\n" << tableRule;
    for (std::size_t i = 0; i < fleet_.size(); ++i) {
        const ReliefVehicle* v = fleet_[i];
        std::unordered_map<std::string, std::string>::const_iterator it = assignments_.find(v->id());
        out << col(7) << v->id() << col(12) << v->kind() << col(18) << v->callSign() << fixed2
            << num(10) << v->maxPayloadKg() << num(9) << v->fuelLitres() << "  "
            << (it == assignments_.end() ? std::string("at depot") : "at " + it->second) << '\n';
    }
    out << tableRule;
}

}  // namespace ReliefOps
