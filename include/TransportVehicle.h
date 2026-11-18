// TransportVehicle.h - Unit II: vehicle ADT owning a dynamically allocated manifest.
#ifndef ERLHUB_TRANSPORT_VEHICLE_H
#define ERLHUB_TRANSPORT_VEHICLE_H

#include <cstddef>
#include <iosfwd>
#include <string>

#include "ReliefCargo.h"

namespace ReliefOps {

class IncidentCommander;

namespace Logistics {

class TransportVehicle {
public:
    // Dynamic constructor: allocates the manifest array on the heap.
    TransportVehicle(const std::string& id, const std::string& callSign, double maxPayloadKg,
                     double tankLitres, std::size_t manifestSlots = 2);
    TransportVehicle(const TransportVehicle& other);             // deep copy
    TransportVehicle& operator=(const TransportVehicle& other);  // deep copy, self-safe
    virtual ~TransportVehicle();                                 // releases the manifest

    const std::string& id() const { return id_; }
    const std::string& callSign() const { return callSign_; }
    double maxPayloadKg() const { return maxPayloadKg_; }
    double fuelLitres() const { return fuelLitres_; }
    double tankLitres() const { return tankLitres_; }
    std::size_t manifestSize() const { return count_; }
    std::size_t manifestSlots() const { return slots_; }
    const Inventory::ReliefCargo& manifestAt(std::size_t index) const;
    double currentLoadKg() const;
    double loadFraction() const { return currentLoadKg() / maxPayloadKg_; }

    // Adds cargo; merges with an identical line (operator==). Throws CargoOverweightException.
    void loadCargo(const Inventory::ReliefCargo& cargo);
    void clearManifest() { count_ = 0; }
    void refuel() { fuelLitres_ = tankLitres_; }
    void burnFuel(double litres);  // throws FuelDepletionException

    // Manifest identity: both vehicles carry exactly the same lines and quantities.
    bool operator==(const TransportVehicle& other) const;

    void printManifest(std::ostream& out) const;

    // ---- Fleet-wide analytics shared by every instance (static members) ----
    static int operationalVehicles() { return operationalVehicles_; }
    static double dispatchedTonnes() { return dispatchedTonnes_; }
    static void recordDispatch(double payloadKg);
    static void setLifecycleTrace(bool enabled) { lifecycleTrace_ = enabled; }
    static bool lifecycleTrace() { return lifecycleTrace_; }

private:
    friend class ReliefOps::IncidentCommander;  // may raise the payload limit in a triage override

    void grow();  // doubles the manifest array

    std::string id_;
    std::string callSign_;
    double maxPayloadKg_;
    double tankLitres_;
    double fuelLitres_;
    Inventory::ReliefCargo* manifest_;  // owned heap array
    std::size_t count_;
    std::size_t slots_;

    static int operationalVehicles_;
    static double dispatchedTonnes_;
    static bool lifecycleTrace_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif
