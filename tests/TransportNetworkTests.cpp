#include <network-monitor/TransportNetwork.h>
#include <network-monitor/FileDownloader.h>

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using NetworkMonitor::Id;
using NetworkMonitor::Line;
using NetworkMonitor::PassengerEvent;
using NetworkMonitor::ParseJsonFile;
using NetworkMonitor::Route;
using NetworkMonitor::Station;
using NetworkMonitor::TravelRoute;
using NetworkMonitor::TransportNetwork;

// Use this to set a timeout on tests that may hang.
using timeout = boost::unit_test::timeout;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_TransportNetwork);

// BOOST_AUTO_TEST_SUITE(AddStation);

// BOOST_AUTO_TEST_CASE(basic)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Add a station.
//     Station station {
//         "station_000",
//         "Station Name",
//     };
//     ok = nw.AddStation(station);
//     BOOST_CHECK(ok);
// }

// BOOST_AUTO_TEST_CASE(duplicate_id)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Can't add the same station twice.
//     Station station {
//         "station_000",
//         "Station Name",
//     };
//     ok = nw.AddStation(station);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddStation(station);
//     BOOST_CHECK(!ok);
// }

// BOOST_AUTO_TEST_CASE(duplicate_name)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // It's ok to add a station with the same name, but different ID.
//     Station station0 {
//         "station_000",
//         "Same Name",
//     };
//     ok = nw.AddStation(station0);
//     BOOST_REQUIRE(ok);
//     Station station1 {
//         "station_001",
//         "Same Name",
//     };
//     ok = nw.AddStation(station1);
//     BOOST_CHECK(ok);
// }

// BOOST_AUTO_TEST_SUITE_END(); // AddStation

// BOOST_AUTO_TEST_SUITE(AddLine);

// BOOST_AUTO_TEST_CASE(basic)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Add a line with 1 route.
//     // route0: 0 ---> 1

//     // First, add the stations.
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     BOOST_REQUIRE(ok);

//     // Then, add the line, with the two routes.
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_001",
//         {"station_000", "station_001"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0},
//     };
//     ok = nw.AddLine(line);
//     BOOST_CHECK(ok);
// }

