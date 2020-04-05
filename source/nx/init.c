#include <switch.h>

#include <unistd.h>

#ifndef NDEBUG
static int s_fd = -1;
#endif

static SocketInitConfig const s_socketInitConfig = {
    .bsdsockets_version = 1,

    .tcp_tx_buf_size     = 1 * 1024 * 1024,
    .tcp_rx_buf_size     = 1 * 1024 * 1024,
    .tcp_tx_buf_max_size = 4 * 1024 * 1024,
    .tcp_rx_buf_max_size = 4 * 1024 * 1024,

    .udp_tx_buf_size = 0x2400,
    .udp_rx_buf_size = 0xA500,

    .sb_efficiency = 8,

    .num_bsd_sessions = 3,
    .bsd_service_type = BsdServiceType_User,
};

u32 __nx_fs_num_sessions = 3;

void userAppInit ()
{
	appletLockExit ();

	romfsInit ();
	plInitialize ();

	if (R_FAILED (socketInitialize (&s_socketInitConfig)))
		return;

#ifndef NDEBUG
	s_fd = nxlinkStdio ();
#endif
}

void userAppExit ()
{
#ifndef NDEBUG
	if (s_fd >= 0)
	{
		close (s_fd);
		socketExit ();
		s_fd = -1;
	}
#endif

	plExit ();
	romfsExit ();
	appletUnlockExit ();
}
