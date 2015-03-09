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

TBD

JSON output format
------------------

TBD

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

[Original MoNav upstream (dead ATM)](https://code.google.com/p/monav/)
[Sailfish OS port of Monav and insipiration for MoNav Light](https://github.com/tunp/monavsailfish)

Licensing
---------

MoNav Light, just as Monav, is licensed under GPLv3+.
