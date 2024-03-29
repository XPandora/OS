#include "ns.h"

extern union Nsipc nsipcbuf;

void output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet request (using ipc_recv)
	//	- send the packet to the device driver (using sys_net_send)
	//	do the above things in a loop
	int r;
	envid_t from_env_store;

	int32_t value;
	for (;;)
	{
		value = ipc_recv(&from_env_store, &nsipcbuf, NULL);
		if (value != NSREQ_OUTPUT || from_env_store != ns_envid)
		{
			continue;
		}

		while ((r = sys_net_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0)
		{
			if (r != -E_AGAIN)
			{
				panic("sys_net_send: %e\n", r);
			}
		}
	}
}

