#pragma once
#ifndef CATA_SRC_PATH_MANAGER_H
#define CATA_SRC_PATH_MANAGER_H

#include <iosfwd>
#include <memory>

#include "coords_fwd.h"

class JsonOut;
class JsonValue;
class path;
class path_manager_impl;

class path_manager
{
        friend path_manager_impl;
        friend path;
    public:
        path_manager();
        ~path_manager();
        /**
         * Record a single step of path. Only if `path_manager_impl::recording_path` is true.
         * If this step makes a loop, remove the whole loop.
         */
        void record_step( const tripoint_abs_ms &new_pos );
        /*serialize and deserialize*/
        bool store();
        void load();
        void serialize( std::ostream &fout );
        void deserialize( const JsonValue &jsin );
        void serialize( JsonOut &jsout );

        void show();
    private:
        std::unique_ptr <path_manager_impl> pimpl;
};

#endif // CATA_SRC_PATH_MANAGER_H
