#ifndef SEARCH_H
#define SEARCH_H

#include "cubiomes/finders.h"

#include <atomic>

#define PRECOMPUTE48_BUFSIZ ((int64_t)1 << 30)

enum
{
    CAT_NONE,
    CAT_48,
    CAT_FULL,
};

struct FilterInfo
{
    int cat;    // seed source category
    bool coord; // requires coordinate entry
    bool area;  // requires area entry
    int layer;  // associated generator layer
    int stype;  // structure type
    int step;   // coordinate multiplier
    int count;  //
    int mcmin;  // minimum version
    const char *icon;
    const char *name;
    const char *desription;
};

enum
{
    F_SELECT,
    F_QH_IDEAL,
    F_QH_CLASSIC,
    F_QH_NORMAL,
    F_QH_BARELY,
    F_QM_95,
    F_QM_90,
    F_BIOME,
    F_BIOME_4_RIVER,
    F_BIOME_16_SHORE,
    F_BIOME_64_RARE,
    F_BIOME_256_BIOME,
    F_BIOME_256_OTEMP,
    F_TEMPS,
    F_SLIME,
    F_SPAWN,
    F_STRONGHOLD,
    F_DESERT,
    F_JUNGLE,
    F_HUT,
    F_IGLOO,
    F_MONUMENT,
    F_VILLAGE,
    F_OUTPOST,
    F_MANSION,
    F_TREASURE,
    F_RUINS,
    F_SHIPWRECK,
    F_PORTAL,
    FILTER_MAX,
};

