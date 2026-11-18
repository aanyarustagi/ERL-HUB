#include "Exceptions.h"

#include <sstream>

namespace ReliefOps {

namespace {
// Builds the diagnostic text once, at throw time.
std::string overweightText(const std::string& id, double attempted, double limit) {
    std::ostringstream out;
    out << "CargoOverweight: vehicle " << id << " would carry " << attempted
        << " kg against a limit of " << limit << " kg";
    return out.str();
}

std::string fuelText(const std::string& id, double required, double available) {
    std::ostringstream out;
    out << "FuelDepletion: vehicle " << id << " needs " << required
        << " L but holds " << available << " L";
    return out.str();
}

std::string stockText(const std::string& id, int requested, int available) {
    std::ostringstream out;
    out << "InsufficientStock: " << requested << " unit(s) of " << id
        << " requested, " << available << " available";
    return out.str();
}
}  // namespace

CargoOverweightException::CargoOverweightException(const std::string& vehicleId,
                                                   double attemptedKg, double limitKg)
    : ReliefException(overweightText(vehicleId, attemptedKg, limitKg)),
      attemptedKg_(attemptedKg),
      limitKg_(limitKg) {}

FuelDepletionException::FuelDepletionException(const std::string& vehicleId,
                                               double requiredLitres, double availableLitres)
    : ReliefException(fuelText(vehicleId, requiredLitres, availableLitres)),
      requiredLitres_(requiredLitres),
      availableLitres_(availableLitres) {}

DamagedCorridorException::DamagedCorridorException(const std::string& corridor,
                                                   const std::string& reason)
    : ReliefException("DamagedCorridor: " + corridor + " - " + reason) {}

InsufficientStockException::InsufficientStockException(const std::string& cargoId,
                                                       int requested, int available)
    : ReliefException(stockText(cargoId, requested, available)) {}

PersistenceException::PersistenceException(const std::string& path, const std::string& reason)
    : ReliefException("Persistence: " + path + " - " + reason) {}

}  // namespace ReliefOps
