#include "HeaderGuardCheck.h"

#include <unordered_set>

#include <clang/Frontend/CompilerInstance.h>

#if !defined(_MSC_VER)
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace clang
{
namespace tidy
{
namespace cata
{

CataHeaderGuardCheck::CataHeaderGuardCheck( StringRef Name,
        ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context ) {}

/// \brief canonicalize a path by removing ./ and ../ components.
static std::string cleanPath( StringRef Path )
{
    SmallString<256> Result = Path;
    llvm::sys::path::remove_dots( Result, true );
    return Result.str();
}

static bool pathExists( const std::string &path )
{
    struct stat buffer;
    return ( stat( path.c_str(), &buffer ) == 0 );
}

static bool isHeaderFileName( StringRef FileName )
{
    return FileName.contains( ".h" );
}

static std::string getHeaderGuard( StringRef Filename )
{
    std::string Guard = tooling::getAbsolutePath( Filename );

    // Look for the top-level directory
    std::string TopDir = Guard;
    size_t LastSlash;
    bool Found = false;
    while( std::string::npos != ( LastSlash = TopDir.find_last_of( "/\\" ) ) ) {
        TopDir = TopDir.substr( 0, LastSlash );
        if( pathExists( TopDir + "/.travis.yml" ) ) {
            Found = true;
            break;
        }
    }

    if( !Found ) {
        return {};
    }

    Guard = Guard.substr( TopDir.length() + 1 );

    std::replace( Guard.begin(), Guard.end(), '/', '_' );
    std::replace( Guard.begin(), Guard.end(), '.', '_' );
    std::replace( Guard.begin(), Guard.end(), '-', '_' );

    Guard = "CATA_" + Guard;

    return StringRef( Guard ).upper();
}

static std::string formatEndIf( StringRef HeaderGuard )
{
    return "endif // " + HeaderGuard.str();
}

struct MacroInfo_ {
    Token Tok;
    const MacroInfo *Info;
};

struct IfndefInfo {
    const IdentifierInfo *MacroId;
    SourceLocation Loc;
    SourceLocation MacroNameLoc;
};

struct FileInfo {
    const FileEntry *Entry;
    std::vector<MacroInfo_> Macros;
    std::vector<IfndefInfo> Ifndefs;
    std::map<SourceLocation, SourceLocation> EndIfs;
    SourceLocation PragmaOnce;
};

class HeaderGuardPPCallbacks : public PPCallbacks
{
    public:
        HeaderGuardPPCallbacks( Preprocessor *PP, CataHeaderGuardCheck *Check )
            : PP( PP ), Check( Check ) {}

        std::string GetFileName( SourceLocation Loc ) {
            SourceManager &SM = PP->getSourceManager();
            FileID Id = SM.getFileID( Loc );
            if( const FileEntry *Entry = SM.getFileEntryForID( Id ) ) {
                return cleanPath( Entry->getName() );
            } else {
                return {};
            }
        }

        void FileChanged( SourceLocation Loc, FileChangeReason Reason,
                          SrcMgr::CharacteristicKind FileType,
                          FileID ) override {
            if( Reason == EnterFile && FileType == SrcMgr::C_User ) {
                std::string FileName = GetFileName( Loc );
                Files.insert( FileName );
                SourceManager &SM = PP->getSourceManager();
                FileID Id = SM.getFileID( Loc );
                FileInfos[FileName].Entry = SM.getFileEntryForID( Id );
            }
        }

        void Ifndef( SourceLocation Loc, const Token &MacroNameTok,
                     const MacroDefinition &MD ) override {
            if( MD ) {
                return;
            }

            // Record #ifndefs that succeeded. We also need the Location of the Name.
            FileInfos[GetFileName( Loc )].Ifndefs.push_back(
                IfndefInfo{ MacroNameTok.getIdentifierInfo(), Loc, MacroNameTok.getLocation() } );
        }

        void MacroDefined( const Token &MacroNameTok,
                           const MacroDirective *MD ) override {
            // Record all defined macros. We store the whole token to get info on the
            // name later.
            SourceLocation Loc = MD->getLocation();
            FileInfos[GetFileName( Loc )].Macros.push_back(
                MacroInfo_{ MacroNameTok, MD->getMacroInfo() } );
        }

        void Endif( SourceLocation Loc, SourceLocation IfLoc ) override {
            // Record all #endif and the corresponding #ifs (including #ifndefs).
            FileInfos[GetFileName( Loc )].EndIfs[IfLoc] = Loc;
        }

        void PragmaDirective( SourceLocation Loc, PragmaIntroducerKind ) override {
            const char *PragmaData = PP->getSourceManager().getCharacterData( Loc );
            static constexpr const char *PragmaOnce = "#pragma once";
            if( 0 == strncmp( PragmaData, PragmaOnce, strlen( PragmaOnce ) ) ) {
                FileInfos[GetFileName( Loc )].PragmaOnce = Loc;
            }
        }

        void EndOfMainFile() override {
            // Now that we have all this information from the preprocessor, use it!
            std::unordered_set<std::string> GuardlessHeaders = Files;

            for( const std::string &FileName : Files ) {
                const FileInfo &Info = FileInfos[FileName];

                if( Info.Macros.empty() || Info.Ifndefs.empty() || Info.EndIfs.empty() ) {
                    continue;
                }

                const IfndefInfo &Ifndef = Info.Ifndefs.front();
                const MacroInfo_ &Macro = Info.Macros.front();
                StringRef CurHeaderGuard = Macro.Tok.getIdentifierInfo()->getName();

                if( Ifndef.MacroId->getName() != CurHeaderGuard ) {
                    // If the #ifndef and #define don't match, then it's
                    // probably not a header guard we're looking at.  Abort.
                    continue;
                }

                // Look up Locations for this guard.
                SourceLocation DefineLoc = Macro.Tok.getLocation();
                auto EndifIt = Info.EndIfs.find( Ifndef.Loc );

                if( EndifIt == Info.EndIfs.end() ) {
                    continue;
                }

                SourceLocation EndIfLoc = EndifIt->second;

                GuardlessHeaders.erase( FileName );

                // If the macro Name is not equal to what we can compute, correct it in
                // the #ifndef and #define.
                std::vector<FixItHint> FixIts;
                std::string NewGuard =
                    checkHeaderGuardDefinition( Ifndef.MacroNameLoc, DefineLoc, FileName,
                                                CurHeaderGuard, FixIts );

                // Now look at the #endif. We want a comment with the header guard. Fix it
                // at the slightest deviation.
                checkEndifComment( EndIfLoc, NewGuard, FixIts );

                // Bundle all fix-its into one warning. The message depends on whether we
                // changed the header guard or not.
                if( !FixIts.empty() ) {
                    if( CurHeaderGuard != NewGuard ) {
                        Check->diag( Ifndef.Loc, "Header guard does not follow preferred style." )
                                << FixIts;
                    } else {
                        Check->diag( EndIfLoc, "#endif for a header guard should reference the "
                                     "guard macro in a comment." )
                                << FixIts;
                    }
                }
            }

            // Emit warnings for headers that are missing guards.
            checkGuardlessHeaders( GuardlessHeaders );

            // Clear all state.
            Files.clear();
            FileInfos.clear();
        }

        bool wouldFixEndifComment( SourceLocation EndIf, StringRef HeaderGuard,
                                   size_t *EndIfLenPtr = nullptr ) {
            if( !EndIf.isValid() ) {
                return false;
            }
            const char *EndIfData = PP->getSourceManager().getCharacterData( EndIf );
            // NOLINTNEXTLINE(cata-text-style)
            size_t EndIfLen = std::strcspn( EndIfData, "\r\n" );
            if( EndIfLenPtr ) {
                *EndIfLenPtr = EndIfLen;
            }

            StringRef EndIfStr( EndIfData, EndIfLen );
            // NOLINTNEXTLINE(cata-text-style)
            EndIfStr = EndIfStr.substr( EndIfStr.find_first_not_of( "#endif \t" ) );

            // Give up if there's an escaped newline.
            size_t FindEscapedNewline = EndIfStr.find_last_not_of( ' ' );
            if( FindEscapedNewline != StringRef::npos &&
                EndIfStr[FindEscapedNewline] == '\\' ) {
                return false;
            }

            return EndIfStr != "// " + HeaderGuard.str();
        }

        /// \brief Look for header guards that don't match the preferred style. Emit
        /// fix-its and return the suggested header guard (or the original if no
        /// change was made.
        static std::string checkHeaderGuardDefinition( SourceLocation Ifndef,
                SourceLocation Define,
                StringRef FileName,
                StringRef CurHeaderGuard,
                std::vector<FixItHint> &FixIts ) {
            std::string CPPVar = getHeaderGuard( FileName );

            if( CPPVar.empty() ) {
                return CurHeaderGuard;
            }

            if( Ifndef.isValid() && CurHeaderGuard != CPPVar ) {
                FixIts.push_back( FixItHint::CreateReplacement(
                                      CharSourceRange::getTokenRange(
                                          Ifndef, Ifndef.getLocWithOffset( CurHeaderGuard.size() ) ),
                                      CPPVar ) );
                FixIts.push_back( FixItHint::CreateReplacement(
                                      CharSourceRange::getTokenRange(
                                          Define, Define.getLocWithOffset( CurHeaderGuard.size() ) ),
                                      CPPVar ) );
                return CPPVar;
            }
            return CurHeaderGuard;
        }

        /// \brief Checks the comment after the #endif of a header guard and fixes it
        /// if it doesn't match \c HeaderGuard.
        void checkEndifComment( SourceLocation EndIf, StringRef HeaderGuard,
                                std::vector<FixItHint> &FixIts ) {
            size_t EndIfLen;
            if( wouldFixEndifComment( EndIf, HeaderGuard, &EndIfLen ) ) {
                FixIts.push_back( FixItHint::CreateReplacement(
                                      CharSourceRange::getCharRange( EndIf,
                                              EndIf.getLocWithOffset( EndIfLen ) ),
                                      formatEndIf( HeaderGuard ) ) );
            }
        }

        /// \brief Looks for files that were visited but didn't have a header guard.
        /// Emits a warning with fixits suggesting adding one.
        void checkGuardlessHeaders( std::unordered_set<std::string> GuardlessHeaders ) {
            // Look for header files that didn't have a header guard. Emit a warning and
            // fix-its to add the guard.
            for( const std::string &FileName : GuardlessHeaders ) {
                if( !isHeaderFileName( FileName ) ) {
                    continue;
                }

                SourceManager &SM = PP->getSourceManager();
                const FileInfo &Info = FileInfos.at( FileName );
                const FileEntry *FE = Info.Entry;
                if( !FE ) {
                    fprintf( stderr, "No FileEntry for %s\n", FileName.c_str() );
                    continue;
                }
                FileID FID = SM.translateFile( FE );
                SourceLocation StartLoc = SM.getLocForStartOfFile( FID );
                if( StartLoc.isInvalid() ) {
                    continue;
                }

                std::string CPPVar = getHeaderGuard( FileName );
                if( CPPVar.empty() ) {
                    continue;
                }
                // If there's a macro with a name that follows the header guard convention
                // but was not recognized by the preprocessor as a header guard there must
                // be code outside of the guarded area. Emit a plain warning without
                // fix-its.
                bool SeenMacro = false;
                for( const auto &MacroEntry : Info.Macros ) {
                    StringRef Name = MacroEntry.Tok.getIdentifierInfo()->getName();
                    SourceLocation DefineLoc = MacroEntry.Tok.getLocation();
                    if( Name == CPPVar &&
                        SM.isWrittenInSameFile( StartLoc, DefineLoc ) ) {
                        Check->diag( DefineLoc, "Code/includes outside of area guarded by "
                                     "header guard; consider moving it." );
                        SeenMacro = true;
                        break;
                    }
                }

                if( SeenMacro ) {
                    continue;
                }

                SourceLocation InsertLoc = StartLoc;

                if( Info.PragmaOnce.isValid() ) {
                    const char *PragmaData =
                        PP->getSourceManager().getCharacterData( Info.PragmaOnce );
                    const char *Newline = strchr( PragmaData, '\n' );
                    if( Newline ) {
                        InsertLoc = Info.PragmaOnce.getLocWithOffset( Newline + 1 - PragmaData );
                    }
                }

                const char *InsertData = PP->getSourceManager().getCharacterData( InsertLoc );
                const char *Newlines = "\n\n";
                if( InsertData[0] == '\n' || InsertData[0] == '\r' ) {
                    Newlines = "\n";
                }

                Check->diag( InsertLoc, "Header is missing header guard." )
                        << FixItHint::CreateInsertion(
                            InsertLoc, "#ifndef " + CPPVar + "\n#define " + CPPVar + Newlines )
                        << FixItHint::CreateInsertion(
                            SM.getLocForEndOfFile( FID ),
                            "\n#" + formatEndIf( CPPVar ) + "\n" );
            }
        }
    private:
        std::unordered_set<std::string> Files;
        std::unordered_map<std::string, FileInfo> FileInfos;

        Preprocessor *PP;
        CataHeaderGuardCheck *Check;
};

void CataHeaderGuardCheck::registerPPCallbacks( CompilerInstance &Compiler )
{
    Compiler.getPreprocessor().addPPCallbacks(
        llvm::make_unique<HeaderGuardPPCallbacks>( &Compiler.getPreprocessor(),
                this ) );
}

} // namespace cata
} // namespace tidy
} // namespace clang
