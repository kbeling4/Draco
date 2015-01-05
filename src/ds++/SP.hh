//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   ds++/SP.hh
 * \author Geoffrey Furnish, Thomas Evans
 * \date   Tue Feb  4 11:27:53 2003
 * \brief  Smart Point class file.
 * \note   Copyright (C) 2003-2014 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//
// $Id$
//---------------------------------------------------------------------------//

#ifndef RTT_ds_SP_HH
#define RTT_ds_SP_HH

#include <typeinfo>
#include "ds++/config.h"
#include "Assert.hh"

namespace rtt_dsxx
{
 
//===========================================================================//
/*!
 * \struct SPref
 * 
 * \brief Reference holder struct for SP class.
 */
//===========================================================================//

struct SPref 
{
    //! Number of references.
    int refs;

    //! Constructor
    SPref(int r = 1) : refs(r) { }

#if DRACO_DIAGNOSTICS & 2
    // To prevent refcounts from being included in memory debugging.
    // These are problematic because we allocate reference counters even
    // for null smart pointers.
    void *operator new(size_t const n)
    {
        return malloc(n);
    }
    void  operator delete(void *const ptr)
    {
        free(ptr);
    }
#endif
};

//===========================================================================//
/*!
 * \class SP
 * 
 * \brief Smart pointer implementation that does reference counting.
 *
 * The smart pointer provides a "safe" encapsulation for a standard C++
 * pointer.  Consider: A function new's an object and return the pointer to
 * that object as its return value.  Now it is the caller's responsibility to
 * free the object.  What if the caller passes the pointer to other objects
 * or functions?  What if it is not known which will be deleted first or
 * last?
 *
 * Instead the function can return a "smart pointer".  This SP class uses
 * reference counting to determine the number of current users of a pointer.
 * Each time an SP goes out of scope, the reference count is decremented.
 * When the last user of a pointer is done, the pointer is freed.
 *
 * Note: I am calling this a "smart pointer", not a "safe pointer".  There
 * are clearly ways you can hose this.  In particular, when you bind an SP<T>
 * to a T*, you yield all rights to the T*.  You'd better not squirrel the
 * bare pointer away somewhere and expect to clandestinely use it in other
 * ways or places--death will be sure to follow.  Consequently then, the
 * safest way to use this smart pointer, is to bind it to the contained
 * pointer and then always use the smart pointer.  Immediately returning the
 * smart pointer as a return value, allowing the original bare pointer to go
 * out of scope never to be seen again, is one good example of how to use
 * this.
 * 
 * One good example of bad usage is assigning the same dumb pointer to
 * multiple SPs.  Consider:
 * \code
 *     SP<Foo> f1;
 *     SP<Foo> f2;
 *     // ...
 *     Foo *f = new Foo;
 *     f1 = f;
 *     // ...
 *     f2 = f; // bad, now f1 and f2 assume they "own" f!
 * \endcode
 * Unfortunately, there is no way to check if another SP owns the dumb
 * pointer that you give to a SP.  This is simply something that needs to be
 * watched by the programmer.
 * 
 * \note
 * Having an std::vector or other array-based container of SPs can have
 * non-obvious implications for object lifetime. Since operations like
 * pop_back() or clear() do not call the destructor of the SP, SP's that are
 * in the "slop" between the vector size and the capacity are still holding
 * references.  It is only once those slots are re-assigned, or the whole
 * vector is deleted, or a resize operation leads to a reallocation, that
 * those SPs are killed.
 */
/*!
 * \example ds++/test/tstSP.cc
 *
 * rtt_dsxx::SP (smart pointer) usage example.
 */
// revision history:
// -----------------
// 0) original
// 1) 020403 : updated with doxygen comments; minor refactoring
// 
//===========================================================================//

template<typename T>
class SP 
{
  private: 
    // >>> DATA

    //! Raw pointer held by smart pointer.
    T *p_;

    //! Pointer to reference counter.
    SPref *r_;

  private:
    // >>> IMPLEMENTATION

    // Free the pointer.
    inline void free();

    //! All instantiations of SP are friends. 
    template<class U> friend class SP;

    template <class V, class U> friend
    SP<V> dynamic_pointer_cast(const SP<U>& sp);
    
  public:
    //! Default constructor.
    inline /* constexpr */ SP();

    // Explicit constructor for type X *.
    template<class U>
    inline explicit SP(U *p);

    // Copy constructor for SP<T>.
    inline SP(const SP<T> &sp_in);

    // Copy constructor for SP<X>.
    template<class X>
    inline SP(const SP<X> &x);

    //! Destructor, memory is released when count goes to zero.
    ~SP(void) { free(); }

    // Assignment operator for type SP<T>.
    inline SP<T>& operator=(const SP<T> &x);

    // Assignment operator for type SP<U>.
    template<class U>
    inline SP<T>& operator=(const SP<U> &x);

    //! CLear the pointer
    void reset() { SP().swap(*this); }

    //! Reset to a different pointer
    template <class U>
    inline void reset(U* p);

    //! Swap pointers
    void swap(SP &r);
	
    //! Access operator.
    T* operator->() const { Require(p_); return p_; }

    //! Dereference operator.
    T& operator*() const { Require(p_); return *p_; }

    //! Get the base-class pointer; better know what you are doing.
    T* get() const { return p_; }

    //! Get the use count.
    long use_count() const { return r_? r_->refs : 0L; }

    bool unique() const { return r_? r_->refs==1L : false; }

