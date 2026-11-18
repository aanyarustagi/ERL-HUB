// ReliefCargo.h - Unit II: abstract data type for one line of relief supplies.
#ifndef ERLHUB_RELIEF_CARGO_H
#define ERLHUB_RELIEF_CARGO_H

#include <iosfwd>
#include <string>

#include "Utils.h"

namespace ReliefOps {
namespace Inventory {

enum class CargoCategory { RationKit, TraumaMedicine, WaterTank, ThermalBlanket };

std::string categoryName(CargoCategory category);
CargoCategory categoryFromName(const std::string& name);  // throws std::invalid_argument

class ReliefCargo {
public:
    ReliefCargo();
    ReliefCargo(const std::string& id, CargoCategory category, double unitWeightKg,
                int quantity, int priority);

    // Read-only interface (const correctness).
    const std::string& id() const { return id_; }
    CargoCategory category() const { return category_; }
    double unitWeightKg() const { return unitWeightKg_; }
    int quantity() const { return quantity_; }
    int priority() const { return priority_; }
    double totalWeightKg() const { return Util::payloadKg(unitWeightKg_, quantity_); }

    // Returns a copy of this cargo line holding a different number of units.
    ReliefCargo withQuantity(int quantity) const;

    ReliefCargo& operator+=(int units);  // add units
    ReliefCargo& operator-=(int units);  // remove units; throws InsufficientStockException

    // Identity check: same item regardless of how many units are held.
    bool operator==(const ReliefCargo& other) const;
    bool operator!=(const ReliefCargo& other) const { return !(*this == other); }

    // Text form "ID Category unitKg quantity priority" for console and manifest files.
    friend std::ostream& operator<<(std::ostream& out, const ReliefCargo& cargo);
    friend std::istream& operator>>(std::istream& in, ReliefCargo& cargo);

    // Binary form used by the audit files.
    void writeBinary(std::ostream& out) const;
    static ReliefCargo readBinary(std::istream& in);

private:
    std::string id_;
    CargoCategory category_;
    double unitWeightKg_;
    int quantity_;
    int priority_;  // 1 (routine) .. 5 (life-critical)
};

}  // namespace Inventory
}  // namespace ReliefOps

#endif
