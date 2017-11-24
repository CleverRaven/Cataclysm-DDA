#pragma once
#ifndef TRAIT_GROUP_H
#define TRAIT_GROUP_H

#include "json.h"
#include "mutation.h"
#include "string.h"
#include "string_id.h"

namespace trait_group
{

typedef string_id<mutation_branch> Trait_id;
typedef std::string Trait_group_tag;
typedef std::vector<Trait_id> Trait_list;

Trait_list traits_from( const Trait_group_tag &gid );
bool group_contains_trait( const Trait_group_tag &gid, const Trait_id &tid );
bool group_is_defined( const Trait_group_tag &gid );
void load_trait_group( JsonObject &jsobj, const Trait_group_tag &gid, const std::string &subtype );
Trait_group_tag load_trait_group( JsonIn &stream, const std::string &default_subtype );

class Trait_creation_data
{
    public:
        typedef std::vector<Trait_group_tag> RecursionList;

        Trait_creation_data( int _probability ) : probability( _probability ) {}
        virtual ~Trait_creation_data() {};

        virtual Trait_list create( RecursionList &rec ) const = 0;
        Trait_list create() const;

        virtual void check_consistency() const = 0;
        virtual bool remove_trait( const Trait_id &tid ) = 0;
        virtual bool has_trait( const Trait_id &tid ) const = 0;

        int probability;
    private:
        // not implemented
        Trait_creation_data( const Trait_creation_data & );
        Trait_creation_data &operator=( const Trait_creation_data & );
};

class Single_trait_creator : public Trait_creation_data
{
    public:
        Single_trait_creator( const Trait_id &id, int probability );

        Trait_id id;

        virtual Trait_list create( RecursionList &rec ) const;
        virtual void check_consistency() const;
        virtual bool remove_trait( const Trait_id &tid );
        virtual bool has_trait( const Trait_id &tid ) const;
};

class Trait_group_creator : public Trait_creation_data
{
    public:
        Trait_group_creator( const Trait_group_tag &id, int probability );

        Trait_group_tag id;

        virtual Trait_list create( RecursionList &rec ) const;
        virtual void check_consistency() const;
        virtual bool remove_trait( const Trait_id &tid );
        virtual bool has_trait( const Trait_id &tid ) const;
};

class Trait_group : public Trait_creation_data
{
    public:
        typedef std::vector<Trait_creation_data *> CreatorList;

        Trait_group( int probability );
        ~Trait_group() override;

        void add_trait_entry( const Trait_id &tid, int probability );
        void add_group_entry( const Trait_group_tag &gid, int probability );
        virtual void add_entry( std::unique_ptr<Trait_creation_data> &ptr ) = 0;

        virtual void check_consistency() const override;
        virtual bool remove_trait( const Trait_id &tid ) override;
        virtual bool has_trait( const Trait_id &tid ) const override;

    protected:
        int sum_prob;
        CreatorList creators;
};

class Trait_group_collection : public Trait_group
{
    public:
        Trait_group_collection( int probability );

        virtual Trait_list create( RecursionList &rec ) const override;
        virtual void add_entry( std::unique_ptr<Trait_creation_data> &ptr ) override;
};

class Trait_group_distribution : public Trait_group
{
    public:
        Trait_group_distribution( int probability ) :
            Trait_group( probability ) {};

        virtual Trait_list create( RecursionList &rec ) const override;
        virtual void add_entry( std::unique_ptr<Trait_creation_data> &ptr ) override;
};

}

#endif
