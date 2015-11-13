#ifndef WEIGHTED_LIST_H
#define WEIGHTED_LIST_H

#include "rng.h"
#include <vector>
#include <functional>
#include <cmath>

template <typename W, typename T> struct weighted_object {
    weighted_object(const T &obj, const W &weight) : obj(obj), weight(weight) {}

    T obj;
    W weight;
};

template <typename W, typename T> struct weighted_list {
    weighted_list() : total_weight(0) { };

    /**
     * This will add a new object to the weighted list.
     * @param obj The object that will be added to the list.
     * @param weight The weight of the object.
     */
    void add(const T &obj, const W &weight) {
        if(weight >= 0) {
            objects.emplace_back(obj, weight);
            total_weight += weight;
        }
    }

    /**
     * This will check to see if the given object is already in the weighted
       list. If it is, we update its weight with the new weight value. If it
       is not, we add it normally.
     * @param obj The object that will be updated or added to the list.
     * @param weight The new weight of the object.
     */
    void add_or_replace(const T &obj, const W &weight) {
        if(weight >= 0) {
            for(auto &itr : objects) {
                if(itr.obj == obj) {
                    total_weight += (weight - itr.weight);
                    itr.weight = weight;
                    return;
                }
            }

            // if not found, add to end of list
            add(obj, weight);
        }
    }

    /**
     * This will call the given callback function once for every object in the weighted list.
     * @param func The callback function.
     */
    void apply(std::function<void(const T&)> func) const {
        for(auto &itr : objects) {
            func(itr.obj);
        }
    }

    /**
     * This will call the given callback function once for every object in the weighted list.
     * This is the non-const version.
     * @param func The callback function.
     */
    void apply(std::function<void(T&)> func) {
        for(auto &itr : objects) {
            func(itr.obj);
        }
    }

    /**
     * This will return a pointer to an object from the list randomly selected
     * and biased by weight. If the weighted list is empty or all items in it
     * have a weight of zero, it will return a NULL pointer.
     */
    const T* pick() const {
        if(total_weight > 0) {
            return &(objects[pick_ent()].obj);
        }
        else {
            return NULL;
        }
    }

    /**
     * This will return a pointer to an object from the list randomly selected
     * and biased by weight. If the weighted list is empty or all items in it
     * have a weight of zero, it will return a NULL pointer. This is the
     * non-const version so that the returned result may be modified.
     */
    T* pick() {
        if(total_weight > 0) {
            return &(objects[pick_ent()].obj);
        }
        else {
            return NULL;
        }
    }

    /**
     * This will remove all objects from the list.
     */
    void clear() {
        total_weight = 0;
        objects.clear();
    }

    /**
     * This will return the weight of a specific object. If the object is not
     * in the weighted list it will return 0.
     */
     double get_specific_weight(const T &obj) const {
        for(auto &itr : objects) {
            if(itr.obj == obj) {
                return itr.weight;
            }
        }
        return 0;
     }

    /**
     * This will return the sum of all the object's weights in the list.
     */
    double get_weight() const {
        return total_weight;
    }

protected:
    W total_weight;
    std::vector<weighted_object<W, T> > objects;

    virtual size_t pick_ent() const =0;
};

template <typename T> struct weighted_int_list : public weighted_list<int, T> {

protected:

    size_t pick_ent() const override {
        int picked = rng(1, this->total_weight);
        int accumulated_weight = 0;
        size_t i;
        for(i=0; i < this->objects.size(); i++) {
            accumulated_weight += this->objects[i].weight;
            if(accumulated_weight >= picked) {
                break;
            }
        }
        return i;
    }
};

template <typename T> struct weighted_float_list : public weighted_list<double, T> {

protected:

    size_t pick_ent() const override {
        double picked = rng_float(nextafter(0.0, 1.0), this->total_weight);
        double accumulated_weight = 0;
        size_t i;
        for(i=0; i < this->objects.size(); i++) {
            accumulated_weight += this->objects[i].weight;
            if(accumulated_weight >= picked) {
                break;
            }
        }
        return i;
    }
};


#endif