// BOOST_AUTO_TEST_CASE(shared_stations)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Define a line with 2 routes going through some shared stations.
//     // route0: 0 ---> 1 ---> 2
//     // route1: 3 ---> 1 ---> 2
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     Station station2 {
//         "station_002",
//         "Station Name 2",
//     };
//     Station station3 {
//         "station_003",
//         "Station Name 3",
//     };
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_002",
//         {"station_000", "station_001", "station_002"},
//     };
//     Route route1 {
//         "route_001",
//         "inbound",
//         "line_000",
//         "station_003",
//         "station_002",
//         {"station_003", "station_001", "station_002"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0, route1},
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     ok &= nw.AddStation(station2);
//     ok &= nw.AddStation(station3);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_CHECK(ok);
// }

// BOOST_AUTO_TEST_CASE(duplicate)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Can't add the same line twice.
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_001",
//         {"station_000", "station_001"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0},
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_CHECK(!ok);
// }

// BOOST_AUTO_TEST_SUITE_END(); // AddLine

// BOOST_AUTO_TEST_SUITE(PassengerEvents);

// BOOST_AUTO_TEST_CASE(basic)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Add a line with 1 route.
//     // route0: 0 ---> 1 ---> 2
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     Station station2 {
//         "station_002",
//         "Station Name 2",
//     };
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_002",
//         {"station_000", "station_001", "station_002"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0},
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     ok &= nw.AddStation(station2);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_REQUIRE(ok);

//     // Check that the network starts empty.
//     BOOST_REQUIRE_EQUAL(nw.GetPassengerCount(station0.id), 0);
//     BOOST_REQUIRE_EQUAL(nw.GetPassengerCount(station1.id), 0);
//     BOOST_REQUIRE_EQUAL(nw.GetPassengerCount(station2.id), 0);
//     try {
//         auto count {nw.GetPassengerCount("station_42")}; // Not in the network
//         BOOST_REQUIRE(false);
//     } catch (const std::runtime_error& e) {
//         BOOST_REQUIRE(true);
//     }

//     // Record events and check the count.
//     using EventType = PassengerEvent::Type;
//     ok = nw.RecordPassengerEvent({station0.id, EventType::In});
//     BOOST_REQUIRE(ok);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station0.id), 1);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station1.id), 0);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station2.id), 0);
//     ok = nw.RecordPassengerEvent({station0.id, EventType::In});
//     BOOST_REQUIRE(ok);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station0.id), 2);
//     ok = nw.RecordPassengerEvent({station1.id, EventType::In});
//     BOOST_REQUIRE(ok);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station0.id), 2);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station1.id), 1);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station2.id), 0);
//     ok = nw.RecordPassengerEvent({station0.id, EventType::Out});
//     BOOST_REQUIRE(ok);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station0.id), 1);
//     ok = nw.RecordPassengerEvent({station2.id, EventType::Out}); // Negative
//     BOOST_REQUIRE(ok);
//     BOOST_CHECK_EQUAL(nw.GetPassengerCount(station2.id), -1);
// }

// BOOST_AUTO_TEST_SUITE_END(); // PassengerEvents

// BOOST_AUTO_TEST_SUITE(GetRoutesServingStation);

// BOOST_AUTO_TEST_CASE(basic)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Add a line with 1 route.
//     // route0: 0 ---> 1 ---> 2
//     // Plus a station served by no routes: 3.
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     Station station2 {
//         "station_002",
//         "Station Name 2",
//     };
//     Station station3 {
//         "station_003",
//         "Station Name 3",
//     };
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_002",
//         {"station_000", "station_001", "station_002"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0},
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     ok &= nw.AddStation(station2);
//     ok &= nw.AddStation(station3);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_REQUIRE(ok);

//     // Check the routes served.
//     std::vector<Id> routes {};
//     routes = nw.GetRoutesServingStation(station0.id);
//     BOOST_REQUIRE_EQUAL(routes.size(), 1);
//     BOOST_CHECK(routes[0] == route0.id);
//     routes = nw.GetRoutesServingStation(station1.id);
//     BOOST_REQUIRE_EQUAL(routes.size(), 1);
//     BOOST_CHECK(routes[0] == route0.id);
//     routes = nw.GetRoutesServingStation(station2.id);
//     BOOST_REQUIRE_EQUAL(routes.size(), 1);
//     BOOST_CHECK(routes[0] == route0.id);
//     routes = nw.GetRoutesServingStation(station3.id);
//     BOOST_CHECK_EQUAL(routes.size(), 0);
// }

// BOOST_AUTO_TEST_SUITE_END(); // GetRoutesServingStation

// BOOST_AUTO_TEST_SUITE(TravelTime);

// BOOST_AUTO_TEST_CASE(basic)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Add a line with 1 route.
//     // route0: 0 ---> 1 ---> 2
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     Station station2 {
//         "station_002",
//         "Station Name 2",
//     };
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_002",
//         {"station_000", "station_001", "station_002"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0},
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     ok &= nw.AddStation(station2);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_REQUIRE(ok);

//     unsigned int travelTime {0};

//     // Get travel time before setting it.
//     travelTime = nw.GetTravelTime(station0.id, station1.id);
//     BOOST_CHECK_EQUAL(travelTime, 0);

//     // Cannot set the travel time between non-adjacent stations.
//     ok = nw.SetTravelTime(station0.id, station2.id, 1);
//     BOOST_CHECK(!ok);

//     // Set the travel time between adjacent stations.
//     ok = nw.SetTravelTime(station0.id, station1.id, 2);
//     BOOST_CHECK(ok);
//     BOOST_CHECK_EQUAL(nw.GetTravelTime(station0.id, station1.id), 2);

//     // Set travel time between adjacend stations, even if they appear in the
//     // reverse order in the route.
//     ok = nw.SetTravelTime(station1.id, station0.id, 3);
//     BOOST_CHECK(ok);
//     BOOST_CHECK_EQUAL(nw.GetTravelTime(station1.id, station0.id), 3);
// }

// BOOST_AUTO_TEST_CASE(over_route)
// {
//     TransportNetwork nw {};
//     bool ok {false};

//     // Add a line with 3 routes.
//     // route0: 0 ---> 1 ---> 2 ---> 3
//     // route1: 3 ---> 1 ---> 2
//     // route2: 3 ---> 1 ---> 0
//     Station station0 {
//         "station_000",
//         "Station Name 0",
//     };
//     Station station1 {
//         "station_001",
//         "Station Name 1",
//     };
//     Station station2 {
//         "station_002",
//         "Station Name 2",
//     };
//     Station station3 {
//         "station_003",
//         "Station Name 3",
//     };
//     Route route0 {
//         "route_000",
//         "inbound",
//         "line_000",
//         "station_000",
//         "station_003",
//         {"station_000", "station_001", "station_002", "station_003"},
//     };
//     Route route1 {
//         "route_001",
//         "inbound",
//         "line_000",
//         "station_003",
//         "station_002",
//         {"station_003", "station_001", "station_002"},
//     };
//     Route route2 {
//         "route_002",
//         "inbound",
//         "line_000",
//         "station_003",
//         "station_000",
//         {"station_003", "station_001", "station_000"},
//     };
//     Line line {
//         "line_000",
//         "Line Name",
//         {route0, route1, route2},
//     };
//     ok = true;
//     ok &= nw.AddStation(station0);
//     ok &= nw.AddStation(station1);
//     ok &= nw.AddStation(station2);
//     ok &= nw.AddStation(station3);
//     BOOST_REQUIRE(ok);
//     ok = nw.AddLine(line);
//     BOOST_REQUIRE(ok);

//     // Set all travel times.
//     ok = true;
//     ok &= nw.SetTravelTime(station0.id, station1.id, 1);
//     ok &= nw.SetTravelTime(station1.id, station2.id, 2);
//     ok &= nw.SetTravelTime(station2.id, station3.id, 3);
//     ok &= nw.SetTravelTime(station3.id, station1.id, 4);
//     BOOST_REQUIRE(ok);

//     // Check the cumulative travel times.
//     unsigned int travelTime {0};
//     // route0
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station0.id, station1.id), 1
//     );
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station0.id, station2.id), 1 + 2
//     );
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station0.id, station3.id), 1 + 2 + 3
//     );
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station1.id, station3.id), 2 + 3
//     );
//     // route1
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route1.id, station3.id, station1.id), 4
//     );
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route1.id, station3.id, station2.id), 4 + 2
//     );
//     // route2
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route2.id, station3.id, station1.id), 4
//     );
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route2.id, station3.id, station0.id), 4 + 1
//     );
//     // Invalid routes
//     // -- 3 -> 1 is possible, but only over route1 and route2.
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station3.id, station1.id), 0
//     );
//     // -- 1 -> 0 is possible, but only over route3.
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station1.id, station0.id), 0
//     );
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime(line.id, route0.id, station1.id, station1.id), 0
//     );
// }

