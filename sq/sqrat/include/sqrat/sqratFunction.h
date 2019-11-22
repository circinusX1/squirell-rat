//
// sqratFunction: Squirrel Function Wrapper
//

//
// Copyright (c) 2009 Brandon Jones
// Copyirght 2011 Li-Cheng (Andy) Tai
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

#if !defined(_SCRAT_SQFUNC_H_)
#define _SCRAT_SQFUNC_H_

#include <squirrel.h>
#include "sqr_imp_exp.h"
#include "sqratObject.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a function in Squirrel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Function  {

	friend class TableBase;
	friend class Table;
	friend class ArrayBase;
	friend struct Var<Function>;

private:

	HSKVM vm;
	HSQOBJECT env, obj;
	bool released=false;
	bool Released=false;

public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Default constructor (null)
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Function() {
		__psq->resetobject(&env);
		__psq->resetobject(&obj);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Copy constructor
	///
	/// \param sf Function to copy
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Function(const Function& sf) : vm(sf.vm), env(sf.env), obj(sf.obj) {
		__psq->addref(vm, &env);
		__psq->addref(vm, &obj);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Constructs a Function from a slot in an Object
	///
	/// \param e    Object that potentially contains a Squirrel function in a slot
	/// \param slot Name of the slot to look for the Squirrel function in
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Function(const Object& e, const SQChar* slot) : vm(e.GetVM()), env(e.GetObject()) {
		__psq->addref(vm, &env);
		Object so = e.GetSlot(slot);
		obj = so.GetObject();
		__psq->addref(vm, &obj);
#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQObjectType value_type = so.GetType();
		if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE && value_type != OT_CLASS) {
			// Note that classes can also be considered functions in Squirrel
			SQTHROW(vm, _SC("function not found in slot"));
		}
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Constructs a Function from two Squirrel objects (one is the environment object and the other is the function object)
	///
	/// \param v VM that the function will exist in
	/// \param e Squirrel object that should represent the environment of the function
	/// \param o Squirrel object that should already represent a Squirrel function
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Function(HSKVM v, HSQOBJECT e, HSQOBJECT o) : vm(v), env(e), obj(o) {
		__psq->addref(vm, &env);
		__psq->addref(vm, &obj);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Destructor
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	~Function() {
		if(!released)
		{
			released=true;
			Release();
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Assignment operator
	///
	/// \param sf Function to copy
	///
	/// \return The Function itself
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Function& operator=(const Function& sf) {
		Release();
		vm = sf.vm;
		env = sf.env;
		obj = sf.obj;
		__psq->addref(vm, &env);
		__psq->addref(vm, &obj);
		return *this;
	}

	///
	/// \brief getFoo
	/// \param o
	/// \param name
	/// \param f
	///
	static void getFoo(Sqrat::Object& o, const SQChar* slot, Sqrat::Function& f)
	{
		Sqrat::Function fl = Sqrat::Function(o,slot);
		if(!fl.IsNull())
			f = fl;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Checks whether the Function is null
	///
	/// \return True if the Function currently has a null value, otherwise false
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool IsNull() {
		return sq_isnull(obj);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Gets the Squirrel environment object for this Function (copy)
	///
	/// \return Squirrel object representing the Function environment
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HSQOBJECT GetEnv() const {
		return env;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Gets the Squirrel environment object for this Function (reference)
	///
	/// \return Squirrel object representing the Function environment
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HSQOBJECT& GetEnv() {
		return env;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Gets the Squirrel function object for this Function (copy)
	///
	/// \return Squirrel object representing the Function
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HSQOBJECT GetFunc() const {
		return obj;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Gets the Squirrel function object for this Function (reference)
	///
	/// \return Squirrel object representing the Function
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HSQOBJECT& GetFunc() {
		return obj;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Gets the Squirrel VM for this Function (copy)
	///
	/// \return Squirrel VM associated with the Function
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HSKVM GetVM() const {
		return vm;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Gets the Squirrel VM for this Function (reference)
	///
	/// \return Squirrel VM associated with the Function
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	HSKVM& GetVM() {
		return vm;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Sets the Function to null (removing its references to underlying Squirrel objects)
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Release() {
		if(!Released)
		{
			Released=true;
			if(!IsNull()) {
				__psq->release(vm, &env);
				__psq->release(vm, &obj);
				__psq->resetobject(&env);
				__psq->resetobject(&obj);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R>
	SharedPtr<R> Evaluate() {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;

		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 1)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}

		SQRESULT result = __psq->call(vm, 1, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 1, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1>
	SharedPtr<R> Evaluate(A1 a1) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;

		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 2)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 2, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 2, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2>
	SharedPtr<R> Evaluate(A1 a1, A2 a2) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;

		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 3)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 3, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 3, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 4)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 4, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 4, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 5)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 5, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 5, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 6)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 6, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 6, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)

		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 7)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 7, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 7, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 8)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 8, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 8, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	/// \param a8 Argument 8 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8 Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 9)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 9, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 9, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	/// \param a8 Argument 8 of the Function
	/// \param a9 Argument 9 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8 Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9 Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 10)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 10, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 10, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 11)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 11, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 11, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 12)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 12, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 12, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 13)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);
		PushVar(vm, a12);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 13, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 13, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	/// \param a13 Argument 13 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A13 Type of argument 13 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 14)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);
		PushVar(vm, a12);
		PushVar(vm, a13);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 14, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 14, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function and returns its value as a SharedPtr
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	/// \param a13 Argument 13 of the Function
	/// \param a14 Argument 14 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A13 Type of argument 13 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A14 Type of argument 14 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam R Type of return value (fails if return value is not of this type)
	///
	/// \return SharedPtr containing the return value (or null if failed)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14>
	SharedPtr<R> Evaluate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 15)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return SharedPtr<R>();
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);
		PushVar(vm, a12);
		PushVar(vm, a13);
		PushVar(vm, a14);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 15, true, ErrorHandling::IsEnabled());

		//handle an error: pop the stack and throw the exception
		if (SQ_FAILED(result)) {
			__psq->settop(vm, top);
			SQTHROW(vm, LastErrorString(vm));
			return SharedPtr<R>();
		}
#else
		__psq->call(vm, 15, true, ErrorHandling::IsEnabled());
#endif

		SharedPtr<R> ret = Var<SharedPtr<R> >(vm, -1).value;
		__psq->settop(vm, top);
		return ret;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Execute() {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 1)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}

		SQRESULT result = __psq->call(vm, 1, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 1, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1>
	void Execute(A1 a1) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 2)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 2, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 2, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2>
	void Execute(A1 a1, A2 a2) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 3)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 3, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 3, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3>
	void Execute(A1 a1, A2 a2, A3 a3) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 4)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 4, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 4, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 5)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 5, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 5, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 6)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 6, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 6, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 7)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 7, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 7, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 8)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 8, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 8, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	/// \param a8 Argument 8 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8 Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 9)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 9, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 9, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	/// \param a8 Argument 8 of the Function
	/// \param a9 Argument 9 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8 Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9 Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 10)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 10, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 10, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 11)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 11, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 11, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 12)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 12, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 12, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);


