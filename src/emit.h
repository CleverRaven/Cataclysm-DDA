#pragma once
#ifndef EMIT_H
#define EMIT_H

#include <map>
#include <string>

#include "field_type.h"
#include "string_id.h"
#include "type_id.h"

class JsonObject;

class emit
{
    public:
        emit();

        const emit_id &id() const {
            return id_;
        }

        /** When null @ref field is always fd_null */
        bool is_null() const;

        /** When valid @ref field is never fd_null */
        bool is_valid() const;

        /** Type of field to emit @see emit::is_valid */
        field_type_id field() const {
            return field_;
        }

        /** Intensity of output fields, range [1..maximum_intensity] */
        int intensity() const {
            return intensity_;
        }

        /** Units of field to generate per turn subject to @ref chance */
        int qty() const {
            return qty_;
        }

        /** Chance to emit each turn, range [1..100] */
        int chance() const {
            return chance_;
        }

        /** Load emission data from JSON definition */
        static void load_emit( JsonObject &jo );

        /** Get all currently loaded emission data */
        static const std::map<emit_id, emit> &all();

        /** Check consistency of all loaded emission data */
        static void finalize();

        /** Check consistency of all loaded emission data */
        static void check_consistency();

        /** Clear all loaded emission data (invalidating any pointers) */
        static void reset();

    private:
        emit_id id_;
        field_type_id field_ = fd_null;
        int intensity_ = 1;
        int qty_ = 1;
        int chance_ = 100;

        /** used during JSON loading only */
        std::string field_name;
};

#endif
