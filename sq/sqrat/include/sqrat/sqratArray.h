//
// SqratArray: Array Binding
//

//
// Copyright 2011 Alston Chen
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

#if !defined(_SCRAT_ARRAY_H_)
#define _SCRAT_ARRAY_H_

#include <squirrel.h>
#include <string.h>
#include "sqr_imp_exp.h"
#include "sqratObject.h"
#include "sqratFunction.h"
#include "sqratGlobalMethods.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for Array that implements almost all of its functionality
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ArrayBase : public Object {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /// \param v VM that the array will exist in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase(HSKVM v = DefaultVM::Get()) : Object(v, true) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the ArrayBase from an Object that already exists
    ///
    /// \param obj An Object that should already represent a Squirrel array
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase(const Object& obj) : Object(obj) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the ArrayBase from a HSQOBJECT and HSKVM that already exist
    ///
    /// \param o Squirrel object that should already represent a Squirrel array
    /// \param v Squirrel VM that contains the Squirrel object given
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase(HSQOBJECT o, HSKVM v = DefaultVM::Get()) : Object(o, v) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a Table or Class to the Array (can be used to facilitate namespaces)
    ///
    /// \param index The index in the array being assigned a Table or Class
    /// \param obj   Table or Class that is being placed in the Array
    ///
    /// \remarks
    /// Bind cannot be called "inline" like other functions because it introduces order-of-initialization bugs.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Bind(const int index, Object& obj) {
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);
        __psq->pushobject(vm, obj.GetObject());
        __psq->set(vm, -3);
        __psq->pop(vm,1); // pop array
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a raw Squirrel closure to the Array
    ///
    /// \param index The index in the array being assigned a function
    /// \param func  Squirrel function that is being placed in the Array
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase& SquirrelFunc(const int index, SQFUNCTION func) {
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);
        __psq->newclosure(vm, func, 0);
        __psq->set(vm, -3);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific value
    ///
    /// \param index The index in the array being assigned a value
    /// \param val   Value that is being placed in the Array
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& SetValue(const int index, const V& val) {
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);
        PushVar(vm, val);
        __psq->set(vm, -3);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific instance (like a reference)
    ///
    /// \param index The index in the array being assigned a value
    /// \param val   Pointer to the instance that is being placed in the Array
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& SetInstance(const int index, V* val) {
        BindInstance<V>(index, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific function
    ///
    /// \param index  The index in the array being assigned a value
    /// \param method Function that is being placed in the Array
    ///
    /// \tparam F Type of function (only define this if you need to choose a certain template specialization or overload)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class F>
    ArrayBase& Func(const int index, F method) {
        BindFunc(index, &method, sizeof(method), SqGlobalFunc(method));
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the element at a given index
    ///
    /// \param index Index of the element
    ///
    /// \tparam T Type of element (fails if element is not of this type)
    ///
    /// \return SharedPtr containing the element (or null if failed)
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    SharedPtr<T> GetValue(int index)
    {
        __psq->pushobject(vm, obj);
        __psq->pushinteger(vm, index);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if (SQ_FAILED(__psq->get(vm, -2))) {
            __psq->pop(vm, 1);
            SQTHROW(vm, _SC("illegal index"));
            return SharedPtr<T>();
        }
#else
        __psq->get(vm, -2);
#endif
        SQTRY()
        Var<SharedPtr<T> > element(vm, -1);
        SQCATCH_NOEXCEPT(vm) {
            __psq->pop(vm, 2);
            return SharedPtr<T>();
        }
        __psq->pop(vm, 2);
        return element.value;
        SQCATCH(vm) {
#if defined (SCRAT_USE_EXCEPTIONS)
            SQUNUSED(e); // avoid "unreferenced local variable" warning
#endif
            __psq->pop(vm, 2);
            SQRETHROW(vm);
        }
        return SharedPtr<T>(); // avoid "not all control paths return a value" warning
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets a Function from an index in the Array
    ///
    /// \param index The index in the array that contains the Function
    ///
    /// \return Function found in the Array (null if failed)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Function GetFunction(const int index) {
        HSQOBJECT funcObj;
        __psq->pushobject(vm, GetObject());
        __psq->pushinteger(vm, index);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(__psq->get(vm, -2))) {
            __psq->pop(vm, 1);
            return Function();
        }
        SQObjectType value_type = __psq->gettype(vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
            __psq->pop(vm, 2);
            return Function();
        }
#else
        __psq->get(vm, -2);
#endif
        __psq->getstackobj(vm, -1, &funcObj);
        Function ret(vm, GetObject(), funcObj); // must addref before the pop!
        __psq->pop(vm, 2);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Fills a C array with the elements of the Array
    ///
    /// \param array C array to be filled
    /// \param size  The amount of elements to fill the C array with
    ///
    /// \tparam T Type of elements (fails if any elements in Array are not of this type)
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    void GetArray(T* array, int size)
    {
        HSQOBJECT value = GetObject();
        __psq->pushobject(vm, value);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if (size > __psq->getsize(vm, -1)) {
            __psq->pop(vm, 1);
            SQTHROW(vm, _SC("array buffer size too big"));
            return;
        }
#endif
        __psq->pushnull(vm);
        int i;
        while (SQ_SUCCEEDED(__psq->next(vm, -2))) {
            __psq->getinteger(vm, -2, &i);
            if (i >= size) break;
            SQTRY()
            Var<const T&> element(vm, -1);
            SQCATCH_NOEXCEPT(vm) {
                __psq->pop(vm, 4);
                return;
            }
            __psq->pop(vm, 2);
            array[i] = element.value;
            SQCATCH(vm) {
#if defined (SCRAT_USE_EXCEPTIONS)
                SQUNUSED(e); // avoid "unreferenced local variable" warning
#endif
                __psq->pop(vm, 4);
                SQRETHROW(vm);
            }
        }
        __psq->pop(vm, 2); // pops the null iterator and the array object
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Appends a value to the end of the Array
    ///
    /// \param val Value that is being placed in the Array
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& Append(const V& val) {
        __psq->pushobject(vm, GetObject());
        PushVar(vm, val);
        __psq->arrayappend(vm, -2);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Appends an instance to the end of the Array (like a reference)
    ///
    /// \param val Pointer to the instance that is being placed in the Array
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& Append(V* val) {
        __psq->pushobject(vm, GetObject());
        PushVar(vm, val);
        __psq->arrayappend(vm, -2);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Inserts a value in a position in the Array
    ///
    /// \param destpos Index to put the new value in
    /// \param val     Value that is being placed in the Array
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& Insert(const int destpos, const V& val) {
        __psq->pushobject(vm, GetObject());
        PushVar(vm, val);
        __psq->arrayinsert(vm, -2, destpos);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Inserts an instance in a position in the Array (like a reference)
    ///
    /// \param destpos Index to put the new value in
    /// \param val     Pointer to the instance that is being placed in the Array
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& Insert(const int destpos, V* val) {
        __psq->pushobject(vm, GetObject());
        PushVar(vm, val);
        __psq->arrayinsert(vm, -2, destpos);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Removes the last element from the Array
    ///
    /// \return Object for the element that was removed (null if failed)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object Pop() {
        HSQOBJECT slotObj;
        __psq->pushobject(vm, GetObject());
        if(SQ_FAILED(__psq->arraypop(vm, -1, true))) {
            __psq->pop(vm, 1);
            return Object(); // Return a NULL object
        } else {
            __psq->getstackobj(vm, -1, &slotObj);
            Object ret(slotObj, vm);
            __psq->pop(vm, 2);
            return ret;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Removes an element at a specific index from the Array
    ///
    /// \param itemidx Index of the element being removed
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase& Remove(const int itemidx) {
        __psq->pushobject(vm, GetObject());
        __psq->arrayremove(vm, -1, itemidx);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Resizes the Array
    ///
    /// \param newsize Desired size of the Array in number of elements
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase& Resize(const int newsize) {
        __psq->pushobject(vm, GetObject());
        __psq->arrayresize(vm, -1, newsize);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Reverses the elements of the array in place
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase& Reverse() {
        __psq->pushobject(vm, GetObject());
        __psq->arrayreverse(vm, -1);
        __psq->pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the length of the Array
    ///
    /// \return Length in number of elements
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int Length() const
    {
        __psq->pushobject(vm, obj);
        int r = __psq->getsize(vm, -1);
        __psq->pop(vm, 1);
        return r;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents an array in Squirrel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Array : public ArrayBase {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /// \remarks
    /// The Array is invalid until it is given a VM to exist in.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array() {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Array
    ///
    /// \param v    VM to create the Array in
    /// \param size An optional size hint
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array(HSKVM v, const int size = 0) : ArrayBase(v) {
        __psq->newarray(vm, size);
        __psq->getstackobj(vm,-1,&obj);
        __psq->addref(vm, &obj);
        __psq->pop(vm,1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the Array from an Object that already exists
    ///
    /// \param obj An Object that should already represent a Squirrel array
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array(const Object& obj) : ArrayBase(obj) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the Array from a HSQOBJECT and HSKVM that already exist
    ///
    /// \param o Squirrel object that should already represent a Squirrel array
    /// \param v Squirrel VM that contains the Squirrel object given
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array(HSQOBJECT o, HSKVM v = DefaultVM::Get()) : ArrayBase(o, v) {
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Array> {

    Array value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as an Array
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSKVM vm, int idx) {
        HSQOBJECT obj;
        __psq->resetobject(&obj);
        __psq->getstackobj(vm,idx,&obj);
        value = Array(obj, vm);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        SQObjectType value_type = __psq->gettype(vm, idx);
        if (value_type != OT_ARRAY) {
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("array")));
        }
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put an Array reference on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSKVM vm, const Array& value) {
        HSQOBJECT obj;
        __psq->resetobject(&obj);
        obj = value.GetObject();
        __psq->pushobject(vm,obj);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Array&> : Var<Array> {Var(HSKVM vm, int idx) : Var<Array>(vm, idx) {}};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<const Array&> : Var<Array> {Var(HSKVM vm, int idx) : Var<Array>(vm, idx) {}};

}

#endif
