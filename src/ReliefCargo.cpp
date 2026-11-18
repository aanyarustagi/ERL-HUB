#include "ReliefCargo.h"

#include <cmath>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Exceptions.h"

namespace ReliefOps {
namespace Inventory {

std::string categoryName(CargoCategory category) {
    switch (category) {
        case CargoCategory::RationKit:      return "RationKit";
        case CargoCategory::TraumaMedicine: return "TraumaMedicine";
        case CargoCategory::WaterTank:      return "WaterTank";
        case CargoCategory::ThermalBlanket: return "ThermalBlanket";
    }
    return "Unknown";
}

CargoCategory categoryFromName(const std::string& name) {
    if (name == "RationKit")      return CargoCategory::RationKit;
    if (name == "TraumaMedicine") return CargoCategory::TraumaMedicine;
    if (name == "WaterTank")      return CargoCategory::WaterTank;
    if (name == "ThermalBlanket") return CargoCategory::ThermalBlanket;
    throw std::invalid_argument("unknown cargo category: " + name);
}

ReliefCargo::ReliefCargo()
    : id_("NONE"), category_(CargoCategory::RationKit), unitWeightKg_(0.0), quantity_(0), priority_(1) {}

ReliefCargo::ReliefCargo(const std::string& id, CargoCategory category, double unitWeightKg,
                         int quantity, int priority)
    : id_(id), category_(category), unitWeightKg_(unitWeightKg), quantity_(quantity), priority_(priority) {
    if (id.empty() || id.find(' ') != std::string::npos)
        throw std::invalid_argument("cargo id must be non-empty and contain no spaces");
    if (unitWeightKg <= 0.0) throw std::invalid_argument("unit weight must be positive");
    if (quantity < 0) throw std::invalid_argument("quantity cannot be negative");
    if (priority < 1 || priority > 5) throw std::invalid_argument("priority must be 1..5");
}

ReliefCargo ReliefCargo::withQuantity(int quantity) const {
    return ReliefCargo(id_, category_, unitWeightKg_, quantity, priority_);
}

ReliefCargo& ReliefCargo::operator+=(int units) {
    if (units < 0) throw std::invalid_argument("cannot add a negative number of units");
    quantity_ += units;
    return *this;
}

ReliefCargo& ReliefCargo::operator-=(int units) {
    if (units < 0) throw std::invalid_argument("cannot remove a negative number of units");
    if (units > quantity_) throw InsufficientStockException(id_, units, quantity_);
    quantity_ -= units;
    return *this;
}

bool ReliefCargo::operator==(const ReliefCargo& other) const {
    return id_ == other.id_ && category_ == other.category_ &&
           std::fabs(unitWeightKg_ - other.unitWeightKg_) < 1e-9;
}

std::ostream& operator<<(std::ostream& out, const ReliefCargo& cargo) {
    return out << cargo.id_ << ' ' << categoryName(cargo.category_) << ' '
               << cargo.unitWeightKg_ << ' ' << cargo.quantity_ << ' ' << cargo.priority_;
}

std::istream& operator>>(std::istream& in, ReliefCargo& cargo) {
    std::string id, category;
    double unitWeight = 0.0;
    int quantity = 0, priority = 0;
    if (in >> id >> category >> unitWeight >> quantity >> priority) {
        try {
            // Build a temporary first so a bad record never half-updates 'cargo'.
            cargo = ReliefCargo(id, categoryFromName(category), unitWeight, quantity, priority);
        } catch (const std::invalid_argument&) {
            in.setstate(std::ios::failbit);
        }
    }
    return in;
}

void ReliefCargo::writeBinary(std::ostream& out) const {
    const std::uint32_t length = static_cast<std::uint32_t>(id_.size());
    const std::int32_t category = static_cast<std::int32_t>(category_);
    const std::int32_t quantity = quantity_;
    const std::int32_t priority = priority_;
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    out.write(id_.data(), length);
    out.write(reinterpret_cast<const char*>(&category), sizeof(category));
    out.write(reinterpret_cast<const char*>(&unitWeightKg_), sizeof(unitWeightKg_));
    out.write(reinterpret_cast<const char*>(&quantity), sizeof(quantity));
    out.write(reinterpret_cast<const char*>(&priority), sizeof(priority));
}

ReliefCargo ReliefCargo::readBinary(std::istream& in) {
    std::uint32_t length = 0;
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (!in || length == 0 || length > 64) throw std::runtime_error("corrupt cargo record");
    std::string id(length, '\0');
    in.read(&id[0], length);
    std::int32_t category = 0, quantity = 0, priority = 0;
    double unitWeight = 0.0;
    in.read(reinterpret_cast<char*>(&category), sizeof(category));
    in.read(reinterpret_cast<char*>(&unitWeight), sizeof(unitWeight));
    in.read(reinterpret_cast<char*>(&quantity), sizeof(quantity));
    in.read(reinterpret_cast<char*>(&priority), sizeof(priority));
    if (!in || category < 0 || category > 3) throw std::runtime_error("corrupt cargo record");
    return ReliefCargo(id, static_cast<CargoCategory>(category), unitWeight, quantity, priority);
}

}  // namespace Inventory
}  // namespace ReliefOps
