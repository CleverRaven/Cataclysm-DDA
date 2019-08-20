#pragma once
#ifndef MATTACK_COMMON_H
#define MATTACK_COMMON_H

#include <memory>
#include <string>

class JsonObject;
class monster;

using mattack_id = std::string;
using mon_action_attack = bool ( * )( monster * );

class mattack_actor
{
    protected:
        mattack_actor() = default;
    public:
        mattack_actor( const mattack_id &new_id ) : id( new_id ) { }

        mattack_id id;
        bool was_loaded = false;

        int cooldown = 0;

        void load( JsonObject &jo, const std::string &src );

        virtual ~mattack_actor() = default;
        virtual bool call( monster & ) const = 0;
        virtual mattack_actor *clone() const = 0;
        virtual void load_internal( JsonObject &jo, const std::string &src ) = 0;
};

struct mtype_special_attack {
    protected:
        // TODO: Remove friend
        friend struct mtype;
        std::unique_ptr<mattack_actor> actor;

    public:
        mtype_special_attack( const mattack_id &id, mon_action_attack f );
        mtype_special_attack( mattack_actor *f ) : actor( f ) { }
        mtype_special_attack( const mtype_special_attack &other ) :
            mtype_special_attack( other.actor->clone() ) { }

        ~mtype_special_attack() = default;

        mtype_special_attack &operator=( const mtype_special_attack &other ) {
            actor.reset( other.actor->clone() );
            return *this;
        }

        const mattack_actor &operator*() const {
            return *actor;
        }

        const mattack_actor *operator->() const {
            return actor.get();
        }

        const mattack_actor *get() const {
            return actor.get();
        }
};

#endif
