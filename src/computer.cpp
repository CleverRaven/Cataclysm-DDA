#include "computer.h"

#include <sstream>
#include <locale>
#include <map>

#include "debug.h"
#include "json.h"
#include "output.h"
#include "translations.h"

computer_option::computer_option()
    : name( "Unknown" ), action( COMPACT_NULL ), security( 0 )
{
}

computer_option::computer_option( const std::string &N, computer_action A, int S )
    : name( N ), action( A ), security( S )
{
}

computer::computer( const std::string &new_name, int new_security )
    : name( new_name ), mission_id( -1 ), security( new_security ), alerts( 0 ),
      next_attempt( calendar::before_time_starts ),
      access_denied( _( "ERROR!  Access denied!" ) )
{
}

void computer::set_security( int Security )
{
    security = Security;
}

void computer::add_option( const computer_option &opt )
{
    options.emplace_back( opt );
}

void computer::add_option( const std::string &opt_name, computer_action action,
                           int security )
{
    add_option( computer_option( opt_name, action, security ) );
}

void computer::add_failure( const computer_failure &failure )
{
    failures.emplace_back( failure );
}

void computer::add_failure( computer_failure_type failure )
{
    add_failure( computer_failure( failure ) );
}

void computer::set_access_denied_msg( const std::string &new_msg )
{
    access_denied = new_msg;
}

void computer::set_mission( const int id )
{
    mission_id = id;
}

std::string computer::save_data() const
{
    std::ostringstream data;

    data.imbue( std::locale::classic() );

    data
            << string_replace( name, " ", "_" ) << ' '
            << security << ' '
            << mission_id << ' '
            << options.size() << ' ';

    for( auto &elem : options ) {
        data
                << string_replace( elem.name, " ", "_" ) << ' '
                << static_cast<int>( elem.action ) << ' '
                << elem.security << ' ';
    }

    data << failures.size() << ' ';
    for( auto &elem : failures ) {
        data << static_cast<int>( elem.type ) << ' ';
    }

    data << string_replace( access_denied, " ", "_" ) << ' ';

    return data.str();
}

void computer::load_data( const std::string &data )
{
    static const std::set<computer_action> blacklisted_options = {{ COMPACT_OBSOLETE }};
    options.clear();
    failures.clear();

    std::istringstream dump( data );
    dump.imbue( std::locale::classic() );

    dump >> name >> security >> mission_id;

    name = string_replace( name, "_", " " );

    // Pull in options
    int optsize;
    dump >> optsize;
    for( int n = 0; n < optsize; n++ ) {
        std::string tmpname;

        int tmpaction;
        int tmpsec;

        dump >> tmpname >> tmpaction >> tmpsec;
        if( blacklisted_options.find( static_cast<computer_action>( tmpaction ) )
            != blacklisted_options.end() ) {
            continue;
        }
        add_option( string_replace( tmpname, "_", " " ), static_cast<computer_action>( tmpaction ),
                    tmpsec );
    }

    // Pull in failures
    int failsize;
    dump >> failsize;
    for( int n = 0; n < failsize; n++ ) {
        int tmpfail;
        dump >> tmpfail;
        add_failure( static_cast<computer_failure_type>( tmpfail ) );
    }

    std::string tmp_access_denied;
    dump >> tmp_access_denied;

    // For backwards compatibility, only set the access denied message if it
    // isn't empty. This is to avoid the message becoming blank when people
    // load old saves.
    if( !tmp_access_denied.empty() ) {
        access_denied = string_replace( tmp_access_denied, "_", " " );
    }
}

void computer::remove_option( computer_action const action )
{
    for( auto it = options.begin(); it != options.end(); ++it ) {
        if( it->action == action ) {
            options.erase( it );
            break;
        }
    }
}

