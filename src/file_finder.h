#ifndef FILE_FINDER_H
#define FILE_FINDER_H

#include <string>
#include <vector>

std::vector<std::string> get_files_from_path(std::string const &extension,
        std::string const &root_path = "", bool recursive_search = false,
        bool match_extension = false);

std::vector<std::string> get_directories_with(std::vector<std::string> const &extensions,
        std::string const &root_path = "", bool recursive_search = false);

#endif
