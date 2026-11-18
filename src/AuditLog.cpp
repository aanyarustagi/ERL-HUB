#include "AuditLog.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "Exceptions.h"
#include "Utils.h"

namespace ReliefOps {
namespace Persistence {

namespace {

const char kLogMagic[4] = {'E', 'R', 'L', 'H'};
const char kManifestMagic[4] = {'E', 'R', 'L', 'M'};
const std::uint32_t kVersion = 1;
const std::streamoff kHeaderBytes = 4 + sizeof(std::uint32_t) * 2;

void copyText(char* destination, std::size_t capacity, const std::string& text) {
    std::memset(destination, 0, capacity);
    std::strncpy(destination, text.c_str(), capacity - 1);
}

// Swaps the finished temporary file into place.
void replaceFile(const std::string& temporary, const std::string& path) {
#ifdef _WIN32
    std::remove(path.c_str());  // Windows rename() refuses to overwrite
#endif
    if (std::rename(temporary.c_str(), path.c_str()) != 0) {
        std::remove(temporary.c_str());
        throw PersistenceException(path, "could not replace file with new version");
    }
}

void requireGood(const std::ios& stream, const std::string& path, const char* action) {
    if (!stream) throw PersistenceException(path, action);
}

}  // namespace

std::string statusName(TransitStatus status) {
    switch (status) {
        case TransitStatus::Delivered: return "Delivered";
        case TransitStatus::Aborted:   return "Aborted";
        case TransitStatus::Confirmed: return "Confirmed";
    }
    return "Unknown";
}

TransitRecord makeRecord(const std::string& vehicleId, const std::string& site,
                         const std::string& corridor, double payloadKg, double distanceKm,
                         double etaHours, TransitStatus status) {
    TransitRecord record;
    std::memset(&record, 0, sizeof(record));  // zero padding bytes for a stable file image
    copyText(record.vehicleId, sizeof(record.vehicleId), vehicleId);
    copyText(record.site, sizeof(record.site), site);
    copyText(record.corridor, sizeof(record.corridor), corridor);
    record.payloadKg = payloadKg;
    record.distanceKm = distanceKm;
    record.etaHours = etaHours;
    record.status = static_cast<std::int32_t>(status);
    record.sequence = 0;
    return record;
}

void TransitLog::append(TransitRecord record) {
    record.sequence = static_cast<std::int32_t>(records_.size() + 1);
    records_.push_back(record);
}

void TransitLog::commit() const {
    const std::string temporary = path_ + ".tmp";
    {
        std::fstream file(temporary.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        requireGood(file, temporary, "cannot open for writing");
        const std::uint32_t count = static_cast<std::uint32_t>(records_.size());
        file.write(kLogMagic, sizeof(kLogMagic));
        file.write(reinterpret_cast<const char*>(&kVersion), sizeof(kVersion));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (count > 0)
            file.write(reinterpret_cast<const char*>(&records_[0]),
                       static_cast<std::streamsize>(count * sizeof(TransitRecord)));
        file.flush();
        if (!file) {
            file.close();
            std::remove(temporary.c_str());
            throw PersistenceException(temporary, "write failed; previous log left untouched");
        }
    }  // file closed here, before the rename
    replaceFile(temporary, path_);
}

bool TransitLog::load() {
    std::fstream file(path_.c_str(), std::ios::in | std::ios::binary);
    if (!file) return false;

    char magic[4];
    std::uint32_t version = 0, count = 0;
    file.read(magic, sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file || std::memcmp(magic, kLogMagic, sizeof(magic)) != 0 || version != kVersion)
        throw PersistenceException(path_, "not a valid ERL-HUB transit log");

    // Check the promised record count against the real file size before trusting it.
    file.seekg(0, std::ios::end);
    const std::streamoff bytes = file.tellg() - kHeaderBytes;
    if (bytes != static_cast<std::streamoff>(count * sizeof(TransitRecord)))
        throw PersistenceException(path_, "record count does not match file size");
    file.seekg(kHeaderBytes, std::ios::beg);

    std::vector<TransitRecord> loaded(count);
    if (count > 0)
        file.read(reinterpret_cast<char*>(&loaded[0]),
                  static_cast<std::streamsize>(count * sizeof(TransitRecord)));
    requireGood(file, path_, "truncated record data");
    records_.swap(loaded);  // only replace our records once the whole read succeeded
    return true;
}

void TransitLog::amendStatus(std::size_t index, TransitStatus status) {
    if (index >= records_.size()) throw std::out_of_range("transit record index out of range");

    std::fstream file(path_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    requireGood(file, path_, "cannot open for update (commit the log first)");

    const std::streamoff offset = kHeaderBytes +
        static_cast<std::streamoff>(index * sizeof(TransitRecord)) +
        static_cast<std::streamoff>(offsetof(TransitRecord, status));
    const std::int32_t value = static_cast<std::int32_t>(status);

    file.seekp(offset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    file.flush();

    std::int32_t check = -1;
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&check), sizeof(check));
    if (!file || check != value) throw PersistenceException(path_, "status update failed verification");

    records_[index].status = value;
}

void TransitLog::print(std::ostream& out) const {
    using namespace Util;
    out << tableRule << col(5) << "Seq" << col(8) << "Vehicle" << col(14) << "Site"
        << col(18) << "Corridor" << num(9) << "Load kg" << num(7) << "ETA h" << "  Status\n"
        << tableRule;
    for (std::size_t i = 0; i < records_.size(); ++i) {
        const TransitRecord& r = records_[i];
        out << col(5) << r.sequence << col(8) << r.vehicleId << col(14) << r.site
            << col(18) << r.corridor << fixed2 << num(9) << r.payloadKg << num(7) << r.etaHours
            << "  " << statusName(static_cast<TransitStatus>(r.status)) << '\n';
    }
    out << tableRule;
}

void saveManifest(const Logistics::TransportVehicle& vehicle, const std::string& path) {
    const std::string temporary = path + ".tmp";
    {
        std::fstream file(temporary.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        requireGood(file, temporary, "cannot open for writing");
        const std::uint32_t count = static_cast<std::uint32_t>(vehicle.manifestSize());
        file.write(kManifestMagic, sizeof(kManifestMagic));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (std::size_t i = 0; i < vehicle.manifestSize(); ++i) vehicle.manifestAt(i).writeBinary(file);
        file.flush();
        if (!file) {
            file.close();
            std::remove(temporary.c_str());
            throw PersistenceException(temporary, "write failed; previous manifest left untouched");
        }
    }
    replaceFile(temporary, path);
}

void restoreManifest(Logistics::TransportVehicle& vehicle, const std::string& path) {
    std::fstream file(path.c_str(), std::ios::in | std::ios::binary);
    requireGood(file, path, "cannot open manifest audit file");

    char magic[4];
    std::uint32_t count = 0;
    file.read(magic, sizeof(magic));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file || std::memcmp(magic, kManifestMagic, sizeof(magic)) != 0 || count > 10000)
        throw PersistenceException(path, "not a valid ERL-HUB manifest file");

    // Read everything into a scratch list first; touch the vehicle only on success.
    std::vector<Inventory::ReliefCargo> lines;
    try {
        for (std::uint32_t i = 0; i < count; ++i)
            lines.push_back(Inventory::ReliefCargo::readBinary(file));
    } catch (const std::exception& error) {
        throw PersistenceException(path, error.what());
    }
    vehicle.clearManifest();
    for (std::size_t i = 0; i < lines.size(); ++i) vehicle.loadCargo(lines[i]);
}

void exportManifestText(const Logistics::TransportVehicle& vehicle, std::ostream& out) {
    for (std::size_t i = 0; i < vehicle.manifestSize(); ++i) out << vehicle.manifestAt(i) << '\n';
}

std::size_t importManifestText(Logistics::TransportVehicle& vehicle, std::istream& in) {
    std::size_t loaded = 0;
    Inventory::ReliefCargo line;
    while (in >> line) {
        vehicle.loadCargo(line);
        ++loaded;
    }
    return loaded;
}

}  // namespace Persistence
}  // namespace ReliefOps
