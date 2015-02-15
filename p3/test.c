/* second allocation is too big to fit */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   int i = 0;   
   assert(Mem_Init(4096) == 0);
   for (i = 0; i < 15; ++i)
   {
      assert(Mem_Alloc(256) != NULL);
   } 
   assert(Mem_Alloc(256) == NULL);

   exit(0);
}
