// Utils.h - Unit I: inline utilities. Unit IV: user-defined stream manipulators.
#ifndef ERLHUB_UTILS_H
#define ERLHUB_UTILS_H

#include <cmath>
#include <iomanip>
#include <ostream>
#include <string>

namespace ReliefOps {
namespace Util {

const double kEarthRadiusKm = 6371.0;
const double kPi = 3.14159265358979323846;

// ---- Inline calculations (expanded at the call site, no call overhead) ----

inline double kgToTonnes(double kg) { return kg / 1000.0; }

inline double payloadKg(double unitWeightKg, int quantity) { return unitWeightKg * quantity; }

inline double toRadians(double degrees) { return degrees * kPi / 180.0; }

// Great-circle distance between two GPS fixes (haversine formula).
inline double distanceKm(double lat1, double lon1, double lat2, double lon2) {
    const double dLat = toRadians(lat2 - lat1);
    const double dLon = toRadians(lon2 - lon1);
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
                     std::sin(dLon / 2) * std::sin(dLon / 2);
    return 2.0 * kEarthRadiusKm * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

inline double etaHours(double distanceKm, double speedKmph) {
    return speedKmph > 0.0 ? distanceKm / speedKmph : 0.0;
}

// A fully loaded vehicle burns up to 40% more fuel than an empty one.
inline double fuelNeededLitres(double distanceKm, double litresPerKm, double loadFraction) {
    return distanceKm * litresPerKm * (1.0 + 0.4 * loadFraction);
}

// ---- User-defined manipulators for tabular manifest printing ----

// Parameterless manipulator: prints a horizontal rule and a newline.
inline std::ostream& tableRule(std::ostream& out) {
    return out << std::string(72, '-') << '\n';
}

// Parameterless manipulator: fixed-point with two decimals.
inline std::ostream& fixed2(std::ostream& out) {
    return out << std::fixed << std::setprecision(2);
}

// Parameterised manipulator: left-aligned column of a given width.
class col {
public:
    explicit col(int width) : width_(width) {}
    friend std::ostream& operator<<(std::ostream& out, const col& c) {
        return out << std::left << std::setw(c.width_);
    }

private:
    int width_;
};

// Parameterised manipulator: right-aligned numeric column.
class num {
public:
    explicit num(int width) : width_(width) {}
    friend std::ostream& operator<<(std::ostream& out, const num& n) {
        return out << std::right << std::setw(n.width_);
    }

private:
    int width_;
};

}  // namespace Util
}  // namespace ReliefOps

#endif