namespace io
{
template<>
std::string enum_to_string<computer_action>( const computer_action act )
{
    switch( act ) {
        // *INDENT-OFF*
        case COMPACT_NULL: return "null";
        case COMPACT_AMIGARA_LOG: return "amigara_log";
        case COMPACT_AMIGARA_START: return "amigara_start";
        case COMPACT_BLOOD_ANAL: return "blood_anal";
        case COMPACT_CASCADE: return "cascade";
        case COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER: return "complete_disable_external_power";
        case COMPACT_CONVEYOR: return "conveyor";
        case COMPACT_DATA_ANAL: return "data_anal";
        case COMPACT_DEACTIVATE_SHOCK_VENT: return "deactivate_shock_vent";
        case COMPACT_DISCONNECT: return "disconnect";
        case COMPACT_DOWNLOAD_SOFTWARE: return "download_software";
        case COMPACT_ELEVATOR_ON: return "elevator_on";
        case COMPACT_EMERG_MESS: return "emerg_mess";
        case COMPACT_EMERG_REF_CENTER: return "emerg_ref_center";
        case COMPACT_EXTRACT_RAD_SOURCE: return "extract_rad_source";
        case COMPACT_GEIGER: return "geiger";
        case COMPACT_IRRADIATOR: return "irradiator";
        case COMPACT_LIST_BIONICS: return "list_bionics";
        case COMPACT_LOCK: return "lock";
        case COMPACT_MAP_SEWER: return "map_sewer";
        case COMPACT_MAP_SUBWAY: return "map_subway";
        case COMPACT_MAPS: return "maps";
        case COMPACT_MISS_DISARM: return "miss_disarm";
        case COMPACT_OBSOLETE: return "obsolete";
        case COMPACT_OPEN: return "open";
        case COMPACT_OPEN_DISARM: return "open_disarm";
        case COMPACT_PORTAL: return "portal";
        case COMPACT_RADIO_ARCHIVE: return "radio_archive";
        case COMPACT_RELEASE: return "release";
        case COMPACT_RELEASE_BIONICS: return "release_bionics";
        case COMPACT_RELEASE_DISARM: return "release_disarm";
        case COMPACT_REPEATER_MOD: return "repeater_mod";
        case COMPACT_RESEARCH: return "research";
        case COMPACT_SAMPLE: return "sample";
        case COMPACT_SHUTTERS: return "shutters";
        case COMPACT_SR1_MESS: return "sr1_mess";
        case COMPACT_SR2_MESS: return "sr2_mess";
        case COMPACT_SR3_MESS: return "sr3_mess";
        case COMPACT_SR4_MESS: return "sr4_mess";
        case COMPACT_SRCF_1_MESS: return "srcf_1_mess";
        case COMPACT_SRCF_2_MESS: return "srcf_2_mess";
        case COMPACT_SRCF_3_MESS: return "srcf_3_mess";
        case COMPACT_SRCF_ELEVATOR: return "srcf_elevator";
        case COMPACT_SRCF_SEAL: return "srcf_seal";
        case COMPACT_SRCF_SEAL_ORDER: return "srcf_seal_order";
        case COMPACT_TERMINATE: return "terminate";
        case COMPACT_TOLL: return "toll";
        case COMPACT_TOWER_UNRESPONSIVE: return "tower_unresponsive";
        case COMPACT_UNLOCK: return "unlock";
        case COMPACT_UNLOCK_DISARM: return "unlock_disarm";
        // *INDENT-OFF*
        case NUM_COMPUTER_ACTIONS:
            break;
    }
    debugmsg( "Invalid computer_action" );
    abort();
}

template<>
std::string enum_to_string<computer_failure_type>( const computer_failure_type fail )
{
    switch( fail ){
        // *INDENT-OFF*
        case COMPFAIL_NULL: return "null";
        case COMPFAIL_ALARM: return "alarm";
        case COMPFAIL_AMIGARA: return "amigara";
        case COMPFAIL_DAMAGE: return "damage";
        case COMPFAIL_DESTROY_BLOOD: return "destroy_blood";
        case COMPFAIL_DESTROY_DATA: return "destroy_data";
        case COMPFAIL_MANHACKS: return "manhacks";
        case COMPFAIL_PUMP_EXPLODE: return "pump_explode";
        case COMPFAIL_PUMP_LEAK: return "pump_leak";
        case COMPFAIL_SECUBOTS: return "secubots";
        case COMPFAIL_SHUTDOWN: return "shutdown";
        // *INDENT-ON*
        case NUM_COMPUTER_FAILURES:
            break;
    }
    debugmsg( "Invalid computer_failure_type" );
    abort();
}
} // namespace io

template<>
struct enum_traits<computer_action> {
    static constexpr computer_action last = NUM_COMPUTER_ACTIONS;
};

template<>
struct enum_traits<computer_failure_type> {
    static constexpr computer_failure_type last = NUM_COMPUTER_FAILURES;
};

computer_option computer_option::from_json( const JsonObject &jo )
{
    const std::string name = jo.get_string( "name" );
    const computer_action action = jo.get_enum_value<computer_action>( "action" );
    const int sec = jo.get_int( "security", 0 );
    return computer_option( name, action, sec );
}

computer_failure computer_failure::from_json( const JsonObject &jo )
{
    const computer_failure_type type = jo.get_enum_value<computer_failure_type>( "action" );
    return computer_failure( type );
}
