#include "DistributionDepot.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>

#include "Exceptions.h"

namespace ReliefOps {
namespace Inventory {

DistributionDepot::DistributionDepot(const std::string& name, double latitude, double longitude)
    : name_(name), latitude_(latitude), longitude_(longitude) {}

DistributionDepot& DistributionDepot::operator+=(const ReliefCargo& delivery) {
    std::unordered_map<std::string, ReliefCargo>::iterator it = stock_.find(delivery.id());
    if (it == stock_.end()) {
        stock_.insert(std::make_pair(delivery.id(), delivery));
    } else if (it->second == delivery) {
        it->second += delivery.quantity();
    } else {
        throw std::invalid_argument("cargo id " + delivery.id() + " already used by a different item");
    }
    return *this;
}

DistributionDepot& DistributionDepot::operator-=(const ReliefCargo& outgoing) {
    const int available = availableUnits(outgoing.id());
    if (outgoing.quantity() > available)
        throw InsufficientStockException(outgoing.id(), outgoing.quantity(), available);
    stock_.find(outgoing.id())->second -= outgoing.quantity();
    return *this;
}

bool DistributionDepot::stocks(const std::string& cargoId) const {
    return stock_.find(cargoId) != stock_.end();
}

int DistributionDepot::reservedUnits(const std::string& cargoId) const {
    std::unordered_map<std::string, int>::const_iterator it = reserve_.find(cargoId);
    return it == reserve_.end() ? 0 : it->second;
}

int DistributionDepot::availableUnits(const std::string& cargoId) const {
    std::unordered_map<std::string, ReliefCargo>::const_iterator it = stock_.find(cargoId);
    if (it == stock_.end()) return 0;
    return std::max(0, it->second.quantity() - reservedUnits(cargoId));
}

ReliefCargo& DistributionDepot::stockOf(const std::string& cargoId) {
    std::unordered_map<std::string, ReliefCargo>::iterator it = stock_.find(cargoId);
    if (it == stock_.end()) throw InsufficientStockException(cargoId, 1, 0);
    return it->second;
}

const ReliefCargo& DistributionDepot::stockOf(const std::string& cargoId) const {
    std::unordered_map<std::string, ReliefCargo>::const_iterator it = stock_.find(cargoId);
    if (it == stock_.end()) throw InsufficientStockException(cargoId, 1, 0);
    return it->second;
}

void DistributionDepot::lockReserve(const std::string& cargoId, int units) {
    if (units < 0) throw std::invalid_argument("reserve cannot be negative");
    if (units > stockOf(cargoId).quantity())
        throw InsufficientStockException(cargoId, units, stockOf(cargoId).quantity());
    reserve_[cargoId] = units;
}

double DistributionDepot::totalStockKg() const {
    double total = 0.0;
    for (std::unordered_map<std::string, ReliefCargo>::const_iterator it = stock_.begin();
         it != stock_.end(); ++it)
        total += it->second.totalWeightKg();
    return total;
}

namespace {
// unordered_map has no stable order, so the report sorts a copy.
bool higherPriorityThenId(const ReliefCargo& a, const ReliefCargo& b) {
    if (a.priority() != b.priority()) return a.priority() > b.priority();
    return a.id() < b.id();
}
}  // namespace

std::vector<ReliefCargo> DistributionDepot::sortedStock() const {
    std::vector<ReliefCargo> lines;
    for (std::unordered_map<std::string, ReliefCargo>::const_iterator it = stock_.begin();
         it != stock_.end(); ++it)
        lines.push_back(it->second);
    std::sort(lines.begin(), lines.end(), higherPriorityThenId);
    return lines;
}

void DistributionDepot::printInventory(std::ostream& out) const {
    using namespace Util;
    out << "Depot: " << name_ << "\n" << tableRule
        << col(10) << "ID" << col(16) << "Category" << num(9) << "Unit kg" << num(8) << "Units"
        << num(9) << "Reserve" << num(11) << "Total kg" << num(6) << "Pri" << '\n'
        << tableRule;
    const std::vector<ReliefCargo> lines = sortedStock();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const ReliefCargo& c = lines[i];
        out << col(10) << c.id() << col(16) << categoryName(c.category()) << fixed2
            << num(9) << c.unitWeightKg() << num(8) << c.quantity()
            << num(9) << reservedUnits(c.id()) << num(11) << c.totalWeightKg()
            << num(6) << c.priority() << '\n';
    }
    out << tableRule << "Total stock: " << fixed2 << kgToTonnes(totalStockKg()) << " t\n";
}

}  // namespace Inventory
}  // namespace ReliefOps
