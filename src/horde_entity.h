#pragma once
#ifndef CATA_SRC_HORDE_ENTITY_H
#define CATA_SRC_HORDE_ENTITY_H

#include <memory>

#include "calendar.h"
#include "coordinates.h"
#include "type_id.h"
#include "monster.h"

struct mtype;

// This represents a single entity that moves around at overmap scale.
// It needs to spawn the related monster, ideally with any notable details intact.
// TODO: There are a LOT of these stored in each overmap, leading to significant memory pressure.
// Need to investigate reducing the size of members pretty agresively.
struct horde_entity {
    // Create a heavy entity based on an existing monster.
    explicit horde_entity( const monster &original );
    // Create a lightweight entity based on a monster id.
    explicit horde_entity( const mtype_id &original );
    // Retrieve the mtype whether it's a light or heavy entity.
    const mtype *get_type() const;
    bool is_active() const;

    // Data here related to processing while acting as a horde entity.
    // a glaring omission is location, the parent horde container knows that.
    // TODO: There are a LOT of horde_entity instances, and destination is a large proportion of it,
    // investigate making it smaller.
    tripoint_abs_ms destination;
    // Shrink this one too?
    int tracking_intensity = 0;
    // Not sure how to shrink this?
    time_point last_processed;
    // Same here, this could probably be a byte.
    int moves = 0;
    mtype_int_id type_id;
    // If this monster was never spawned, this member can be empty.
    // If it was, it has this populated to capture all the random bits of state that a monster can accumulate.
    // The vast majority of entities in an overmap have never actually been spawned,
    // meaning they don't have this member populated.
    // This could be a 16-bit index instead of a 32-to-64-bit+ unique_pointer
    std::unique_ptr<monster> monster_data;
};

#endif // CATA_SRC_HORDE_ENTITY_H
