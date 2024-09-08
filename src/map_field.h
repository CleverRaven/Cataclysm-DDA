#pragma once
#ifndef CATA_SRC_MAP_FIELD_H
#define CATA_SRC_MAP_FIELD_H

struct tripoint;
class field_entry;
struct field_type;
struct field_proc_data;

namespace map_field_processing
{

/**
 * Pointer to the "field processor" - a function that will be called each alive field entry of a matching field type
 * during field processing.
 */
using FieldProcessorPtr = void( * )( const tripoint &p, field_entry &cur, field_proc_data &pd );

/**
 * Returns list of "field processors" for a given field type
 */
std::vector<FieldProcessorPtr> processors_for_type( const field_type &ft );

} // namespace map_field_processing

#endif // CATA_SRC_MAP_FIELD_H
