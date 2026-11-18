// Exceptions.h - Unit IV: exception framework derived from std::exception.
#ifndef ERLHUB_EXCEPTIONS_H
#define ERLHUB_EXCEPTIONS_H

#include <exception>
#include <string>

namespace ReliefOps {

// Common base so callers can catch every hub-specific failure in one handler.
class ReliefException : public std::exception {
public:
    explicit ReliefException(const std::string& message) : message_(message) {}
    virtual ~ReliefException() noexcept {}
    virtual const char* what() const noexcept { return message_.c_str(); }

private:
    std::string message_;
};

// Thrown when loading a vehicle would exceed its certified payload.
class CargoOverweightException : public ReliefException {
public:
    CargoOverweightException(const std::string& vehicleId, double attemptedKg, double limitKg);
    double excessKg() const noexcept { return attemptedKg_ - limitKg_; }

private:
    double attemptedKg_;
    double limitKg_;
};

// Thrown when a route needs more fuel than the vehicle carries.
class FuelDepletionException : public ReliefException {
public:
    FuelDepletionException(const std::string& vehicleId, double requiredLitres, double availableLitres);
    double shortfallLitres() const noexcept { return requiredLitres_ - availableLitres_; }

private:
    double requiredLitres_;
    double availableLitres_;
};

// Thrown when a corridor cannot be used by a particular vehicle.
class DamagedCorridorException : public ReliefException {
public:
    DamagedCorridorException(const std::string& corridor, const std::string& reason);
};

// Thrown when a depot cannot supply the requested quantity.
class InsufficientStockException : public ReliefException {
public:
    InsufficientStockException(const std::string& cargoId, int requested, int available);
};

// Thrown when a binary log or manifest file cannot be read or written.
class PersistenceException : public ReliefException {
public:
    PersistenceException(const std::string& path, const std::string& reason);
};

}  // namespace ReliefOps

#endif
