# squirell-rat
## sqrat_newapi, sqrat_import, sqrat_register_importlib DONE !

This is a modified version of squirrel and is not compatibel with the main stream releases. 
Some variable and types were aligned to std and shorten. 
Sqrat was changed to be used in libraries as well.

# changes.

   * the sqrat calls into squirrel are now done over pointers to functions. 
   * Therefore the structure carying the functors has to be initalised once in the main program as:
   

```c++
   # main.cpp
   
   #include "sqr_imp_exp.h"
   
   HSQAPI      __psq; // do not chnage this name. Define it on your main.cpp
   
   int main()
   {
        __psq = sqrat_newapi();
        // rest of the program
   }
   
```

    * now the modules. 
        * The way of having loadable modules can be seen in the 'sqratThread.cpp'. 
        * The way it was is kindof okward. See how 'sqrat_pushsleep' is done.
        
    * I just put all the sqrat calls towards squirrel trough the __psq strucure.
    * Now the sqrat can be used in dll's and so libraries wit elegancy.
  
  
```php
### the script
  
::import("./modules/libardulike.so");

  print(gets());
  
  puts("dddddddddd");
  
  ```


### the lib

```c++
//------------------------------------------- the header
#if !defined(_SIMPLEMODULE_H_)
#define _SIMPLEMODULE_H_

#include "sqmodule.h"
#include "squirrel.h"

#ifdef __cplusplus
extern "C" {
#endif
EXPORT SQRESULT sqmodule_load(HSKVM v, HSQAPI api);
#ifdef __cplusplus
}
#endif
#endif


// the ---------------------------------------------cpp
#include "ardulike.h"
#include "sqrat.h"

HSQAPI __psq;  // global one / lib

int puts(const char* ss)
{
    printf("in puts: %s  %s\n",__FUNCTION__,ss);
    return 5;
}

const char* gets()
{
    printf("%s\n",__FUNCTION__);
    return "sdfsdf5";
}

// Module registration
SQRESULT sqmodule_load(HSKVM v, HSQAPI api)
{
	__psq = api;
	Sqrat::RootTable(v).Func("puts",&puts);  // <<< === export using sqrat
	Sqrat::RootTable(v).Func("gets",&gets);
	return SQ_OK;
}

```
### result

```bash
Loading: './modules/libardulike.so'

gets
sdfsdf5dddddddddd

```
    














