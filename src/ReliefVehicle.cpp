#include "ReliefVehicle.h"

#include <iostream>

#include "Exceptions.h"
#include "Utils.h"

namespace ReliefOps {
namespace Logistics {

namespace {
void trace(const char* text, const std::string& id) {
    if (TransportVehicle::lifecycleTrace()) std::cout << "      " << text << " " << id << '\n';
}
}  // namespace

std::string corridorTypeName(CorridorType type) {
    switch (type) {
        case CorridorType::Road:  return "Road";
        case CorridorType::River: return "River";
        case CorridorType::Air:   return "Air";
    }
    return "Unknown";
}

// ---------------------------------------------------------------- ReliefVehicle

ReliefVehicle::ReliefVehicle(const VehicleSpec& spec, double latitude, double longitude)
    : TransportVehicle(spec.id, spec.callSign, spec.maxPayloadKg, spec.tankLitres),
      beacon_(latitude, longitude),
      cruiseKmph_(spec.cruiseKmph),
      litresPerKm_(spec.litresPerKm) {}

ReliefVehicle::~ReliefVehicle() { trace("~ReliefVehicle    ", id()); }

RouteReport ReliefVehicle::buildReport(double effectiveKm, double speedKmph) const {
    RouteReport report;
    report.effectiveKm = effectiveKm;
    report.etaHours = Util::etaHours(effectiveKm, speedKmph);
    report.fuelLitres = Util::fuelNeededLitres(effectiveKm, litresPerKm_, loadFraction());
    report.payloadKg = currentLoadKg();
    return report;
}

RouteReport ReliefVehicle::dispatch(const Corridor& corridor) {
    const RouteReport report = evaluateRoute(corridor);  // virtual call; may throw
    burnFuel(report.fuelLitres);                         // may throw; state still untouched
    beacon_.moveTo(corridor.endLatitude, corridor.endLongitude).ping();
    recordDispatch(report.payloadKg);
    clearManifest();  // supplies handed over at the relief site
    return report;
}

// -------------------------------------------------------------- AerialTransport

AerialTransport::AerialTransport(const VehicleSpec& spec, double latitude, double longitude,
                                 int stretchers, double oxygenLitres)
    : ReliefVehicle(spec, latitude, longitude), lifeSupport_(stretchers, oxygenLitres) {}

AerialTransport::~AerialTransport() { trace("~AerialTransport  ", id()); }

RouteReport AerialTransport::evaluateRoute(const Corridor& corridor) const {
    // Aircraft fly the straight line from the beacon fix; ground damage is irrelevant.
    double latitude = 0.0, longitude = 0.0;
    beacon().readFix(latitude, longitude);
    const double direct = Util::distanceKm(latitude, longitude,
                                           corridor.endLatitude, corridor.endLongitude);
    return buildReport(direct, cruiseKmph());
}

// ----------------------------------------------------------------- GroundConvoy

GroundConvoy::GroundConvoy(const VehicleSpec& spec, double latitude, double longitude, int trucks)
    : ReliefVehicle(spec, latitude, longitude), trucks_(trucks) {}

GroundConvoy::~GroundConvoy() { trace("~GroundConvoy     ", id()); }

RouteReport GroundConvoy::evaluateRoad(const Corridor& corridor, double blockedAt) const {
    if (corridor.damage >= blockedAt)
        throw DamagedCorridorException(corridor.name, "road damage too severe for " + id());
    // Damage forces detours (longer) and slow driving (slower).
    const double effectiveKm = corridor.distanceKm * (1.0 + corridor.damage);
    const double speed = cruiseKmph() * (1.0 - 0.6 * corridor.damage);
    return buildReport(effectiveKm, speed);
}

RouteReport GroundConvoy::evaluateRoute(const Corridor& corridor) const {
    if (corridor.type != CorridorType::Road)
        throw DamagedCorridorException(corridor.name, "ground convoy " + id() + " needs a road");
    return evaluateRoad(corridor, 0.70);
}

// -------------------------------------------------------------- AmphibiousCraft

AmphibiousCraft::AmphibiousCraft(const VehicleSpec& spec, double latitude, double longitude,
                                 double draftMetres)
    : ReliefVehicle(spec, latitude, longitude), draftMetres_(draftMetres) {}

AmphibiousCraft::~AmphibiousCraft() { trace("~AmphibiousCraft  ", id()); }

RouteReport AmphibiousCraft::evaluateRiver(const Corridor& corridor) const {
    if (corridor.damage >= 0.90)
        throw DamagedCorridorException(corridor.name, "river channel blocked by debris");
    const double speed = cruiseKmph() * (1.0 - 0.4 * corridor.damage);
    return buildReport(corridor.distanceKm, speed);
}

RouteReport AmphibiousCraft::evaluateRoute(const Corridor& corridor) const {
    if (corridor.type != CorridorType::River)
        throw DamagedCorridorException(corridor.name, "craft " + id() + " needs a waterway");
    return evaluateRiver(corridor);
}

// -------------------------------------------------------- AmphibiousMedicalUnit

// The most-derived class constructs the virtual base ReliefVehicle directly;
// the ReliefVehicle initialisers inside GroundConvoy and AmphibiousCraft are skipped.
AmphibiousMedicalUnit::AmphibiousMedicalUnit(const VehicleSpec& spec, double latitude,
                                             double longitude, int stretchers, double oxygenLitres)
    : ReliefVehicle(spec, latitude, longitude),
      GroundConvoy(spec, latitude, longitude, 1),
      AmphibiousCraft(spec, latitude, longitude, 0.6),
      lifeSupport_(stretchers, oxygenLitres) {}

AmphibiousMedicalUnit::~AmphibiousMedicalUnit() { trace("~AmphibiousMedUnit", id()); }

RouteReport AmphibiousMedicalUnit::evaluateRoute(const Corridor& corridor) const {
    switch (corridor.type) {
        case CorridorType::Road:  return evaluateRoad(corridor, 0.85);  // can ford broken sections
        case CorridorType::River: return evaluateRiver(corridor);
        case CorridorType::Air:   break;
    }
    throw DamagedCorridorException(corridor.name, "hybrid unit " + id() + " cannot fly");
}

}  // namespace Logistics
}  // namespace ReliefOps
