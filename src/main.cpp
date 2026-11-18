// ERL-HUB - Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
// CIC-211 Object-Oriented Programming Using C++ - Project Assignment, Option 2
//
//   ./erl_hub          runs the guided demonstration (Units I-IV)
//   ./erl_hub --menu   opens the interactive coordinator console

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "Allocation.h"
#include "AuditLog.h"
#include "Exceptions.h"
#include "IncidentCommander.h"
#include "PriorityDispatchQueue.h"
#include "ReliefHub.h"
#include "Scenario.h"
#include "Utils.h"

using namespace ReliefOps;
using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;
using ReliefOps::Util::col;
using ReliefOps::Util::fixed2;
using ReliefOps::Util::num;
using ReliefOps::Util::tableRule;

namespace {

const char* const kLogFile = "transit_log.bin";
const char* const kManifestFile = "manifest_audit.bin";

void heading(const std::string& title) {
    std::cout << '\n' << std::string(72, '=') << '\n' << title << '\n'
              << std::string(72, '=') << '\n';
}

void step(const std::string& text) { std::cout << "\n> " << text << '\n'; }

// Functor used with std::for_each to total the weight of a cargo list.
class WeightAccumulator {
public:
    WeightAccumulator() : totalKg_(0.0) {}
    void operator()(const ReliefCargo& cargo) { totalKg_ += cargo.totalWeightKg(); }
    double totalKg() const { return totalKg_; }

private:
    double totalKg_;
};

// ------------------------------------------------------------------ Unit I
void demoUnitOne() {
    heading("UNIT I - Namespaces, inline functions, overloading, references, friend class");

    step("Inline utilities from ReliefOps::Util");
    const double km = Util::distanceKm(Scenario::kHubLatitude, Scenario::kHubLongitude,
                                       30.7268, 78.4354);
    std::cout << "  Dehradun -> Uttarkashi straight line: " << fixed2 << km << " km, "
              << Util::etaHours(km, 220.0) << " h by helicopter\n"
              << "  200 trauma kits x 2.5 kg = " << Util::payloadKg(2.5, 200) << " kg = "
              << Util::kgToTonnes(Util::payloadKg(2.5, 200)) << " t\n";

    DistributionDepot depot = Scenario::makeDepot();
    const VehicleSpec spec = {"GC-90", "Training Convoy", 5000.0, 400.0, 45.0, 0.9};
    GroundConvoy convoy(spec, Scenario::kHubLatitude, Scenario::kHubLongitude, 2);

    step("Overloaded allocateSupply(): volume-based payload vs priority medical triage");
    const int kits = allocateSupply(depot, convoy, "RAT-02", 2000.0);  // (depot, vehicle, id, kg)
    std::cout << "  volume overload : " << kits << " ration kits loaded into a 2000 kg allowance\n";
    const TriageRequest urgent = {"Tehri", "MED-01", 40, 5, 12, 3.0};
    const int medical = allocateSupply(depot, convoy, urgent);         // (depot, vehicle, request)
    std::cout << "  triage overload : " << medical << " trauma kits loaded for " << urgent.site << '\n';

    step("Return by reference: stockOf() hands back the ledger entry itself");
    ReliefCargo& blankets = depot.stockOf("BLK-04");
    std::cout << "  blankets before: " << blankets.quantity();
    blankets += 500;  // edits the depot's own record, no copy made
    std::cout << ", after 'depot.stockOf(\"BLK-04\") += 500': "
              << depot.stockOf("BLK-04").quantity() << '\n';

    step("Call by reference and chained return by reference on the GPS beacon");
    double latitude = 0.0, longitude = 0.0;
    convoy.beacon().moveTo(30.3780, 78.4800).ping().ping();  // each call returns the beacon
    convoy.beacon().readFix(latitude, longitude);            // fills the caller's variables
    std::cout << "  beacon fix (" << std::setprecision(4) << latitude << ", " << longitude
              << ") after " << convoy.beacon().pings() << " pings\n";

    step("Friend class IncidentCommander reads and overrides private state");
    IncidentCommander commander("Cmdt. R. Negi (NDRF)");
    commander.audit(convoy, std::cout << "  ");
    std::cout << "  ordinary MED-01 availability: " << depot.availableUnits("MED-01") << " units\n";
    const int released = commander.releaseReserve(depot, "MED-01");
    std::cout << "  commander released " << released << " reserve units -> "
              << depot.availableUnits("MED-01") << " units available\n";
    commander.authoriseOverload(convoy, 10.0);
    std::cout << "  overload clearance +10% -> payload limit " << fixed2 << convoy.maxPayloadKg()
              << " kg (" << commander.overridesIssued() << " overrides issued)\n";
}

// ----------------------------------------------------------------- Unit II
void demoUnitTwo() {
    heading("UNIT II - Encapsulation, static members, dynamic memory, operator overloading");

    step("Static members: one counter shared by every vehicle object");
    std::cout << "  operational vehicles before: " << TransportVehicle::operationalVehicles() << '\n';
    const VehicleSpec spec = {"GC-91", "Alpha Convoy", 6000.0, 500.0, 45.0, 0.9};
    GroundConvoy alpha(spec, Scenario::kHubLatitude, Scenario::kHubLongitude, 3);
    std::cout << "  after constructing GC-91   : " << TransportVehicle::operationalVehicles() << '\n';

    step("Dynamic constructor allocates the manifest; it grows on demand");
    std::cout << "  slots at construction: " << alpha.manifestSlots() << '\n';
    alpha.loadCargo(ReliefCargo("MED-01", CargoCategory::TraumaMedicine, 2.5, 100, 5));
    alpha.loadCargo(ReliefCargo("RAT-02", CargoCategory::RationKit, 8.0, 200, 3));
    alpha.loadCargo(ReliefCargo("BLK-04", CargoCategory::ThermalBlanket, 1.2, 300, 2));
    std::cout << "  slots after 3 cargo lines: " << alpha.manifestSlots() << " (array reallocated)\n";

    step("operator== detects duplicate cargo: the same item merges into one line");
    alpha.loadCargo(ReliefCargo("MED-01", CargoCategory::TraumaMedicine, 2.5, 50, 5));
    std::cout << "  lines: " << alpha.manifestSize() << ", MED-01 units now "
              << alpha.manifestAt(0).quantity() << '\n';

    step("Deep copy constructor: the copy owns a separate manifest array");
    GroundConvoy bravo(alpha);
    std::cout << "  vehicles after copy: " << TransportVehicle::operationalVehicles()
              << ", manifests identical (operator==): " << std::boolalpha << (alpha == bravo) << '\n';
    bravo.loadCargo(ReliefCargo("WTR-03", CargoCategory::WaterTank, 500.0, 2, 4));
    std::cout << "  after loading water on the copy -> original lines: " << alpha.manifestSize()
              << ", copy lines: " << bravo.manifestSize() << ", identical: " << (alpha == bravo) << '\n';

    step("operator+= restocks the depot, operator-= deducts on departure");
    DistributionDepot depot = Scenario::makeDepot();
    std::cout << "  RAT-02 at start: " << depot.stockOf("RAT-02").quantity();
    depot += ReliefCargo("RAT-02", CargoCategory::RationKit, 8.0, 500, 3);
    std::cout << "  | after += 500: " << depot.stockOf("RAT-02").quantity();
    depot -= ReliefCargo("RAT-02", CargoCategory::RationKit, 8.0, 200, 3);
    std::cout << "  | after -= 200: " << depot.stockOf("RAT-02").quantity() << '\n';

    step("operator<< and operator>> round-trip a cargo line through a text stream");
    std::stringstream wire;
    wire << alpha.manifestAt(1);
    ReliefCargo decoded;
    wire >> decoded;
    std::cout << "  written : " << alpha.manifestAt(1) << "\n  read back: " << decoded
              << "\n  same item: " << (decoded == alpha.manifestAt(1)) << std::noboolalpha << '\n';

    std::cout << '\n';
    alpha.printManifest(std::cout);
    std::cout << "\n  (alpha and bravo go out of scope here; their destructors free both arrays)\n";
}

// ---------------------------------------------------------------- Unit III
void demoUnitThree() {
    heading("UNIT III - Inheritance, virtual base classes, containership, polymorphism");

    ReliefHub hub(Scenario::makeDepot(), kLogFile);
    Scenario::populate(hub);

    step("One loop, one call site: evaluateRoute() resolved at run time through ReliefVehicle*");
    const std::vector<std::string> sites = hub.sites();
    std::cout << col(7) << "ID" << col(12) << "Type";
    for (std::size_t s = 0; s < sites.size(); ++s) std::cout << num(13) << sites[s];
    std::cout << "\n" << tableRule;
    const std::vector<ReliefVehicle*>& fleet = hub.fleet();
    for (std::size_t i = 0; i < fleet.size(); ++i) {
        const ReliefVehicle* vehicle = fleet[i];  // base pointer
        std::cout << col(7) << vehicle->id() << col(12) << vehicle->kind();
        for (std::size_t s = 0; s < sites.size(); ++s) {
            try {
                const RouteReport r = vehicle->evaluateRoute(hub.corridorTo(sites[s]));
                std::ostringstream cell;
                cell << fixed2 << r.etaHours << " h";
                std::cout << num(13) << cell.str();
            } catch (const DamagedCorridorException&) {
                std::cout << num(13) << "blocked";
            }
        }
        std::cout << '\n';
    }
    std::cout << tableRule << "  (ETA in hours for an empty vehicle; 'blocked' = DamagedCorridorException)\n";

    step("Diamond resolved: the hybrid holds exactly one ReliefVehicle sub-object");
    AmphibiousMedicalUnit* hybrid = dynamic_cast<AmphibiousMedicalUnit*>(fleet.back());
    GroundConvoy* asGround = hybrid;       // path 1 up the diamond
    AmphibiousCraft* asCraft = hybrid;     // path 2 up the diamond
    const ReliefVehicle* viaGround = asGround;
    const ReliefVehicle* viaCraft = asCraft;
    std::cout << "  id via GroundConvoy path   : " << asGround->id() << '\n'
              << "  id via AmphibiousCraft path: " << asCraft->id() << '\n'
              << "  both paths reach the same base object: " << std::boolalpha
              << (viaGround == viaCraft) << std::noboolalpha << '\n';

    step("Containership: vehicles HAVE a GPSBeacon and an EmergencyLifeSupportUnit");
    hybrid->lifeSupport().admitPatient();
    hybrid->lifeSupport().admitPatient();
    std::cout << "  " << hybrid->id() << " beacon at (" << std::setprecision(4)
              << hybrid->beacon().latitude() << ", " << hybrid->beacon().longitude() << "), "
              << hybrid->lifeSupport().patientsAboard() << " patients aboard, "
              << hybrid->lifeSupport().freeStretchers() << " stretchers free, oxygen for "
              << fixed2 << hybrid->lifeSupport().oxygenHoursLeft() << " h\n";

    step("Fleet deallocation through base pointers: virtual destructors run the full chain");
    TransportVehicle::setLifecycleTrace(true);
    hub.releaseFleet();
    TransportVehicle::setLifecycleTrace(false);
    std::cout << "  operational vehicles now: " << TransportVehicle::operationalVehicles() << '\n';
}

// ----------------------------------------------------------------- Unit IV
void demoUnitFour() {
    heading("UNIT IV - Exceptions, binary persistence, templates, STL, manipulators");

    std::remove(kLogFile);  // start the demonstration from a clean log
    ReliefHub hub(Scenario::makeDepot(), kLogFile);
    Scenario::populate(hub);
    IncidentCommander commander("Cmdt. R. Negi (NDRF)");

    step("Depot inventory printed with user-defined manipulators (tableRule, col, num, fixed2)");
    hub.depot().printInventory(std::cout);

    step("Template PriorityDispatchQueue<TriageRequest, TriageUrgency> orders the requests");
    PriorityDispatchQueue<TriageRequest, TriageUrgency> queue;
    const std::vector<TriageRequest> incoming = Scenario::requests();
    for (std::size_t i = 0; i < incoming.size(); ++i) queue.push(incoming[i]);
    const std::vector<TriageRequest> order = queue.ordered();
    for (std::size_t i = 0; i < order.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << col(13) << order[i].site << col(8) << order[i].cargoId
                  << "x" << col(5) << order[i].units << "severity " << order[i].severity
                  << "  urgency score " << fixed2 << urgencyScore(order[i]) << '\n';

    step("The same template with a different type: PriorityDispatchQueue<int>");
    PriorityDispatchQueue<int> numbers;
    numbers.push(3); numbers.push(9); numbers.push(5);
    std::cout << "  pushed 3, 9, 5 -> popped " << numbers.pop() << ", then " << numbers.pop() << '\n';

    step("Dispatch cycle with exception handling and commander override");
    while (!queue.empty()) {
        const TriageRequest request = queue.pop();
        std::cout << "  Request: " << request.site << " needs " << request.units << " x "
                  << request.cargoId << '\n';
        try {
            hub.serve(request, std::cout);
        } catch (const InsufficientStockException& error) {
            std::cout << "    CAUGHT " << error.what() << '\n';
            const int released = commander.releaseReserve(hub.depot(), request.cargoId);
            std::cout << "    " << commander.name() << " releases " << released
                      << " reserve units; retrying\n";
            try {
                hub.serve(request, std::cout);
            } catch (const ReliefException& second) {
                std::cout << "    UNSERVED " << second.what() << '\n';
            }
        } catch (const ReliefException& error) {
            std::cout << "    UNSERVED " << error.what() << '\n';
        }
    }

    step("Remaining exception types raised deliberately");
    ReliefVehicle* quad = hub.fleet()[2];
    try {
        quad->loadCargo(hub.depot().stockOf("WTR-03").withQuantity(1));
    } catch (const CargoOverweightException& error) {
        std::cout << "  CAUGHT " << error.what() << " (excess " << fixed2 << error.excessKg() << " kg)\n";
    }
    try {
        const TriageRequest scoutDrop = {"Chamoli", "MED-01", 8, 4, 3, 1.0};
        allocateSupply(hub.depot(), *quad, scoutDrop);
        const Corridor farAirLane = {"Air lane C", CorridorType::Air, 0.0, 0.0, 30.4000, 79.3300};
        quad->dispatch(farAirLane);
    } catch (const FuelDepletionException& error) {
        std::cout << "  CAUGHT " << error.what() << " (short by " << fixed2
                  << error.shortfallLitres() << " L)\n";
        hub.log().append(Persistence::makeRecord(quad->id(), "Chamoli", "Air lane C",
                                                 quad->currentLoadKg(), 0.0, 0.0,
                                                 Persistence::TransitStatus::Aborted));
    }
    try {
        hub.corridorTo("Joshimath");
    } catch (const std::exception& error) {  // caught through the std::exception base
        std::cout << "  CAUGHT (as std::exception) " << error.what() << '\n';
    }

    step("Binary transit log: atomic commit, reload from disk, in-place amend with seekp/seekg");
    hub.log().commit();
    std::cout << "  committed " << hub.log().size() << " records to " << kLogFile
              << " (written to .tmp, then renamed)\n";
    Persistence::TransitLog reloaded(kLogFile);
    reloaded.load();
    reloaded.amendStatus(0, Persistence::TransitStatus::Confirmed);
    Persistence::TransitLog verify(kLogFile);
    verify.load();
    std::cout << "  record 1 amended on disk to 'Confirmed'; log as read back from the file:\n";
    verify.print(std::cout);

    step("Manifest audit record: binary save and restore");
    Persistence::saveManifest(*quad, kManifestFile);
    const VehicleSpec spareSpec = {"QC-04", "Spare Quad", 25.0, 40.0, 60.0, 0.5};
    AerialTransport spare(spareSpec, Scenario::kHubLatitude, Scenario::kHubLongitude, 0, 0.0);
    Persistence::restoreManifest(spare, kManifestFile);
    std::cout << "  " << quad->id() << " manifest saved; restored into " << spare.id()
              << "; identical (operator==): " << std::boolalpha << (*quad == spare)
              << std::noboolalpha << '\n';
    try {
        Persistence::TransitLog wrong(kManifestFile);  // a manifest file is not a transit log
        wrong.load();
    } catch (const PersistenceException& error) {
        std::cout << "  CAUGHT " << error.what() << '\n';
    }

    step("STL algorithms: std::sort with a lambda, std::for_each with a functor");
    std::vector<ReliefVehicle*> byCapacity(hub.fleet());
    std::sort(byCapacity.begin(), byCapacity.end(),
              [](const ReliefVehicle* a, const ReliefVehicle* b) {
                  return a->maxPayloadKg() > b->maxPayloadKg();
              });
    std::cout << "  fleet by payload limit:";
    for (std::size_t i = 0; i < byCapacity.size(); ++i) std::cout << ' ' << byCapacity[i]->id();
    const std::vector<ReliefCargo> stock = hub.depot().sortedStock();
    const WeightAccumulator total = std::for_each(stock.begin(), stock.end(), WeightAccumulator());
    std::cout << "\n  stock left in depot: " << fixed2 << Util::kgToTonnes(total.totalKg()) << " t\n";

    step("End-of-cycle status");
    hub.printFleet(std::cout);
    std::cout << "  Total dispatched (static metric): " << fixed2
              << TransportVehicle::dispatchedTonnes() << " t   Vehicle objects alive: "
              << TransportVehicle::operationalVehicles() << " (" << hub.fleet().size()
              << " fleet + spare " << spare.id() << ")\n";
}

// ------------------------------------------------------- Interactive console
int readInt(const std::string& prompt) {
    int value = 0;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        if (std::cin.eof()) return 0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  please enter a number: ";
    }
    return value;
}

void runMenu() {
    ReliefHub hub(Scenario::makeDepot(), kLogFile);
    Scenario::populate(hub);
    IncidentCommander commander("Duty Commander");
    if (hub.log().load())
        std::cout << "Loaded " << hub.log().size() << " earlier transit records from " << kLogFile << '\n';

    for (;;) {
        std::cout << "\n=== ERL-HUB Coordinator Console ===\n"
                  << " 1. Depot inventory        4. Dispatch a relief request\n"
                  << " 2. Fleet status           5. Transit log\n"
                  << " 3. Restock depot          6. Release reserve (commander)\n"
                  << " 0. Save log and exit\n";
        const int choice = readInt("Choice: ");
        try {
            if (choice == 0 || std::cin.eof()) {
                hub.log().commit();
                std::cout << "Transit log saved to " << kLogFile << ". Goodbye.\n";
                return;
            } else if (choice == 1) {
                hub.depot().printInventory(std::cout);
            } else if (choice == 2) {
                hub.printFleet(std::cout);
            } else if (choice == 3) {
                std::cout << "Enter: ID Category unitKg units priority\n"
                          << "  e.g. RAT-02 RationKit 8 500 3\n> ";
                ReliefCargo delivery;
                if (std::cin >> delivery) {
                    hub.depot() += delivery;
                    std::cout << "Restocked. " << delivery.id() << " now "
                              << hub.depot().stockOf(delivery.id()).quantity() << " units.\n";
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Could not read that cargo line.\n";
                }
            } else if (choice == 4) {
                TriageRequest request;
                std::cout << "Sites:";
                const std::vector<std::string> sites = hub.sites();
                for (std::size_t i = 0; i < sites.size(); ++i) std::cout << ' ' << sites[i];
                std::cout << "\nSite: ";
                std::cin >> request.site;
                std::cout << "Cargo ID: ";
                std::cin >> request.cargoId;
                request.units = readInt("Units: ");
                request.severity = readInt("Severity (1-5): ");
                request.casualties = 0;
                request.hoursWaiting = 0.0;
                hub.serve(request, std::cout);
            } else if (choice == 5) {
                hub.log().print(std::cout);
            } else if (choice == 6) {
                std::string cargoId;
                std::cout << "Cargo ID: ";
                std::cin >> cargoId;
                std::cout << "Released " << commander.releaseReserve(hub.depot(), cargoId)
                          << " reserve units.\n";
            } else {
                std::cout << "Unknown option.\n";
            }
        } catch (const ReliefException& error) {
            std::cout << "Request refused: " << error.what() << '\n';
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << '\n';
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--menu") {
            runMenu();
            return 0;
        }
        std::cout << "ERL-HUB | Post-Earthquake Multi-Agency Emergency Relief Logistics Hub\n"
                  << "CIC-211 OOP Using C++ | Guided demonstration (run with --menu for the console)\n";
        demoUnitOne();
        demoUnitTwo();
        demoUnitThree();
        demoUnitFour();
        std::cout << "\nDemonstration complete. Vehicles still allocated: "
                  << TransportVehicle::operationalVehicles() << " (0 = no leaks)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal: " << error.what() << '\n';
        return 1;
    }
}
