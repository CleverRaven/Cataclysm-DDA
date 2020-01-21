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
    : name( new_name ), mission_id( -1 ), security( new_security ),
      access_denied( _( "ERROR!  Access denied!" ) )
{
}

computer::computer( const computer &rhs )
{
    *this = rhs;
}

computer::~computer() = default;

computer &computer::operator=( const computer &rhs )
{
    security = rhs.security;
    name = rhs.name;
    access_denied = rhs.access_denied;
    mission_id = rhs.mission_id;
    options = rhs.options;
    failures = rhs.failures;
    return *this;
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
    static const std::set<std::string> blacklisted_options = {{ "Launch_Missile" }};
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
        if( blacklisted_options.find( tmpname ) != blacklisted_options.end() ) {
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

static computer_action computer_action_from_string( const std::string &str )
{
    static const std::map<std::string, computer_action> actions = {{
            { "null", COMPACT_NULL },
            { "open", COMPACT_OPEN },
            { "open_disarm", COMPACT_OPEN_DISARM },
            { "lock", COMPACT_LOCK },
            { "unlock", COMPACT_UNLOCK },
            { "unlock_disarm", COMPACT_UNLOCK_DISARM },
            { "toll", COMPACT_TOLL },
            { "sample", COMPACT_SAMPLE },
            { "release", COMPACT_RELEASE },
            { "release_bionics", COMPACT_RELEASE_BIONICS },
            { "release_disarm", COMPACT_RELEASE_DISARM },
            { "terminate", COMPACT_TERMINATE },
            { "portal", COMPACT_PORTAL },
            { "cascade", COMPACT_CASCADE },
            { "research", COMPACT_RESEARCH },
            { "maps", COMPACT_MAPS },
            { "map_sewer", COMPACT_MAP_SEWER },
            { "map_subway", COMPACT_MAP_SUBWAY },
            { "miss_disarm", COMPACT_MISS_DISARM },
            { "list_bionics", COMPACT_LIST_BIONICS },
            { "elevator_on", COMPACT_ELEVATOR_ON },
            { "amigara_log", COMPACT_AMIGARA_LOG },
            { "amigara_start", COMPACT_AMIGARA_START },
            { "complete_disable_external_power", COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER },
            { "repeater_mod", COMPACT_REPEATER_MOD },
            { "download_software", COMPACT_DOWNLOAD_SOFTWARE },
            { "blood_anal", COMPACT_BLOOD_ANAL },
            { "data_anal", COMPACT_DATA_ANAL },
            { "disconnect", COMPACT_DISCONNECT },
            { "emerg_mess", COMPACT_EMERG_MESS },
            { "emerg_ref_center", COMPACT_EMERG_REF_CENTER },
            { "tower_unresponsive", COMPACT_TOWER_UNRESPONSIVE },
            { "sr1_mess", COMPACT_SR1_MESS },
            { "sr2_mess", COMPACT_SR2_MESS },
            { "sr3_mess", COMPACT_SR3_MESS },
            { "sr4_mess", COMPACT_SR4_MESS },
            { "srcf_1_mess", COMPACT_SRCF_1_MESS },
            { "srcf_2_mess", COMPACT_SRCF_2_MESS },
            { "srcf_3_mess", COMPACT_SRCF_3_MESS },
            { "srcf_seal_order", COMPACT_SRCF_SEAL_ORDER },
            { "srcf_seal", COMPACT_SRCF_SEAL },
            { "srcf_elevator", COMPACT_SRCF_ELEVATOR },
            { "irradiator", COMPACT_IRRADIATOR },
            { "geiger", COMPACT_GEIGER },
            { "conveyor", COMPACT_CONVEYOR },
            { "shutters", COMPACT_SHUTTERS },
            { "extract_rad_source", COMPACT_EXTRACT_RAD_SOURCE },
            { "deactivate_shock_vent", COMPACT_DEACTIVATE_SHOCK_VENT },
            { "radio_archive", COMPACT_RADIO_ARCHIVE }
        }
    };

    const auto iter = actions.find( str );
    if( iter != actions.end() ) {
        return iter->second;
    }

    debugmsg( "Invalid computer action %s", str );
    return COMPACT_NULL;
}

static computer_failure_type computer_failure_type_from_string( const std::string &str )
{
    static const std::map<std::string, computer_failure_type> fails = {{
            { "null", COMPFAIL_NULL },
            { "shutdown", COMPFAIL_SHUTDOWN },
            { "alarm", COMPFAIL_ALARM },
            { "manhacks", COMPFAIL_MANHACKS },
            { "secubots", COMPFAIL_SECUBOTS },
            { "damage", COMPFAIL_DAMAGE },
            { "pump_explode", COMPFAIL_PUMP_EXPLODE },
            { "pump_leak", COMPFAIL_PUMP_LEAK },
            { "amigara", COMPFAIL_AMIGARA },
            { "destroy_blood", COMPFAIL_DESTROY_BLOOD },
            { "destroy_data", COMPFAIL_DESTROY_DATA }
        }
    };

    const auto iter = fails.find( str );
    if( iter != fails.end() ) {
        return iter->second;
    }

    debugmsg( "Invalid computer failure %s", str );
    return COMPFAIL_NULL;
}
computer_option computer_option::from_json( const JsonObject &jo )
{
    std::string name = jo.get_string( "name" );
    computer_action action = computer_action_from_string( jo.get_string( "action" ) );
    int sec = jo.get_int( "security", 0 );
    return computer_option( name, action, sec );
}

computer_failure computer_failure::from_json( const JsonObject &jo )
{
    computer_failure_type type = computer_failure_type_from_string( jo.get_string( "action" ) );
    return computer_failure( type );
}
