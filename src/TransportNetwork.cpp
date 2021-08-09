#include "network-monitor/TransportNetwork.h"
#include <iostream>
#include <algorithm>

using namespace NetworkMonitor;

bool Station::operator==(const Station& other) const
{
    return id == other.id;
}

bool Route::operator==(const Route& other) const
{
    return id == other.id;
}

bool Line::operator==(const Line& other) const
{
    return id == other.id;
}

TransportNetwork::TransportNetwork()
{

}

TransportNetwork::~TransportNetwork()
{

}

bool TransportNetwork::AddStation(const Station& station)
{
    if(getStation(station.id) != nullptr){
        return false;
    }
    // Create a new station node and add it to the map.
    auto node {std::make_shared<GraphNode>(GraphNode {
        station.id,
        station.name,
        0, // We start with no passengers.
        {} // We start with no edges.
    })};
    stations_.emplace(station.id, std::move(node));
    return true;
}

bool TransportNetwork::AddLine(const Line& line)
{
    if(getLine(line.id) != nullptr){
        return false;
    }
    // Create the internal version of the line.
    auto lineInternal {std::make_shared<LineInternal>(LineInternal {
        line.id,
        line.name,
        {}
    })};

    for(const auto& route : line.routes){
        bool ok {addRouteToLine(route,lineInternal)};
        if(!ok){
            return false;
        }
    }

    lines_.emplace(line.id, std::move(lineInternal));
    return true;
}

bool TransportNetwork::RecordPassengerEvent(
        const PassengerEvent& event
)
{
    const auto& station {getStation(event.stationId)};

    if(station == nullptr){
        return false;
    }
    if(event.type == PassengerEvent::Type::In){
        station->passengerCount++;
    }
    else{
        station->passengerCount--;
    }
    return true;
}

long long int TransportNetwork::GetPassengerCount(
        const Id& stationId
) const
{
    const auto& station {getStation(stationId)};

    if(station == nullptr){
        throw std::runtime_error("Could not find station in network: " + stationId);
    }

    return station->passengerCount;
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(
        const Id& stationId
) const
{
    std::vector<Id> routes{};
    const auto& station {getStation(stationId)};
    if(station == nullptr){
        return routes;
    }
    for(const auto& edge : station->edges){
        routes.push_back(edge->route->id);
    }

    for(const auto& [_,line] : lines_){
        for(const auto& [_,route] : line->routes){
            const auto& endStation{*(--route->stops.end())};
            if(station->id == endStation->id){
                routes.push_back(route->id);
            }
        }
    }
    return routes;
}

bool TransportNetwork::SetTravelTime(
        const Id& stationAId,
        const Id& stationBId,
        const unsigned int travelTime
)
{
    const auto& stationA {getStation(stationAId)};
    const auto& stationB {getStation(stationBId)};

    if(stationA == nullptr || stationB == nullptr){
        return false;
    }

    bool foundAnyEdge {false};
    auto setTravelTimeHelper
    {
        [&foundAnyEdge,&travelTime](const auto& from, const auto& to){
            for(const auto& edge : from->edges){
                if(edge->nextStop == to){
                    edge->travelTime = travelTime;
                    foundAnyEdge = true;
                }
            }
        }
    };

    setTravelTimeHelper(stationA, stationB);
    setTravelTimeHelper(stationB, stationA);

    return foundAnyEdge;
}
unsigned int TransportNetwork::GetTravelTime(
        const Id& stationAId,
        const Id& stationBId
) const
{
    const auto& stationA {getStation(stationAId)};
    const auto& stationB {getStation(stationBId)};

    if(stationA == nullptr || stationB == nullptr){
        return 0;
    }

    for (const auto& edge: stationA->edges) {
        if (edge->nextStop == stationB) {
            return edge->travelTime;
        }
    }
    for (const auto& edge: stationB->edges) {
        if (edge->nextStop == stationA) {
            return edge->travelTime;
        }
    }

    return 0;
}

unsigned int TransportNetwork::GetTravelTime(
        const Id& line,
        const Id& route,
        const Id& stationAId,
        const Id& stationBId
) const
{
    const auto& stationA {getStation(stationAId)};
    const auto& stationB {getStation(stationBId)};
    if(stationA == nullptr || stationB == nullptr){
        return 0;
    }

    const auto& routeInternal {getRoute(line, route)};
    if(routeInternal == nullptr){
        return 0;    
    }

    unsigned int travelTime {0};
    
    bool stationAFoundOnRoute {false};
    for(const auto& stop : routeInternal->stops){
        if(stop == stationA)
            stationAFoundOnRoute = true;

        if(stop == stationB)
            return travelTime;

        if(stationAFoundOnRoute){
            const auto& edgeIt {stop->FindEdgeForRoute(routeInternal)};
            if(edgeIt == stop->edges.end()){
                return 0;
            }
            travelTime += (*edgeIt)->travelTime;
        }
    }

    return travelTime;
}

std::vector<
            std::shared_ptr<TransportNetwork::GraphEdge>
        >::const_iterator TransportNetwork::GraphNode::FindEdgeForRoute(
            const std::shared_ptr<TransportNetwork::RouteInternal>& route
) const
{
    return std::find_if(
        edges.begin(),
        edges.end(),
        [&route](const auto& edge) {
            return edge->route == route;
        }
    );
}

std::shared_ptr<TransportNetwork::GraphNode> TransportNetwork::getStation(const Id& stationId) const
{
    auto stationIt {stations_.find(stationId)};
    if(stationIt == stations_.end()){
        return nullptr;
    }
    return stationIt->second;
}

std::shared_ptr<TransportNetwork::LineInternal> TransportNetwork::getLine(const Id& LineId) const
{
    auto lineIt {lines_.find(LineId)};
    if(lineIt == lines_.end())
    {
        return nullptr;
    }
    return lineIt->second;
}

std::shared_ptr<TransportNetwork::RouteInternal> TransportNetwork::getRoute(const Id& lineId, const Id& routeId) const
{
    const auto& line {getLine(lineId)};
    if (line == nullptr) {
        return nullptr;
    }

    const auto& routeIt {line->routes.find(routeId)};
    if(routeIt == line->routes.end())
    {
        return nullptr;
    }
    return routeIt->second;
}

bool TransportNetwork::addRouteToLine(const Route& route, std::shared_ptr<TransportNetwork::LineInternal>& lineInternal)
{
    std::vector<std::shared_ptr<TransportNetwork::GraphNode>> stops{};
    stops.reserve(route.stops.size());

    for(const auto& stopId : route.stops){
        const auto station {getStation(stopId)};
        if(station == nullptr)
            return false;
        stops.push_back(station);
    }

    auto routeInternal {std::make_shared<TransportNetwork::RouteInternal>(RouteInternal{
        route.id,
        lineInternal,
        std::move(stops)
    })};

    for(size_t i=0;i<routeInternal->stops.size()-1;++i){
        const auto& thisStop {routeInternal->stops[i]};
        const auto& nextStop {routeInternal->stops[i+1]};
        thisStop->edges.emplace_back(std::make_shared<TransportNetwork::GraphEdge>(GraphEdge{
            routeInternal,
            nextStop,
            0
        }));
    }

    lineInternal->routes[route.id] = std::move(routeInternal);

    return true;
}