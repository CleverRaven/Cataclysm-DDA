#pragma once
#ifndef CATA_SRC_WEIGHTED_DBL_OR_VAR_LIST_H
#define CATA_SRC_WEIGHTED_DBL_OR_VAR_LIST_H

#include "dialogue.h"
#include "weighted_list.h"

// Works similarly to weighted_list except weights are stored as dbl_or_var, so total_weight isn't constant unless all weights are etc

template <typename T> struct weighted_dbl_or_var_list {
        weighted_dbl_or_var_list() = default;
        weighted_dbl_or_var_list( const weighted_dbl_or_var_list & ) = default;
        weighted_dbl_or_var_list( weighted_dbl_or_var_list && ) noexcept = default;
        weighted_dbl_or_var_list &operator=( const weighted_dbl_or_var_list & ) = default;
        weighted_dbl_or_var_list &operator=( weighted_dbl_or_var_list && ) noexcept = default;
        virtual ~weighted_dbl_or_var_list() = default;

        void load( const JsonValue &jsv, const double default_weight ) {
            const dbl_or_var def = dbl_or_var( default_weight );
            for( const JsonValue entry : jsv.get_array() ) {
                if( entry.test_array() ) {
                    JsonArray ja = entry.get_array();
                    T object;
                    ja.next_value().read( object );
                    const dbl_or_var weight = get_dbl_or_var( ja.next_value() );
                    add( object, weight );
                } else {
                    T object;
                    entry.read( object );
                    add( object, def );
                }
            }
            precalc();
        }

        /**
         * This will add a new object to the weighted list. Returns a pointer to
           the added object, or NULL if weight was zero.
         * @param obj The object that will be added to the list.
         * @param weight The weight of the object.
         */
        T *add( const T &obj, const dbl_or_var &weight ) {
            if( weight.is_constant() && weight.min.dbl_val.value() <= 0 ) {
                return nullptr;
            }
            objects.emplace_back( obj, weight );
            invalidate_precalc();
            return &( objects[objects.size() - 1].obj );
        }

        void precalc() {
            for( const weighted_object<dbl_or_var, T>  &object : objects ) {
                _is_constant &= object.weight.is_constant();
            }
            if( _is_constant ) {
                for( const weighted_object<dbl_or_var, T>  &object : objects ) {
                    _constant_total_weight += object.weight.evaluate( d() );
                }
            }
            _precalced = true;
        }



        bool is_constant() const {
            if( !_precalced ) {
                debugmsg( "weighted_dbl_or_var_list precalc has been invalidated, call weighted_dbl_or_var_list::precalc() first" );
            }
            return _is_constant;
        }

        void remove( const T &obj ) {
            auto itr_end = std::remove_if( objects.begin(),
            objects.end(), [&obj]( typename decltype( objects )::value_type const & itr ) {
                return itr.obj == obj;
            } );
            objects.erase( itr_end, objects.end() );
            invalidate_precalc();
        }

        /**
         * This will check to see if the given object is already in the weighted
           list. If it is, we update its weight with the new weight value, or remove if 0 weight. If it
           is not, we add it normally. Returns a pointer to the added or updated
           object, or NULL if weight was zero.
         * @param obj The object that will be updated or added to the list.
         * @param weight The new weight of the object.
         */
        T *add_or_replace( const T &obj, const dbl_or_var &weight ) {

            if( weight.is_constant() && weight.min.dbl_val.value() <= 0 ) {
                remove( obj );
                invalidate_precalc();
                return nullptr;
            }
            for( weighted_object<dbl_or_var, T> &itr : objects ) {
                if( itr.obj == obj ) {
                    if( itr.weight != weight ) {
                        itr.weight = weight;
                        invalidate_precalc();
                    }
                    return &( itr.obj );
                }
            }
            // if not found, add to end of list
            invalidate_precalc();
            return add( obj, weight );
        }

        /**
         * This will call the given callback function once for every object in the weighted list.
         * @param func The callback function.
         */
        void apply( std::function<void( const T & )> func ) const {
            for( const weighted_object<dbl_or_var, T> &itr : objects ) {
                func( itr.obj );
            }
        }

        /**
         * This will call the given callback function once for every object in the weighted list.
         * This is the non-const version.
         * @param func The callback function.
         */
        void apply( std::function<void( T & )> func ) {
            for( const weighted_object<dbl_or_var, T> &itr : objects ) {
                func( itr.obj );
            }
        }


        /**
         * This will return a pointer to an object from the list randomly selected
         * and biased by weight. If the weighted list is empty or all items in it
         * have a weight of zero, it will return a NULL pointer.
         */
        const T *pick( unsigned int randi ) const {
            if( get_weight() > 0 ) {
                return &( objects[pick_ent( randi )].obj );
            }
            return nullptr;
        }
        const T *pick() const {
            return pick( rng_bits() );
        }

        /**
         * This will return a pointer to an object from the list randomly selected
         * and biased by weight. If the weighted list is empty or all items in it
         * have a weight of zero, it will return a NULL pointer. This is the
         * non-const version so that the returned result may be modified.
         */
        T *pick( unsigned int randi ) {
            if( get_weight() > 0 ) {
                return &( objects[pick_ent( randi )].obj );
            }
            return nullptr;
        }
        T *pick() {
            return pick( rng_bits() );
        }

        /**
         * This will remove all objects from the list.
         */
        void clear() {
            objects.clear();
            invalidate_precalc();
        }

        /**
         * This will return the weight of a specific object. If the object is not
         * in the weighted list it will return 0.
         */
        double get_specific_weight( const T &obj ) const {
            for( const weighted_object<dbl_or_var, T> &itr : objects ) {
                if( itr.obj == obj ) {
                    return itr.weight.evaluate( d() );
                }
            }
            return 0;
        }

        /**
         * This will return the sum of all the object's weights in the list.
         */
        double get_weight() const {
            if( is_constant() ) {
                return _constant_total_weight;
            } else {
                double ret = 0.0;
                for( const weighted_object<dbl_or_var, T> &itr : objects ) {
                    ret += itr.weight.evaluate( d() );
                }
                return ret;
            }
        }

        bool is_valid() {
            return !is_constant() || _constant_total_weight > 0.0;
        }

        typename std::vector<weighted_object<dbl_or_var, T> >::iterator begin() {
            return objects.begin();
        }
        typename std::vector<weighted_object<dbl_or_var, T> >::iterator end() {
            return objects.end();
        }
        typename std::vector<weighted_object<dbl_or_var, T> >::const_iterator begin() const {
            return objects.begin();
        }
        typename std::vector<weighted_object<dbl_or_var, T> >::const_iterator end() const {
            return objects.end();
        }
        typename std::vector<weighted_object<dbl_or_var, T> >::iterator erase(
            typename std::vector<weighted_object<dbl_or_var, T> >::iterator first,
            typename std::vector<weighted_object<dbl_or_var, T> >::iterator last ) {
            return objects.erase( first, last );
        }
        size_t size() const noexcept {
            return objects.size();
        }
        bool empty() const noexcept {
            return objects.empty();
        }

        std::string to_debug_string() const {
            std::ostringstream os;
            os << "[ ";
            for( const weighted_object<dbl_or_var, T> &o : objects ) {
                os << o.obj << ":" << o.weight.evaluate( d() ) << ", ";
            }
            os << "]";
            return os.str();
        }

        friend bool operator==( const weighted_dbl_or_var_list &l, const weighted_dbl_or_var_list &r ) {
            return l.objects == r.objects;
        }

        friend bool operator!=( const weighted_dbl_or_var_list &l, const weighted_dbl_or_var_list &r ) {
            return !( l == r );
        }

    protected:
        double _constant_total_weight = 0.0;
        bool _precalced = true; // True for empty lists
        bool _is_constant = true;
        std::vector<weighted_object<dbl_or_var, T>> objects;

        size_t pick_ent( unsigned int randi ) const {
            const double picked = static_cast<double>( randi ) / UINT_MAX * get_weight();
            double accumulated_weight = 0.0;
            size_t i;
            if( is_constant() ) {
                for( i = 0; i < objects.size(); i++ ) {
                    accumulated_weight += objects[i].weight.min.dbl_val.value();
                    if( accumulated_weight >= picked ) {
                        break;
                    }
                }
            } else {
                for( i = 0; i < objects.size(); i++ ) {
                    accumulated_weight += objects[i].weight.evaluate( d() );
                    if( accumulated_weight >= picked ) {
                        break;
                    }
                }
            }
            return i;
        }

    private:
        dialogue d() const {
            return { get_talker_for( get_avatar() ), std::make_unique<talker>() };
        }
        void invalidate_precalc() {
            _is_constant = true;
            _constant_total_weight = 0.0;
            _precalced = false;
        }
};

#endif // CATA_SRC_WEIGHTED_DBL_OR_VAR_LIST_H