    //! Boolean conversion operator.
    operator bool() const { return p_ != NULL; }
};

DLL_PUBLIC void incompatible(std::type_info const &X, std::type_info const &T);

//---------------------------------------------------------------------------//
// OVERLOADED OPERATORS
//---------------------------------------------------------------------------//
/*!
 * \brief Do equality check between smart pointers.
 */
template<typename T, typename U>
bool operator==(const SP<T> &lhs, const SP<U> &rhs)
{
    return lhs.get() == rhs.get();
}

//---------------------------------------------------------------------------//
/*!
 * \brief Do equality check with a free pointer.
 *
 * This is not part of the C++11 shared_ptr library, but we include it because
 * we do not have the C++11 nullptr type.
 */
template<typename T>
bool operator==(const T *pt, const SP<T> &sp)
{
    return sp.get() == pt;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Do inequality check with a free pointer.
 *
 * This is not part of the C++11 shared_ptr library, but we include it because
 * we do not have the C++11 nullptr type.
 */
template<typename T>
bool operator!=(const T *pt, const SP<T> &sp)
{
    return sp.get() != pt;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Do equality check with a free pointer.
 *
 * This is not part of the C++11 shared_ptr library, but we include it because
 * we do not have the C++11 nullptr type.
 */
template<typename T>
bool operator==(const SP<T> &sp, const T *pt)
{
    return sp.get() == pt;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Do inequality check with a free pointer.
 *
 * This is not part of the C++11 shared_ptr library, but we include it because
 * we do not have the C++11 nullptr type.
 */
template<typename T>
bool operator!=(const SP<T> &sp, const T *pt)
{
    return sp.get() != pt;
}

//---------------------------------------------------------------------------//
// INLINE FUNCTIONS
//---------------------------------------------------------------------------//
/*!
 * \brief  Default constructor
 *
 * This constructs an empty smart pointer (owns no pointer, use_count()==0).
 */
template<typename T>
SP<T>::SP()
    : p_(NULL), r_(NULL)
{
    Ensure(get()==NULL);
    Ensure(use_count()==0);
}
//---------------------------------------------------------------------------//
/*!
 * \brief  Explicit constructor for type U *.
 *
 * This constructor is used to initialize a smart pointer of type
 * T with a smart pointer of type U, where U must be implicitly convertible to
 * T. This generally means U is either T or a child class of T.
 *
 * \param p pointer to type U that is convertible to T *
 */
template<typename T>
template<class U>
SP<T>::SP(U *p)
    : p_(p),
      r_(new SPref)
{
    Ensure (get()==p);
    Ensure (unique());
}

//---------------------------------------------------------------------------//
/*!
 * \brief Copy constructor for SP<T>.
 *
 * \param sp_in smart pointer of type SP<T>
 */
template<typename T>
SP<T>::SP(const SP<T> &sp_in)
    : p_(sp_in.p_),
      r_(sp_in.r_)
{
    // advance the reference to T if not empty
    if (r_) r_->refs++;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Copy constructor for SP<U>.
 *
 * This copy constructor requires that U * is implicitly convertible to T *.
 *
 * \param x smart pointer of type SP<U>
 */
template<typename T>
template<class U>
SP<T>::SP(const SP<U> &x)
    : p_(x.p_),
      r_(x.r_)
{
    // advance the reference to T if not empty
    if (r_) r_->refs++;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Assignment operator for type SP<T>.
 *
 * \param x smart pointer of type SP<T>
 */
template<typename T>
SP<T>& SP<T>::operator=(const SP<T> &x)
{
    SP<T>(x).swap(*this);
    return *this;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Assignment operator for type SP<U>.
 *
 * This assignment requires that U * is implicitly convertible to T *.  The
 * pointer in x can point to NULL.
 *
 * \param x smart pointer of type SP<U>
 */
template<typename T>
template<class U>
SP<T>& SP<T>::operator=(const SP<U> &x)
{
    SP<T>(x).swap(*this);
    return *this;
}

//---------------------------------------------------------------------------//
template <class T, class U>
SP<T> dynamic_pointer_cast (const SP<U>& sp)
{
    SP<T> Result;
    T *pt = dynamic_cast<T*>(sp.p_);
    if (pt)
    {
        Result.p_ = pt;
        Result.r_ = sp.r_;
        sp.r_->refs++;
    }
    return Result;
}

//---------------------------------------------------------------------------//
template <class T>
template <typename U>
void SP<T>::reset(U* p)
{
    free();
    p_ = p;
    r_ = new SPref;
}

//---------------------------------------------------------------------------//
template<class T>
void SP<T>::swap(SP<T> &r)
{
    T *tp = p_;
    p_ = r.p_;
    r.p_ = tp;
    SPref *rp = r_;
    r_ = r.r_;
    r.r_ = rp;
}

//---------------------------------------------------------------------------//
// PRIVATE IMPLEMENTATION
//---------------------------------------------------------------------------//
/*!
 * \brief Decrement the count and free the pointer if count is zero.
 *
 * Note that it is perfectly acceptable to call delete on a NULL pointer.
 */
template<typename T>
void SP<T>::free()
{
    // if the count goes to zero then we free the data
    if (r_ && --r_->refs == 0)
    {
	delete p_;
	delete r_;
    }
}

} // end namespace rtt_dsxx

#endif                          // RTT_ds_SP_HH

//---------------------------------------------------------------------------//
// end of ds++/SP.hh
//---------------------------------------------------------------------------//
