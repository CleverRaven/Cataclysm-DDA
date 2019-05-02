#pragma once
#ifndef WEIGHTED_LIST_H
#define WEIGHTED_LIST_H

#include "rng.h"

#include <climits>
#include <cstdlib>
#include <functional>
#include <vector>

template <typename W, typename T> struct weighted_object {
    weighted_object( const T &obj, const W &weight ) : obj( obj ), weight( weight ) {}

    T obj;
    W weight;
};

template <typename W, typename T> struct weighted_list {
        weighted_list() : total_weight( 0 ) { }

        virtual ~weighted_list() = default;

        /**
         * This will add a new object to the weighted list. Returns a pointer to
           the added object, or NULL if weight was zero.
         * @param obj The object that will be added to the list.
         * @param weight The weight of the object.
         */
        T *add( const T &obj, const W &weight ) {
            if( weight >= 0 ) {
                objects.emplace_back( obj, weight );
                total_weight += weight;
                invalidate_precalc();
                return &( objects[objects.size() - 1].obj );
            }
            return nullptr;
        }

        /**
         * This will check to see if the given object is already in the weighted
           list. If it is, we update its weight with the new weight value. If it
           is not, we add it normally. Returns a pointer to the added or updated
           object, or NULL if weight was zero.
         * @param obj The object that will be updated or added to the list.
         * @param weight The new weight of the object.
         */
        T *add_or_replace( const T &obj, const W &weight ) {
            if( weight >= 0 ) {
                invalidate_precalc();
                for( auto &itr : objects ) {
                    if( itr.obj == obj ) {
                        total_weight += ( weight - itr.weight );
                        itr.weight = weight;
                        return &( itr.obj );
                    }
                }

                // if not found, add to end of list
                return add( obj, weight );
            }
            return nullptr;
        }

        /**
         * This will call the given callback function once for every object in the weighted list.
         * @param func The callback function.
         */
        void apply( std::function<void( const T & )> func ) const {
            for( auto &itr : objects ) {
                func( itr.obj );
            }
        }

        /**
         * This will call the given callback function once for every object in the weighted list.
         * This is the non-const version.
         * @param func The callback function.
         */
        void apply( std::function<void( T & )> func ) {
            for( auto &itr : objects ) {
                func( itr.obj );
            }
        }

        /**
         * This will return a pointer to an object from the list randomly selected
         * and biased by weight. If the weighted list is empty or all items in it
         * have a weight of zero, it will return a NULL pointer.
         */
        const T *pick( unsigned int randi ) const {
            if( total_weight > 0 ) {
                return &( objects[pick_ent( randi )].obj );
            } else {
                return nullptr;
            }
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
            if( total_weight > 0 ) {
                return &( objects[pick_ent( randi )].obj );
            } else {
                return nullptr;
            }
        }
        T *pick() {
            return pick( rng_bits() );
        }

        /**
         * This will remove all objects from the list.
         */
        void clear() {
            total_weight = 0;
            objects.clear();
            invalidate_precalc();
        }

        /**
         * This will return the weight of a specific object. If the object is not
         * in the weighted list it will return 0.
         */
        W get_specific_weight( const T &obj ) const {
            for( auto &itr : objects ) {
                if( itr.obj == obj ) {
                    return itr.weight;
                }
            }
            return 0;
        }

        /**
         * This will return the sum of all the object's weights in the list.
         */
        W get_weight() const {
            return total_weight;
        }

        typename std::vector<weighted_object<W, T> >::iterator begin() {
            return objects.begin();
        }
        typename std::vector<weighted_object<W, T> >::iterator end() {
            return objects.end();
        }
        typename std::vector<weighted_object<W, T> >::const_iterator begin() const {
            return objects.begin();
        }
        typename std::vector<weighted_object<W, T> >::const_iterator end() const {
            return objects.end();
        }
        typename std::vector<weighted_object<W, T> >::iterator erase(
            typename std::vector<weighted_object<W, T> >::iterator first,
            typename std::vector<weighted_object<W, T> >::iterator last ) {
            invalidate_precalc();
            return objects.erase( first, last );
        }
        size_t size() const noexcept {
            return objects.size();
        }
        bool empty() const noexcept {
            return objects.empty();
        }

        void precalc();

    protected:
        W total_weight;
        std::vector<weighted_object<W, T> > objects;

        virtual size_t pick_ent( unsigned int ) const = 0;
        virtual void invalidate_precalc() {}
};

template <typename T> struct weighted_int_list : public weighted_list<int, T> {

        // populate the precalc_array for O(1) lookups
        void precalc() {
            precalc_array.clear();
            precalc_array.reserve( this->total_weight ); // to avoid additional reallocations
            // weights [3,1,5] will produce vector of indices [0,0,0,1,2,2,2,2,2]
            for( size_t i = 0; i < this->objects.size(); i++ ) {
                precalc_array.resize( precalc_array.size() + this->objects[i].weight, i );
            }
        }

    protected:

        size_t pick_ent( unsigned int randi ) const override {
            if( this->objects.size() == 1 ) {
                return 0;
            }
            size_t i;
            const int picked = ( randi % ( this->total_weight ) ) + 1;
            if( !precalc_array.empty() ) {
                // if the precalc_array is populated, use it for O(1) lookup
                i = precalc_array[picked - 1];
            } else {
                // otherwise do O(N) search through items
                int accumulated_weight = 0;
                for( i = 0; i < this->objects.size(); i++ ) {
                    accumulated_weight += this->objects[i].weight;
                    if( accumulated_weight >= picked ) {
                        break;
                    }
                }
            }
            return i;
        }

        void invalidate_precalc() override {
            precalc_array.clear();
        }

        std::vector<int> precalc_array;
};

template <typename T> struct weighted_float_list : public weighted_list<double, T> {

        // TODO: precalc using alias method

    protected:

        size_t pick_ent( unsigned int randi ) const override {
            const double picked = static_cast<double>( randi ) / UINT_MAX * this->total_weight;
            double accumulated_weight = 0;
            size_t i;
            for( i = 0; i < this->objects.size(); i++ ) {
                accumulated_weight += this->objects[i].weight;
                if( accumulated_weight >= picked ) {
                    break;
                }
            }
            return i;
        }

};

#endif
