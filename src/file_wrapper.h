#ifndef FILE_WRAPPER_H
#define FILE_WRAPPER_H

#include <string>

bool assure_dir_exist(std::string const &path);
bool file_exist(const std::string &path);
// Remove a file, does not remove folders,
// returns true on success
bool remove_file(const std::string &path);
// Rename a file, overriding the target!
bool rename_file(const std::string &old_path, const std::string &new_path);

#endif
