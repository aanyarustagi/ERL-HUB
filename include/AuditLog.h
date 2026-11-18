// AuditLog.h - Unit IV: binary persistence with atomic file updates.
#ifndef ERLHUB_AUDIT_LOG_H
#define ERLHUB_AUDIT_LOG_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "TransportVehicle.h"

namespace ReliefOps {
namespace Persistence {

enum class TransitStatus { Delivered = 0, Aborted = 1, Confirmed = 2 };

std::string statusName(TransitStatus status);

// Fixed-size plain record so that record N sits at a computable file offset.
struct TransitRecord {
    char vehicleId[16];
    char site[24];
    char corridor[32];
    double payloadKg;
    double distanceKm;
    double etaHours;
    std::int32_t status;
    std::int32_t sequence;
};

TransitRecord makeRecord(const std::string& vehicleId, const std::string& site,
                         const std::string& corridor, double payloadKg, double distanceKm,
                         double etaHours, TransitStatus status);

// Transit log file layout: [magic "ERLH"][version][record count][records...]
class TransitLog {
public:
    explicit TransitLog(const std::string& path) : path_(path) {}

    const std::string& path() const { return path_; }
    const std::vector<TransitRecord>& records() const { return records_; }
    std::size_t size() const { return records_.size(); }

    void append(TransitRecord record);  // in memory; assigns the sequence number

    // Atomic save: the full log is written to "<path>.tmp" and then renamed over
    // the real file, so a crash mid-write can never leave a half-written log.
    void commit() const;

    // Replaces the in-memory records with the file contents. Returns false if no file exists.
    bool load();

    // Random-access update: overwrites one status field in place with seekp(),
    // then reads it back with seekg() to verify.
    void amendStatus(std::size_t index, TransitStatus status);

    void print(std::ostream& out) const;

private:
    std::string path_;
    std::vector<TransitRecord> records_;
};

// Manifest audit records (binary, atomic) ...
void saveManifest(const Logistics::TransportVehicle& vehicle, const std::string& path);
void restoreManifest(Logistics::TransportVehicle& vehicle, const std::string& path);

// ... and the human-readable text form built on ReliefCargo's << and >> operators.
void exportManifestText(const Logistics::TransportVehicle& vehicle, std::ostream& out);
std::size_t importManifestText(Logistics::TransportVehicle& vehicle, std::istream& in);

}  // namespace Persistence
}  // namespace ReliefOps

#endif
