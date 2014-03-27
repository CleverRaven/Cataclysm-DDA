#ifndef PATH_INFO_H_INCLUDED
#define PATH_INFO_H_INCLUDED

#include <string>
#include <map>

extern std::map<std::string,std::string> FILENAMES;

void init_base_path(std::string path);
void init_user_dir(const char *ud = "");
void set_standart_filenames(void);

#endif // PATH_INFO_H_INCLUDED
