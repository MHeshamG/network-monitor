# Live Transport Network Monitor

In this Project, we built a Live Transport Network Monitor for the London Transport Company 
to optimize the passenger load on the London Underground lines during the busiest times.

# Description

Every day, the London Underground, better known as the Tube, 
handles up to 5 million passenger journeys on its 11 lines and across 270 stations.

<img src="https://projects.learncppthroughprojects.com/_next/image?url=%2Funderground.png&w=3840&q=75">

The London Transport Company (LTC) is thinking about ways to manage passenger load on the busiest lines to avoid overcrowding.

So London Transport Company (LTC) has asked us to implement an itinerary recommendation engine that generates Tube trips based on real-time network load. 
We will call these itineraries quiet routes.

Here is the system arcitucture which we have developed to provide the quite route recommendations by monitoring the underground network the passengers load through stations.

<img src="https://projects.learncppthroughprojects.com/_next/image?url=%2Farchitecture.png&w=3840&q=75">

What are the main functional blocks? What are their inputs and outputs?
  * network-events client: A WebSocket/STOMP client that connects to the network-events service and processes the passenger events as they come in.  
    * Inputs: Settings to establish a WebSocket/STOMP connection.
    * Outputs: Passenger events received from the network-events service where events are entrance to stations.
  * Network representation: A component that maintains a view of the Underground network.
    * Inputs: Network layout JSON file and live passenger events.
    * Output: A network representation that can be used to produce quiet route itineraries.
  * Recommendation engine: A component that, given a request to go from A to B, looks at the network representation and produces a quiet route travel recommendation.
    * Inputs: Network representation and a request to go from A to B.
    * Outputs: The itinerary recommendation.
  * quiet-route server: A WebSocket/STOMP server that handles user requests and forwards them to the recommendation engine.
    * Inputs: Settings to setup a WebSocket/STOMP server.
    * Outputs: STOMP frames in response to user requests for route recommendations.
    
# Technologies and libraries
This project is built using C++17 and here are some of the technologies and libraries used:
* Websockets
* TLS
* JSON
* STOMP
* [Boost asio](https://www.boost.org/doc/libs/1_76_0/doc/html/boost_asio.html)
* [Boost beast](https://www.boost.org/doc/libs/1_73_0/libs/beast/doc/html/index.html)
* [Conan package manager](https://conan.io/)
* [CMake](https://cmake.org/)
* [nlohmann](https://github.com/nlohmann/json)
* [libcurl](https://curl.se/libcurl/)
