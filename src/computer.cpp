#include "computer.h"

#include <algorithm>
#include <cstdlib>
#include <locale>
#include <sstream>

#include "debug.h"
#include "enum_conversions.h"
#include "json.h"
#include "output.h"
#include "translations.h"

template <typename E> struct enum_traits;

computer_option::computer_option()
    : name( "Unknown" ), action( COMPACT_NULL ), security( 0 )
{
}

computer_option::computer_option( const std::string &N, computer_action A, int S )
    : name( N ), action( A ), security( S )
{
}

void computer_option::serialize( JsonOut &jout ) const
{
    jout.start_object();
    jout.member( "name", name );
    jout.member( "action" );
    jout.write_as_string( action );
    jout.member( "security", security );
    jout.end_object();
}

void computer_option::deserialize( JsonIn &jin )
{
    const JsonObject jo = jin.get_object();
    name = jo.get_string( "name" );
    action = jo.get_enum_value<computer_action>( "action" );
    security = jo.get_int( "security" );
}

computer_failure::computer_failure()
    : type( COMPFAIL_NULL )
{
}

void computer_failure::serialize( JsonOut &jout ) const
{
    jout.start_object();
    jout.member( "action" );
    jout.write_as_string( type );
    jout.end_object();
}

void computer_failure::deserialize( JsonIn &jin )
{
    const JsonObject jo = jin.get_object();
    type = jo.get_enum_value<computer_failure_type>( "action" );
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

static computer_action computer_action_from_legacy_enum( int val );
static computer_failure_type computer_failure_type_from_legacy_enum( int val );

void computer::load_legacy_data( const std::string &data )
{
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
        // Legacy missle launch option that got removed before `computer_action` was
        // refactored to be saved and loaded as string ids. Do not change this number:
        // `computer_action` now has different underlying values from back then!
        if( tmpaction == 15 ) {
            continue;
        }
        add_option( string_replace( tmpname, "_", " " ), computer_action_from_legacy_enum( tmpaction ),
                    tmpsec );
    }

    // Pull in failures
    int failsize;
    dump >> failsize;
    for( int n = 0; n < failsize; n++ ) {
        int tmpfail;
        dump >> tmpfail;
        add_failure( computer_failure_type_from_legacy_enum( tmpfail ) );
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

void computer::serialize( JsonOut &jout ) const
{
    jout.start_object();
    jout.member( "name", name );
    jout.member( "mission", mission_id );
    jout.member( "security", security );
    jout.member( "alerts", alerts );
    jout.member( "next_attempt", next_attempt );
    jout.member( "options", options );
    jout.member( "failures", failures );
    jout.member( "access_denied", access_denied );
    jout.end_object();
}

void computer::deserialize( JsonIn &jin )
{
    if( jin.test_string() ) {
        load_legacy_data( jin.get_string() );
    } else {
        const JsonObject jo = jin.get_object();
        jo.read( "name", name );
        jo.read( "mission", mission_id );
        jo.read( "security", security );
        jo.read( "alerts", alerts );
        jo.read( "next_attempt", next_attempt );
        jo.read( "options", options );
        jo.read( "failures", failures );
        jo.read( "access_denied", access_denied );
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

static computer_action computer_action_from_legacy_enum( const int val )
{
    switch( val ) {
        // Used to migrate old saves. Do not change the numbers!
        // *INDENT-OFF*
        default: return COMPACT_NULL;
        case 0: return COMPACT_NULL;
        case 1: return COMPACT_OPEN;
        case 2: return COMPACT_LOCK;
        case 3: return COMPACT_UNLOCK;
        case 4: return COMPACT_TOLL;
        case 5: return COMPACT_SAMPLE;
        case 6: return COMPACT_RELEASE;
        case 7: return COMPACT_RELEASE_BIONICS;
        case 8: return COMPACT_TERMINATE;
        case 9: return COMPACT_PORTAL;
        case 10: return COMPACT_CASCADE;
        case 11: return COMPACT_RESEARCH;
        case 12: return COMPACT_MAPS;
        case 13: return COMPACT_MAP_SEWER;
        case 14: return COMPACT_MAP_SUBWAY;
        // options with action enum 15 are removed in load_legacy_data()
        case 16: return COMPACT_MISS_DISARM;
        case 17: return COMPACT_LIST_BIONICS;
        case 18: return COMPACT_ELEVATOR_ON;
        case 19: return COMPACT_AMIGARA_LOG;
        case 20: return COMPACT_AMIGARA_START;
        case 21: return COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER;
        case 22: return COMPACT_REPEATER_MOD;
        case 23: return COMPACT_DOWNLOAD_SOFTWARE;
        case 24: return COMPACT_BLOOD_ANAL;
        case 25: return COMPACT_DATA_ANAL;
        case 26: return COMPACT_DISCONNECT;
        case 27: return COMPACT_EMERG_MESS;
        case 28: return COMPACT_EMERG_REF_CENTER;
        case 29: return COMPACT_TOWER_UNRESPONSIVE;
        case 30: return COMPACT_SR1_MESS;
        case 31: return COMPACT_SR2_MESS;
        case 32: return COMPACT_SR3_MESS;
        case 33: return COMPACT_SR4_MESS;
        case 34: return COMPACT_SRCF_1_MESS;
        case 35: return COMPACT_SRCF_2_MESS;
        case 36: return COMPACT_SRCF_3_MESS;
        case 37: return COMPACT_SRCF_SEAL_ORDER;
        case 38: return COMPACT_SRCF_SEAL;
        case 39: return COMPACT_SRCF_ELEVATOR;
        case 40: return COMPACT_OPEN_DISARM;
        case 41: return COMPACT_UNLOCK_DISARM;
        case 42: return COMPACT_RELEASE_DISARM;
        case 43: return COMPACT_IRRADIATOR;
        case 44: return COMPACT_GEIGER;
        case 45: return COMPACT_CONVEYOR;
        case 46: return COMPACT_SHUTTERS;
        case 47: return COMPACT_EXTRACT_RAD_SOURCE;
        case 48: return COMPACT_DEACTIVATE_SHOCK_VENT;
        case 49: return COMPACT_RADIO_ARCHIVE;
        // *INDENT-ON*
    }
}

static computer_failure_type computer_failure_type_from_legacy_enum( const int val )
{
    switch( val ) {
        // Used to migrate old saves. Do not change the numbers!
        // *INDENT-OFF*
        default: return COMPFAIL_NULL;
        case 0: return COMPFAIL_NULL;
        case 1: return COMPFAIL_SHUTDOWN;
        case 2: return COMPFAIL_ALARM;
        case 3: return COMPFAIL_MANHACKS;
        case 4: return COMPFAIL_SECUBOTS;
        case 5: return COMPFAIL_DAMAGE;
        case 6: return COMPFAIL_PUMP_EXPLODE;
        case 7: return COMPFAIL_PUMP_LEAK;
        case 8: return COMPFAIL_AMIGARA;
        case 9: return COMPFAIL_DESTROY_BLOOD;
        case 10: return COMPFAIL_DESTROY_DATA;
        // *INDENT-ON*
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
    translation name;
    jo.read( "name", name );
    const computer_action action = jo.get_enum_value<computer_action>( "action" );
    const int sec = jo.get_int( "security", 0 );
    return computer_option( name.translated(), action, sec );
}

computer_failure computer_failure::from_json( const JsonObject &jo )
{
    const computer_failure_type type = jo.get_enum_value<computer_failure_type>( "action" );
    return computer_failure( type );
}
