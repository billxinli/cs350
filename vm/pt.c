#include "opt-A3.h"

#if OPT_A3
struct page_table;
struct page_details;

struct page_table{
    int size;
    struct page_details[];    
};

struct page_details{
    int vpn;        //virtual page num
    int pfn;        //physical frame num
    
    int valid;      //valid bit
    int dirty;      //dirty bit (can we modify)
    int modified;   //modified (have we modified)
    int use;        //use bit (have we used the page recently)
};
#endif /* OPT_A3 */