#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 13)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);
		PushVar(vm, a12);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 13, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 13, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	/// \param a13 Argument 13 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A13 Type of argument 13 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 14)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);
		PushVar(vm, a12);
		PushVar(vm, a13);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 14, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 14, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	/// \param a13 Argument 13 of the Function
	/// \param a14 Argument 14 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A13 Type of argument 13 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A14 Type of argument 14 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14>
	void Execute(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14) {
		int top = __psq->gettop(vm);

		__psq->pushobject(vm, obj);
		__psq->pushobject(vm, env);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		size_t nparams;
		size_t nfreevars;
		if (SQ_SUCCEEDED(__psq->getclosureinfo(vm, -2, &nparams, &nfreevars)) && (nparams != 15)) {
			__psq->pop(vm, 2);
			SQTHROW(vm, _SC("wrong number of parameters"));
			return;
		}
#endif

		PushVar(vm, a1);
		PushVar(vm, a2);
		PushVar(vm, a3);
		PushVar(vm, a4);
		PushVar(vm, a5);
		PushVar(vm, a6);
		PushVar(vm, a7);
		PushVar(vm, a8);
		PushVar(vm, a9);
		PushVar(vm, a10);
		PushVar(vm, a11);
		PushVar(vm, a12);
		PushVar(vm, a13);
		PushVar(vm, a14);

#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQRESULT result = __psq->call(vm, 15, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);

		//handle an error: throw the exception
		if (SQ_FAILED(result)) {
			SQTHROW(vm, LastErrorString(vm));
			return;
		}
#else
		__psq->call(vm, 15, false, ErrorHandling::IsEnabled());
		__psq->settop(vm, top);
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void operator()() {
		Execute();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1>
	void operator()(A1 a1) {
		Execute(a1);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2>
	void operator()(A1 a1, A2 a2) {
		Execute(a1, a2);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3>
	void operator()(A1 a1, A2 a2, A3 a3) {
		Execute(a1, a2, a3);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4) {
		Execute(a1, a2, a3, a4);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
		Execute(a1, a2, a3, a4, a5);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
		Execute(a1, a2, a3, a4, a5, a6);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
		Execute(a1, a2, a3, a4, a5, a6, a7);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	/// \param a8 Argument 8 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8 Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1 Argument 1 of the Function
	/// \param a2 Argument 2 of the Function
	/// \param a3 Argument 3 of the Function
	/// \param a4 Argument 4 of the Function
	/// \param a5 Argument 5 of the Function
	/// \param a6 Argument 6 of the Function
	/// \param a7 Argument 7 of the Function
	/// \param a8 Argument 8 of the Function
	/// \param a9 Argument 9 of the Function
	///
	/// \tparam A1 Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2 Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3 Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4 Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5 Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6 Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7 Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8 Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9 Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8, a9);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	/// \param a13 Argument 13 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A13 Type of argument 13 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Runs the Function
	///
	/// \param a1  Argument 1 of the Function
	/// \param a2  Argument 2 of the Function
	/// \param a3  Argument 3 of the Function
	/// \param a4  Argument 4 of the Function
	/// \param a5  Argument 5 of the Function
	/// \param a6  Argument 6 of the Function
	/// \param a7  Argument 7 of the Function
	/// \param a8  Argument 8 of the Function
	/// \param a9  Argument 9 of the Function
	/// \param a10 Argument 10 of the Function
	/// \param a11 Argument 11 of the Function
	/// \param a12 Argument 12 of the Function
	/// \param a13 Argument 13 of the Function
	/// \param a14 Argument 14 of the Function
	///
	/// \tparam A1  Type of argument 1 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A2  Type of argument 2 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A3  Type of argument 3 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A4  Type of argument 4 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A5  Type of argument 5 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A6  Type of argument 6 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A7  Type of argument 7 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A8  Type of argument 8 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A9  Type of argument 9 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A10 Type of argument 10 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A11 Type of argument 11 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A12 Type of argument 12 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A13 Type of argument 13 of the Function (usually doesnt need to be defined explicitly)
	/// \tparam A14 Type of argument 14 of the Function (usually doesnt need to be defined explicitly)
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14>
	void operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14) {
		Execute(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Function instances to and from the stack as references (functions are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Function> {

	Function value; ///< The actual value of get operations

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Attempts to get the value off the stack at idx as a Function
	///
	/// \param vm  Target VM
	/// \param idx Index trying to be read
	///
	/// \remarks
	/// Assumes the Function environment is at index 1.
	///
	/// \remarks
	/// This function MUST have its Error handled if it occurred.
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Var(HSKVM vm, int idx) {
		HSQOBJECT sqEnv;
		HSQOBJECT sqValue;
		__psq->getstackobj(vm, 1, &sqEnv);
		__psq->getstackobj(vm, idx, &sqValue);
		value = Function(vm, sqEnv, sqValue);
#if !defined (SCRAT_NO_ERROR_CHECKING)
		SQObjectType value_type = __psq->gettype(vm, idx);
		if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
			SQTHROW(vm, FormatTypeError(vm, idx, _SC("closure")));
		}
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Called by Sqrat::PushVar to put a Function on the stack
	///
	/// \param vm    Target VM
	/// \param value Value to push on to the VM's stack
	///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static void push(HSKVM vm, const Function& value) {
		__psq->pushobject(vm, value.GetFunc());
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Function instances to and from the stack as references (functions are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Function&> : Var<Function> {Var(HSKVM vm, int idx) : Var<Function>(vm, idx) {}};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Function instances to and from the stack as references (functions are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<const Function&> : Var<Function> {Var(HSKVM vm, int idx) : Var<Function>(vm, idx) {}};

}

#endif
