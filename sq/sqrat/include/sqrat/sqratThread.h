//
// SqratMemberMethods: Member Methods
//

//
// Copyright (c) 2009 Brandon Jones
// Copyright 2011 Li-Cheng (Andy) Tai
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

#if !defined(_SCRAT_THREAD_H_)
#define _SCRAT_THREAD_H_

#include <squirrel.h>
#include "sqr_imp_exp.h"
#ifdef WITH_OSTHREAD
#include "osthread.h"


struct  BaseSqEnvi
{
    umutex  _m;
    umutex& mutex(){return _m;}
};

class  sq_env;
extern sq_env*		__sqenv;          // squirrel interpreter enviroment object
extern BaseSqEnvi*	__bsqenv;


#define APP_LOCK() AutoLock a(&__bsqenv->mutex());
#else
#define APP_LOCK()
#endif // WITH_OSTHREAD


#endif
