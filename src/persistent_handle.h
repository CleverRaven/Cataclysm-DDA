#ifndef PERSISTENT_HANDLE_H
#define PERSISTENT_HANDLE_H

#include <limits>

#include "json.h"

template <class R> class PersistentControlBlock;
template <class R> class PersistentHandle;
template <class R> class Persistent;

// We take all IDs from the same sequence rather than having
// this as part of the template and having a separate sequence
// for each type. IDs for different types are not interchangeable,
// but ID collisions are confusing. It also means fewer things
// to handle on save/load.
namespace persistent_id_sequence {
    extern long next_id_value;

    void reset( long hint );
};

/// A typed persistent ID; IDs are just longs, but
/// they are not interchangeable between referent types.
///
/// ID 0 has a special meaning ("no object")
/// Negative IDs have userdefined meanings and never map to a real object
template <class R>
class PersistentID : public JsonSerializer, public JsonDeserializer {
public:
    PersistentID() : id( 0 ) {}
    explicit PersistentID( long fromid ) : id( fromid ) {
        if( persistent_id_sequence::next_id_value <= fromid ) {
            persistent_id_sequence::next_id_value = fromid + 1;
        }
    }
    
    PersistentID( const PersistentID<R> &from )
        : JsonSerializer(), JsonDeserializer(), id( from.id )
    {
    }        
    
    PersistentID( PersistentID<R> &&from )
        : id( from.id )
    {
        from.id = 0;
    }        

    PersistentID<R> &operator=( const PersistentID<R> &from ) {
        id = from.id;
        return *this;
    }
    
    PersistentID<R> &operator=( PersistentID<R> &&from ) {
        id = from.id;
        from.id = 0;
        return *this;
    }
    
    operator bool() const {
        return id > 0;
    }

    bool operator<( const PersistentID<R> &other ) const {
        return id < other.id;
    }

    bool operator==( const PersistentID<R> &other ) const {
        return id == other.id;
    }

    bool operator!=( const PersistentID<R> &other ) const {
        return id != other.id;
    }

    explicit operator long() const {
        return id;
    }

    static PersistentID<R> allocate_id() {
        return PersistentID<R>( persistent_id_sequence::next_id_value++ );
    }
    
    // Should be specialized for each type:
    static R *find_by_id( PersistentID<R> id ) {
        return nullptr;
    }

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override {
        jsout.write( id );
    }

    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override {
        id = jsin.get_long();
        if( persistent_id_sequence::next_id_value <= id ) {
            persistent_id_sequence::next_id_value = id + 1;
        }
    }
    
private:
    long id;
};

/// Control block used to maintain weak references to the
/// currently-constructed object for a given persistent
/// ID. There are zero or one control blocks per
/// constructed persistent object, and the control block
/// may outlive the underlying object. Potentially, there
/// might be several different control blocks, all with
/// the same persistent ID, but at most one has a valid
/// referent.
template <class R>
class PersistentControlBlock {
public:
    PersistentControlBlock( R *ref )
        : referent(ref), refcount(1)
    {}

    void incref() {
        if( refcount != std::numeric_limits<unsigned>::max() ) {
            ++refcount;
        }
    }

    /// decrease refcount, delete self if it hits zero
    void decref() {
        if( refcount != std::numeric_limits<unsigned>::max() && refcount != 0 && --refcount == 0 ) {
            delete this;
        }
    }

    R* get_referent() {
        return referent;
    }

    const R* get_referent() const {
        return referent;
    }

    void clear_referent() {
        referent = nullptr;
    }

private:
    R *referent;
    unsigned refcount;
};    

/// A handle to an object with a persistent ID.
/// If there is an in-memory version of the object,
/// the handle will discover it on use. If the
/// object is destroyed, the handle will notice
/// and re-discover any new version of it on use.
template <class R>
class PersistentHandle : public JsonSerializer, public JsonDeserializer {
public:
    /// A null handle that refers to nothing
    PersistentHandle()
        : control( nullptr ), id()
    {}
    
    /// A handle that refers to the given persistent ID
    explicit PersistentHandle( long rid )
        : control( nullptr ), id( rid )
    {}

    PersistentHandle( const PersistentID<R> &rid )
        : JsonSerializer(), JsonDeserializer(), control( nullptr ), id( rid )
    {}

    PersistentHandle( const PersistentHandle &other )
        : control( other.control ), id( other.id )
    {
        if( control ) {
            control->incref();
        }
    }
    
    PersistentHandle( PersistentHandle &&other )
        : control( other.control ), id( std::move( other.id ) )
    {
        other.control = nullptr;
    }
    
    ~PersistentHandle()
    {
        discard();
    }

