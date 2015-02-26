#ifndef OSM_TYPES_H
#define OSM_TYPES_H

enum osm_type_t {
    AREA_PARK =             1,
    AREA_CAMPSITE =         2,
    AREA_NATURE =           3,
    AREA_CEMETERY =         4,
    AREA_RESIDENTIAL =      5,
    AREA_BARRACKS =         6,
    AREA_MILITARY =         7,
    AREA_FIELD =            8,
    AREA_DANGER_AREA =      9,
    AREA_MEADOW =          10,
    AREA_COMMON =          11,
    AREA_FOREST =          12,
    AREA_WATER =           13,
    AREA_WOOD =            14,
    AREA_RETAIL =          15,
    AREA_INDUSTRIAL =      16,
    AREA_PARKING =         16,
    AREA_BUILDING =        17,
   
    HW_PEDESTRIAN =       64,
    HW_PATH =             65,
    HW_FOOTWAY =          66,
    HW_STEPS =            67,
    HW_BRIDLEWAY =        68,
    HW_CYCLEWAY =         69,
    HW_PRIVATE =          70,
    HW_UNSURFACED =       71,
    HW_UNCLASSIFIED =     72,
    HW_RESIDENTIAL =      73,
    HW_LIVING_STREET =    74,
    HW_SERVICE =          75,
    HW_TERTIARY =         76,
    HW_SECONDARY =        77,
    HW_PRIMARY =          78,
    HW_TRUNK =            79,
    HW_MOTORWAY =         80,

    RW_RAIL =             96,
    WATERWAY =            97,

    PLACE_TOWN =          110,
    DONE =                127
};

#endif
