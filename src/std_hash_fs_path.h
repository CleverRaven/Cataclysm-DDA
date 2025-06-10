#pragma once
#ifndef CATA_SRC_STD_HASH_FS_PATH_H
#define CATA_SRC_STD_HASH_FS_PATH_H

#include <filesystem>
#include <utility>

// VS2019 and earlier don't have std::hash<std::filesystem::path>
// Also some versions of the stdlib that shipped with xcode which CI uses.
// This is definitionally the same implementation however.
struct std_fs_path_hash {
    auto operator()( const std::filesystem::path &p ) const {
        return std::filesystem::hash_value( p );
    }
};

#endif // CATA_SRC_STD_HASH_FS_PATH_H
