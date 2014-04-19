#ifndef PATH_INFO_H_INCLUDED
#define PATH_INFO_H_INCLUDED

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

extern std::map<std::string,std::string> FILENAMES;

namespace PATH_INFO {
void init_base_path(std::string path);
void init_user_dir(const char *ud = "");
void update_datadir(void);
void update_config_dir(void);
void update_pathname(std::string name, std::string path);
void set_standart_filenames(void);
}

#endif // PATH_INFO_H_INCLUDED
