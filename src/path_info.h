#pragma once
#ifndef PATH_INFO_H
#define PATH_INFO_H

#include <map>
#include <string>

class Path
{
public:

    Path( const std::string basePath, const std::string userDirectoryPath );

    static void initBasePath(std::string path);
    static void initUserDirectory(std::string path);
    static void setStandardFilenames();
    static void updateDataDirectory();
    static void updateConfigurationDirectory();
    static void updatePathname( const std::string &name, const std::string &path);
};

extern std::map<std::string, std::string> FILENAMES;

namespace PATH_INFO
{

    /**
 * Return a locale specific path, or if there is no path for the current
 * locale, return the default path.
 * @param pathid The key in the @ref FILENAMES map. The local path is based
 * on that value.
 * @param extension File name extension, is automatically added to the path
 * of the translated file. Can be empty, but must otherwise include the
 * initial '.', e.g. ".json"
 * @param fallbackid The path id of the fallback filename. As like pathid it's
 * the key into the @ref FILENAMES map. It is used if no translated file can be
 * found.
 */
std::string find_translated_file( const std::string &pathid,
        const std::string &extension, const std::string &fallbackid );
}

#endif
