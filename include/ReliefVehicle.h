// ReliefVehicle.h - Unit III: inheritance tree, virtual base classes, polymorphism.
//
//                    TransportVehicle            (ADT: manifest, fuel, statics)
//                           |
//                     ReliefVehicle              (abstract: evaluateRoute() = 0)
//                 /  virtual |  virtual \ virtual
//      AerialTransport  GroundConvoy  AmphibiousCraft
//                            \          /
//                       AmphibiousMedicalUnit    (hybrid; ONE shared ReliefVehicle)
#ifndef ERLHUB_RELIEF_VEHICLE_H
#define ERLHUB_RELIEF_VEHICLE_H

#include <string>

#include "Components.h"
#include "TransportVehicle.h"

namespace ReliefOps {
namespace Logistics {

enum class CorridorType { Road, River, Air };

std::string corridorTypeName(CorridorType type);

// One leg from the depot to a relief site.
struct Corridor {
    std::string name;
    CorridorType type;
    double distanceKm;
    double damage;        // 0.0 intact .. 1.0 destroyed
    double endLatitude;
    double endLongitude;
};

struct RouteReport {
    double effectiveKm;
    double etaHours;
    double fuelLitres;
    double payloadKg;
};

// Construction parameters shared by every vehicle type.
struct VehicleSpec {
    std::string id;
    std::string callSign;
    double maxPayloadKg;
    double tankLitres;
    double cruiseKmph;
    double litresPerKm;
};

class ReliefVehicle : public TransportVehicle {
public:
    ReliefVehicle(const VehicleSpec& spec, double latitude, double longitude);
    virtual ~ReliefVehicle();

    // Each vehicle type decides whether and how it can use a corridor.
    // Throws DamagedCorridorException when the corridor is unusable for this type.
    virtual RouteReport evaluateRoute(const Corridor& corridor) const = 0;
    virtual std::string kind() const = 0;

    // Evaluates the route, burns fuel, moves the beacon and delivers the manifest.
    // Throws DamagedCorridorException or FuelDepletionException; on a throw nothing changes.
    RouteReport dispatch(const Corridor& corridor);

    GPSBeacon& beacon() { return beacon_; }
    const GPSBeacon& beacon() const { return beacon_; }
    double cruiseKmph() const { return cruiseKmph_; }

protected:
    // Shared arithmetic for the derived evaluateRoute() overrides.
    RouteReport buildReport(double effectiveKm, double speedKmph) const;

private:
    GPSBeacon beacon_;  // containership: every relief vehicle HAS a beacon
    double cruiseKmph_;
    double litresPerKm_;
};

// Helicopters and quadcopters: fly straight over damaged ground.
class AerialTransport : virtual public ReliefVehicle {
public:
    AerialTransport(const VehicleSpec& spec, double latitude, double longitude,
                    int stretchers, double oxygenLitres);
    virtual ~AerialTransport();

    virtual RouteReport evaluateRoute(const Corridor& corridor) const;
    virtual std::string kind() const { return "Aerial"; }

    EmergencyLifeSupportUnit& lifeSupport() { return lifeSupport_; }
    const EmergencyLifeSupportUnit& lifeSupport() const { return lifeSupport_; }

private:
    EmergencyLifeSupportUnit lifeSupport_;  // containership
};

// All-terrain trucks: roads only, slowed or stopped by damage.
class GroundConvoy : virtual public ReliefVehicle {
public:
    GroundConvoy(const VehicleSpec& spec, double latitude, double longitude, int trucks);
    virtual ~GroundConvoy();

    virtual RouteReport evaluateRoute(const Corridor& corridor) const;
    virtual std::string kind() const { return "Ground"; }
    int trucks() const { return trucks_; }

protected:
    RouteReport evaluateRoad(const Corridor& corridor, double blockedAt) const;

private:
    int trucks_;
};

// Boats: rivers only, stopped by heavy debris.
class AmphibiousCraft : virtual public ReliefVehicle {
public:
    AmphibiousCraft(const VehicleSpec& spec, double latitude, double longitude, double draftMetres);
    virtual ~AmphibiousCraft();

    virtual RouteReport evaluateRoute(const Corridor& corridor) const;
    virtual std::string kind() const { return "Amphibious"; }
    double draftMetres() const { return draftMetres_; }

protected:
    RouteReport evaluateRiver(const Corridor& corridor) const;

private:
    double draftMetres_;
};

// Hybrid: drives on roads AND crosses rivers, with a medical bay on board.
// GroundConvoy and AmphibiousCraft inherit ReliefVehicle virtually, so this
// object holds a single ReliefVehicle sub-object (one id, one manifest, one beacon).
class AmphibiousMedicalUnit : public GroundConvoy, public AmphibiousCraft {
public:
    AmphibiousMedicalUnit(const VehicleSpec& spec, double latitude, double longitude,
                          int stretchers, double oxygenLitres);
    virtual ~AmphibiousMedicalUnit();

    virtual RouteReport evaluateRoute(const Corridor& corridor) const;
    virtual std::string kind() const { return "Hybrid-Med"; }

    EmergencyLifeSupportUnit& lifeSupport() { return lifeSupport_; }
    const EmergencyLifeSupportUnit& lifeSupport() const { return lifeSupport_; }

private:
    EmergencyLifeSupportUnit lifeSupport_;  // containership
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif
