#pragma once
#ifndef PATH_INFO_H
#define PATH_INFO_H

#include <map>
#include <string>

class Path
{
private:

    std::map<std::string, std::string> pathname;

    /*
     * Here will be the instace stored.
     */
    static Path* instance;

    /*
     * Private constructor to prevent instancing.
     */
    Path( std::string basePath, std::string userDirectoryPath );

    /*
     * Configure the path to the 'data' and 'gfx' directory, which contains
     * the necessary information for the application to work.
     *
     * The location of the 'data' and 'gfx' directory depends on where the
     * base path is located and the platform on which it is executed.
     *
     * @param pathname Map where we keep the routes.
     */
    static void initDataDirectory(std::map<std::string, std::string> pathname);

    /*
     * To use the route it is necessary that the route has the appropriate
     * format, the routes with the appropriate format must end with '/' in
     * the case of linux and '\' in the case of windows.
     *
     * Example:
     *
     *      LINUX:
     *      - "/usr/home/example/"  (God)
     *      - "/usr/home/example"   (Bad)
     *
     *      WINDOWS:
     *      - "C:\Programs\Example\"    (God)
     *      - "C:\Programs\Example"     (Bad)
     *
     * @brief: The method adds '/' at the end of the route if necessary.
     *
     * @param path Path to apply the format.
     * @return Formatted path.
     */
    static std::string formatPath(std::string path);

public:

    static void initBasePath(std::string path);
    static void initUserDirectory(std::string path);

    /*
     * Notes: This method needs to be called first
     * to build the object properly, as it requires
     * certain parameters.
     *
     * Static access method.
     */
    static Path *getInstance( std::string basePath, std::string userDirectoryPath );

    /*
     * Static access method.
     */
    static Path *getInstace();

    /*
     * TODO: Make safe method, launch an error in case the key does not exist.
     * Returns the path according to the value of the key.
     */
    std::string getPathForValueKey( std::string valueKey );

    /*
     * Postcondition:
     *      - pathanme["BASE_PATH"] has been initialized.
     *      - pathname["USER_DIRE"] has been initialized.
     */
    static void setStandardFilenames();

    static void updateDataDirectory();
    static void updateConfigurationDirectory();
    static void updatePathname( const std::string &name, const std::string &path);
    void toString();
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
