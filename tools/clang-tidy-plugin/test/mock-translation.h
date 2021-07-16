// check_clang_tidy uses -nostdinc++, so we add dummy declaration of std::string here
namespace std
{
template<class CharT, class Traits = void, class Allocator = void>
class basic_string
{
    public:
        basic_string();
        basic_string( const CharT * );
        CharT *c_str();
        const CharT *c_str() const;
};
using string = basic_string<char>;
string operator+( const string &, const string & );
} // namespace std

// check_clang_tidy uses -nostdinc++, so we add dummy translation interface here instead of including translations.h
std::string _( const std::string & );
std::string gettext( const std::string & );
std::string pgettext( const std::string &, const std::string & );
std::string ngettext( const std::string &, const std::string &, int );
std::string npgettext( const std::string &, const std::string &, const std::string &, int );

class translation
{
    public:
        static translation to_translation( const std::string & );
        static translation to_translation( const std::string &, const std::string & );
        static translation pl_translation( const std::string &, const std::string & );
        static translation pl_translation( const std::string &, const std::string &, const std::string & );
        static translation no_translation( const std::string & );
};

translation to_translation( const std::string & );
translation to_translation( const std::string &, const std::string & );
translation pl_translation( const std::string &, const std::string & );
translation pl_translation( const std::string &, const std::string &, const std::string & );
translation no_translation( const std::string & );
