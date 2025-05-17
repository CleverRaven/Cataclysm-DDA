/*
  _____              _____         _  ______  _  _        _____   _         _
 |_   _|            / ____|       (_)|  ____|(_)| |      |  __ \ (_)       | |
   | |   _ __ ___  | |  __  _   _  _ | |__    _ | |  ___ | |  | | _   __ _ | |  ___    __ _
   | |  | '_ ` _ \ | | |_ || | | || ||  __|  | || | / _ \| |  | || | / _` || | / _ \  / _` |
  _| |_ | | | | | || |__| || |_| || || |     | || ||  __/| |__| || || (_| || || (_) || (_| |
 |_____||_| |_| |_| \_____| \__,_||_||_|     |_||_| \___||_____/ |_| \__,_||_| \___/  \__, |
                                                                                       __/ |
                                                                                      |___/
                                      ___      __     ___
                                     / _ \    / /    / _ \
                             __   __| | | |  / /_   | (_) |
                             \ \ / /| | | | | '_ \   > _ <
                              \ V / | |_| |_| (_) |_| (_) |
                               \_/   \___/(_)\___/(_)\___/

GITHUB REPOT : https://github.com/aiekick/ImGuiFileDialog
DOCUMENTATION : see the attached Documentation.md

generated with "Text to ASCII Art Generator (TAAG)"
https://patorjk.com/software/taag/#p=display&h=1&v=0&f=Big&t=ImGuiFileDialog%0Av0.6.8

MIT License

Copyright (c) 2018-2024 Stephane Cuillerdier (aka aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#define IGFD_VERSION "v0.6.8"
#define IGFD_IMGUI_SUPPORTED_VERSION "1.90.5 WIP"

// Config file
#ifndef CUSTOM_IMGUIFILEDIALOG_CONFIG
#include "ImGuiFileDialogConfig.h"
#else  // CUSTOM_IMGUIFILEDIALOG_CONFIG
#include CUSTOM_IMGUIFILEDIALOG_CONFIG
#endif  // CUSTOM_IMGUIFILEDIALOG_CONFIG

// Define attributes of all API symbols declarations (e.g. for DLL under Windows)
// Using ImGuiFileDialog via a shared library is not recommended, because we don't guarantee
// backward nor forward ABI compatibility and also function call overhead. If you
// do use ImGuiFileDialog as a DLL, be sure to call ImGui::SetImGuiContext (see ImGui doc Miscellanous section).

#ifndef IGFD_API
#define IGFD_API
#endif  // IGFD_API

///////////////////////////////////////////////////////////
/////////////// FLAGS /////////////////////////////////////
///////////////////////////////////////////////////////////

// file style enum for file display (color, icon, font)
typedef int IGFD_FileStyleFlags;  // -> enum IGFD_FileStyleFlags_
enum IGFD_FileStyleFlags_         // by evaluation / priority order
{
    IGFD_FileStyle_None                 = 0,         // define none style
    IGFD_FileStyleByTypeFile            = (1 << 0),  // define style for all files
    IGFD_FileStyleByTypeDir             = (1 << 1),  // define style for all dir
    IGFD_FileStyleByTypeLink            = (1 << 2),  // define style for all link
    IGFD_FileStyleByExtention           = (1 << 3),  // define style by extention, for files or links
    IGFD_FileStyleByFullName            = (1 << 4),  // define style for particular file/dir/link full name (filename + extention)
    IGFD_FileStyleByContainedInFullName = (1 << 5),  // define style for file/dir/link when criteria is contained in full name
};

typedef int ImGuiFileDialogFlags;  // -> enum ImGuiFileDialogFlags_
enum ImGuiFileDialogFlags_ {
    ImGuiFileDialogFlags_None                              = 0,          // define none default flag
    ImGuiFileDialogFlags_ConfirmOverwrite                  = (1 << 0),   // show confirm to overwrite dialog
    ImGuiFileDialogFlags_DontShowHiddenFiles               = (1 << 1),   // dont show hidden file (file starting with a .)
    ImGuiFileDialogFlags_DisableCreateDirectoryButton      = (1 << 2),   // disable the create directory button
    ImGuiFileDialogFlags_HideColumnType                    = (1 << 3),   // hide column file type
    ImGuiFileDialogFlags_HideColumnSize                    = (1 << 4),   // hide column file size
    ImGuiFileDialogFlags_HideColumnDate                    = (1 << 5),   // hide column file date
    ImGuiFileDialogFlags_NoDialog                          = (1 << 6),   // let the dialog embedded in your own imgui begin / end scope
    ImGuiFileDialogFlags_ReadOnlyFileNameField             = (1 << 7),   // don't let user type in filename field for file open style dialogs
    ImGuiFileDialogFlags_CaseInsensitiveExtentionFiltering = (1 << 8),   // the file extentions filtering will not take into account the case
    ImGuiFileDialogFlags_Modal                             = (1 << 9),   // modal
    ImGuiFileDialogFlags_DisableThumbnailMode              = (1 << 10),  // disable the thumbnail mode
    ImGuiFileDialogFlags_DisablePlaceMode                  = (1 << 11),  // disable the place mode
    ImGuiFileDialogFlags_DisableQuickPathSelection         = (1 << 12),  // disable the quick path selection
    ImGuiFileDialogFlags_ShowDevicesButton                 = (1 << 13),  // show the devices selection button
    ImGuiFileDialogFlags_NaturalSorting                    = (1 << 14),  // enable the antural sorting for filenames and extentions, slower than standard sorting

    // default behavior when no flags is defined. seems to be the more common cases
    ImGuiFileDialogFlags_Default = ImGuiFileDialogFlags_ConfirmOverwrite |  //
                                   ImGuiFileDialogFlags_Modal |             //
                                   ImGuiFileDialogFlags_HideColumnType
};

// flags used for GetFilePathName(flag) or GetSelection(flag)
typedef int IGFD_ResultMode;  // -> enum IGFD_ResultMode_
enum IGFD_ResultMode_ {
    // IGFD_ResultMode_AddIfNoFileExt
    // add the file ext only if there is no file ext
    //   filter {.cpp,.h} with file :
    //     toto.h => toto.h
    //     toto.a.h => toto.a.h
    //     toto.a. => toto.a.cpp
    //     toto. => toto.cpp
    //     toto => toto.cpp
    //   filter {.z,.a.b} with file :
    //     toto.a.h => toto.a.h
    //     toto. => toto.z
    //     toto => toto.z
    //   filter {.g.z,.a} with file :
    //     toto.a.h => toto.a.h
    //     toto. => toto.g.z
    //     toto => toto.g.z
    IGFD_ResultMode_AddIfNoFileExt = 0,  // default

    // IGFD_ResultMode_OverwriteFileExt
    // Overwrite the file extention by the current filter :
    //   filter {.cpp,.h} with file :
    //     toto.h => toto.cpp
    //     toto.a.h => toto.a.cpp
    //     toto.a. => toto.a.cpp
    //     toto.a.h.t => toto.a.h.cpp
    //     toto. => toto.cpp
    //     toto => toto.cpp
    //   filter {.z,.a.b} with file :
    //     toto.a.h => toto.z
    //     toto.a.h.t => toto.a.z
    //     toto. => toto.z
    //     toto => toto.z
    //   filter {.g.z,.a} with file :
    //     toto.a.h => toto.g.z
    //     toto.a.h.y => toto.a.g.z
    //     toto.a. => toto.g.z
    //     toto. => toto.g.z
    //     toto => toto.g.z
    IGFD_ResultMode_OverwriteFileExt = 1,  // behavior pre IGFD v0.6.6

    // IGFD_ResultMode_KeepInputFile
    // keep the input file => no modification :
    //   filter {.cpp,.h} with file :
    //      toto.h => toto.h
    //      toto. => toto.
    //      toto => toto
    //   filter {.z,.a.b} with file :
    //      toto.a.h => toto.a.h
    //      toto. => toto.
    //      toto => toto
    //   filter {.g.z,.a} with file :
    //      toto.a.h => toto.a.h
    //      toto. => toto.
    //      toto => toto
    IGFD_ResultMode_KeepInputFile = 2
};

///////////////////////////////////////////////////////////
/////////////// STRUCTS ///////////////////////////////////
///////////////////////////////////////////////////////////

#ifdef USE_THUMBNAILS
struct IGFD_Thumbnail_Info {
    int isReadyToDisplay            = 0;  // ready to be rendered, so texture created
    int isReadyToUpload             = 0;  // ready to upload to gpu
    int isLoadingOrLoaded           = 0;  // was sent to laoding or loaded
    int textureWidth                = 0;  // width of the texture to upload
    int textureHeight               = 0;  // height of the texture to upload
    int textureChannels             = 0;  // count channels of the texture to upload
    unsigned char* textureFileDatas = 0;  // file texture datas, will be rested to null after gpu upload
    void* textureID                 = 0;  // 2d texture id (void* is like ImtextureID type) (GL, DX, VK, Etc..)
    void* userDatas                 = 0;  // user datas
};
#endif  // USE_THUMBNAILS

// stdint is used for cpp and c apî (cstdint is only for cpp)
#include <stdint.h>

#ifdef __cplusplus

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif  // IMGUI_DEFINE_MATH_OPERATORS

#ifdef IMGUI_INCLUDE
#include IMGUI_INCLUDE
#else  // IMGUI_INCLUDE
#include <imgui.h>
#endif  // IMGUI_INCLUDE

#include <set>
#include <map>
#include <list>
#include <regex>
#include <array>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cfloat>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <functional>
#include <unordered_map>

#ifndef defaultSortField
#define defaultSortField FIELD_FILENAME
#endif  // defaultSortField

#ifndef defaultSortOrderFilename
#define defaultSortOrderFilename true
#endif  // defaultSortOrderFilename
#ifndef defaultSortOrderType
#define defaultSortOrderType true
#endif  // defaultSortOrderType
#ifndef defaultSortOrderSize
#define defaultSortOrderSize true
#endif  // defaultSortOrderSize
#ifndef defaultSortOrderDate
#define defaultSortOrderDate true
#endif  // defaultSortOrderDate
#ifndef defaultSortOrderThumbnails
#define defaultSortOrderThumbnails true
#endif  // defaultSortOrderThumbnails

#ifndef MAX_FILE_DIALOG_NAME_BUFFER
#define MAX_FILE_DIALOG_NAME_BUFFER 1024
#endif  // MAX_FILE_DIALOG_NAME_BUFFER

#ifndef MAX_PATH_BUFFER_SIZE
#define MAX_PATH_BUFFER_SIZE 1024
#endif  // MAX_PATH_BUFFER_SIZE

#ifndef EXT_MAX_LEVEL
#define EXT_MAX_LEVEL 10U
#endif  // EXT_MAX_LEVEL

namespace IGFD {

template <typename T>
class SearchableVector {
private:
    std::unordered_map<T, size_t> m_Dico;
    std::vector<T> m_Array;

public:
    void clear() {
        m_Dico.clear();
        m_Array.clear();
    }
    bool empty() const {
        return m_Array.empty();
    }
    size_t size() const {
        return m_Array.size();
    }
    T& operator[](const size_t& vIdx) {
        return m_Array[vIdx];
    }
    T& at(const size_t& vIdx) {
        return m_Array.at(vIdx);
    }
    typename std::vector<T>::iterator begin() {
        return m_Array.begin();
    }
    typename std::vector<T>::const_iterator begin() const {
        return m_Array.begin();
    }
    typename std::vector<T>::iterator end() {
        return m_Array.end();
    }
    typename std::vector<T>::const_iterator end() const {
        return m_Array.end();
    }

    bool try_add(T vKey) {
        if (!exist(vKey)) {
            m_Dico[vKey] = m_Array.size();
            m_Array.push_back(vKey);
            return true;
        }
        return false;
    }

    bool try_set_existing(T vKey) {
        if (exist(vKey)) {
            auto row     = m_Dico.at(vKey);
            m_Array[row] = vKey;
            return true;
        }
        return false;
    }

    bool exist(const std::string& vKey) const {
        return (m_Dico.find(vKey) != m_Dico.end());
    }
};

class IGFD_API FileInfos;
class IGFD_API FileDialogInternal;

class IGFD_API Utils {
public:
    struct PathStruct {
        std::string path;
        std::string name;
        std::string ext;
        bool isOk = false;
    };

public:
    static bool ImSplitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
    static bool ReplaceString(std::string& str, const std::string& oldStr, const std::string& newStr, const size_t& vMaxRecursion = 10U);
    static void AppendToBuffer(char* vBuffer, size_t vBufferLen, const std::string& vStr);
    static void ResetBuffer(char* vBuffer);
    static void SetBuffer(char* vBuffer, size_t vBufferLen, const std::string& vStr);
    static std::string UTF8Encode(const std::wstring& wstr);
    static std::wstring UTF8Decode(const std::string& str);
    static std::vector<std::string> SplitStringToVector(const std::string& vText, const std::string& vDelimiterPattern, const bool vPushEmpty);
    static std::vector<std::string> SplitStringToVector(const std::string& vText, const char& vDelimiter, const bool vPushEmpty);
    static std::string LowerCaseString(const std::string& vString);  // turn all text in lower case for search facilitie
    static size_t GetCharCountInString(const std::string& vString, const char& vChar);
    static size_t GetLastCharPosWithMinCharCount(const std::string& vString, const char& vChar, const size_t& vMinCharCount);
    static std::string GetPathSeparator();                                                                              // return the slash for any OS ( \\ win, / unix)
    static std::string RoundNumber(double vvalue, int n);                                                               // custom rounding number
    static std::string FormatFileSize(size_t vByteSize);                                                                // format file size field
    static bool NaturalCompare(const std::string& vA, const std::string& vB, bool vInsensitiveCase, bool vDescending);  // natural sorting

#ifdef NEED_TO_BE_PUBLIC_FOR_TESTS
public:
#else
private:
#endif
    static bool M_IsAValidCharExt(const char& c);
    static bool M_IsAValidCharSuffix(const char& c);
    static bool M_ExtractNumFromStringAtPos(const std::string& str, size_t& pos, double& vOutNum);
};

class IGFD_API FileStyle {
public:
    typedef std::function<bool(const FileInfos&, FileStyle&)> FileStyleFunctor;

public:
    ImVec4 color = ImVec4(0, 0, 0, 0);
    std::string icon;
    ImFont* font              = nullptr;
    IGFD_FileStyleFlags flags = 0;

public:
    FileStyle();
    FileStyle(const FileStyle& vStyle);
    FileStyle(const ImVec4& vColor, const std::string& vIcon = "", ImFont* vFont = nullptr);
};

class IGFD_API SearchManager {
public:
    std::string searchTag;
    char searchBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";
    bool searchInputIsActive                       = false;

public:
    void Clear();                                                 // clear datas
    void DrawSearchBar(FileDialogInternal& vFileDialogInternal);  // draw the search bar
};

class IGFD_API FilterInfos {
private:
    // just for return a default const std::string& in getFirstFilter.
    // cannot be const, because FilterInfos must be affected to an another FilterInfos
    // must stay empty all time
    std::string empty_string;

public:
    std::string title;                                // displayed filter.can be different than rela filter
    SearchableVector<std::string> filters;            // filters
    SearchableVector<std::string> filters_optimized;  // optimized filters for case insensitive search
    std::vector<std::regex> filters_regex;            // collection of regex filter type
    size_t count_dots = 0U;                           // the max count dot the max per filter of all filters

public:
    void clear();                                                                        // clear the datas
    bool empty() const;                                                                  // is filter empty
    const std::string& getFirstFilter() const;                                           // get the first filter
    bool regexExist(const std::string& vFilter) const;                                   // is regex filter exist
    bool exist(const FileInfos& vFileInfos, bool vIsCaseInsensitive) const;              // is filter exist
    void setCollectionTitle(const std::string& vTitle);                                  // set the collection title
    void addFilter(const std::string& vFilter, const bool vIsRegex);                    // add a filter
    void addCollectionFilter(const std::string& vFilter, const bool vIsRegex);          // add a filter in collection
    static std::string transformAsteriskBasedFilterToRegex(const std::string& vFilter);  // will transform a filter who contain * to a regex
};

class IGFD_API FilterManager {
#ifdef NEED_TO_BE_PUBLIC_FOR_TESTS
public:
#else
private:
#endif
    std::vector<FilterInfos> m_ParsedFilters;
    std::unordered_map<IGFD_FileStyleFlags, std::unordered_map<std::string, std::shared_ptr<FileStyle> > > m_FilesStyle;  // file infos for file
                                                                                                                          // extention only
    std::vector<FileStyle::FileStyleFunctor> m_FilesStyleFunctors;                                                        // file style via lambda function
    FilterInfos m_SelectedFilter;

public:
    std::string dLGFilters;
    std::string dLGdefaultExt;

public:
    const FilterInfos& GetSelectedFilter() const;
    void ParseFilters(const char* vFilters);                                                               // Parse filter syntax, detect and parse filter collection
    void SetSelectedFilterWithExt(const std::string& vFilter);                                             // Select filter
    bool FillFileStyle(std::shared_ptr<FileInfos> vFileInfos) const;                                       // fill with the good style
    void SetFileStyle(const IGFD_FileStyleFlags& vFlags, const char* vCriteria, const FileStyle& vInfos);  // Set FileStyle
    void SetFileStyle(const IGFD_FileStyleFlags& vFlags, const char* vCriteria, const ImVec4& vColor, const std::string& vIcon,
                      ImFont* vFont);                         // link file style to Color and Icon and Font
    void SetFileStyle(FileStyle::FileStyleFunctor vFunctor);  // lambda functor for set file style.
    bool GetFileStyle(const IGFD_FileStyleFlags& vFlags, const std::string& vCriteria, ImVec4* vOutColor, std::string* vOutIcon,
                      ImFont** vOutFont);  // Get Color and Icon for Filter
    void ClearFilesStyle();                // clear m_FileStyle
    bool IsCoveredByFilters(const FileInfos& vFileInfos,
                            bool vIsCaseInsensitive) const;            // check if current file extention (vExt) is covered by current filter, or by regex (vNameExt)
    float GetFilterComboBoxWidth() const;                              // will return the current combo box widget width
    bool DrawFilterComboBox(FileDialogInternal& vFileDialogInternal);  // draw the filter combobox 	// get the current selected filter
    std::string ReplaceExtentionWithCurrentFilterIfNeeded(const std::string& vFileName,
                                                          IGFD_ResultMode vFlag) const;  // replace the extention of the current file by the selected filter
    void SetDefaultFilterIfNotDefined();                                                 // define the first filter if no filter is selected
};

class IGFD_API FileType {
public:
    enum class ContentType {
        // The ordering will be used during sort.
        Invalid       = -1,
        Directory     = 0,
        File          = 1,
        LinkToUnknown = 2,  // link to something that is not a regular file or directory.
    };

private:
    ContentType m_Content = ContentType::Invalid;
    bool m_Symlink        = false;

public:
    FileType();
    FileType(const ContentType& vContentType, const bool vIsSymlink);

    void SetContent(const ContentType& vContentType);
    void SetSymLink(const bool vIsSymlink);

    bool isValid() const;
    bool isDir() const;
    bool isFile() const;
    bool isLinkToUnknown() const;
    bool isSymLink() const;

    // Comparisons only care about the content type, ignoring whether it's a symlink or not.
    bool operator==(const FileType& rhs) const;
    bool operator!=(const FileType& rhs) const;
    bool operator<(const FileType& rhs) const;
    bool operator>(const FileType& rhs) const;
};

class IGFD_API FileInfos {
public:
    static std::shared_ptr<FileInfos> create();

public:
    // extention of the file, the array is the levels of ext, by ex : .a.b.c, will be save in {.a.b.c, .b.c, .c}
    // 10 level max are sufficient i guess. the others levels will be checked if countExtDot > 1
    std::array<std::string, EXT_MAX_LEVEL> fileExtLevels;
    std::array<std::string, EXT_MAX_LEVEL> fileExtLevels_optimized;  // optimized for search => insensitivecase
    // same for file name, can be sued in userFileAttributesFun
    std::array<std::string, EXT_MAX_LEVEL> fileNameLevels;
    std::array<std::string, EXT_MAX_LEVEL> fileNameLevels_optimized;  // optimized for search => insensitivecase
    size_t countExtDot = 0U;                                          // count dots in file extention. this count will give the levels in fileExtLevels
    FileType fileType;                                                // fileType
    std::string filePath;                                             // path of the file
    std::string fileName;                                             // file name only
    std::string fileNameExt;                                          // filename of the file (file name + extention) (but no path)
    std::string fileNameExt_optimized;                                // optimized for search => insensitivecase
    std::string deviceInfos;                                          // quick infos to display after name for devices
    std::string tooltipMessage;                                       // message to display on the tooltip, is not empty
    int32_t tooltipColumn = -1;                                       // the tooltip will appears only when the mouse is over the tooltipColumn if > -1
    size_t fileSize       = 0U;                                       // for sorting operations
    std::string formatedFileSize;                                     // file size formated (10 o, 10 ko, 10 mo, 10 go)
    std::string fileModifDate;                                        // file user defined format of the date (data + time by default)
    std::shared_ptr<FileStyle> fileStyle = nullptr;                   // style of the file
#ifdef USE_THUMBNAILS
    IGFD_Thumbnail_Info thumbnailInfo;  // structre for the display for image file tetxure
#endif                                  // USE_THUMBNAILS

public:
    bool SearchForTag(const std::string& vTag) const;  // will search a tag in fileNameExt and fileNameExt_optimized
    bool SearchForExt(const std::string& vExt, const bool vIsCaseInsensitive,
                      const size_t& vMaxLevel = EXT_MAX_LEVEL) const;  // will check the fileExtLevels levels for vExt, until vMaxLevel
    bool SearchForExts(const std::string& vComaSepExts, const bool vIsCaseInsensitive,
                       const size_t& vMaxLevel = EXT_MAX_LEVEL) const;  // will check the fileExtLevels levels for vExts (ext are coma separated), until vMaxLevel
    bool FinalizeFileTypeParsing(const size_t& vMaxDotToExtract);       // finalize the parsing the file (only a file or link to file. no dir)
};

typedef std::pair<std::string, std::string> PathDisplayedName;

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    // say if a directory can be openened or for any reason locked
    virtual bool IsDirectoryCanBeOpened(const std::string& vName) = 0;
    // say if a directory exist
    virtual bool IsDirectoryExist(const std::string& vName) = 0;
    // say if a file exist
    virtual bool IsFileExist(const std::string& vName) = 0;
    // say if a directory was created, return false if vName is invalid or alreayd exist
    virtual bool CreateDirectoryIfNotExist(const std::string& vName) = 0;
    // extract the component of a file path name, like path, name, ext
    virtual IGFD::Utils::PathStruct ParsePathFileName(const std::string& vPathFileName) = 0;
    // will return a list of files inside a path
    virtual std::vector<IGFD::FileInfos> ScanDirectory(const std::string& vPath) = 0;
    // say if the path is well a directory
    virtual bool IsDirectory(const std::string& vFilePathName) = 0;
    // return a device list (<path, device name>) on windows, but can be used on other platforms for give to the user a list of devices paths.
    virtual std::vector<IGFD::PathDisplayedName> GetDevicesList() = 0;
};

class IGFD_API FileManager {
public:                            // types
    enum class SortingFieldEnum {  // sorting for filetering of the file lsit
        FIELD_NONE = 0,            // no sorting reference, result indetermined haha..
        FIELD_FILENAME,            // sorted by filename
        FIELD_TYPE,                // sorted by filetype
        FIELD_SIZE,                // sorted by filesize (formated file size)
        FIELD_DATE,                // sorted by filedate
        FIELD_THUMBNAILS,          // sorted by thumbnails (comparaison by width then by height)
    };

#ifdef NEED_TO_BE_PUBLIC_FOR_TESTS
public:
#else
private:
#endif
    std::string m_CurrentPath;                                    // current path (to be decomposed in m_CurrentPathDecomposition
    std::vector<std::string> m_CurrentPathDecomposition;          // part words
    std::vector<std::shared_ptr<FileInfos> > m_FileList;          // base container
    std::vector<std::shared_ptr<FileInfos> > m_FilteredFileList;  // filtered container (search, sorting, etc..)
    std::vector<std::shared_ptr<FileInfos> > m_PathList;          // base container for path selection
    std::vector<std::shared_ptr<FileInfos> > m_FilteredPathList;  // filtered container for path selection (search, sorting, etc..)
    std::vector<std::string>::iterator m_PopupComposedPath;       // iterator on m_CurrentPathDecomposition for Current Path popup
    std::string m_LastSelectedFileName;                           // for shift multi selection
    std::set<std::string> m_SelectedFileNames;                    // the user selection of FilePathNames
    bool m_CreateDirectoryMode = false;                           // for create directory widget
    std::string m_FileSystemName;
    std::unique_ptr<IFileSystem> m_FileSystemPtr = nullptr;

public:
    bool inputPathActivated                               = false;  // show input for path edition
    bool devicesClicked                                   = false;  // event when a drive button is clicked
    bool pathClicked                                      = false;  // event when a path button was clicked
    char inputPathBuffer[MAX_PATH_BUFFER_SIZE]            = "";     // input path buffer for imgui widget input text (displayed in palce of composer)
    char variadicBuffer[MAX_FILE_DIALOG_NAME_BUFFER]      = "";     // called by m_SelectableItem
    char fileNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER]      = "";     // file name buffer in footer for imgui widget input text
    char directoryNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";     // directory name buffer (when in directory mode)
    std::string headerFileName;                                     // detail view name of column file
    std::string headerFileType;                                     // detail view name of column type
    std::string headerFileSize;                                     // detail view name of column size
    std::string headerFileDate;                                     // detail view name of column date + time
#ifdef USE_THUMBNAILS
    std::string headerFileThumbnails;  // detail view name of column thumbnails
    bool sortingDirection[5] = {       // true => Ascending, false => Descending
        defaultSortOrderFilename, defaultSortOrderType, defaultSortOrderSize, defaultSortOrderDate, defaultSortOrderThumbnails};
#else
    bool sortingDirection[4] = {  // true => Ascending, false => Descending
        defaultSortOrderFilename, defaultSortOrderType, defaultSortOrderSize, defaultSortOrderDate};
#endif
    SortingFieldEnum sortingField = SortingFieldEnum::FIELD_FILENAME;  // detail view sorting column
    bool showDevices              = false;                             // devices are shown (only on os windows)

    std::string dLGpath;                  // base path set by user when OpenDialog was called
    std::string dLGDefaultFileName;       // base default file path name set by user when OpenDialog was called
    size_t dLGcountSelectionMax = 1U;     // 0 for infinite				// base max selection count set by user when OpenDialog was called
    bool dLGDirectoryMode       = false;  // is directory mode (defiend like : dLGDirectoryMode = (filters.empty()))

    std::string fsRoot;

#ifdef NEED_TO_BE_PUBLIC_FOR_TESTS
public:
#else
private:
#endif
    static void m_CompleteFileInfos(const std::shared_ptr<FileInfos>& vInfos);                    // set time and date infos of a file (detail view mode)
    void m_RemoveFileNameInSelection(const std::string& vFileName);                               // selection : remove a file name
    void m_AddFileNameInSelection(const std::string& vFileName, bool vSetLastSelectionFileName);  // selection : add a file name
    void m_AddFile(const FileDialogInternal& vFileDialogInternal, const std::string& vPath, const std::string& vFileName,
                   const FileType& vFileType);  // add file called by scandir
    void m_AddPath(const FileDialogInternal& vFileDialogInternal, const std::string& vPath, const std::string& vFileName,
                   const FileType& vFileType);  // add file called by scandir
    void m_ScanDirForPathSelection(const FileDialogInternal& vFileDialogInternal,
                                   const std::string& vPath);  // scan the directory for retrieve the path list
    void m_OpenPathPopup(const FileDialogInternal& vFileDialogInternal,
                         std::vector<std::string>::iterator vPathIter);   // open the popup list of paths
    void m_SetCurrentPath(std::vector<std::string>::iterator vPathIter);  // set the current path, update the path bar
    void m_ApplyFilteringOnFileList(const FileDialogInternal& vFileDialogInternal, std::vector<std::shared_ptr<FileInfos> >& vFileInfosList, std::vector<std::shared_ptr<FileInfos> >& vFileInfosFilteredList);
    static bool M_SortStrings(const FileDialogInternal& vFileDialogInternal,             //
                              const bool vInsensitiveCase, const bool vDescendingOrder,  //
                              const std::string& vA, const std::string& vB);
    void m_SortFields(const FileDialogInternal& vFileDialogInternal, std::vector<std::shared_ptr<FileInfos> >& vFileInfosList,
                      std::vector<std::shared_ptr<FileInfos> >& vFileInfosFilteredList);  // will sort a column
    bool m_CompleteFileInfosWithUserFileAttirbutes(const FileDialogInternal& vFileDialogInternal, const std::shared_ptr<FileInfos>& vInfos);

public:
    FileManager();
    bool IsComposerEmpty() const;
    size_t GetComposerSize() const;
    bool IsFileListEmpty() const;
    bool IsPathListEmpty() const;
    bool IsFilteredListEmpty() const;
    bool IsPathFilteredListEmpty() const;
    size_t GetFullFileListSize() const;
    std::shared_ptr<FileInfos> GetFullFileAt(size_t vIdx);
    size_t GetFilteredListSize() const;
    size_t GetPathFilteredListSize() const;
    std::shared_ptr<FileInfos> GetFilteredFileAt(size_t vIdx);
    std::shared_ptr<FileInfos> GetFilteredPathAt(size_t vIdx);
    std::vector<std::string>::iterator GetCurrentPopupComposedPath() const;
    bool IsFileNameSelected(const std::string& vFileName);
    std::string GetBack();
    void ClearComposer();
    void ClearFileLists();  // clear file list, will destroy thumbnail textures
    void ClearPathLists();  // clear path list, will destroy thumbnail textures
    void ClearAll();
    void ApplyFilteringOnFileList(const FileDialogInternal& vFileDialogInternal);
    void SortFields(const FileDialogInternal& vFileDialogInternal);        // will sort a column
    void OpenCurrentPath(const FileDialogInternal& vFileDialogInternal);   // set the path of the dialog, will launch the scandir for populate the file listview
    bool GetDevices();                                                     // list devices
    bool CreateDir(const std::string& vPath);                              // create a directory on the file system
    std::string ComposeNewPath(std::vector<std::string>::iterator vIter);  // compose a path from the compose path widget
    bool SetPathOnParentDirectoryIfAny();                                  // compose paht on parent directory
    std::string GetCurrentPath();                                          // get the current path
    void SetCurrentPath(const std::string& vCurrentPath);                  // set the current path
    void SetDefaultFileName(const std::string& vFileName);
    bool SelectDirectory(const std::shared_ptr<FileInfos>& vInfos);  // enter directory
    void SelectAllFileNames();
    void SelectFileName(const std::shared_ptr<FileInfos>& vInfos);            // add a filename in selection
    void SelectOrDeselectFileName(const FileDialogInternal& vFileDialogInternal, const std::shared_ptr<FileInfos>& vInfos);  // add/remove a filename in selection
    void SetCurrentDir(const std::string& vPath);                                                                            // define current directory for scan
    void ScanDir(const FileDialogInternal& vFileDialogInternal,
                 const std::string& vPath);  // scan the directory for retrieve the file list
    std::string GetResultingPath();
    std::string GetResultingFileName(FileDialogInternal& vFileDialogInternal, IGFD_ResultMode vFlag);
    std::string GetResultingFilePathName(FileDialogInternal& vFileDialogInternal, IGFD_ResultMode vFlag);
    std::map<std::string, std::string> GetResultingSelection(FileDialogInternal& vFileDialogInternal, IGFD_ResultMode vFlag);

    void DrawDirectoryCreation(const FileDialogInternal& vFileDialogInternal);  // draw directory creation widget
    void DrawPathComposer(const FileDialogInternal& vFileDialogInternal);

    IFileSystem* GetFileSystemInstance() const {
        return m_FileSystemPtr.get();
    }
    const std::string& GetFileSystemName() const {
        return m_FileSystemName;
    }
};

typedef void* UserDatas;
typedef std::function<void(const char*, UserDatas, bool*)> PaneFun;        // side pane function binding
typedef std::function<bool(FileInfos*, UserDatas)> UserFileAttributesFun;  // custom file Attributes call back, reject file if false

struct IGFD_API FileDialogConfig {
    std::string path;                                        // path
    std::string fileName;                                    // defaut file name
    std::string filePathName;                                // if not empty, the filename and the path will be obtained from filePathName
    int32_t countSelectionMax  = 1;                          // count selection max, 0 for infinite
    UserDatas userDatas        = nullptr;                    // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags flags = ImGuiFileDialogFlags_None;  // ImGuiFileDialogFlags
    PaneFun sidePane;                                        // side pane callback
    float sidePaneWidth = 250.0f;                            // side pane width
    UserFileAttributesFun userFileAttributes;                // user file Attibutes callback
};

class IGFD_API FileDialogInternal {
public:
    FileManager fileManager;      // the file manager
    FilterManager filterManager;  // the filter manager
    SearchManager searchManager;  // the search manager

public:
    std::string name;                          // the internal dialog name (title + ##word)
    bool showDialog           = false;         // the dialog is shown
    ImVec2 dialogCenterPos    = ImVec2(0, 0);  // center pos for display the confirm overwrite dialog
    int lastImGuiFrameCount   = 0;             // to be sure than only one dialog displayed per frame
    float footerHeight        = 0.0f;          // footer height
    bool canWeContinue        = true;          // events
    bool okResultToConfirm    = false;         // to confim if ok for OverWrite
    bool isOk                 = false;         // is dialog ok button click
    bool fileInputIsActive    = false;         // when input text for file or directory is active
    bool fileListViewIsActive = false;         // when list view is active
    std::string dLGkey;                        // the dialog key
    std::string dLGtitle;                      // the dialog title
    bool needToExitDialog  = false;            // we need to exit the dialog
    bool puUseCustomLocale = false;            // custom user locale
    int localeCategory     = LC_ALL;           // locale category to use
    std::string localeBegin;                   // the locale who will be applied at start of the display dialog
    std::string localeEnd;                     // the locale who will be applaied at end of the display dialog

private:
    FileDialogConfig m_DialogConfig;

public:
    void NewFrame();           // new frame, so maybe neded to do somethings, like reset events
    void EndFrame();           // end frame, so maybe neded to do somethings fater all
    void ResetForNewDialog();  // reset what is needed to reset for the openging of a new dialog

    void configureDialog(                  // open simple dialog
        const std::string& vKey,           // key dialog
        const std::string& vTitle,         // title
        const char* vFilters,              // filters, if null, will display only directories
        const FileDialogConfig& vConfig);  // FileDialogConfig
    const FileDialogConfig& getDialogConfig() const;
    FileDialogConfig& getDialogConfigRef();
};

class IGFD_API ThumbnailFeature {
protected:
    ThumbnailFeature();
    ~ThumbnailFeature();

    void m_NewThumbnailFrame(FileDialogInternal& vFileDialogInternal);
    void m_EndThumbnailFrame(FileDialogInternal& vFileDialogInternal);
    void m_QuitThumbnailFrame(FileDialogInternal& vFileDialogInternal);

#ifdef USE_THUMBNAILS
public:
    typedef std::function<void(IGFD_Thumbnail_Info*)> CreateThumbnailFun;   // texture 2d creation function binding
    typedef std::function<void(IGFD_Thumbnail_Info*)> DestroyThumbnailFun;  // texture 2d destroy function binding

protected:
    enum class DisplayModeEnum { FILE_LIST = 0, THUMBNAILS_LIST, THUMBNAILS_GRID };

private:
    uint32_t m_CountFiles                                    = 0U;
    bool m_IsWorking                                         = false;
    std::shared_ptr<std::thread> m_ThumbnailGenerationThread = nullptr;
    std::list<std::shared_ptr<FileInfos> > m_ThumbnailFileDatasToGet;  // base container
    std::mutex m_ThumbnailFileDatasToGetMutex;
    std::condition_variable m_ThumbnailFileDatasToGetCv;
    std::list<std::shared_ptr<FileInfos> > m_ThumbnailToCreate;  // base container
    std::mutex m_ThumbnailToCreateMutex;
    std::list<IGFD_Thumbnail_Info> m_ThumbnailToDestroy;  // base container
    std::mutex m_ThumbnailToDestroyMutex;

    CreateThumbnailFun m_CreateThumbnailFun   = nullptr;
    DestroyThumbnailFun m_DestroyThumbnailFun = nullptr;

protected:
    DisplayModeEnum m_DisplayMode = DisplayModeEnum::FILE_LIST;

private:
    void m_VariadicProgressBar(float fraction, const ImVec2& size_arg, const char* fmt, ...);

protected:
    // will be call in cpu zone (imgui computations, will call a texture file retrieval thread)
    void m_StartThumbnailFileDatasExtraction();                               // start the thread who will get byte buffer from image files
    bool m_StopThumbnailFileDatasExtraction();                                // stop the thread who will get byte buffer from image files
    void m_ThreadThumbnailFileDatasExtractionFunc();                          // the thread who will get byte buffer from image files
    void m_DrawThumbnailGenerationProgress();                                 // a little progressbar who will display the texture gen status
    void m_AddThumbnailToLoad(const std::shared_ptr<FileInfos>& vFileInfos);  // add texture to load in the thread
    void m_AddThumbnailToCreate(const std::shared_ptr<FileInfos>& vFileInfos);
    void m_AddThumbnailToDestroy(const IGFD_Thumbnail_Info& vIGFD_Thumbnail_Info);
    void m_DrawDisplayModeToolBar();  // draw display mode toolbar (file list, thumbnails list, small thumbnails grid, big thumbnails grid)
    void m_ClearThumbnails(FileDialogInternal& vFileDialogInternal);

public:
    void SetCreateThumbnailCallback(const CreateThumbnailFun& vCreateThumbnailFun);
    void SetDestroyThumbnailCallback(const DestroyThumbnailFun& vCreateThumbnailFun);

    // must be call in gpu zone (rendering, possibly one rendering thread)
    void ManageGPUThumbnails();  // in gpu rendering zone, whill create or destroy texture
#endif
};

class IGFD_API PlacesFeature {
protected:
    PlacesFeature();

#ifdef USE_PLACES_FEATURE
private:
    struct PlaceStruct {
        std::string name;  // name of the place
        // todo: the path could be relative, better if the app is moved but place path can be outside of the app
        std::string path;        // absolute path of the place
        bool canBeSaved = true;  // defined by code, can be used for prevent serialization / deserialization
        FileStyle style;
        float thickness = 0.0f;  // when more than 0.0f, is a separator
    };

    struct GroupStruct {
        bool canBeSaved                              = false;  // defined by code, can be used for prevent serialization / deserialization
        size_t displayOrder                          = 0U;     // the display order will be usedf first, then alphanumeric
        bool defaultOpened                           = false;  // the group is opened by default
        bool canBeEdited                             = false;  // will show +/- button for add/remove place in the group
        char editBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";     // temp buffer for name edition
        int32_t selectedPlaceForEdition              = -1;
        ImGuiTreeNodeFlags collapsingHeaderFlag      = ImGuiTreeNodeFlags_None;
        ImGuiListClipper clipper;           // the list clipper of the grou
        std::string name;                   // the group name, will be displayed
        std::vector<PlaceStruct> places;    // the places (name + path)
        bool AddPlace(                      // add a place by code
            const std::string& vPlaceName,  // place name
            const std::string& vPlacePath,  // place path
            const bool vCanBeSaved,        // prevent serialization
            const FileStyle& vStyle = {});  // style
        void AddPlaceSeparator(const float& vThickness = 1.0f);
        bool RemovePlace(                    // remove a place by code, return true if succeed
            const std::string& vPlaceName);  // place name to remove
    };

private:
    std::unordered_map<std::string, std::shared_ptr<GroupStruct> > m_Groups;
    std::map<size_t, std::weak_ptr<GroupStruct> > m_OrderedGroups;

protected:
    float m_PlacesPaneWidth = 200.0f;
    bool m_PlacesPaneShown  = false;

protected:
    void m_InitPlaces(FileDialogInternal& vFileDialogInternal);
    void m_DrawPlacesButton();                                                            // draw place button
    bool m_DrawPlacesPane(FileDialogInternal& vFileDialogInternal, const ImVec2& vSize);  // draw place Pane

public:
    std::string SerializePlaces(                                    // serialize place : return place buffer to save in a file
        const bool vForceSerialisationForAll = true);              // for avoid serialization of places with flag 'canBeSaved to false'
    void DeserializePlaces(                                         // deserialize place : load place buffer to load in the dialog (saved from
        const std::string& vPlaces);                                // previous use with SerializePlaces()) place buffer to load
    bool AddPlacesGroup(                                            // add a group
        const std::string& vGroupName,                              // the group name
        const size_t& vDisplayOrder,                                // the display roder of the group
        const bool vCanBeEdited     = false,                       // let the user add/remove place in the group
        const bool vOpenedByDefault = true);                       // hte group is opened by default
    bool RemovePlacesGroup(const std::string& vGroupName);          // remove the group
    GroupStruct* GetPlacesGroupPtr(const std::string& vGroupName);  // get the group, if not existed, will be created
#endif                                                              // USE_PLACES_FEATURE
};

// file localization by input chat // widget flashing
class IGFD_API KeyExplorerFeature {
protected:
    KeyExplorerFeature();

#ifdef USE_EXPLORATION_BY_KEYS
private:
    bool m_LocateFileByInputChar_lastFound               = false;
    ImWchar m_LocateFileByInputChar_lastChar             = 0;
    float m_FlashAlpha                                   = 0.0f;  // flash when select by char
    float m_FlashAlphaAttenInSecs                        = 1.0f;  // fps display dependant
    int m_LocateFileByInputChar_InputQueueCharactersSize = 0;
    size_t m_FlashedItem                                 = 0;  // flash when select by char
    size_t m_LocateFileByInputChar_lastFileIdx           = 0;

protected:
    void m_LocateByInputKey(FileDialogInternal& vFileDialogInternal);  // select a file line in listview according to char key
    bool m_LocateItem_Loop(FileDialogInternal& vFileDialogInternal,
                           ImWchar vC);  // restrat for start of list view if not found a corresponding file
    void m_ExploreWithkeys(FileDialogInternal& vFileDialogInternal,
                           ImGuiID vListViewID);  // select file/directory line in listview accroding to up/down enter/backspace keys
    void m_StartFlashItem(size_t vIdx);           // define than an item must be flashed
    bool m_BeginFlashItem(size_t vIdx);           // start the flashing of a line in lsit view
    static void m_EndFlashItem();                 // end the fleshing accrdoin to var m_FlashAlphaAttenInSecs
    static bool m_FlashableSelectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, bool vFlashing = false,
                                      const ImVec2& size = ImVec2(0, 0));  // custom flashing selectable widgets, for flash the selected line in a short time

public:
    void SetFlashingAttenuationInSeconds(  // set the flashing time of the line in file list when use exploration keys
        float vAttenValue);                // set the attenuation (from flashed to not flashed) in seconds
#endif                                     // USE_EXPLORATION_BY_KEYS
};

class IGFD_API FileDialog : public PlacesFeature, public KeyExplorerFeature, public ThumbnailFeature {
protected:
    FileDialogInternal m_FileDialogInternal;
    ImGuiListClipper m_FileListClipper;
    ImGuiListClipper m_PathListClipper;
    float prOkCancelButtonWidth = 0.0f;
    ImGuiWindowFlags m_CurrentDisplayedFlags;

public:
    // Singleton for easier accces form anywhere but only one dialog at a time
    // vCopy or vForce can be used for share a memory pointer in a new memory space like a dll module
    static FileDialog* Instance(FileDialog* vCopy = nullptr, bool vForce = false) {
        static FileDialog _instance;
        static FileDialog* _instance_copy = nullptr;
        if (vCopy || vForce) {
            _instance_copy = vCopy;
        }
        if (_instance_copy) {
            return _instance_copy;
        }
        return &_instance;
    }

public:
    FileDialog();           // ImGuiFileDialog Constructor. can be used for have many dialog at same time (not possible with singleton)
    virtual ~FileDialog();  // ImGuiFileDialog Destructor

    //  standard dialog
    virtual void OpenDialog(                    // open simple dialog
        const std::string& vKey,                // key dialog
        const std::string& vTitle,              // title
        const char* vFilters,                   // filters, if null, will display only directories
        const FileDialogConfig& vConfig = {});  // FileDialogConfig

    // Display / Close dialog form
    bool Display(                                               // Display the dialog. return true if a result was obtained (Ok or not)
        const std::string& vKey,                                // key dialog to display (if not the same key as defined by OpenDialog => no opening)
        ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse,  // ImGuiWindowFlags
        ImVec2 vMinSize         = ImVec2(0, 0),                 // mininmal size contraint for the ImGuiWindow
        ImVec2 vMaxSize         = ImVec2(FLT_MAX, FLT_MAX));    // maximal size contraint for the ImGuiWindow

    void Close();  // close dialog

    // queries
    bool WasOpenedThisFrame(const std::string& vKey) const;  // say if the dialog key was already opened this frame
    bool WasOpenedThisFrame() const;                         // say if the dialog was already opened this frame
    bool IsOpened(const std::string& vKey) const;            // say if the key is opened
    bool IsOpened() const;                                   // say if the dialog is opened somewhere
    std::string GetOpenedKey() const;                        // return the dialog key who is opened, return nothing if not opened

    // get result
    bool IsOk() const;                                                                                       // true => Dialog Closed with Ok result / false : Dialog closed with cancel result
    std::map<std::string, std::string> GetSelection(IGFD_ResultMode vFlag = IGFD_ResultMode_KeepInputFile);  // Open File behavior : will return selection via a
                                                                                                             // map<FileName, FilePathName>
    std::string GetFilePathName(IGFD_ResultMode vFlag = IGFD_ResultMode_AddIfNoFileExt);                     // Save File behavior : will return the current file path name
    std::string GetCurrentFileName(IGFD_ResultMode vFlag = IGFD_ResultMode_AddIfNoFileExt);                  // Save File behavior : will return the content file name
    std::string GetCurrentPath();                                                                            // will return current file path
    std::string GetCurrentFilter();                                                                          // will return current filter
    UserDatas GetUserDatas() const;                                                                          // will return user datas send with Open Dialog

    // file style by extentions
    void SetFileStyle(                                        // SetExtention datas for have custom display of particular file type
        const IGFD_FileStyleFlags& vFlags,                    // file style
        const char* vCriteria,                                // extention filter to tune
        const FileStyle& vInfos);                             // Filter Extention Struct who contain Color and Icon/Text for the display of the
                                                              // file with extention filter
    void SetFileStyle(                                        // SetExtention datas for have custom display of particular file type
        const IGFD_FileStyleFlags& vFlags,                    // file style
        const char* vCriteria,                                // extention filter to tune
        const ImVec4& vColor,                                 // wanted color for the display of the file with extention filter
        const std::string& vIcon = "",                        // wanted text or icon of the file with extention filter
        ImFont* vFont            = nullptr);                             // wanted font
    void SetFileStyle(FileStyle::FileStyleFunctor vFunctor);  // set file style via lambda function
    bool GetFileStyle(                                        // GetExtention datas. return true is extention exist
        const IGFD_FileStyleFlags& vFlags,                    // file style
        const std::string& vCriteria,                         // extention filter (same as used in SetExtentionInfos)
        ImVec4* vOutColor,                                    // color to retrieve
        std::string* vOutIcon = nullptr,                      // icon or text to retrieve
        ImFont** vOutFont     = nullptr);                         // font to retreive
    void ClearFilesStyle();                                   // clear extentions setttings

    void SetLocales(                      // set locales to use before and after the dialog display
        const int& vLocaleCategory,       // set local category
        const std::string& vLocaleBegin,  // locale to use at begining of the dialog display
        const std::string& vLocaleEnd);   // locale to use at the end of the dialog display

protected:
    void m_NewFrame();   // new frame just at begining of display
    void m_EndFrame();   // end frame just at end of display
    void m_QuitFrame();  // quit frame when qui quit the dialog

    // others
    bool m_Confirm_Or_OpenOverWriteFileDialog_IfNeeded(bool vLastAction, ImGuiWindowFlags vFlags);  // treatment of the result, start the confirm to overwrite dialog
                                                                                                    // if needed (if defined with flag)

    // dialog parts
    virtual void m_DrawHeader();   // draw header part of the dialog (place btn, dir creation, path composer, search
                                   // bar)
    virtual void m_DrawContent();  // draw content part of the dialog (place pane, file list, side pane)
    virtual bool m_DrawFooter();   // draw footer part of the dialog (file field, fitler combobox, ok/cancel btn's)

    // widgets components
    virtual void m_DisplayPathPopup(ImVec2 vSize);  // draw path popup when click on a \ or /
    virtual bool m_DrawValidationButtons();         // draw validations btns, ok, cancel buttons
    virtual bool m_DrawOkButton();                  // draw ok button
    virtual bool m_DrawCancelButton();              // draw cancel button
    virtual void m_DrawSidePane(float vHeight);     // draw side pane
    virtual void m_SelectableItem(int vidx, std::shared_ptr<FileInfos> vInfos, bool vSelected, const char* vFmt,
                                  ...);             // draw a custom selectable behavior item
    virtual void m_DrawFileListView(ImVec2 vSize);  // draw file list view (default mode)

#ifdef USE_THUMBNAILS
    virtual void m_DrawThumbnailsListView(ImVec2 vSize);  // draw file list view with small thumbnails on the same line
    virtual void m_DrawThumbnailsGridView(ImVec2 vSize);  // draw a grid of small thumbnails
#endif

    // to be called only by these function and theirs overrides
    // - m_DrawFileListView
    // - m_DrawThumbnailsListView
    // - m_DrawThumbnailsGridView
    void m_BeginFileColorIconStyle(std::shared_ptr<FileInfos> vFileInfos, bool& vOutShowColor, std::string& vOutStr,
                                   ImFont** vOutFont);                    // begin style apply of filter with color an icon if any
    void m_EndFileColorIconStyle(const bool vShowColor, ImFont* vFont);  // end style apply of filter

    void m_DisplayFileInfosTooltip(const int32_t& vRowIdx, const int32_t& vColumnIdx, std::shared_ptr<FileInfos> vFileInfos);
};

}  // namespace IGFD

#endif  // __cplusplus

/////////////////////////////////////////////////
////// C LANG API ///////////////////////////////
/////////////////////////////////////////////////

#ifdef __cplusplus
#define IGFD_C_API extern "C" IGFD_API
typedef IGFD::UserDatas IGFDUserDatas;
typedef IGFD::PaneFun IGFDPaneFun;
typedef IGFD::FileDialog ImGuiFileDialog;
#else  // __cplusplus
#define IGFD_C_API
typedef struct ImGuiFileDialog ImGuiFileDialog;
typedef struct IGFD_Selection_Pair IGFD_Selection_Pair;
typedef struct IGFD_Selection IGFD_Selection;
#endif  // __cplusplus

typedef void (*IGFD_PaneFun)(const char*, void*, bool*);  // callback fucntion for display the pane

struct IGFD_FileDialog_Config {
    const char* path;            // path
    const char* fileName;        // defaut file name
    const char* filePathName;    // if not empty, the filename and the path will be obtained from filePathName
    int32_t countSelectionMax;   // count selection max
    void* userDatas;             // user datas (can be retrieved in pane)
    IGFD_PaneFun sidePane;       // side pane callback
    float sidePaneWidth;         // side pane width};
    ImGuiFileDialogFlags flags;  // ImGuiFileDialogFlags
};
IGFD_C_API struct IGFD_FileDialog_Config IGFD_FileDialog_Config_Get();  // return an initialized IGFD_FileDialog_Config

struct IGFD_Selection_Pair {
    char* fileName;
    char* filePathName;
};

IGFD_C_API IGFD_Selection_Pair IGFD_Selection_Pair_Get();                                  // return an initialized IGFD_Selection_Pair
IGFD_C_API void IGFD_Selection_Pair_DestroyContent(IGFD_Selection_Pair* vSelection_Pair);  // destroy the content of a IGFD_Selection_Pair

struct IGFD_Selection {
    IGFD_Selection_Pair* table;  // 0
    size_t count;                // 0U
};

IGFD_C_API IGFD_Selection IGFD_Selection_Get();                             // return an initialized IGFD_Selection
IGFD_C_API void IGFD_Selection_DestroyContent(IGFD_Selection* vSelection);  // destroy the content of a IGFD_Selection

// constructor / destructor
IGFD_C_API ImGuiFileDialog* IGFD_Create(void);               // create the filedialog context
IGFD_C_API void IGFD_Destroy(ImGuiFileDialog* vContextPtr);  // destroy the filedialog context

#ifdef USE_THUMBNAILS
typedef void (*IGFD_CreateThumbnailFun)(IGFD_Thumbnail_Info*);   // callback function for create thumbnail texture
typedef void (*IGFD_DestroyThumbnailFun)(IGFD_Thumbnail_Info*);  // callback fucntion for destroy thumbnail texture
#endif                                                           // USE_THUMBNAILS

IGFD_C_API void IGFD_OpenDialog(                   // open a standard dialog
    ImGuiFileDialog* vContextPtr,                  // ImGuiFileDialog context
    const char* vKey,                              // key dialog
    const char* vTitle,                            // title
    const char* vFilters,                          // filters/filter collections. set it to null for directory mode
    const struct IGFD_FileDialog_Config vConfig);  // config

IGFD_C_API bool IGFD_DisplayDialog(  // Display the dialog
    ImGuiFileDialog* vContextPtr,    // ImGuiFileDialog context
    const char* vKey,                // key dialog to display (if not the same key as defined by OpenDialog => no opening)
    ImGuiWindowFlags vFlags,         // ImGuiWindowFlags
    ImVec2 vMinSize,                 // mininmal size contraint for the ImGuiWindow
    ImVec2 vMaxSize);                // maximal size contraint for the ImGuiWindow

IGFD_C_API void IGFD_CloseDialog(   // Close the dialog
    ImGuiFileDialog* vContextPtr);  // ImGuiFileDialog context

IGFD_C_API bool IGFD_IsOk(          // true => Dialog Closed with Ok result / false : Dialog closed with cancel result
    ImGuiFileDialog* vContextPtr);  // ImGuiFileDialog context

IGFD_C_API bool IGFD_WasKeyOpenedThisFrame(  // say if the dialog key was already opened this frame
    ImGuiFileDialog* vContextPtr,            // ImGuiFileDialog context
    const char* vKey);

IGFD_C_API bool IGFD_WasOpenedThisFrame(  // say if the dialog was already opened this frame
    ImGuiFileDialog* vContextPtr);        // ImGuiFileDialog context

IGFD_C_API bool IGFD_IsKeyOpened(    // say if the dialog key is opened
    ImGuiFileDialog* vContextPtr,    // ImGuiFileDialog context
    const char* vCurrentOpenedKey);  // the dialog key

IGFD_C_API bool IGFD_IsOpened(      // say if the dialog is opened somewhere
    ImGuiFileDialog* vContextPtr);  // ImGuiFileDialog context

IGFD_C_API IGFD_Selection IGFD_GetSelection(  // Open File behavior : will return selection via a map<FileName, FilePathName>
    ImGuiFileDialog* vContextPtr,             // user datas (can be retrieved in pane)
    IGFD_ResultMode vMode);                   // Result Mode

IGFD_C_API char* IGFD_GetFilePathName(  // Save File behavior : will always return the content of the field with current
                                        // filter extention and current path, WARNINGS you are responsible to free it
    ImGuiFileDialog* vContextPtr,       // ImGuiFileDialog context
    IGFD_ResultMode vMode);             // Result Mode

IGFD_C_API char* IGFD_GetCurrentFileName(  // Save File behavior : will always return the content of the field with
                                           // current filter extention, WARNINGS you are responsible to free it
    ImGuiFileDialog* vContextPtr,          // ImGuiFileDialog context
    IGFD_ResultMode vMode);                // Result Mode

IGFD_C_API char* IGFD_GetCurrentPath(  // will return current path, WARNINGS you are responsible to free it
    ImGuiFileDialog* vContextPtr);     // ImGuiFileDialog context

IGFD_C_API char* IGFD_GetCurrentFilter(  // will return selected filter, WARNINGS you are responsible to free it
    ImGuiFileDialog* vContextPtr);       // ImGuiFileDialog context

IGFD_C_API void* IGFD_GetUserDatas(  // will return user datas send with Open Dialog
    ImGuiFileDialog* vContextPtr);   // ImGuiFileDialog context

IGFD_C_API void IGFD_SetFileStyle(        // SetExtention datas for have custom display of particular file type
    ImGuiFileDialog* vContextPtr,         // ImGuiFileDialog context
    IGFD_FileStyleFlags vFileStyleFlags,  // file style type
    const char* vFilter,                  // extention filter to tune
    ImVec4 vColor,                        // wanted color for the display of the file with extention filter
    const char* vIconText,                // wanted text or icon of the file with extention filter (can be sued with font icon)
    ImFont* vFont);                       // wanted font pointer

IGFD_C_API void IGFD_SetFileStyle2(       // SetExtention datas for have custom display of particular file type
    ImGuiFileDialog* vContextPtr,         // ImGuiFileDialog context
    IGFD_FileStyleFlags vFileStyleFlags,  // file style type
    const char* vFilter,                  // extention filter to tune
    float vR, float vG, float vB,
    float vA,               // wanted color channels RGBA for the display of the file with extention filter
    const char* vIconText,  // wanted text or icon of the file with extention filter (can be sued with font icon)
    ImFont* vFont);         // wanted font pointer

IGFD_C_API bool IGFD_GetFileStyle(ImGuiFileDialog* vContextPtr,         // ImGuiFileDialog context
                                  IGFD_FileStyleFlags vFileStyleFlags,  // file style type
                                  const char* vFilter,                  // extention filter (same as used in SetExtentionInfos)
                                  ImVec4* vOutColor,                    // color to retrieve
                                  char** vOutIconText,                  // icon or text to retrieve, WARNINGS you are responsible to free it
                                  ImFont** vOutFont);                   // font pointer to retrived

IGFD_C_API void IGFD_ClearFilesStyle(  // clear extentions setttings
    ImGuiFileDialog* vContextPtr);     // ImGuiFileDialog context

IGFD_C_API void SetLocales(        // set locales to use before and after display
    ImGuiFileDialog* vContextPtr,  // ImGuiFileDialog context
    const int vCategory,           // set local category
    const char* vBeginLocale,      // locale to use at begining of the dialog display
    const char* vEndLocale);       // locale to set at end of the dialog display

#ifdef USE_EXPLORATION_BY_KEYS
IGFD_C_API void IGFD_SetFlashingAttenuationInSeconds(  // set the flashing time of the line in file list when use
                                                       // exploration keys
    ImGuiFileDialog* vContextPtr,                      // ImGuiFileDialog context
    float vAttenValue);                                // set the attenuation (from flashed to not flashed) in seconds
#endif

#ifdef USE_PLACES_FEATURE
IGFD_C_API char* IGFD_SerializePlaces(    // serialize place : return place buffer to save in a file, WARNINGS
                                          // you are responsible to free it
    ImGuiFileDialog* vContextPtr,         // ImGuiFileDialog context
    bool vDontSerializeCodeBasedPlaces);  // for avoid serialization of place added by code

IGFD_C_API void IGFD_DeserializePlaces(  // deserialize place : load bookmar buffer to load in the dialog (saved
                                         // from previous use with SerializePlaces())
    ImGuiFileDialog* vContextPtr,        // ImGuiFileDialog context
    const char* vPlaces);                // place buffer to load

IGFD_C_API bool IGFD_AddPlacesGroup(  // add a places group by code
    ImGuiFileDialog* vContextPtr,     // ImGuiFileDialog context
    const char* vGroupName,           // the group name
    size_t vDisplayOrder,             // the display roder of the group
    bool vCanBeEdited);               // let the user add/remove place in the group

IGFD_C_API bool IGFD_RemovePlacesGroup(  // remove a place group by code, return true if succeed
    ImGuiFileDialog* vContextPtr,        // ImGuiFileDialog context
    const char* vGroupName);             // place name to remove

IGFD_C_API bool IGFD_AddPlace(     // add a place by code
    ImGuiFileDialog* vContextPtr,  // ImGuiFileDialog context
    const char* vGroupName,        // the group name
    const char* vPlaceName,        // place name
    const char* vPlacePath,        // place path
    bool vCanBeSaved,              // place can be saved
    const char* vIconText);        // wanted text or icon of the file with extention filter (can be used with font icon)

IGFD_C_API bool IGFD_RemovePlace(  // remove a place by code, return true if succeed
    ImGuiFileDialog* vContextPtr,  // ImGuiFileDialog context
    const char* vGroupName,        // the group name
    const char* vPlaceName);       // place name to remove

#endif

#ifdef USE_THUMBNAILS
IGFD_C_API void SetCreateThumbnailCallback(        // define the callback for create the thumbnails texture
    ImGuiFileDialog* vContextPtr,                  // ImGuiFileDialog context
    IGFD_CreateThumbnailFun vCreateThumbnailFun);  // the callback for create the thumbnails texture

IGFD_C_API void SetDestroyThumbnailCallback(         // define the callback for destroy the thumbnails texture
    ImGuiFileDialog* vContextPtr,                    // ImGuiFileDialog context
    IGFD_DestroyThumbnailFun vDestroyThumbnailFun);  // the callback for destroy the thumbnails texture

IGFD_C_API void ManageGPUThumbnails(  // must be call in gpu zone, possibly a thread, will call the callback for create
                                      // / destroy the textures
    ImGuiFileDialog* vContextPtr);    // ImGuiFileDialog context
#endif                                // USE_THUMBNAILS
