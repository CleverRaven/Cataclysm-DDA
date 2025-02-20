#pragma once
#ifndef CATA_SRC_TRAIT_GROUP_H
#define CATA_SRC_TRAIT_GROUP_H

#include <memory>
#include <string>
#include <vector>

#include "type_id.h"

class JsonObject;
class JsonValue;
class Trait_group;
struct trait_and_var;

namespace trait_group
{

using Trait_group_tag = string_id<Trait_group>;
using Trait_list = std::vector<trait_and_var>;

/**
 * Returns a randomized list of traits from the given trait group.
 */
Trait_list traits_from( const Trait_group_tag &gid );

/**
 * Check whether a particular trait is a member of the given group.
 */
bool group_contains_trait( const Trait_group_tag &gid, const trait_id &tid );

/**
 * See @ref mutation_branch::load_trait_group
 */
void load_trait_group( const JsonObject &jsobj, const Trait_group_tag &gid,
                       const std::string &subtype );

/**
 * Get a trait group ID and optionally load an inlined trait group.
 *
 * If the value is string, it's assumed to be a trait group id and it's
 * returned directly.
 *
 * If the value is a JSON object, it is loaded as a trait group. The group will be given a
 * unique id (if the JSON object contains an id, it is ignored) and that id will be returned.
 * If the JSON object does not contain a subtype, the given default is used.
 *
 * If the value is a JSON array, it is loaded as a trait group: the default_subtype will be
 * used as subtype of the new trait group and the array is loaded like the "entries" array of
 * a trait group definition (see format of trait groups).
 *
 * @param default_subtype If an inlined trait group is loaded this is used as the default
 * subtype. It must be either "distribution" or "collection". See @ref Trait_group.
 * @throw JsonError as usual for JSON errors, including invalid input values.
 */
Trait_group_tag load_trait_group( const JsonValue &value, const std::string &default_subtype );

/**
 * Show a debug menu for testing trait groups.
 */
void debug_spawn();

} // namespace trait_group

/**
 * Base interface for trait generation.
 * Used to generate a list of traits.
 */
class Trait_creation_data
{
    public:
        using RecursionList = std::vector<trait_group::Trait_group_tag>;

        explicit Trait_creation_data( int _probability ) : probability( _probability ) {}
        virtual ~Trait_creation_data() = default;
        Trait_creation_data( const Trait_creation_data & ) = delete;
        Trait_creation_data &operator=( const Trait_creation_data & ) = delete;

        /**
         * Create a list of traits. The resulting list might be empty.
         * @param[out] rec Recursion list, output goes here
         */
        virtual trait_group::Trait_list create( RecursionList &rec ) const = 0;
        trait_group::Trait_list create() const;

        /**
         * Check the trait templates for consistency (valid trait types, etc).
         */
        virtual void check_consistency() const = 0;

        /**
         * Remove the given trait from this and all linked groups.
         * Used for blacklisting.
         */
        virtual bool remove_trait( const trait_id &tid ) = 0;

        /**
         * Check if this group or any linked groups contain the given trait.
         */
        virtual bool has_trait( const trait_id &tid ) const = 0;

        // Probability, used by the parent object.
        int probability;
};

/**
 * Interface for creating a single trait. This corresponds to JSON objects like
 * { "trait": "TRAIT_NAME", prob: 100 }
 */
class Single_trait_creator : public Trait_creation_data
{
    public:
        Single_trait_creator( const trait_id &id, const std::string &var, int probability );
        ~Single_trait_creator() override = default;

        trait_id id;
        std::string variant;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void check_consistency() const override;
        bool remove_trait( const trait_id &tid ) override;
        bool has_trait( const trait_id &tid ) const override;
};

/**
 * Interface for creating a group of traits. This corresponds to a JSON list
 * of traits.
 */
class Trait_group_creator : public Trait_creation_data
{
    public:
        Trait_group_creator( const trait_group::Trait_group_tag &id, int probability );
        ~Trait_group_creator() override = default;

        trait_group::Trait_group_tag id;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void check_consistency() const override;
        bool remove_trait( const trait_id &tid ) override;
        bool has_trait( const trait_id &tid ) const override;
};

/**
 * A group of traits to generate.
 */
class Trait_group : public Trait_creation_data
{
    public:
        using CreatorList = std::vector<std::unique_ptr<Trait_creation_data> >;

        explicit Trait_group( int probability );
        ~Trait_group() override = default;

        void add_trait_entry( const trait_id &tid, const std::string &var, int probability );
        void add_group_entry( const trait_group::Trait_group_tag &gid, int probability );

        /**
         * Once the relevant data has been read from JSON, this function is always called (either from
         * @ref mutation_branch::add_entry, @ref add_trait_entry or @ref add_group_entry). Its purpose
         * is to add a Single_trait_creator or Trait_group_creator to @ref creators.
         */
        virtual void add_entry( std::unique_ptr<Trait_creation_data> ptr ) = 0;

        void check_consistency() const override;
        bool remove_trait( const trait_id &tid ) override;
        bool has_trait( const trait_id &tid ) const override;

    protected:
        // Links to all entries in this group.
        CreatorList creators;
        // The sum of the probability of all entries contained in this group.
        int sum_prob;
};

/**
 * A collection of traits to generate. At creation time, each trait will be
 * included (or not) based on its own probability roll.
 */
class Trait_group_collection : public Trait_group
{
    public:
        explicit Trait_group_collection( int probability );
        ~Trait_group_collection() override = default;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void add_entry( std::unique_ptr<Trait_creation_data> ptr ) override;
};

/**
 * A distribution of traits to generate. At creation time, each trait will be
 * included (or not) based on its own probability measured against the sum
 * of all other traits' probabilities in the distribution.
 */
class Trait_group_distribution : public Trait_group
{
    public:
        explicit Trait_group_distribution( int probability ) :
            Trait_group( probability ) {}
        ~Trait_group_distribution() override = default;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void add_entry( std::unique_ptr<Trait_creation_data> ptr ) override;
};

#endif // CATA_SRC_TRAIT_GROUP_H
