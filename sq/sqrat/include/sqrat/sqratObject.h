//
// SqratObject: Referenced Squirrel Object Wrapper
//

//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#if !defined(_SCRAT_OBJECT_H_)
#define _SCRAT_OBJECT_H_

#include <squirrel.h>
#include <string.h>
#include "sqr_imp_exp.h"
#include "sqratAllocator.h"
#include "sqratTypes.h"
#include "sqratOverloadMethods.h"
#include "sqratUtil.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for classes that represent Squirrel objects
///
/// \remarks
/// All Object and derived classes MUST be destroyed before calling __psq->close or your application will crash when exiting.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Object {
protected:

/// @cond DEV
    HSKVM vm;
    HSQOBJECT obj;
    bool release;

    Object(HSKVM v, bool releaseOnDestroy = true) : vm(v), release(releaseOnDestroy) {
        __psq->resetobject(&obj);
    }
/// @endcond

public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object() : vm(0), release(true) {
        __psq->resetobject(&obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Copy constructor
    ///
    /// \param so Object to copy
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object(const Object& so) : vm(so.vm), obj(so.obj), release(so.release) {
        __psq->addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Object from a Squirrel object
    ///
    /// \param o Squirrel object
    /// \param v VM that the object will exist in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object(HSQOBJECT o, HSKVM v = DefaultVM::Get()) : vm(v), obj(o), release(true) {
        __psq->addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Object from a C++ instance
    ///
    /// \param instance Pointer to a C++ class instance that has been bound already
    /// \param v        VM that the object will exist in
    ///
    /// \tparam T Type of instance
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class T>
    Object(T* instance, HSKVM v = DefaultVM::Get()) : vm(v), release(true) {
        ClassType<T>::PushInstance(vm, instance);
        __psq->getstackobj(vm, -1, &obj);
        __psq->addref(vm, &obj);
    }


    template<class T>
    Object(T* instance, bool b, HSKVM v = DefaultVM::Get()) : vm(v), release(b) {
        ClassType<T>::PushInstance(vm, instance);
        __psq->getstackobj(vm, -1, &obj);
        __psq->addref(vm, &obj);
    }


    template <class T>
    void BindCppObject(T* instance)
    {
        Release();
        vm  = DefaultVM::Get();
        ClassType<T>::PushInstance(vm, instance);
        __psq->getstackobj(vm, -1, &obj);
        __psq->addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destructor
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Object() {
        if(release) {
            Release();
            release = false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Assignment operator
    ///
    /// \param so Object to copy
    ///
    /// \return The Object itself
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object& operator=(const Object& so) {
        if(release) {
            Release();
        }
        vm = so.vm;
        obj = so.obj;
        release = so.release;
        __psq->addref(vm, &GetObject());
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel VM for this Object (reference)
    ///
    /// \return Squirrel VM associated with the Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HSKVM& GetVM() {
        return vm;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel VM for this Object (copy)
    ///
    /// \return Squirrel VM associated with the Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HSKVM GetVM() const {
        return vm;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the type of the Object as defined by the Squirrel API
    ///
    /// \return SQObjectType for the Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SQObjectType GetType() const {
        return GetObject()._type;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Checks whether the Object is null
    ///
    /// \return True if the Object currently has a null value, otherwise false
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsNull() const {
        return sq_isnull(GetObject());
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel object for this Object (copy)
    ///
    /// \return Squirrel object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HSQOBJECT GetObject() const {
        return obj;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel object for this Object (reference)
    ///
    /// \return Squirrel object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HSQOBJECT& GetObject() {
        return obj;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Allows the Object to be inputted directly into places that expect a HSQOBJECT
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    operator HSQOBJECT&() {
        return GetObject();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets the Object to null (removing its references to underlying Squirrel objects)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Release() {
        if(vm){
            __psq->release(vm, &obj);
            __psq->resetobject(&obj);
            vm=0;
        }
    }


    /// mco-mco

    void    AddRef()
    {
        __psq->addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of a slot from the object
    ///
    /// \param slot Name of the slot
    ///
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object GetSlot(const SQChar* slot) const {
        HSQOBJECT slotObj;
        __psq->pushobject(vm, GetObject());
        __psq->pushstring(vm, slot, -1);

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(__psq->get(vm, -2))) {
            __psq->pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            __psq->getstackobj(vm, -1, &slotObj);
            Object ret(slotObj, vm); // must addref before the pop!
            __psq->pop(vm, 2);
            return ret;
        }
#else
        __psq->get(vm, -2);
        __psq->getstackobj(vm, -1, &slotObj);
        Object ret(slotObj, vm); // must addref before the pop!
        __psq->pop(vm, 2);
        return ret;
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of an index from the object
    ///
    /// \param index Index of the slot
    ///
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object GetSlot(int index) const {
        HSQOBJECT slotObj;
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(__psq->get(vm, -2))) {
            __psq->pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            __psq->getstackobj(vm, -1, &slotObj);
            Object ret(slotObj, vm); // must addref before the pop!
            __psq->pop(vm, 2);
            return ret;
        }
#else
        __psq->get(vm, -2);
        __psq->getstackobj(vm, -1, &slotObj);
        Object ret(slotObj, vm); // must addref before the pop!
        __psq->pop(vm, 2);
        return ret;
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Casts the object to a certain C++ type
    ///
    /// \tparam T Type to cast to
    ///
    /// \return A copy of the value of the Object with the given type
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    T Cast(int index=-1) const {
        __psq->pushobject(vm, GetObject());
        T ret = Var<T>(vm, index).value;
        __psq->pop(vm, 1);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Allows Object to be used like C++ arrays with the [] operator
    ///
    /// \param slot The slot key
    ///
    /// \tparam T Type of the slot key (usually doesnt need to be defined explicitly)
    ///
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    inline Object operator[](T slot)
    {
        return GetSlot(slot);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the size of the Object
    ///
    /// \return Size of Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int GetSize() const {
        __psq->pushobject(vm, GetObject());
        int ret = __psq->getsize(vm, -1);
        __psq->pop(vm, 1);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Iterator for going over the slots in the object using Object::Next
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct iterator
    {
        /// @cond DEV
        friend class Object;
        /// @endcond

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Default constructor (null)
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        iterator()
        {
            Index = 0;
            __psq->resetobject(&Key);
            __psq->resetobject(&Value);
            Key._type = OT_NULL;
            Value._type = OT_NULL;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Returns the string value of the key the iterator is on if possible
        ///
        /// \return String or NULL
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        const SQChar* getName() { return __psq->objtostring(&Key); }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Gets the Squirrel object for the key the iterator is on
        ///
        /// \return HSQOBJECT representing a key
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        HSQOBJECT getKey() { return Key; }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Gets the Squirrel object for the value the iterator is on
        ///
        /// \return HSQOBJECT representing a value
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        HSQOBJECT getValue() { return Value; }
    private:

        HSQOBJECT Key;
        HSQOBJECT Value;
        int Index;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Used to go through all the slots in an Object (same limitations as __psq->next)
    ///
    /// \param iter An iterator being used for going through the slots
    ///
    /// \return Whether there is a next slot
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Next(iterator& iter) const
    {
        __psq->pushobject(vm,obj);
        __psq->pushinteger(vm,iter.Index);
        if(SQ_SUCCEEDED(__psq->next(vm,-2)))
        {
            __psq->getstackobj(vm,-1,&iter.Value);
            __psq->getstackobj(vm,-2,&iter.Key);
            __psq->getinteger(vm,-3,&iter.Index);
            __psq->pop(vm,4);
            return true;
        }
        else
        {
            __psq->pop(vm,2);
            return false;
        }
    }

protected:
/// @cond DEV

    // Bind a function and it's associated Squirrel closure to the object
    inline void BindFunc(const SQChar* name, void* method, size_t methodSize, SQFUNCTION func, bool staticVar = false) {
        __psq->pushobject(vm, GetObject());
        __psq->pushstring(vm, name, -1);

        PVOID methodPtr = __psq->newuserdata(vm, static_cast<size_t>(methodSize));
        memcpy(methodPtr, method, methodSize);

        __psq->newclosure(vm, func, 1);
        __psq->newslot(vm, -3, staticVar);
        __psq->pop(vm,1); // pop table
    }

    inline void BindFunc(const int index, void* method, size_t methodSize, SQFUNCTION func, bool staticVar = false) {
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);

        PVOID methodPtr = __psq->newuserdata(vm, static_cast<size_t>(methodSize));
        memcpy(methodPtr, method, methodSize);

        __psq->newclosure(vm, func, 1);
        __psq->newslot(vm, -3, staticVar);
        __psq->pop(vm,1); // pop table
    }


    // Bind a function and it's associated Squirrel closure to the object
    inline void BindOverload(const SQChar* name, void* method, size_t methodSize, SQFUNCTION func, SQFUNCTION overload, int argCount, bool staticVar = false) {
        string overloadName = SqOverloadName::Get(name, argCount);

        __psq->pushobject(vm, GetObject());

        // Bind overload handler
        __psq->pushstring(vm, name, -1);
        __psq->pushstring(vm, name, -1); // function name is passed as a free variable
        __psq->newclosure(vm, overload, 1);
        __psq->newslot(vm, -3, staticVar);

        // Bind overloaded function
        __psq->pushstring(vm, overloadName.c_str(), -1);
        PVOID methodPtr = __psq->newuserdata(vm, static_cast<size_t>(methodSize));
        memcpy(methodPtr, method, methodSize);
        __psq->newclosure(vm, func, 1);
        __psq->newslot(vm, -3, staticVar);

        __psq->pop(vm,1); // pop table
    }

    // Set the value of a variable on the object. Changes to values set this way are not reciprocated
    template<class V>
    inline void BindValue(const SQChar* name, const V& val, bool staticVar = false) {
        __psq->pushobject(vm, GetObject());
        __psq->pushstring(vm, name, -1);
        PushVar(vm, val);
        __psq->newslot(vm, -3, staticVar);
        __psq->pop(vm,1); // pop table
    }
    template<class V>
    inline void BindValue(const int index, const V& val, bool staticVar = false) {
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);
        PushVar(vm, val);
        __psq->newslot(vm, -3, staticVar);
        __psq->pop(vm,1); // pop table
    }

    // Set the value of an instance on the object. Changes to values set this way are reciprocated back to the source instance
    template<class V>
    inline void BindInstance(const SQChar* name, V* val, bool staticVar = false) {
        __psq->pushobject(vm, GetObject());
        __psq->pushstring(vm, name, -1);
        PushVar(vm, val);
        __psq->newslot(vm, -3, staticVar);
        __psq->pop(vm,1); // pop table
    }
    template<class V>
    inline void BindInstance(const int index, V* val, bool staticVar = false) {
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);
        PushVar(vm, val);
        __psq->newslot(vm, -3, staticVar);
        __psq->pop(vm,1); // pop table
    }

/// @endcond
};

/// @cond DEV
template<>
inline void Object::BindValue<int>(const SQChar* name, const int & val, bool staticVar /* = false */) {
    __psq->pushobject(vm, GetObject());
    __psq->pushstring(vm, name, -1);
    PushVar<int>(vm, val);
    __psq->newslot(vm, -3, staticVar);
    __psq->pop(vm,1); // pop table
}
/// @endcond

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Object> {

    Object value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as an Object
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSKVM vm, int idx) {
        HSQOBJECT sqValue;
        __psq->getstackobj(vm, idx, &sqValue);
        value = Object(sqValue, vm);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put an Object on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSKVM vm, const Object& value) {
        __psq->pushobject(vm, value.GetObject());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Object&> : Var<Object> {Var(HSKVM vm, int idx) : Var<Object>(vm, idx) {}};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<const Object&> : Var<Object> {Var(HSKVM vm, int idx) : Var<Object>(vm, idx) {}};

}

#endif
