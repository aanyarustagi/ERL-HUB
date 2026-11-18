#include "Scenario.h"

namespace ReliefOps {
namespace Scenario {

using namespace Inventory;
using namespace Logistics;

DistributionDepot makeDepot() {
    DistributionDepot depot("Dehradun Field Hub", kHubLatitude, kHubLongitude);
    depot += ReliefCargo("MED-01", CargoCategory::TraumaMedicine, 2.5, 400, 5);
    depot += ReliefCargo("RAT-02", CargoCategory::RationKit, 8.0, 2000, 3);
    depot += ReliefCargo("WTR-03", CargoCategory::WaterTank, 500.0, 20, 4);
    depot += ReliefCargo("BLK-04", CargoCategory::ThermalBlanket, 1.2, 3000, 2);
    depot.lockReserve("MED-01", 100);  // strategic reserve; only the commander can release it
    return depot;
}

void populate(ReliefHub& hub) {
    //                      id       call sign          limit kg  tank L  km/h   L/km
    const VehicleSpec convoy = {"GC-01", "Himalaya Convoy", 12000.0, 600.0, 45.0, 0.9};
    const VehicleSpec heli   = {"HX-07", "Medevac Garuda",   2500.0, 1800.0, 220.0, 6.0};
    const VehicleSpec quad   = {"QC-03", "Scout Quad",         25.0, 40.0, 60.0, 0.5};
    const VehicleSpec boat   = {"AB-02", "River Rescue",     1500.0, 300.0, 30.0, 1.2};
    const VehicleSpec hybrid = {"AM-05", "Amphi-Med",        3000.0, 400.0, 35.0, 1.1};

    hub.addVehicle(new GroundConvoy(convoy, kHubLatitude, kHubLongitude, 4));
    hub.addVehicle(new AerialTransport(heli, kHubLatitude, kHubLongitude, 4, 6000.0));
    hub.addVehicle(new AerialTransport(quad, kHubLatitude, kHubLongitude, 0, 0.0));
    hub.addVehicle(new AmphibiousCraft(boat, kHubLatitude, kHubLongitude, 0.9));
    hub.addVehicle(new AmphibiousMedicalUnit(hybrid, kHubLatitude, kHubLongitude, 2, 3000.0));

    //                           name              type                 km   damage  end lat   end lon
    const Corridor uttarkashi  = {"NH-34",          CorridorType::Road,  145.0, 0.75, 30.7268, 78.4354};
    const Corridor rudraprayag = {"NH-7",           CorridorType::Road,  160.0, 0.20, 30.2844, 78.9811};
    const Corridor tehri       = {"Bhagirathi",     CorridorType::River,  70.0, 0.30, 30.3780, 78.4800};
    const Corridor chamoli     = {"NH-58",          CorridorType::Road,  140.0, 0.80, 30.4000, 79.3300};
    hub.addCorridor("Uttarkashi", uttarkashi);
    hub.addCorridor("Rudraprayag", rudraprayag);
    hub.addCorridor("Tehri", tehri);
    hub.addCorridor("Chamoli", chamoli);
}

std::vector<TriageRequest> requests() {
    //                        site           cargo     units  severity  casualties  hours waiting
    const TriageRequest list[] = {
        {"Rudraprayag", "BLK-04", 800, 2, 10, 4.0},
        {"Chamoli",     "MED-01", 150, 4, 35, 12.0},
        {"Tehri",       "WTR-03",   2, 3, 20, 6.0},
        {"Uttarkashi",  "MED-01", 200, 5, 60, 9.0},
    };
    return std::vector<TriageRequest>(list, list + 4);
}

}  // namespace Scenario
}  // namespace ReliefOps
