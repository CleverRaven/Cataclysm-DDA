#include "string_id.h"

class activity_type;
class effect_type;
class furn_t;
class harvest_list;
class material_type;
class MonsterGroup;
class mtype;
class mutation_branch;
class oter_t;
class oter_type_t;
class requirement_data;
class species_type;
class ter_t;
class trap;

// These are centralized in this file because they must appear in one and only one compilation unit,
// and it's problematic to define them in the respective cpp files for each class.

template<> const string_id<activity_type> string_id<activity_type>::NULL_ID = string_id<activity_type>( "ACT_NULL", -1 );

template<> const string_id<harvest_list> string_id<harvest_list>::NULL_ID{};

template<> const string_id<effect_type> string_id<effect_type>::NULL_ID = string_id<effect_type>( "null" );

template<> const string_id<furn_t> string_id<furn_t>::NULL_ID = string_id<furn_t>( "f_null", 0 );

template<> const string_id<material_type> string_id<material_type>::NULL_ID = string_id<material_type>( "null", 0 );

template<> const string_id<MonsterGroup> string_id<MonsterGroup>::NULL_ID = string_id<MonsterGroup>( "GROUP_NULL" );

template<> const string_id<mtype> string_id<mtype>::NULL_ID = string_id<mtype>( "mon_null" );

template<> const string_id<mutation_branch> string_id<mutation_branch>::NULL_ID( "" );

template<> const string_id<oter_t> string_id<oter_t>::NULL_ID = string_id<oter_t>("", 0 );

template<> const string_id<oter_type_t> string_id<oter_type_t>::NULL_ID = string_id<oter_type_t>("", 0 );

template <> const string_id<requirement_data> string_id<requirement_data>::NULL_ID = string_id<requirement_data>( "null" );

template<> const string_id<species_type> string_id<species_type>::NULL_ID( "spec_null" );

template<> const string_id<ter_t> string_id<ter_t>::NULL_ID( "t_null", 0 );

template<> const string_id<trap> string_id<trap>::NULL_ID = string_id<trap>( "tr_null" );
