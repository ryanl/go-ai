#ifndef __STATIC_VECTOR_HPP
#define __STATIC_VECTOR_HPP

#include "assert.h"
#include "boost/type_traits/alignment_of.hpp"
#include "boost/type_traits/type_with_alignment.hpp"
/*!
@class StaticVector

@brief This is a replacement for vector of elements of type T
       when an upper-bound on the size of the array (n) is known
       in advance.

StaticVector doesn't dynamically allocate memory. The idea is that working on the stack will be a little faster heap allocation. The downside is that StaticVector uses more memory than necessary if the actual size is less than the maximum. Also, you must be careful when allocating arrays on the stack not to run out of stack space.

StaticVector does not call constructors or destructors.

@author Ryan Lothian
*/


template <class T, unsigned int n>
class StaticVector {
private:
    unsigned int entries;


#ifdef STATIC_VECTOR_NOINIT_HACK
    union dummy_u
    {
        char data[ sizeof(T) ];
        BOOST_DEDUCED_TYPENAME boost::type_with_alignment<
          ::boost::alignment_of<T>::value >::type aligner_;
    } _vec[n] ;
    T* vec;

#else
    T vec[n];
#endif

public:
    /*!
        Sets the number of entries to 0 (but does not call object destructors).
    */
    void clear() {
        entries = 0;
    }

    /*!
        Does nothing except assert i <= n (if NDEBUG is not set). Provided for
        convenience so StaticVector can replace std::vector more easily.
    */
    void reserve(unsigned int i) {
        assert(i <= n);
    }

    /*!
        Creates an empty array.
    */
    StaticVector() :
        entries(0)
    {
#ifdef STATIC_VECTOR_NOINIT_HACK
        vec = (T*)(_vec);
#endif
    }

    /*!
        Adds an element to the end of the array.
    */
    void push_back(const T &t) {
        assert(entries < n);
        vec[entries] = t;
        entries++;
    }

    /*!
        Element access (const).
    */
    const T& operator [] (unsigned int i) const {
        assert(i < entries);
        return vec[i];
    }

    /*!
        Element access.
    */
    T& operator [] (unsigned int i) {
        assert(i < entries);
        return vec[i];
    }

    /*!
        Returns the number of entries of the array.
    */
    unsigned int size() const {
        return entries;
    }

    // fast copy - don't have to copy parts of the array that aren't in use
    void operator = (const StaticVector<T,n>& other) {
        entries = other.entries;
        for (unsigned int i = 0; i < entries; i++) {
            vec[i] = other[i];
        }
    }
};

#endif

