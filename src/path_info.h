#ifndef PATH_INFO_H
#define PATH_INFO_H

#include <string>
#include <map>

#ifndef FILE_SEP
#if (defined _WIN32 || defined WINDOW)
#define FILE_SEP '\\'
#else
#define FILE_SEP '/'
#endif // if
#define is_filesep(ch) (ch == '/' || ch == '\\')
#endif // ifndef

extern std::map<std::string, std::string> FILENAMES;

namespace PATH_INFO
{
void init_base_path( std::string path );
void init_user_dir( const char *ud = "" );
void update_datadir( void );
void update_config_dir( void );
void update_pathname( std::string name, std::string path );
void set_standard_filenames( void );
/**
 * Return a locale specific path, or if there is no path for the current
 * locale, return the default path.
 * @param pathid The key in the @ref FILENAMES map. The local path is based
 * on that value.
 * @param extension File name extension, is automatically added to the path
 * of the translated file. Can be empty, but must otherwise include the
 * inital '.', e.g. ".json"
 * @param defaultid The path id of the fallback filename. As like pathid it's
 * the key into the @ref FILENAMES map. It is used if no translated file can be
 * found.
 */
std::string find_translated_file( const std::string &pathid, const std::string &extension,
                                  const std::string &defaultid );
}

#endif
