#include "mod_factory.h"

mod_factory::mod_factory()
{
    //ctor
}

mod_factory::~mod_factory()
{
    //dtor
}

void mod_factory::refresh_mod_list()
{
    DebugLog() << "Attempting to refresh mod list\n";
    // starts in ./data, then works through /cores and /mods directories to collect modpack information
    const std::string data_folder = "data", core_folder = "/cores", mods_folder = "/mods";

    mods.clear(); // may need to delete manually?

    // try to open data folder
    DIR *data = opendir(data_folder.c_str());

    // if data doesn't exist, we can't continue as there is nothing to load
    if (!data)
    {
        return;
    }
    // close data
    closedir(data);

    // try to load cores
    if (!load_mods_from(data_folder + core_folder))
    {

    }
    // try to load supplementary mods
    if (!load_mods_from(data_folder + mods_folder))
    {

    }
    // for debuggery purposes, print to debug log all of the mods found!
    print_debug_mods();
}

bool mod_factory::load_mods_from(std::string path)
{
    // return false on non-existance of folder path
    DIR *folder = opendir(path.c_str());

    if (!folder)
    {
        return false;
    }
    // look through folder and read in all of the subdirectories as mods
    DIR *subfolder;
    struct dirent *dp, *sdp;
    struct stat _buff;

    while ((dp = readdir(folder)))
    {
        if (stat(dp->d_name, &_buff) != 0x4)
        {
            // ignore "." and ".."
            if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
            {
                std::string subpath = path + "/" + dp->d_name;

                subfolder = opendir(subpath.c_str());
                MOD_INFORMATION *modinfo;

                // if we can't open it for some reason, abort this operation and continue
                if (!subfolder)
                {
                    continue;
                }
                // now we need to look through the folder to get the information file!
                else
                {
                    modinfo = new MOD_INFORMATION;
                    modinfo->_path = subpath;
                }
                if (!load_mod_info(modinfo))
                {
                    // malformed modinfo, continue
                    continue;
                }
                // save modinfo into storage vector
                mods.push_back(modinfo);
            }
        }
    }
    return true;
}

bool mod_factory::load_mod_info(MOD_INFORMATION *mod)
{
    // treat it as a catajson object
    const std::string modfile_name = "/modinfo.json";

    std::stringstream modfile;

    modfile << mod->path() << modfile_name;

    catajson moddata(modfile.str());

    if (!json_good())
    {
        return false;
    }
    // required, all others have defaults
    if (!moddata.has("ident") || !moddata.get("ident").is_string())
    {
        return false;
    }
    else
    {
        mod->_ident = moddata.get("ident").as_string();
    }

    // load each of the pieces of information
    if (!moddata.has("type") || !moddata.get("type").is_string())
    {
        mod->_type = MOD_UNKNOWN;
    }
    else
    {
        std::string type = moddata.get("type").as_string();
        if (type == "CORE")
        {
            mod->_type = MOD_CORE;
        }
        else if (type == "SUPPLEMENTAL")
        {
            mod->_type = MOD_SUPPLEMENTAL;
        }
        else
        {
            mod->_type = MOD_UNKNOWN;
        }
    }
    if (!moddata.has("author") || !moddata.get("author").is_string())
    {
        mod->_author = "Unknown Author";
    }
    else
    {
        mod->_author = moddata.get("author").as_string();
    }
    if (!moddata.has("name") || !moddata.get("name").is_string())
    {
        mod->_name = "No Name";
    }
    else
    {
        mod->_name = moddata.get("name").as_string();
    }

    if (!moddata.has("description") || !moddata.get("description").is_string())
    {
        mod->_desc = "No Description Available";
    }
    else
    {
        mod->_desc = moddata.get("description").as_string();
    }

    if (!moddata.has("dependencies"))
    {
        mod->_dependencies = std::vector<std::string>();
    }
    else
    {
        catajson mod_dependencies = moddata.get("dependencies");

        for (mod_dependencies.set_begin(); mod_dependencies.has_curr(); mod_dependencies.next())
        {
            if (mod_dependencies.curr().is_string())
            {
                mod->_dependencies.push_back(mod_dependencies.curr().as_string());
            }
        }
    }
}

void mod_factory::print_debug_mods()
{
    if (mods.size() == 0)
    {
        DebugLog() << "[NO MODS FOUND]\n";
    }
    for (int i = 0; i < mods.size(); ++i)
    {
        std::stringstream debugstring;

        debugstring << "***********************\n";
        debugstring << "\tName: " << mods[i]->name() << "\n";
        debugstring << "\tAuthor: " << mods[i]->author() << "\n";
        debugstring << "\tType: " << mods[i]->type() << "\n";
        debugstring << "\tIdent: " << mods[i]->ident() << "\n";
        debugstring << "\tDescription: \"" << mods[i]->desc() << "\"\n";
        debugstring << "\tMod Path: " << mods[i]->path() << "\n";
        if (mods[i]->dependencies().size() > 0)
        {
            debugstring << "\tDependencies:\n";
            std::vector<std::string> mod_dependencies = mods[i]->dependencies();
            for (int j = 0; j < mod_dependencies.size(); ++j)
            {
                debugstring << "\t\t" << mod_dependencies[j] << "\n";
            }
        }
        debugstring << "\n";

        DebugLog() << debugstring.str();
    }
}
