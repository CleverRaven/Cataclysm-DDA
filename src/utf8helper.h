#pragma once
#ifndef CATA_SRC_UTF8HELPER
#define CATA_SRC_UTF8HELPER

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <unordered_set>


/**
 * Safe, locale and platform independent UTF-8 handle helper.
 *
 * The goal is not to implement full support for Unicode and all special cases.
 * The goal is to implement only what is actually used in Cataclysm-DDA, but do it well.
 * Native speakers can implement support for their language better,
 * than the default on different platforms. And bugs with Chinese (issue #44263, PR #44288)
 * and Cyrillic (also PR #44288 and comment about Ë) on MacOS are an example of this.
 *
 * Use this class to add handling for any special cases.
 * What is not implemented here will be handled std::locale by default.
 */

class utf8helper
{
    private:
        // data for handling (should be fast):
        // dictionary for toupper, tolower, forsearch converting
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> dictionary;
        // asc order for sort
        std::unordered_map<std::string, size_t> ascorder;
        // asc order for case insensitive sort; use tolower, then sort is to expensive
        std::unordered_map<std::string, size_t> ascorder_case_insensitive;
        // supported langs with setted alphabet
        std::unordered_set<std::string> langs = { "ru", "pl", "de", "es", "fr", "pt", "hu", "en" };
        // supported langs formated as in the Cataclysm-DDA lang_options
        std::unordered_set<std::string> supported_langs = { "ru", "pl", "de", "es", "es_AR", "es_ES", "fr", "pt", "pt_BR", "hu", "en" };
        // data for init only (can be slow):
        // alphabet for all supported Cataclysm-DDA langs exclude zh_CN, zh_TW, ko
        std::unordered_map<std::string, std::string> alphabet = {
            { "ru_upper", "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" }, // Cyrillic
            { "ru_lower", "абвгдеёжзийклмнопрстуфхцчшщъыьэюя" }, // non-ASCII all
            { "pl_upper", "AĄBCĆDEĘFGHIJKLŁMNŃOÓPQRSŚTUVWXYZŹŻ" },
            { "pl_lower", "aąbcćdeęfghijklłmnńoópqrsśtuvwxyzźż" }, // non-ASCII ąćęłńóśźż
            { "de_upper", "AÄBCDEFGHIJKLMNOÖPQRSẞTUÜVWXYZ" }, // ẞ since 2017
            { "de_lower", "aäbcdefghijklmnoöpqrsßtuüvwxyz" }, // non-ASCII äößü
            { "es_upper", "ABCDEFGHIJKLMNÑOPQRSTUVWXYZ" },
            { "es_lower", "abcdefghijklmnñopqrstuvwxyz" }, // non-ASCII ñ, since 2010 excluded ch, ll
            { "fr_upper", "AÀÂÆBCÇDEÈÉÊËFGHIÎÏJKLMNOÔŒPQRSTUÙÛÜVWXYŸZ" },
            { "fr_lower", "aàâæbcçdeèéêëfghiîïjklmnoôœpqrstuùûüvwxyÿz" }, // non-ASCII àâæçèéêëîïôœùûüÿ
            { "pt_upper", "AÀÁÂÃBCÇDEÉÊFGHIÍJKLMNOÓÔÕPQRSTUÚVWXYZ" },
            { "pt_lower", "aàáâãbcçdeéêfghiíjklmnoóôõpqrstuúvwxyz" }, // non-ASCII àáâãçéêíóôõú
            { "hu_upper", "AÁBCCsDDzDzsEÉFGGyHIÍJKLLyMNNyOÓÖŐPQRSSzTTyUÚÜŰVWXYZZs"}, // digraphs Cs Dz Dzs Gy Ly Ny Sz Ty Zs
            { "hu_lower", "aábccsddzdzseéfggyhiíjkllymnnyoóöőpqrsszttyuúüűvwxyzzs"}, // non-ASCII áéíóöőúüű
            { "en_upper", "ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
            { "en_lower", "abcdefghijklmnopqrstuvwxyz" },
        };
        std::map<std::string, std::string> asc = { // asc order left to right, up to down
            { "00-Latin-Extended", "AaÀàÁáÂâÃãÄäĄąÆæBbCcĆćÇçDdEeÈèÉéÊêËëĘęFfGgHhIiÍíÎîÏïJjKkLlŁłMmNnŃńÑñOoÓóÔôÕõÖöŐőŒœPpQqRrSsŚśẞßTtUuÙùÚúÛûÜüŰűVvWwXxYyŸÿZzŹźŻż" },
            { "01-Cyrillic", "АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоПпРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯя"}
        }; // digraphs sorted as independent letters for now
        // private methods:
        std::string convert( const std::string &s, const std::string &rules ) const;
        size_t char_width( const char &ch ) const;
    public:
        utf8helper();
        /** Is this language supported? Lang format as in the Cataclysm-DDA lang_options */
        bool is_supported( const std::string &lang ) const;
        /**
         * Splits a std::string into UTF-8 characters (letters)
         * @param s UTF-8 std::string
         * @param exclude_ascii set it to exclude ASCII characters (with code less than 0xE0)
         * @return  std::vector of std::strings with UTF-8 characters from the original UTF-8 std::string
         */
        std::vector<std::string> split( const std::string &s, bool exclude_ascii = false ) const;
        /**
         * Converts the given std::string to uppercase according to the conversion rules defined in the utf8helper::dictionary
         * @param s UTF-8 std::string
         * @return an uppercase UTF-8 std::string
         */
        std::string to_upper_case( const std::string &s ) const;
        /**
         * Converts the given std::string to lowercase according to the conversion rules defined in the utf8helper::dictionary
         * @param s UTF-8 std::string
         * @return an lowercase UTF-8 std::string
         */
        std::string to_lower_case( const std::string &s ) const;
        /**
         * Converts the given std::string for the case insensetive search. In common, this is equal to to_lower_case, but with special rules for some languages.
         * @param s UTF-8 std::string
         * @return an UTF-8 std::string with special transformations for case insensitive search
         */
        std::string for_search( const std::string &s ) const;
        /**
         * Compares two std::string arguments l and r according to the comparison rules defined in the utf8helper::ascorder or utf8helper::ascorder_case_insensitive
         * @param l left UTF-8 std::string
         * @param r right UTF-8 std::string
         * @param case_sensitive if true a > A, but a < B; if false a == A
         *
         * @return true if left is lexicographically less than right, false otherwise
         */
        bool compare( const std::string &l, const std::string &r, bool case_sensitive = false ) const;
        /**
         * Compares two std::string arguments l and r according to the comparison rules defined in the utf8helper::ascorder or utf8helper::ascorder_case_insensitive
         * @param l left UTF-8 std::string
         * @param r right UTF-8 std::string
         * @param case_sensitive if true a > A, but a < B; if false a == A
         *
         * @return true if left is lexicographically less than right, false otherwise
         */
        bool operator()( const std::string &l, const std::string &r, bool case_sensitive = false ) const;
        /** Print dictionary and ascorder to the console (for debug purposes only) */
        void print_debug() const;
};
#endif // CATA_SRC_UTF8HELPER
