// DistributionDepot.h - Unit II: depot stock ledger with += / -= operators.
#ifndef ERLHUB_DISTRIBUTION_DEPOT_H
#define ERLHUB_DISTRIBUTION_DEPOT_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include "ReliefCargo.h"

namespace ReliefOps {

class IncidentCommander;  // forward declaration for the friend grant below

namespace Inventory {

class DistributionDepot {
public:
    DistributionDepot(const std::string& name, double latitude, double longitude);

    const std::string& name() const { return name_; }
    double latitude() const { return latitude_; }
    double longitude() const { return longitude_; }

    // Restock: adds units to an existing line or opens a new one.
    DistributionDepot& operator+=(const ReliefCargo& delivery);
    // Deduct on vehicle departure; throws InsufficientStockException.
    DistributionDepot& operator-=(const ReliefCargo& outgoing);

    bool stocks(const std::string& cargoId) const;
    // Units that ordinary allocation may draw (total minus the locked reserve).
    int availableUnits(const std::string& cargoId) const;

    // Return by reference: callers update the ledger entry in place, no copy.
    ReliefCargo& stockOf(const std::string& cargoId);
    const ReliefCargo& stockOf(const std::string& cargoId) const;

    // Holds back units that only an IncidentCommander may release.
    void lockReserve(const std::string& cargoId, int units);

    double totalStockKg() const;
    std::vector<ReliefCargo> sortedStock() const;  // highest priority first
    void printInventory(std::ostream& out) const;

private:
    friend class ReliefOps::IncidentCommander;  // may release the private reserve

    int reservedUnits(const std::string& cargoId) const;

    std::string name_;
    double latitude_;
    double longitude_;
    std::unordered_map<std::string, ReliefCargo> stock_;
    std::unordered_map<std::string, int> reserve_;
};

}  // namespace Inventory
}  // namespace ReliefOps

#endif