// global table of filter data (as constants with enum indexing)
static const struct FilterList
{
    FilterInfo list[FILTER_MAX];

    FilterList() : list{}
    {
        list[F_SELECT] = FilterInfo{
            CAT_NONE, 0, 0, 0, 0, 0, 0, MC_1_0,
            NULL,
            "",
            ""
        };

        list[F_QH_IDEAL] = FilterInfo{
            CAT_48, 1, 1, 0, Swamp_Hut, 512, 0, MC_1_4,
            ":icons/quad.png",
            "Quad-hut (ideal)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the best configurations that exist."
        };

        list[F_QH_CLASSIC] = FilterInfo{
            CAT_48, 1, 1, 0, Swamp_Hut, 512, 0, MC_1_4,
            ":icons/quad.png",
            "Quad-hut (classic)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the \"classic\" configurations. "
            "(Checks for huts in the nearest 2x2 chunk corners of each "
            "region.)"
        };

        list[F_QH_NORMAL] = FilterInfo{
            CAT_48, 1, 1, 0, Swamp_Hut, 512, 0, MC_1_4,
            ":icons/quad.png",
            "Quad-hut (normal)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, such that all of them are within 128 blocks "
            "of a single AFK location, including a vertical tollerance "
            "for a fall damage chute."
        };

        list[F_QH_BARELY] = FilterInfo{
            CAT_48, 1, 1, 0, Swamp_Hut, 512, 0, MC_1_4,
            ":icons/quad.png",
            "Quad-hut (barely)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in any configuration, such that the bounding "
            "boxes are within 128 blocks of a single AFK location."
        };

        list[F_QM_95] = FilterInfo{
            CAT_48, 1, 1, 0, Monument, 512, 0, MC_1_8,
            ":icons/quad.png",
            "Quad-ocean-monument (>95%)",
            "The lower 48-bits provide potential for 95% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location."
        };

        list[F_QM_90] = FilterInfo{
            CAT_48, 1, 1, 0, Monument, 512, 0, MC_1_8,
            ":icons/quad.png",
            "Quad-ocean-monument (>90%)",
            "The lower 48-bits provide potential for 90% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location."
        };

        list[F_BIOME] = FilterInfo{
            CAT_FULL, 1, 1, L_VORONOI_1, 0, 1, 0, MC_1_0,
            ":icons/map.png",
            "Biome filter 1:1",
            "Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-)."
        };

        list[F_BIOME_4_RIVER] = FilterInfo{
            CAT_FULL, 1, 1, L_RIVER_MIX_4, 0, 4, 0, MC_1_0,
            ":icons/map.png",
            "Biome filter 1:4 RIVER",
            "Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer RIVER with scale 1:4."
        };

        list[F_BIOME_16_SHORE] = FilterInfo{
            CAT_FULL, 1, 1, L_SHORE_16, 0, 16, 0, MC_1_0,
            ":icons/map.png",
            "Biome filter 1:16 SHORE",
            "Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer SHORE with scale 1:16."
        };

        list[F_BIOME_64_RARE] = FilterInfo{
            CAT_FULL, 1, 1, L_SUNFLOWER_64, 0, 64, 0, MC_1_7,
            ":icons/map.png",
            "Biome filter 1:64 RARE",
            "Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer RARE_BIOME with scale 1:64."
        };

        list[F_BIOME_256_BIOME] = FilterInfo{
            CAT_FULL, 1, 1, L_BIOME_256, 0, 256, 0, MC_1_0,
            ":icons/map.png",
            "Biome filter 1:256 BIOME",
            "Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer BIOME with scale 1:256."
        };

        list[F_BIOME_256_OTEMP] = FilterInfo{
            CAT_48, 1, 1, L_OCEAN_TEMP_256, 0, 256, 0, MC_1_13,
            ":icons/map.png",
            "Biome filter 1:256 O.TEMP",
            "Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer OCEAN TEMPERATURE with scale 1:256. "
            "This generation layer depends only on the lower 48-bits of the seed."
        };

        list[F_TEMPS] = FilterInfo{
            CAT_FULL, 1, 1, 0, 0, 1024, 0, MC_1_7,
            ":icons/tempcat.png",
            "Temperature categories",
            "Checks that the area has a minimum of all the required temperature categories."
        };

        list[F_SLIME] = FilterInfo{
            CAT_FULL, 1, 1, 0, 0, 16, 1, MC_1_0,
            ":icons/slime.png",
            "Slime chunk",
            ""
        };

        list[F_SPAWN] = FilterInfo{
            CAT_FULL, 1, 1, 0, 0, 1, 0, MC_1_0,
            ":icons/spawn.png",
            "Spawn",
            ""
        };

        list[F_STRONGHOLD] = FilterInfo{
            CAT_FULL, 1, 1, 0, 0, 1, 1, MC_1_0,
            ":icons/stronghold.png",
            "Stronghold",
            ""
        };

        list[F_DESERT] = FilterInfo{
            CAT_FULL, 1, 1, 0, Desert_Pyramid, 1, 1, MC_1_3,
            ":icons/desert.png",
            "Desert pyramid",
            ""
        };

        list[F_JUNGLE] = FilterInfo{
            CAT_FULL, 1, 1, 0, Jungle_Pyramid, 1, 1, MC_1_3,
            ":icons/jungle.png",
            "Jungle temple",
            ""
        };

        list[F_HUT] = FilterInfo{
            CAT_FULL, 1, 1, 0, Swamp_Hut, 1, 1, MC_1_4,
            ":icons/hut.png",
            "Swamp hut",
            ""
        };

        list[F_IGLOO] = FilterInfo{
            CAT_FULL, 1, 1, 0, Igloo, 1, 1, MC_1_9,
            ":icons/igloo.png",
            "Igloo",
            ""
        };

        list[F_MONUMENT] = FilterInfo{
            CAT_FULL, 1, 1, 0, Monument, 1, 1, MC_1_8,
            ":icons/monument.png",
            "Ocean monument",
            ""
        };

        list[F_VILLAGE] = FilterInfo{
            CAT_FULL, 1, 1, 0, Village, 1, 1, MC_1_0,
            ":icons/village.png",
            "Village",
            ""
        };

        list[F_OUTPOST] = FilterInfo{
            CAT_FULL, 1, 1, 0, Outpost, 1, 1, MC_1_14,
            ":icons/outpost.png",
            "Pillager outpost",
            ""
        };

        list[F_MANSION] = FilterInfo{
            CAT_FULL, 1, 1, 0, Mansion, 1, 1, MC_1_11,
            ":icons/mansion.png",
            "Woodland mansion",
            ""
        };

        list[F_TREASURE] = FilterInfo{
            CAT_FULL, 1, 1, 0, Treasure, 1, 1, MC_1_13,
            ":icons/treasure.png",
            "Buried treasure",
            ""
        };

        list[F_RUINS] = FilterInfo{
            CAT_FULL, 1, 1, 0, Ocean_Ruin, 1, 1, MC_1_13,
            ":icons/ruins.png",
            "Ocean ruins",
            ""
        };

        list[F_SHIPWRECK] = FilterInfo{
            CAT_FULL, 1, 1, 0, Shipwreck, 1, 1, MC_1_13,
            ":icons/shipwreck.png",
            "Shipwreck",
            ""
        };

        list[F_PORTAL] = FilterInfo{
            CAT_FULL, 1, 1, 0, Ruined_Portal, 1, 1, MC_1_16,
            ":icons/portal.png",
            "Ruined portal",
            ""
        };
    }
}
g_filterinfo;


struct Condition
{
    int type;
    int x1, z1, x2, z2;
    int save;
    int relative;
    BiomeFilter bfilter;
    uint64_t exclb; // excluded biome
    uint64_t exclm; // excluded modified
    int temps[9];
    int count;
};

struct StructPos
{
    StructureConfig sconf;
    int cx, cz; // effective center position
};


int testCond(StructPos *spos, int64_t seed, const Condition *cond, int mc, LayerStack *g, std::atomic_bool *abort);


#endif // SEARCH_H
