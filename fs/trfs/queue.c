#include "queue.h"

struct workqueue_struct *eWq=0;

int eworkqueue_init(void)
{
  if (!eWq)
    	{
        	printk("ewq..Single Thread created\r\n");
	        eWq = create_singlethread_workqueue("eWorkqueue");
    	}
     
   /** Init the work struct with the work handler **/
    return 0;
}

void eworkqueue_exit(void)
{
	if (eWq)
        	destroy_workqueue(eWq);
	printk("GoodBye..WorkQueue");
}