// BOOST_AUTO_TEST_SUITE_END(); // TravelTime

// BOOST_AUTO_TEST_SUITE(FromJson);

// std::vector<Id> GetSortedIds(std::vector<Id>& routes)
// {
//     std::vector<Id> ids {routes};
//     std::sort(ids.begin(), ids.end());
//     return ids;
// }

// BOOST_AUTO_TEST_CASE(ParseStations)
// {
//     auto testFilePath {
//         std::filesystem::path(TEST_DATA) / "from_json_1line_1route.json"
//     };
//     auto src = NetworkMonitor::ParseJsonFile(testFilePath);

//     TransportNetwork nw {};
//     auto ok {nw.FromJson(std::move(src))};
//     BOOST_REQUIRE(ok);

//     auto routes {nw.GetRoutesServingStation("station_0")};
//     BOOST_REQUIRE_EQUAL(routes.size(), 1);
//     BOOST_CHECK_EQUAL(routes[0], "route_0");
// }

// BOOST_AUTO_TEST_CASE(from_json_1line_2routes)
// {
//     auto testFilePath {
//         std::filesystem::path(TEST_DATA) / "from_json_1line_2routes.json"
//     };
//     auto src = NetworkMonitor::ParseJsonFile(testFilePath);

//     TransportNetwork nw {};
//     auto ok {nw.FromJson(std::move(src))};
//     BOOST_REQUIRE(ok);

