// Self-checking tests for ERL-HUB. Build and run with:  make test
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "Allocation.h"
#include "AuditLog.h"
#include "Exceptions.h"
#include "IncidentCommander.h"
#include "PriorityDispatchQueue.h"
#include "ReliefHub.h"
#include "Scenario.h"

using namespace ReliefOps;
using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;

namespace {

int checks = 0;
int failures = 0;

void check(bool condition, const char* expression, int line) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cout << "  FAIL line " << line << ": " << expression << '\n';
    }
}

#define CHECK(expression) check((expression), #expression, __LINE__)
#define CHECK_THROWS(statement, Type)                               \
    do {                                                            \
        bool caught = false;                                        \
        try { statement; } catch (const Type&) { caught = true; }   \
        check(caught, #statement " throws " #Type, __LINE__);       \
    } while (false)

bool near(double a, double b) { return a - b < 1e-6 && b - a < 1e-6; }

const VehicleSpec kTruck = {"T-1", "Test Truck", 1000.0, 200.0, 50.0, 1.0};

void testCargo() {
    ReliefCargo med("MED-01", CargoCategory::TraumaMedicine, 2.5, 10, 5);
    CHECK(near(med.totalWeightKg(), 25.0));
    med += 5;
    CHECK(med.quantity() == 15);
    med -= 15;
    CHECK(med.quantity() == 0);
    CHECK_THROWS(med -= 1, InsufficientStockException);
    CHECK(med == med.withQuantity(99));  // identity ignores quantity
    CHECK(med != ReliefCargo("MED-02", CargoCategory::TraumaMedicine, 2.5, 10, 5));
    CHECK_THROWS(ReliefCargo("BAD ID", CargoCategory::RationKit, 1.0, 1, 1), std::invalid_argument);

    std::stringstream text;
    text << ReliefCargo("WTR-03", CargoCategory::WaterTank, 500.0, 4, 4);
    ReliefCargo parsed;
    CHECK(static_cast<bool>(text >> parsed));
    CHECK(parsed.id() == "WTR-03" && parsed.quantity() == 4 && parsed.priority() == 4);

    std::stringstream bad("X-1 NotACategory 1 1 1");
    CHECK(!(bad >> parsed));
    CHECK(parsed.id() == "WTR-03");  // failed read leaves the object untouched
}

void testDepot() {
    DistributionDepot depot = Scenario::makeDepot();
    CHECK(depot.availableUnits("MED-01") == 300);  // 400 minus 100 reserved
    depot += ReliefCargo("MED-01", CargoCategory::TraumaMedicine, 2.5, 50, 5);
    CHECK(depot.stockOf("MED-01").quantity() == 450);
    depot -= depot.stockOf("MED-01").withQuantity(350);
    CHECK(depot.availableUnits("MED-01") == 0);
    CHECK_THROWS(depot -= depot.stockOf("MED-01").withQuantity(1), InsufficientStockException);
    CHECK_THROWS(depot.stockOf("NOPE"), InsufficientStockException);

    IncidentCommander commander("Tester");
    CHECK(commander.releaseReserve(depot, "MED-01") == 100);
    CHECK(depot.availableUnits("MED-01") == 100);
}

void testVehicleMemory() {
    const int before = TransportVehicle::operationalVehicles();
    {
        GroundConvoy a(kTruck, 0.0, 0.0, 1);
        for (int i = 0; i < 9; ++i) {  // forces several reallocations
            std::ostringstream id;
            id << "ITEM-" << i;
            a.loadCargo(ReliefCargo(id.str(), CargoCategory::RationKit, 1.0, 10, 3));
        }
        CHECK(a.manifestSize() == 9 && a.manifestSlots() >= 9);
        a.loadCargo(ReliefCargo("ITEM-0", CargoCategory::RationKit, 1.0, 5, 3));
        CHECK(a.manifestSize() == 9 && a.manifestAt(0).quantity() == 15);  // merged

        GroundConvoy b(a);  // deep copy
        CHECK(a == b);
        b.loadCargo(ReliefCargo("EXTRA", CargoCategory::RationKit, 1.0, 1, 3));
        CHECK(!(a == b) && a.manifestSize() == 9);

        GroundConvoy c(kTruck, 0.0, 0.0, 1);
        c = b;  // copy assignment
        GroundConvoy& alias = c;
        c = alias;  // self-assignment must be harmless
        CHECK(c == b);
        CHECK(TransportVehicle::operationalVehicles() == before + 3);

        CHECK_THROWS(a.loadCargo(ReliefCargo("HEAVY", CargoCategory::WaterTank, 500.0, 3, 4)),
                     CargoOverweightException);
        CHECK(a.manifestSize() == 9);  // failed load changed nothing

        IncidentCommander("Tester").authoriseOverload(a, 25.0);
        CHECK(near(a.maxPayloadKg(), 1250.0));
    }
    CHECK(TransportVehicle::operationalVehicles() == before);
}

void testRoutesAndPolymorphism() {
    ReliefHub hub(Scenario::makeDepot(), "test_log.bin");
    Scenario::populate(hub);
    const std::vector<ReliefVehicle*>& fleet = hub.fleet();
    const Corridor& blockedRoad = hub.corridorTo("Uttarkashi");
    const Corridor& river = hub.corridorTo("Tehri");

    CHECK(fleet[0]->kind() == "Ground" && fleet[4]->kind() == "Hybrid-Med");
    CHECK_THROWS(fleet[0]->evaluateRoute(blockedRoad), DamagedCorridorException);
    CHECK_THROWS(fleet[0]->evaluateRoute(river), DamagedCorridorException);
    CHECK_THROWS(fleet[3]->evaluateRoute(blockedRoad), DamagedCorridorException);
    CHECK(fleet[1]->evaluateRoute(blockedRoad).etaHours < 1.0);   // helicopter flies over
    CHECK(fleet[4]->evaluateRoute(blockedRoad).etaHours > 1.0);   // hybrid fords the damage
    CHECK(fleet[4]->evaluateRoute(river).etaHours > 0.0);         // and also sails

    // Diamond: both inheritance paths lead to one shared base object.
    AmphibiousMedicalUnit* hybrid = dynamic_cast<AmphibiousMedicalUnit*>(fleet[4]);
    CHECK(hybrid != nullptr);
    CHECK(static_cast<ReliefVehicle*>(static_cast<GroundConvoy*>(hybrid)) ==
          static_cast<ReliefVehicle*>(static_cast<AmphibiousCraft*>(hybrid)));

    // Fuel exhaustion leaves the vehicle exactly as it was.
    ReliefVehicle* quad = fleet[2];
    const Corridor far = {"Far lane", CorridorType::Air, 0.0, 0.0, 30.40, 79.33};
    CHECK_THROWS(quad->dispatch(far), FuelDepletionException);
    CHECK(near(quad->fuelLitres(), 40.0) && quad->beacon().pings() == 0);
}

void testAllocationAndServe() {
    const double tonnesBefore = TransportVehicle::dispatchedTonnes();
    ReliefHub hub(Scenario::makeDepot(), "test_log.bin");
    Scenario::populate(hub);
    std::ostringstream quiet;

    CHECK(allocateSupply(hub.depot(), *hub.fleet()[0], "RAT-02", 800.0) == 100);
    CHECK(hub.depot().stockOf("RAT-02").quantity() == 1900);
    hub.fleet()[0]->clearManifest();

    const TriageRequest tooMuch = {"Uttarkashi", "MED-01", 301, 5, 1, 1.0};
    CHECK_THROWS(hub.serve(tooMuch, quiet), InsufficientStockException);
    CHECK(hub.depot().stockOf("MED-01").quantity() == 400);  // nothing deducted

    const TriageRequest fine = {"Uttarkashi", "MED-01", 200, 5, 60, 9.0};
    const RouteReport report = hub.serve(fine, quiet);
    CHECK(near(report.payloadKg, 500.0));
    CHECK(hub.depot().stockOf("MED-01").quantity() == 200);
    CHECK(hub.log().size() == 1);
    CHECK(near(TransportVehicle::dispatchedTonnes() - tonnesBefore, 0.5));

    const TriageRequest nowhere = {"Atlantis", "MED-01", 1, 5, 1, 1.0};
    CHECK_THROWS(hub.serve(nowhere, quiet), DamagedCorridorException);
}

void testQueue() {
    PriorityDispatchQueue<TriageRequest, TriageUrgency> queue;
    const std::vector<TriageRequest> requests = Scenario::requests();
    for (std::size_t i = 0; i < requests.size(); ++i) queue.push(requests[i]);
    CHECK(queue.size() == 4 && queue.top().site == "Uttarkashi");
    CHECK(queue.ordered().back().site == "Rudraprayag" && queue.size() == 4);
    CHECK(queue.pop().site == "Uttarkashi");
    CHECK(queue.pop().site == "Chamoli");
    CHECK(queue.pop().site == "Tehri");
    CHECK(queue.pop().site == "Rudraprayag");
    CHECK(queue.empty());
    CHECK_THROWS(queue.pop(), std::out_of_range);

    PriorityDispatchQueue<std::string> words;  // default std::less
    words.push("boat"); words.push("truck"); words.push("heli");
    CHECK(words.pop() == "truck");
}

void testPersistence() {
    const char* path = "test_log.bin";
    std::remove(path);
    Persistence::TransitLog log(path);
    CHECK(!log.load());  // no file yet
    log.append(Persistence::makeRecord("HX-07", "Uttarkashi", "NH-34", 500.0, 59.8, 0.27,
                                       Persistence::TransitStatus::Delivered));
    log.append(Persistence::makeRecord("GC-01", "Rudraprayag", "NH-7", 960.0, 192.0, 4.85,
                                       Persistence::TransitStatus::Delivered));
    log.commit();
    CHECK(std::fopen("test_log.bin.tmp", "rb") == nullptr);  // temporary file was renamed away

    Persistence::TransitLog copy(path);
    CHECK(copy.load() && copy.size() == 2);
    CHECK(std::string(copy.records()[1].vehicleId) == "GC-01" && copy.records()[1].sequence == 2);
    copy.amendStatus(1, Persistence::TransitStatus::Confirmed);

    Persistence::TransitLog again(path);
    again.load();
    CHECK(again.records()[1].status == static_cast<int>(Persistence::TransitStatus::Confirmed));
    CHECK(again.records()[0].status == static_cast<int>(Persistence::TransitStatus::Delivered));
    CHECK(near(again.records()[1].payloadKg, 960.0));  // neighbours of the amended field intact

    // Manifest: binary and text round trips.
    GroundConvoy source(kTruck, 0.0, 0.0, 1), target(kTruck, 0.0, 0.0, 1), viaText(kTruck, 0.0, 0.0, 1);
    source.loadCargo(ReliefCargo("MED-01", CargoCategory::TraumaMedicine, 2.5, 40, 5));
    source.loadCargo(ReliefCargo("BLK-04", CargoCategory::ThermalBlanket, 1.2, 100, 2));
    Persistence::saveManifest(source, "test_manifest.bin");
    Persistence::restoreManifest(target, "test_manifest.bin");
    CHECK(source == target);
    std::stringstream text;
    Persistence::exportManifestText(source, text);
    CHECK(Persistence::importManifestText(viaText, text) == 2 && source == viaText);

    // Corrupt or mismatched files are rejected and leave the object unchanged.
    Persistence::TransitLog wrong("test_manifest.bin");
    CHECK_THROWS(wrong.load(), PersistenceException);
    CHECK_THROWS(Persistence::restoreManifest(target, path), PersistenceException);
    CHECK(source == target);
    CHECK_THROWS(Persistence::restoreManifest(target, "does_not_exist.bin"), PersistenceException);

    std::remove(path);
    std::remove("test_manifest.bin");
}

}  // namespace

int main() {
    const int startVehicles = TransportVehicle::operationalVehicles();
    testCargo();
    testDepot();
    testVehicleMemory();
    testRoutesAndPolymorphism();
    testAllocationAndServe();
    testQueue();
    testPersistence();
    CHECK(TransportVehicle::operationalVehicles() == startVehicles);  // every vehicle released

    std::cout << (checks - failures) << " of " << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
