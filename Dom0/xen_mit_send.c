#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <xenctrl.h>
#include <xen/domctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <signal.h>

xc_interface * my_hyp;
xc_evtchn *  my_evt;

int main(int argc, char** argv)
{
	int pending_port, lport, rport;
    struct xen_domctl hvm_op;

	xc_evtchn_status_t xes;
	my_hyp = xc_interface_open(0,0,0);
	my_evt = xc_evtchn_open(0,0);
	if( 0== my_hyp )
	{
		printf(" open interface to libxc error\n");
	}

	if( 0==my_evt )
	{
		printf("open interface to libxc event channels\n");
	}


	/*调用do_domctl,建立时间通道，分配共享内存
	hvm_op.cmd = XEN_DOMCTL_mitctl;
	hvm_op.u.mitctl_op.cmd = XEN_DOMCTL_MIT_bind_evt;
	hvm_op.domain = (domid_t)atoi(argv[1]);
	if(xc_domctl(my_hyp, &hvm_op)== -1 )
	{
		printf("could not run xc_domctl\n");
	}
    */
	printf("------------------1\n");
	hvm_op.domain = (domid_t)atoi(argv[1]);
	hvm_op.cmd = XEN_DOMCTL_mitctl;
	hvm_op.u.mitctl_op.mit_type = HVM_MIT_SYSCALL_ring;
    hvm_op.u.mitctl_op.cmd = XEN_DOMCTL_MIT_set_mit_type;


	if(xc_domctl(my_hyp, &hvm_op)== -1 )
	{
		printf("could not run xc_domctl\n");
	}

	xc_evtchn_close(my_evt);
	xc_interface_close(my_hyp);
	return 0;
}









