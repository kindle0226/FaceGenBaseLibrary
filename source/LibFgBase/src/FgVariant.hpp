//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     Nov 16, 2005
//
// Runtime polymorphic encapsulation of any copy-constructible type for C++.
//
//      USE:
//
// See Libfgen/tests/FgVariant.cpp
//
// Uses the copy-on-copy idiom for heap object ownership.
// Types must have a copy constructor.
// boost::any could not be used since it was deemed necessary to make this compatible with serialization.
// boost::variant perhaps could have been used; not sure.

#ifndef FGVARIANT_HPP
#define FGVARIANT_HPP

#include "FgStdLibs.hpp"

#include "FgTypes.hpp"
#include "FgException.hpp"
#include "FgOut.hpp"
#include "FgStdPair.hpp"
#include "FgSharedPtr.hpp"

class FgVariant
{
    struct PolyBase
    {
        virtual ~PolyBase() {};

        // Returns a pointer to a *new* copy of the object:
        virtual 
        FgSharedPtr<PolyBase>          
        clone() const = 0;

        virtual
        std::string 
        typeName() const = 0;
    };

    FgSharedPtr<PolyBase>   m_poly;

    // The data container makes use of the default copy constructor and (virtual) destructor.
    template<class T>
    struct Poly : public PolyBase
    {
        T       m_data;

        Poly() {}

        explicit
        Poly(const T & val) : m_data(val) {}

        virtual 
        FgSharedPtr<PolyBase>
        clone() const 
        {return FgSharedPtr<PolyBase>(new Poly(m_data)); }

        virtual 
        std::string 
        typeName() const 
        {return typeid(T).name(); }

        const T &
        getCRef() const
        {return m_data; }
    };

public:
    FgVariant()
    {}

    template<class T>
    explicit
    FgVariant(const T & val)
    : m_poly(new Poly<T>(val))
    {}

    // Copy constructor is a deep copy:
    FgVariant(const FgVariant & var)
    {
        if (var.m_poly)
            m_poly = var.m_poly->clone();
    }

    template<class T>
    void
    operator=(const T & val)
    {
        m_poly.reset(new Poly<T>(val));
    }

    template<class T>
    void
    set(const T & val)
    {
        T &     tv = getRef<T>();
        tv = val;
    }

    // Assigning from an FgVariant copies the value within:
    FgVariant &
    operator=(const FgVariant & var);

    template<class T>
    bool
    isType() const
    {
        Poly<T> * ptr = dynamic_cast<Poly<T>*>(m_poly.get());
        return (ptr != NULL);
    }

    // Use for explicit value access, for instance when usage context type is ambiguous:
    template<class T>
    const T &
    getCRef() const
    {
        if (!m_poly)
            fgThrow("Variant NULL dereferenced as",typeid(T).name());
        Poly<T> * ptr = dynamic_cast<Poly<T>*>(m_poly.get());
        if (!ptr)
            fgThrow("Variant incompatible type dereference from/to",
                    m_poly->typeName() + " / " + typeid(T).name());
        return ptr->getCRef();
    }

    // Use for explicit value modification:
    template<class T>
    T &
    getRef()
    {return const_cast<T&>(getCRef<T>()); }

    // A [Const]ValueProxy class is used as an opaque type to allow automatic conversions.
    // Direct conversion was not possible as GCC could not support the const version,
    // and VS2010 std::vector has a bug that performs a char& conversion on elements.
    // This class should not be used by clients except through the valueRef functions.
    struct ValueProxy
    {
        explicit
        ValueProxy(FgVariant*v):
            m_variant(v)
        {}

        template<typename T>
        operator T & ()
        { 
            // T can be const here, so remove the const from the
            // type otherwise multiple registrations can occur.
            typedef typename boost::remove_const<T>::type type;
            return m_variant->getRef<type>(); 
        }

    private:
        FgVariant *m_variant;
    };

    struct ConstValueProxy
    {
        explicit
        ConstValueProxy(const FgVariant *v):
            m_variant(v)
        {}

        template<typename T>
        operator T () const
        {                                                       
            typedef typename boost::remove_const<T>::type type;
            return m_variant->getCRef<type>(); 
        }

    private:
        const FgVariant *m_variant;
    };

    ConstValueProxy
    valueRef() const 
    { return ConstValueProxy(this); }

    ValueProxy
    valueRef()
    { return ValueProxy(this); }
    
    std::string
    typeName() const
    {return m_poly->typeName(); }
};

#endif