//     std::vector<Id> routes {};
//     routes = nw.GetRoutesServingStation("station_0");
//     BOOST_REQUIRE_EQUAL(routes.size(), 1);
//     BOOST_CHECK_EQUAL(routes[0], "route_0");
//     routes = nw.GetRoutesServingStation("station_1");
//     BOOST_REQUIRE_EQUAL(routes.size(), 2);
//     BOOST_CHECK(
//         GetSortedIds(routes) == std::vector<Id>({"route_0", "route_1"})
//     );
// }

// BOOST_AUTO_TEST_CASE(from_json_2line_2routes)
// {
//     auto testFilePath {
//         std::filesystem::path(TEST_DATA) / "from_json_2lines_2routes.json"
//     };
//     auto src = NetworkMonitor::ParseJsonFile(testFilePath);

//     TransportNetwork nw {};
//     auto ok {nw.FromJson(std::move(src))};
//     BOOST_REQUIRE(ok);

//     std::vector<Id> routes {};
//     routes = nw.GetRoutesServingStation("station_0");
//     BOOST_REQUIRE_EQUAL(routes.size(), 2);
//     BOOST_CHECK(
//         GetSortedIds(routes) == std::vector<Id>({"route_0", "route_1"})
//     );
//     routes = nw.GetRoutesServingStation("station_1");
//     BOOST_REQUIRE_EQUAL(routes.size(), 2);
//     BOOST_CHECK(
//         GetSortedIds(routes) == std::vector<Id>({"route_0", "route_1"})
//     );
// }

// BOOST_AUTO_TEST_CASE(from_json_bad_travel_times)
// {
//     auto testFilePath {
//         std::filesystem::path(TEST_DATA) / "from_json_bad_travel_times.json"
//     };
//     auto src = NetworkMonitor::ParseJsonFile(testFilePath);

//     TransportNetwork nw {};
//     auto ok {nw.FromJson(std::move(src))};
//     BOOST_CHECK(!ok);
// }

// BOOST_AUTO_TEST_CASE(from_json_travel_times)
// {
//     auto testFilePath {
//         std::filesystem::path(TEST_DATA) / "from_json_travel_times.json"
//     };
//     auto src = NetworkMonitor::ParseJsonFile(testFilePath);

//     TransportNetwork nw {};
//     auto ok {nw.FromJson(std::move(src))};
//     BOOST_REQUIRE(ok);

//     BOOST_CHECK_EQUAL(nw.GetTravelTime("station_0", "station_1"), 1);
//     BOOST_CHECK_EQUAL(nw.GetTravelTime("station_1", "station_0"), 1);
//     BOOST_CHECK_EQUAL(nw.GetTravelTime("station_1", "station_2"), 2);
//     BOOST_CHECK_EQUAL(
//         nw.GetTravelTime("line_0", "route_0", "station_0", "station_2"), 1 + 2
//     );
// }

// BOOST_AUTO_TEST_CASE(fail_on_bad_json)
// {
//     nlohmann::json src {
//         // Missing "stations"!
//         {"lines", {}},
//         {"travel_times", {}},
//     };
//     TransportNetwork nw {};
//     BOOST_CHECK_THROW(nw.FromJson(std::move(src)), nlohmann::json::exception);
// }

// BOOST_AUTO_TEST_CASE(fail_on_good_json_bad_items)
// {
//     nlohmann::json src {
//         {"stations", {
//             {
//                 {"station_id", "station_0"},
//                 {"name", "Station 0 Name"},
//             },
//             {
//                 {"station_id", "station_0"},
//                 {"name", "Station 0 Name"}, // station_0 is a duplicate!
//             },
//         }},
//         {"lines", {}},
//         {"travel_times", {}},
//     };
//     TransportNetwork nw {};
//     BOOST_CHECK_THROW(nw.FromJson(std::move(src)), std::runtime_error);
// }


// BOOST_AUTO_TEST_SUITE_END(); // FromJson

BOOST_AUTO_TEST_SUITE(Routes);

