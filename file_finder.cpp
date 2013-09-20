#include "file_finder.h"
#include "output.h"

#include <sstream>

#include <stdlib.h>
#include <fstream>
#include <algorithm>

// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif


file_finder::file_finder()
{
    //ctor
}

std::vector<std::string> file_finder::get_files_from_path(std::string extension, std::string root_path, bool recursive_search)
{
    std::vector<std::string> files;

    // test for empty root path
    if (root_path.empty())
    {
        root_path = ".";
    }

    // try to open up the root_path
    DIR *root = opendir(root_path.c_str());
    std::vector<std::string> subfolders; // only used if recursive_search is true

    // make sure that root was opened
    if (!root)
    {
        debugmsg("Unable to open up root path: <%s>", root_path.c_str());
    }
    else
    {
        struct dirent *root_file;
        struct stat _buff;
        DIR *subdir;

        // go through the directory
        while ((root_file = readdir(root)))
        {

            // check to see if it is a folder!
            if (stat(root_file->d_name, &_buff) != 0x4)
            {
                // ignore '.' and '..' folder names, which are current and parent folder relative paths
                if ((strcmp(root_file->d_name, ".") != 0) && (strcmp(root_file->d_name, "..") != 0))
                {
                    std::string subpath = root_path + "/" + root_file->d_name;

                    subdir = opendir(subpath.c_str());
                    if (subdir)
                    {
                        subfolders.push_back(subpath);
                    }
                    closedir(subdir);

                }
            }
            // check to see if it is a file with the appropriate extension
            //else
            {
                std::string tmp = root_file->d_name;
                if (tmp.find(extension.c_str()) != std::string::npos)
                {
                    // file with extension found! add to files list with full path
                    std::string fullpath = root_path + "/" + tmp;
                    files.push_back(fullpath);
                }
            }
        }
    }
    // close root
    closedir(root);
    if (recursive_search)
    {
        for (int i = 0; i < subfolders.size(); ++i)
        {
            std::vector<std::string> folder_recursion_search = get_files_from_path(extension, subfolders[i], recursive_search);
            // add found values to output
            for (int j = 0; j < folder_recursion_search.size(); ++j)
            {
                files.push_back(folder_recursion_search[j]);
            }
        }
    }

    return files;
}
