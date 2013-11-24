#ifndef FILE_FINDER_H
#define FILE_FINDER_H

#include <string>
#include <vector>

class file_finder
{
    public:
        static std::vector<std::string> get_files_from_path(std::string extension, std::string root_path = "", bool recursive_search = false, bool match_extension = false);
        static std::vector<std::string> get_directories_with(std::vector<std::string> extensions, std::string root_path = "", bool recursive_search = false);
    protected:
    private:
};

#endif // FILE_FINDER_H
