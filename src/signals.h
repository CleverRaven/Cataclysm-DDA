
// Template courtesy of Simon Schneegans, see: http://simmesimme.github.io/tutorials/2015/09/20/signal-slot

#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <functional>
#include <map>


template <typename... Args>
class Signal
{

    public:

        Signal() : current_id_( 0 ) {}

        // copy creates new Signal
        Signal( Signal const & ) : current_id_( 0 ) {}

        // connects a member function to this Signal
        template <typename T>
        int connect_member( T *inst, void ( T::*func )( Args... ) ) {
            return connect( [ = ]( Args... args ) {
                ( inst->*func )( args... );
            } );
        }

        // connects a const member function to this Signal
        template <typename T>
        int connect_member( T *inst, void ( T::*func )( Args... ) const ) {
            return connect( [ = ]( Args... args ) {
                ( inst->*func )( args... );
            } );
        }

        // connects a std::function to the Signal. The returned
        // value can be used to disconnect the function again
        int connect( std::function<void( Args... )> const &slot ) const {
            slots_.insert( std::make_pair( ++current_id_, slot ) );
            return current_id_;
        }

        // disconnects a previously connected function
        void disconnect( int id ) const {
            slots_.erase( id );
        }

        // disconnects all previously connected functions
        void disconnect_all() const {
            slots_.clear();
        }

        // calls all connected functions
        void emit( Args... p ) {
            for( auto it : slots_ ) {
                it.second( p... );
            }
        }

        // assignment creates new Signal
        Signal &operator=( Signal const & ) {
            disconnect_all();
        }

    private:
        mutable std::map<int, std::function<void( Args... )>> slots_;
        mutable int current_id_;
};

#endif /* SIGNALS_H */
