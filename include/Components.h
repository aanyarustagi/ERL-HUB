// Components.h - Unit III: independent parts composed INTO vehicles (has-a, not is-a).
#ifndef ERLHUB_COMPONENTS_H
#define ERLHUB_COMPONENTS_H

#include <string>

namespace ReliefOps {
namespace Logistics {

// Position tracker carried by every relief vehicle.
class GPSBeacon {
public:
    GPSBeacon(double latitude, double longitude)
        : latitude_(latitude), longitude_(longitude), pings_(0) {}

    double latitude() const { return latitude_; }
    double longitude() const { return longitude_; }
    int pings() const { return pings_; }

    // Returns *this by reference so position updates can be chained.
    GPSBeacon& moveTo(double latitude, double longitude) {
        latitude_ = latitude;
        longitude_ = longitude;
        return *this;
    }
    GPSBeacon& ping() {
        ++pings_;
        return *this;
    }

    // Call by reference: writes the current fix into the caller's variables.
    void readFix(double& latitude, double& longitude) const {
        latitude = latitude_;
        longitude = longitude_;
    }

private:
    double latitude_;
    double longitude_;
    int pings_;
};

// Medical module fitted to medevac-capable vehicles.
class EmergencyLifeSupportUnit {
public:
    EmergencyLifeSupportUnit(int stretchers, double oxygenLitres)
        : stretchers_(stretchers), occupied_(0), oxygenLitres_(oxygenLitres) {}

    int freeStretchers() const { return stretchers_ - occupied_; }
    int patientsAboard() const { return occupied_; }
    double oxygenLitres() const { return oxygenLitres_; }

    // Each patient draws about 10 L of oxygen per minute.
    double oxygenHoursLeft() const {
        return occupied_ == 0 ? 0.0 : oxygenLitres_ / (occupied_ * 600.0);
    }

    bool admitPatient() {
        if (occupied_ >= stretchers_) return false;
        ++occupied_;
        return true;
    }
    void dischargeAll() { occupied_ = 0; }

private:
    int stretchers_;
    int occupied_;
    double oxygenLitres_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif
