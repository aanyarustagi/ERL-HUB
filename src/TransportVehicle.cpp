#include "TransportVehicle.h"

#include <iostream>
#include <stdexcept>

#include "Exceptions.h"
#include "Utils.h"

namespace ReliefOps {
namespace Logistics {

using Inventory::ReliefCargo;

// Static members are defined exactly once, outside the class.
int TransportVehicle::operationalVehicles_ = 0;
double TransportVehicle::dispatchedTonnes_ = 0.0;
bool TransportVehicle::lifecycleTrace_ = false;

TransportVehicle::TransportVehicle(const std::string& id, const std::string& callSign,
                                   double maxPayloadKg, double tankLitres, std::size_t manifestSlots)
    : id_(id), callSign_(callSign), maxPayloadKg_(maxPayloadKg), tankLitres_(tankLitres),
      fuelLitres_(tankLitres), manifest_(nullptr), count_(0),
      slots_(manifestSlots == 0 ? 1 : manifestSlots) {
    if (maxPayloadKg <= 0.0 || tankLitres <= 0.0)
        throw std::invalid_argument("payload limit and tank size must be positive");
    manifest_ = new ReliefCargo[slots_];
    ++operationalVehicles_;
}

TransportVehicle::TransportVehicle(const TransportVehicle& other)
    : id_(other.id_), callSign_(other.callSign_), maxPayloadKg_(other.maxPayloadKg_),
      tankLitres_(other.tankLitres_), fuelLitres_(other.fuelLitres_),
      manifest_(new ReliefCargo[other.slots_]), count_(other.count_), slots_(other.slots_) {
    try {
        for (std::size_t i = 0; i < count_; ++i) manifest_[i] = other.manifest_[i];
    } catch (...) {
        delete[] manifest_;  // no leak if copying a cargo line throws
        throw;
    }
    ++operationalVehicles_;
}

TransportVehicle& TransportVehicle::operator=(const TransportVehicle& other) {
    if (this == &other) return *this;
    // Allocate and copy first; the old array is released only after that succeeds.
    ReliefCargo* fresh = new ReliefCargo[other.slots_];
    try {
        for (std::size_t i = 0; i < other.count_; ++i) fresh[i] = other.manifest_[i];
    } catch (...) {
        delete[] fresh;
        throw;
    }
    delete[] manifest_;
    manifest_ = fresh;
    id_ = other.id_;
    callSign_ = other.callSign_;
    maxPayloadKg_ = other.maxPayloadKg_;
    tankLitres_ = other.tankLitres_;
    fuelLitres_ = other.fuelLitres_;
    count_ = other.count_;
    slots_ = other.slots_;
    return *this;
}

TransportVehicle::~TransportVehicle() {
    delete[] manifest_;
    --operationalVehicles_;
    if (lifecycleTrace_) std::cout << "      ~TransportVehicle  manifest freed for " << id_ << '\n';
}

const ReliefCargo& TransportVehicle::manifestAt(std::size_t index) const {
    if (index >= count_) throw std::out_of_range("manifest index out of range");
    return manifest_[index];
}

double TransportVehicle::currentLoadKg() const {
    double total = 0.0;
    for (std::size_t i = 0; i < count_; ++i) total += manifest_[i].totalWeightKg();
    return total;
}

void TransportVehicle::grow() {
    const std::size_t larger = slots_ * 2;
    ReliefCargo* bigger = new ReliefCargo[larger];
    try {
        for (std::size_t i = 0; i < count_; ++i) bigger[i] = manifest_[i];
    } catch (...) {
        delete[] bigger;
        throw;
    }
    delete[] manifest_;
    manifest_ = bigger;
    slots_ = larger;
}

void TransportVehicle::loadCargo(const ReliefCargo& cargo) {
    const double after = currentLoadKg() + cargo.totalWeightKg();
    if (after > maxPayloadKg_ + 1e-9) throw CargoOverweightException(id_, after, maxPayloadKg_);
    for (std::size_t i = 0; i < count_; ++i) {
        if (manifest_[i] == cargo) {  // duplicate line: merge instead of adding a second row
            manifest_[i] += cargo.quantity();
            return;
        }
    }
    if (count_ == slots_) grow();
    manifest_[count_++] = cargo;
}

void TransportVehicle::burnFuel(double litres) {
    if (litres > fuelLitres_ + 1e-9) throw FuelDepletionException(id_, litres, fuelLitres_);
    fuelLitres_ -= litres;
}

bool TransportVehicle::operator==(const TransportVehicle& other) const {
    if (count_ != other.count_) return false;
    for (std::size_t i = 0; i < count_; ++i) {
        bool matched = false;
        for (std::size_t j = 0; j < other.count_ && !matched; ++j)
            matched = manifest_[i] == other.manifest_[j] &&
                      manifest_[i].quantity() == other.manifest_[j].quantity();
        if (!matched) return false;
    }
    return true;
}

void TransportVehicle::printManifest(std::ostream& out) const {
    using namespace Util;
    out << "Manifest: " << id_ << " (" << callSign_ << ")\n" << tableRule
        << col(10) << "ID" << col(16) << "Category" << num(9) << "Unit kg" << num(8) << "Units"
        << num(11) << "Total kg" << num(6) << "Pri" << '\n'
        << tableRule;
    for (std::size_t i = 0; i < count_; ++i) {
        const ReliefCargo& c = manifest_[i];
        out << col(10) << c.id() << col(16) << Inventory::categoryName(c.category()) << fixed2
            << num(9) << c.unitWeightKg() << num(8) << c.quantity()
            << num(11) << c.totalWeightKg() << num(6) << c.priority() << '\n';
    }
    out << tableRule << "Load: " << fixed2 << currentLoadKg() << " / " << maxPayloadKg_
        << " kg   Fuel: " << fuelLitres_ << " / " << tankLitres_ << " L\n";
}

void TransportVehicle::recordDispatch(double payloadKg) {
    dispatchedTonnes_ += Util::kgToTonnes(payloadKg);
}

}  // namespace Logistics
}  // namespace ReliefOps
