#pragma once
#ifndef PATH_INFO_H
#define PATH_INFO_H

#include <map>
#include <string>
#include <clocale>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <memory>

#include "filesystem.h"
#include "options.h"

class Path
{

private:

    std::map <std::string, std::string> pathname;

    /*
     * Private constructor to prevent instancing.
     */
    Path( std::string base_path, std::string user_directory_path );

    /*
     * Configure the path to the 'data' and 'gfx' directory, which contains
     * the necessary information for the application to work.
     *
     * The location of the 'data' and 'gfx' directory depends on where the
     * base path is located and the platform on which it is executed.
     *
     * Precondition:
     *      pathname["BASE_PATH"] has been initialized.
     *
     * Poscondition:
     *      pathname["DATA_DIRE"] has been initialized.
     *      pathname["GFX_DIRE"] has been initialized.
     */
    void init_data_directory( );

    /*
     * To use the route it is necessary that the route has the appropriate
     * format, the routes with the appropriate format must end with '/' in
     * the case of linux and '\' in the case of windows.
     *
     * Example:
     *
     *      LINUX:
     *      - "/usr/home/example/"  (Accepted)
     *      - "/usr/home/example"   (Rejected)
     *
     *      WINDOWS:
     *      - "C:\Programs\Example\"    (Accepted)
     *      - "C:\Programs\Example"     (Rejected)
     *
     * @brief: The method adds '/' at the end of the route if necessary.
     *
     * @param path Path to apply the format.
     * @return Formatted path.
     */
    static std::string format_path( std::string path );

public:

    /*
     * @brief: Sets the path to the user directory,
     * IF IT HAS NOT BEEN previously set.
     *
     * If the path to the user directory
     * has not been previously initialized
     * (see, when invoking the object for the first time)
     * the method will set it automatically and by default.
     *
     * Notes:
     *      1) The path to the user directory for UNIX systems is HOME.
     *      2) The path to the user directory for WINDOWS systems is APPDATA.
     *
     * Poscondition:
     *      pathname["USER_DIRE"] has been initialized.
     */
    void init_user_directory( );

    /*
     * Notes: This method needs to be called first
     * to build the object properly, as it requires
     * certain parameters.
     *
     * Static access method.
     */
    static Path &get_instance( std::string basePath, std::string userDirectoryPath );

    /*
     * Static access method.
     *
     * Precondition:
     *      The method { get_instance( string, string ) } has been called.
     */
    static Path &get_instance( );

    /*
     * TODO: Make safe method, launch an error in case the key does not exist.
     * Returns the path according to the value of the key.
     */
    std::string get_path_for_value_key( std::string valueKey );
};

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