static std::pair<TransportNetwork, TravelRoute> GetTestNetwork(
    const std::string& filenameNoExtension,
    bool useOriginalNetworkLayoutFile = false,
    bool readPassengerCounts = false,
    const std::string& resultFilenamePost = ""
)
{
    auto testFilePath {useOriginalNetworkLayoutFile ?
        std::filesystem::path(TESTS_NETWORK_LAYOUT_JSON) :
        std::filesystem::path(TEST_DATA) / (filenameNoExtension + ".json")
    };
    auto src = ParseJsonFile(testFilePath);
    BOOST_REQUIRE(src != nlohmann::json::object());
    TransportNetwork nw {};
    auto ok {nw.FromJson(std::move(src))};
    BOOST_REQUIRE(ok);

    if (readPassengerCounts) {
        auto passengerCountsFilePath {
            std::filesystem::path(TEST_DATA) /
            (filenameNoExtension + ".counts.json")
        };
        auto passengerCountsJson = ParseJsonFile(passengerCountsFilePath);
        std::unordered_map<Id, int> passengerCounts {};
        try {
            passengerCounts = passengerCountsJson.get<
                std::unordered_map<Id, int>
            >();
        } catch (...) {
            BOOST_FAIL(std::string("Failed to parse passenger counts file: ") +
                       passengerCountsFilePath.string());
        }
        for (const auto& [stationId, passengerCount]: passengerCounts) {
            auto type {passengerCount > 0 ? PassengerEvent::Type::In :
                                            PassengerEvent::Type::Out};
            for (size_t _ {0}; _ < std::abs(passengerCount); ++_) {
                nw.RecordPassengerEvent({stationId, type, {}});
            }
        }
    }

    auto resultFilePath {
        std::filesystem::path(TEST_DATA) /
        (filenameNoExtension + ".result." +
         (resultFilenamePost == "" ? "json" : (resultFilenamePost + ".json")))
    };
    auto travelRouteJson = ParseJsonFile(resultFilePath);
    TravelRoute travelRoute;
    try {
        travelRoute = travelRouteJson.get<TravelRoute>();
    } catch (...) {
        BOOST_FAIL(std::string("Failed to parse result JSON file: ") +
                   resultFilePath.string());
    }
    return std::make_pair<TransportNetwork, TravelRoute>(
        std::move(nw),
        std::move(travelRoute)
    );
}

// BOOST_AUTO_TEST_SUITE(GetFastestTravelRoute);

