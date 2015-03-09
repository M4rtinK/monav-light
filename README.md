MoNav Light
===========

MoNav Light is a lightweight and platform independent offline routing software that works on
pre-rpocessed OpenStreetMap data and uses JSON for input and output.

Building
--------

    qmake monav-light.pro
    make

JSON input format
-----------------

Example:

{
    "dataDirectory": "czech-republic/routing_car",
    "lookupEdgeNames": true,
    "routingRadius": 10000,
    "waypoints": [
        [
            49.200000000000003,
            16.616667,
            0,
            0
        ],
        [
            50.083333000000003,
            14.416667,
            0,
            0
        ]
    ]
}

* **dataDirectory**  - path to a valid MoNav offline routing data directory
* **lookupEdges** - if name and type of the edges comprising the route should bee looked up
* **routingRadius** - how far too look (in meters) around start/destination/waypoints for the nearest edge
* **waypoints** a list waypoints the resulting route should pass through, the waypoint list has fields like this:
  * *latitude*
  * *longitude*
  * *heading penalty* - penalty in meters for edge with direction opposite of heading
  * *heading* - Degrees from North

 At least two waypoints are needed for the routing request to be valid.

JSON output format
------------------

The output JSON is quite self describing - just run the example and it should be fairly evident what is what.

In short there are 4 main lists:

* **edgeNames** - ordered list of names of the edges comprising the route
* **edgeTypes** - ordered list of the types (based on OSM tag) of the edges comprising the route
* **edges** - an ordered list of edges comprising the route, each edge segment is represented as a list:
  * *number of segments* - how many segments the given edge has
  * *name index* - index of the name of the edge in the *edgeNames* list
   * 0 -> first name in *edgeNames*, 1 -> second name, etc.
  * *seconds* - most probably how many seconds a given mode of transport will need to traverse the edge
  * *branching possible* - most probably if it is possible to turn to other edges than those comprising the route
* *nodes* - list of *latitude*, *longitude* lists expressing the points comprising the route

To simply draw the route you can just render all the points from **nodes**, for turn-by-turn navigation you
would usually iterate over all the edges comprising the route and assigning them names based on the *name index*
and assigning nodes by adding *number of segements* together and using it as an index for the **nodes** list.

Usage example
-------------
Download a preprocessed Monav offline routing data pack:

    wget http://data.modrana.org/monav/europe/czech-republic/czech-republic_car.tar.gz

Unpack it:

    tar xfv czech-republic_car.tar.gz

Run a routing query:

    monav-light "{\"lookupEdgeNames\":true,\"routingRadius\":10000,\"dataDirectory\":\"czech-republic/routing_car\",\"waypoints\":[[49.2,16.616667,0,0],[50.083333,14.416667,0,0]]}"

Resources
---------

* [Original MoNav upstream (dead ATM)](https://code.google.com/p/monav/)
* [Sailfish OS port of Monav and insipiration for MoNav Light](https://github.com/tunp/monavsailfish)

Licensing
---------

MoNav Light is, just as Monav, licensed under GPLv3+.
