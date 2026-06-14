#include "ui_style_picker.h"

#include <filesystem>
#include <string>
#include <vector>

#include "filesystem.h"
#include "cata_imgui.h"
#include "cata_path.h"
#include "path_info.h"
#include "uilist.h"

static std::vector<cata_path> load_styles()
{
    const cata_path styles_path = PATH_INFO::datadir_path() / "raw" / "imgui_styles";
    std::vector<cata_path> files = get_files_from_path( ".json", styles_path, false, true );

    const cata_path custom_path = PATH_INFO::config_dir_path() / "custom_styles";
    if( dir_exist( custom_path.get_unrelative_path() ) ) {
        std::vector<cata_path> custom_files = get_files_from_path( ".json", custom_path, false, true );
        files.insert( files.end(), custom_files.begin(), custom_files.end() );
    }

    return files;
}

void style_picker::show()
{
    uilist picker;

    std::vector<cata_path> styles = load_styles();
    for( const cata_path &style : styles ) {
        picker.addentry( style.get_relative_path().filename().generic_u8string() );
    }
    picker.query();
    int selected = picker.ret;

    if( selected < 0 ) {
        return;
    }

    assure_dir_exist( PATH_INFO::config_dir() );
    copy_file( styles[selected], PATH_INFO::config_dir_path() / "imgui_style.json" );

    cataimgui::init_colors();
}
