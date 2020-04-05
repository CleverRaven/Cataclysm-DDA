#pragma once
#ifndef CATA_EVENT_FIELD_TRANSFORMATIONS_H
#define CATA_EVENT_FIELD_TRANSFORMATIONS_H

#include <vector>
#include <unordered_map>

#include "cata_variant.h"

using EventFieldTransformation = std::vector<cata_variant>( * )( const cata_variant & );
extern const std::unordered_map<std::string, EventFieldTransformation> event_field_transformations;

#endif
