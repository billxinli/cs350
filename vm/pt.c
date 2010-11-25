#include "opt-A3.h"

#if OPT_A3
#include <pt.h>

struct page_table* pt_init(){
	struct page_table * pt = kmalloc(sizeof(struct page_table));
	if(pt==NULL){
		//Panic?
		return NULL;
	}
	return pt;
}

#endif /* OPT_A3 */