// BOOST_AUTO_TEST_CASE(same_station)
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork(
//         "network_fastest_path_same_station"
//     );
//     auto travelRoute {nw.GetFastestTravelRoute("station_A", "station_A")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(missing_station)
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork(
//         "network_fastest_path_missing_station"
//     );
//     auto travelRoute {nw.GetFastestTravelRoute("station_A", "station_X")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(no_path)
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork(
//         "network_fastest_path_no_path"
//     );
//     auto travelRoute {nw.GetFastestTravelRoute("station_A", "station_B")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(one_route, *timeout {1})
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork(
//         "network_fastest_path_1route"
//     );
//     auto travelRoute {nw.GetFastestTravelRoute("station_A", "station_B")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(two_routes, *timeout {1})
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork(
//         "network_fastest_path_2routes"
//     );
//     auto travelRoute {nw.GetFastestTravelRoute("station_A", "station_B")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(two_routes_overlap, *timeout {1})
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork(
//         "network_fastest_path_2routes_overlap"
//     );
//     auto travelRoute {nw.GetFastestTravelRoute("station_A", "station_B")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(ltc_path1, *timeout {1})
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork("ltc_path1", true);
//     auto travelRoute {nw.GetFastestTravelRoute("station_003", "station_019")};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_CASE(ltc_path2, *timeout {1})
// {
//     auto [nw, resultTravelRoute] = GetTestNetwork("ltc_path2", true);
//     auto travelRoute {nw.GetFastestTravelRoute("station_211", "station_119")};
//     for(auto id : nw.GetRoutesServingStation("station_021")){
//         std::cout<<"Route: "<<id<<std::endl;
//     }
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

// BOOST_AUTO_TEST_SUITE_END(); // GetFastestTravelRoute

BOOST_AUTO_TEST_SUITE(GetQuietTravelRoute);

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(same_station)
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    auto [nw, resultTravelRoute] = GetTestNetwork(
        "network_fastest_path_same_station"
    );
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_A",
        "station_A",
        maxSlowdownPc,
        minQuietnessPc
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(missing_station)
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    auto [nw, resultTravelRoute] = GetTestNetwork(
        "network_fastest_path_missing_station"
    );
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_A",
        "station_X",
        maxSlowdownPc,
        minQuietnessPc
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(no_path)
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    auto [nw, resultTravelRoute] = GetTestNetwork(
        "network_fastest_path_no_path"
    );
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_A",
        "station_B",
        maxSlowdownPc,
        minQuietnessPc
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(one_route, *timeout {1})
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    auto [nw, resultTravelRoute] = GetTestNetwork(
        "network_fastest_path_1route"
    );
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_A",
        "station_B",
        maxSlowdownPc,
        minQuietnessPc
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(two_routes, *timeout {1})
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    auto [nw, resultTravelRoute] = GetTestNetwork(
        "network_fastest_path_2routes"
    );
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_A",
        "station_B",
        maxSlowdownPc,
        minQuietnessPc
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(two_routes_overlap, *timeout {1})
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    auto [nw, resultTravelRoute] = GetTestNetwork(
        "network_fastest_path_2routes_overlap"
    );
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_A",
        "station_B",
        maxSlowdownPc,
        minQuietnessPc
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
BOOST_AUTO_TEST_CASE(ltc_path1, *timeout {1})
{
    double maxSlowdownPc {0.1};
    double minQuietnessPc {0.1};
    size_t maxNPaths {20};
    auto [nw, resultTravelRoute] = GetTestNetwork("ltc_path1", true);
    auto travelRoute {nw.GetQuietTravelRoute(
        "station_003",
        "station_019",
        maxSlowdownPc,
        minQuietnessPc,
        maxNPaths
    )};
    BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
}

// Same as fastest-route counterpart
// BOOST_AUTO_TEST_CASE(ltc_path2, *timeout {10})
// {
//     double maxSlowdownPc {0.1};
//     double minQuietnessPc {0.1};
//     size_t maxNPaths {20};
//     auto [nw, resultTravelRoute] = GetTestNetwork("ltc_path2", true);
//     auto travelRoute {nw.GetQuietTravelRoute(
//         "station_211",
//         "station_119",
//         maxSlowdownPc,
//         minQuietnessPc,
//         maxNPaths
//     )};
//     BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
// }

BOOST_AUTO_TEST_CASE(network_quiet_path_2routes, *timeout {1})
{
    // Network under test:
    //
    // Route 0: [A]--3--[0]--1--[1]--2--[2]--3--[3]--1--[B]
    //                           |       |               |
    // Route 1:                  |       |---3--[4]--2---|
    //                           |                       |
    // Route 2:                  |---2--[5]--3--[6]--3---|
    //
    // All 3 routes have a path from A to B.
    // They all share the same travel times except for the last segment.
    // All stations are given a passenger count of 0 except for [3], [4], [6],
    // which alone determine the crowding level of the 3 routes.

    // Baseline: No leeway, which should find the fastest route.
    {
        double maxSlowdownPc {0.0};
        double minQuietnessPc {0.0};
        auto [nw, resultTravelRoute] = GetTestNetwork(
            "network_quiet_path_2routes", false, true, "route_0"
        );
        auto travelRoute {nw.GetQuietTravelRoute(
            "station_A",
            "station_B",
            maxSlowdownPc,
            minQuietnessPc
        )};
        BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
    }

    // We allow up to 10% travel time increase for a 10%+ crowding decrease.
    {
        double maxSlowdownPc {0.1};
        double minQuietnessPc {0.1};
        auto [nw, resultTravelRoute] = GetTestNetwork(
            "network_quiet_path_2routes", false, true, "route_1"
        );
        auto travelRoute {nw.GetQuietTravelRoute(
            "station_A",
            "station_B",
            maxSlowdownPc,
            minQuietnessPc
        )};
        BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
    }

    // We allow up to 20% travel time increase for a 20%+ crowding decrease.
    {
        double maxSlowdownPc {0.2};
        double minQuietnessPc {0.2};
        auto [nw, resultTravelRoute] = GetTestNetwork(
            "network_quiet_path_2routes", false, true, "route_2"
        );
        auto travelRoute {nw.GetQuietTravelRoute(
            "station_A",
            "station_B",
            maxSlowdownPc,
            minQuietnessPc
        )};
        BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
    }

    // Same as before, with a lower threshold for the crowding levels. We get
    // the same result because we still look for the maximum crowding gain
    // given the travel time requirements.
    {
        double maxSlowdownPc {0.2};
        double minQuietnessPc {0.1}; // !
        auto [nw, resultTravelRoute] = GetTestNetwork(
            "network_quiet_path_2routes", false, true, "route_2"
        );
        auto travelRoute {nw.GetQuietTravelRoute(
            "station_A",
            "station_B",
            maxSlowdownPc,
            minQuietnessPc
        )};
        BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
    }

    // When the crowding requirement cannot be met, we pick the fastest route.
    {
        double maxSlowdownPc {0.1};
        double minQuietnessPc {0.2}; // Can't be met with only 10% travel
                                     // time increase.
        auto [nw, resultTravelRoute] = GetTestNetwork(
            "network_quiet_path_2routes", false, true, "route_0"
        );
        auto travelRoute {nw.GetQuietTravelRoute(
            "station_A",
            "station_B",
            maxSlowdownPc,
            minQuietnessPc
        )};
        BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
    }
}

// BOOST_AUTO_TEST_CASE(ltc_quiet1, *timeout {1})
// {
//     // This is a path where route_000 is the obvious and most convenient choice.
//     // We must relax the travel time requirements by a lot to even consider
//     // more quiet routes.

//     // Note: Setting this value affects the the results we find. The results
//     //       may be subotpimal.
//     size_t maxNPaths {20};

//     // 10% travel time relaxation is not enough to use other routes.
//     {
//         double maxSlowdownPc {0.1};
//         double minQuietnessPc {0.1};
//         auto [nw, resultTravelRoute] = GetTestNetwork(
//             "ltc_quiet1", true, true, "route_000"
//         );
//         auto travelRoute {nw.GetQuietTravelRoute(
//             "station_003",
//             "station_019",
//             maxSlowdownPc,
//             minQuietnessPc,
//             maxNPaths
//         )};
//         BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
//     }

//     // With 40% travel time relaxation we can pick a route with 10% crowding
//     // improvement.
//     {
//         double maxSlowdownPc {0.4};
//         double minQuietnessPc {0.1};
//         auto [nw, resultTravelRoute] = GetTestNetwork(
//             "ltc_quiet1", true, true, "route_032"
//         );
//         auto travelRoute {nw.GetQuietTravelRoute(
//             "station_003",
//             "station_019",
//             maxSlowdownPc,
//             minQuietnessPc,
//             maxNPaths
//         )};
//         BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
//     }
// }

// BOOST_AUTO_TEST_CASE(ltc_quiet2, *timeout {10})
// {
//     // This is a path where many different routes compete for the fastest path.
//     // In this case, we can win a more quiet path by giving up only a tiny bit
//     // of travel time.

//     // Note: Setting this value affects the the results we find. The results
//     //       may be subotpimal.
//     size_t maxNPaths {20};

//     // Here we select the fastest path because no path gives a 20% crowding
//     // improvement.
//     {
//         double maxSlowdownPc {0.1};
//         double minQuietnessPc {0.2};
//             auto [nw, resultTravelRoute] = GetTestNetwork(
//                 "ltc_quiet2", true, true, "route_053"
//             );
//         auto travelRoute {nw.GetQuietTravelRoute(
//             "station_211",
//             "station_119",
//             maxSlowdownPc,
//             minQuietnessPc,
//             maxNPaths
//         )};
//         BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
//     }

//     // A 10% crowding improvement is possible via route_049.
//     {
//         double maxSlowdownPc {0.1};
//         double minQuietnessPc {0.1};
//             auto [nw, resultTravelRoute] = GetTestNetwork(
//                 "ltc_quiet2", true, true, "route_049"
//             );
//         auto travelRoute {nw.GetQuietTravelRoute(
//             "station_211",
//             "station_119",
//             maxSlowdownPc,
//             minQuietnessPc,
//             maxNPaths
//         )};
//         BOOST_CHECK_EQUAL(travelRoute, resultTravelRoute);
//     }
// }

BOOST_AUTO_TEST_SUITE_END(); // GetQuietTravelRoute

BOOST_AUTO_TEST_SUITE_END(); // Routes

BOOST_AUTO_TEST_SUITE_END(); // class_TransportNetwork

BOOST_AUTO_TEST_SUITE_END(); // network-monitor