    PersistentHandle &operator=( const PersistentHandle &other ) {
        if( control != other.control ) {
            if( other.control ) {
                other.control->incref();
            }
            discard();
            control = other->control;
        }

        return *this;
    }
    
    PersistentHandle &operator=( PersistentHandle &&other ) {
        control = other.control;
        other.control = nullptr;
        id = std::move ( other.id );
        return *this;
    }
    
    PersistentID<R> get_persistent_id() const {
        return id;
    }

    explicit operator long() const {
        return ( long )id;
    }

    /// returns true if this is not a null handle
    operator bool() const {
        return !!id;
    }

    /// returns true if there is currently a referent
    /// object for this handle
    bool valid() {
        return get() != nullptr;
    }
    
    /// Try to get a referent object for this handle,
    /// looking it up if needed. Returns nullptr if
    /// this is a null handle or if the lookup failed.
    R *get() {
        if( !id ) {
            return nullptr;
        }

        if( control ) {
            R *ref = control->get_referent();
            if( ref ) {
                return ref;
            }
            discard();
        }

        R *discovered = PersistentID<R>::find_by_id( id );
        if( discovered ) {
            control = static_cast<Persistent<R>*>(discovered)->get_persistent_control();
            control->incref();
            return discovered;
        } else {
            return nullptr;
        }
    }

    /// Return a reference to the referent object.
    /// Asserts if this is the null handle or if the referent
    /// cannot be found
    R &operator*() {
        R *ref = get();
        assert( ref != nullptr );
        return *ref;
    }
    
    /// Return a pointer to the referent object for
    /// call-through behaviour. Asserts if this is the null
    /// handle or if the referent cannot be found
    R *operator->() {
        R *ref = get();
        assert( ref != nullptr );
        return ref;
    }
    
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override {
        jsout.write( id );
    }

    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override {
        discard();
        jsin.read( id );
    }
private:
    /// A handle using the given ID & control block directly.
    PersistentHandle( PersistentControlBlock<R> *cb, PersistentID<R> rid )
        : control( cb ), id( rid )
    {
        control->incref();
    }

    /// If we have a reference to the control block, drop it.
    void discard() {
        if( control ) {
            control->decref();
            control = nullptr;
        }
    }        

    PersistentControlBlock<R> *control;
    PersistentID<R> id;

    friend class Persistent<R>;
};

/// Mixin for objects with a persistent ID
///
/// A persistent object of type R should inherit from Persistent<R>
/// and also specialize PersistentID<R>::find_by_id:
///
///  template<> R *PersistentID<R>::find_by_id( PersistentID<R> id );
///
/// On load, persistent objects should either pass their ID to the ctor or
/// call set_persistent_id() early in their lifetime
template <class R>
class Persistent
{
public:
    /// Return a long-lived handle to this object. If this particular (in-memory)
    /// object is destroyed, or if the handle is serialized and then recovered,
    /// then on next use the handle will try to rediscover the current object
    /// for the underlying persistent ID.
    PersistentHandle<R> get_handle() {
        if( !persistent_id ) {
            // object has no persistent identity yet, assign one
            persistent_id = PersistentID<R>::allocate_id();
        }

        return PersistentHandle<R>( get_persistent_control(), persistent_id );
    }

    /// Returns the persistent ID for this object
    /// Does _not_ assign an ID if not yet persistent
    PersistentID<R> get_persistent_id() const {
        return persistent_id;
    }

protected:
    /// On destruction, if there are any handles directly refererring to this object,
    /// invalidate their referent to force a lookup on next access
    virtual ~Persistent() {
        if( persistent_control ) {
            persistent_control->clear_referent();
            persistent_control->decref();
        }
    }

    /// Constructs an object with no persistent identity
    Persistent()
        : persistent_control( nullptr ), persistent_id()
    {
    }

    /// Constructs an object with a particular persistent ID (e.g. on load)
    Persistent( PersistentID<R> rid )
        : persistent_control( nullptr ), persistent_id( rid )
    {
    }

    /// Set the persistent ID for this object.
    /// Should only be used very early in the object lifetime, before anything
    /// calls get_handle()
    void set_persistent_id(PersistentID<R> id) {
        assert( id == persistent_id || !persistent_control );
        persistent_id = id;
    }

private:
    PersistentControlBlock<R> *get_persistent_control() {
        if( !persistent_control ) {
            persistent_control = new PersistentControlBlock<R>( static_cast<R*>( this ) );
        }
        return persistent_control;
    }
    
    PersistentControlBlock<R> *persistent_control;
    PersistentID<R> persistent_id;

    friend class PersistentHandle<R>;
};

#endif
