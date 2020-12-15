#include "simple_mgmt.h"

static int ble_fd = -1;
static str_mutex_context mgmt_lock;

static int handle_set_reply (int op)
{
	str_mgmt_basic_cmd_t answer;
	struct mgmt_ev_cmd_complete evt;
	struct mgmt_ev_cmd_status status;
	fd_set rset;
	struct timeval tv;
	char buf[MAX_PATH];
	int n = 0;
	int offset = 0;
	int attempt = 5;

retry:
	CHECK_INF_GOTO (attempt <= 0, "Reach max attempt. No more retry");

	// Wait up to two seconds.
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	// handler answwer.
	FD_ZERO(&rset);
	FD_SET(ble_fd, &rset);

	n = -1;
	while (n < 0)
	{
		n = select(ble_fd + 1, &rset, NULL, NULL, &tv);
		if (n < 0)
		{
			CHECK_ERR_GOTO (errno != EINTR && errno != EAGAIN, "Error on select");
			PRINT_DEBUG ("Interrupt on select. Continue..");
			continue;
		}
		CHECK_INF_GOTO (n == 0, "Timeout on select");
	}

	CHECK_ERR_GOTO (!FD_ISSET(ble_fd, &rset), "Signal from unexpected fd");

	memset (&answer, 0, sizeof(answer));
	memset (&evt, 0, sizeof(evt));
	offset = 0;
	n = read (ble_fd, &buf[0], sizeof(buf));
	CHECK_ERR_GOTO (n <= 0, "Cannot read mgmt answer (%d)", n);
	PRINT_DEBUG ("Read %d bytes", n);

	memcpy (&answer, &buf[0], sizeof(answer));
	offset += sizeof(answer);

	PRINT_DEBUG ("first op code : 0x%x", le16toh(answer.opcode));
	if (le16toh(answer.opcode) == MGMT_EV_CMD_STATUS)
	{
		CHECK_ERR_GOTO (n < offset + (int)sizeof(status), "Not enought data to fill status (%d)", n);
		memcpy (&status, &buf[offset], sizeof(status));
		offset += sizeof(status);
		PRINT_INFO ("Error on reply. Status : 0x%x (%s)", status.status, mgmt_errstr(status.status));
		attempt--;
		goto retry;
	}
	if (le16toh(answer.opcode) != MGMT_EV_CMD_COMPLETE)
	{
		PRINT_DEBUG ("Unexpected op code 0x%x", le16toh(answer.opcode));
		attempt--;
		goto retry;
	}
	CHECK_ERR_GOTO (n < offset + (int)sizeof(evt), "Not enought data to fill evt (%d)", n);
	memcpy (&evt, &buf[offset], sizeof(evt));
	offset += sizeof(evt);
	PRINT_DEBUG ("second op code : 0x%x", le16toh(evt.opcode));
	CHECK_INF_GOTO (le16toh(evt.opcode) != op, "Unexpected answer complete event 0x%x", le16toh(evt.opcode));
	PRINT_DEBUG ("Status answer : 0x%x", evt.status);
	CHECK_INF_GOTO (evt.status != 0, "Unexpected status 0x%x on op 0x%x", evt.status, op);
	return 0;
error:
	return 1;
}

static int handle_info_reply (int* status)
{
	str_mgmt_basic_cmd_t answer;
	struct mgmt_ev_cmd_complete evt;
	struct mgmt_rp_read_info info;
	fd_set rset;
	struct timeval tv;
	char buf[MAX_PATH];
	int n = 0;
	int offset = 0;
	uint32_t current;
	int attempt = 5;

	PRINT_DEBUG ("Check hci0 status..");
retry:
	CHECK_INF_GOTO (attempt <= 0, "Reach max attempt. No more retry");

	// Wait up to two seconds.
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	// handler answwer.
	FD_ZERO(&rset);
	FD_SET(ble_fd, &rset);

	*status = -1;
	n = -1;
	while (n < 0)
	{
		n = select(ble_fd + 1, &rset, NULL, NULL, &tv);
		if (n < 0)
		{
			CHECK_ERR_GOTO (errno != EINTR && errno != EAGAIN, "Error on select");
			PRINT_DEBUG ("Interrupt on select. Continue..");
			continue;
		}
		CHECK_INF_GOTO (n == 0, "Timeout on select");
	}

	CHECK_ERR_GOTO (!FD_ISSET(ble_fd, &rset), "Signal from unexpected fd");

	memset (&answer, 0, sizeof(answer));
	memset (&evt, 0, sizeof(evt));
	offset = 0;
	n = read (ble_fd, &buf[0], sizeof(buf));
	CHECK_ERR_GOTO (n <= 0, "Cannot read mgmt answer (%d)", n);

	memcpy (&answer, &buf[0], sizeof(answer));
	offset += sizeof(answer);

	if (le16toh(answer.opcode) != MGMT_EV_CMD_COMPLETE)
	{
		PRINT_DEBUG ("Unexpected op code 0x%x", le16toh(answer.opcode));
		attempt--;
		goto retry;
	}
	CHECK_ERR_GOTO (n < offset + (int)sizeof(evt), "Not enought data to fill evt (%d)", n);
	memcpy (&evt, &buf[offset], sizeof(evt));
	offset += sizeof(evt);
	if (le16toh(evt.opcode) != MGMT_OP_READ_INFO)
	{
		PRINT_DEBUG ("Unexpected answer complete op code 0x%x", le16toh(evt.opcode));
		attempt--;
		goto retry;
	}
	CHECK_ERR_GOTO (n < offset + (int)sizeof(info), "Not enought data to fill info (%d)", n);
	memcpy (&info, &buf[offset], sizeof(info));
	offset += sizeof(info);
	current = le32toh(info.current_settings);
	PRINT_DEBUG ("Supports BR/EDR %d", current & MGMT_SETTING_BREDR ? 1 : 0);
	PRINT_DEBUG ("Supports LE %d", current & MGMT_SETTING_LE ? 1 : 0);
	PRINT_DEBUG ("Supports Power %d", current & MGMT_SETTING_POWERED ? 1 : 0);
	PRINT_DEBUG ("Supports Discoverable %d", current & MGMT_SETTING_DISCOVERABLE ? 1 : 0);
	PRINT_DEBUG ("Supports Advertising %d", current & MGMT_SETTING_ADVERTISING ? 1 : 0);
	PRINT_DEBUG ("Supports Bondable %d", current & MGMT_SETTING_BONDABLE ? 1 : 0);
	PRINT_DEBUG ("Supports Connectable %d", current & MGMT_SETTING_CONNECTABLE ? 1 : 0);
	*status = 0;
	if ((current & MGMT_SETTING_BREDR) == 0)
	{
		PRINT_WARN ("HCI BTC mode is not enabled");
		*status = 1;
	}
	else if ((current & MGMT_SETTING_LE) == 0)
	{
		PRINT_WARN ("HCI LE mode is not enabled");
		*status = 1;
	}
	else if ((current & MGMT_SETTING_POWERED) == 0)
	{
		PRINT_WARN ("HCI Powered is not enabled");
		*status = 1;
	}
	else if ((current & MGMT_SETTING_DISCOVERABLE) == 0)
	{
		PRINT_WARN ("HCI Discoverable is not enabled");
		*status = 1;
	}
	else if ((current & MGMT_SETTING_ADVERTISING) == 0)
	{
		PRINT_WARN ("HCI Advertising is not enabled");
		*status = 1;
	}
	else if ((current & MGMT_SETTING_BONDABLE) == 0)
	{
		PRINT_WARN ("HCI Bondable mode is not enabled");
		*status = 1;
	}
	else if ((current & MGMT_SETTING_CONNECTABLE) == 0)
	{
		PRINT_WARN ("HCI Connectable mode is not enabled");
		*status = 1;
	}
	return 0;
error:
	return 1;
}

int mgmt_initialization (void)
{
	struct sockaddr_hci addr;

	CHECK_ERR_GOTO (mutex_init (&mgmt_lock) != 0, "Cannot init mgmt mutex");

	ble_fd = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK , BTPROTO_HCI);
	CHECK_ERR_GOTO (ble_fd < 0, "Cannot init bluetooth socket");

	memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = HCI_DEV_NONE;
	addr.hci_channel = HCI_CHANNEL_CONTROL;

	CHECK_ERR_GOTO (bind(ble_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0, "Cannot bind socket");
	return 0;
error:
	CLOSE (ble_fd);
	return 1;

}

int mgmt_clean (void)
{
	CLOSE (ble_fd);
	CHECK_WARN (mutex_destroy (&mgmt_lock) != 0, "Cannot destroy mgmt mutex");
	return 0;
}


int mgmt_set_simple_setting(uint8_t enable, int op)
{
	str_mgmt_simple_cmd_t cmd;
	size_t pksz = sizeof(cmd);
	int n = 0;

	CHECK_ERR_GOTO (mutex_lock (&mgmt_lock), "Cannot lock mutex");
	PRINT_DEBUG ("Set op  0x%x with value : 0x%x", op, enable);
	CHECK_ERR_GOTO (ble_fd < 0, "Invalid socket");
	cmd.opcode = htole16(op);
	cmd.index = htole16(0); // hci0
	cmd.length = htole16(sizeof(enable));
	cmd.enable = enable;

	n = send(ble_fd, &cmd, pksz, 0);
	CHECK_ERR_GOTO (n < 0, "Cannot send mgmt data");
	CHECK_INF_GOTO (handle_set_reply (op) != 0, "Error on reply process");
	CHECK_ERR_GOTO (mutex_unlock (&mgmt_lock), "Cannot unlock mutex");
	return 0;
error:
	mutex_error (&mgmt_lock);
	return 1;
}

int mgmt_set_discoverable(uint8_t enable, uint16_t timeout)
{
	str_mgmt_discov_cmd_t cmd;
	size_t pksz = sizeof(cmd);
	int n = 0;

	CHECK_ERR_GOTO (mutex_lock (&mgmt_lock), "Cannot lock mutex");
	PRINT_DEBUG ("Set op 0x%x with value : 0x%x/0x%x", MGMT_OP_SET_DISCOVERABLE, enable, timeout);
	CHECK_ERR_GOTO (ble_fd < 0, "Invalid socket");
	cmd.opcode = htole16(MGMT_OP_SET_DISCOVERABLE);
	cmd.index = htole16(0); // hci0
	cmd.length = htole16(sizeof(enable) + sizeof(timeout));
	cmd.enable = enable;
	cmd.timeout = timeout;

	n = send(ble_fd, &cmd, pksz, 0);
	CHECK_ERR_GOTO (n < 0, "Cannot send mgmt data");
	CHECK_INF_GOTO (handle_set_reply (MGMT_OP_SET_DISCOVERABLE) != 0, "Error on reply process");
	CHECK_ERR_GOTO (mutex_unlock (&mgmt_lock), "Cannot unlock mutex");
	return 0;
error:
	mutex_error (&mgmt_lock);
	return 1;
}

int mgmt_set_local_name(const char* name)
{
	str_mgmt_name_cmd_t cmd;
	size_t pksz = sizeof(cmd);
	int n = 0;

	CHECK_ERR_GOTO (mutex_lock (&mgmt_lock), "Cannot lock mutex");
	PRINT_DEBUG ("Set op 0x%x", MGMT_OP_SET_LOCAL_NAME);
	memset (&cmd.mgmt_name, 0, sizeof(cmd.mgmt_name));
	memcpy (cmd.mgmt_name.short_name, name, MIN(MGMT_MAX_SHORT_NAME_LENGTH - 1, strlen(name)));
	memcpy (cmd.mgmt_name.name, name, strlen(name));

	CHECK_ERR_GOTO (ble_fd < 0, "Invalid socket");
	cmd.opcode = htole16(MGMT_OP_SET_LOCAL_NAME);
	cmd.index = htole16(0); // hci0
	cmd.length = htole16(sizeof(struct mgmt_ev_local_name_changed));

	n = send(ble_fd, &cmd, pksz, 0);
	CHECK_ERR_GOTO (n < 0, "Cannot send mgmt data");
	CHECK_INF_GOTO (handle_set_reply (MGMT_OP_SET_LOCAL_NAME) != 0, "Error on reply process");
	CHECK_ERR_GOTO (mutex_unlock (&mgmt_lock), "Cannot unlock mutex");
	return 0;
error:
	mutex_error (&mgmt_lock);
	return 1;
}

int mgmt_check_status(void)
{
	str_mgmt_basic_cmd_t cmd;
	size_t pksz = sizeof(cmd);
	ssize_t n;
	int status = -1;

	CHECK_ERR_GOTO (mutex_lock (&mgmt_lock), "Cannot lock mutex");
	CHECK_ERR_GOTO (ble_fd < 0, "Invalid socket");
	cmd.opcode = htole16(MGMT_OP_READ_INFO);
	cmd.index = htole16(0); // hci0
	cmd.length = htole16(0);

	n = send(ble_fd, &cmd, pksz, 0);
	CHECK_ERR_GOTO (n < 0, "Cannot send mgmt data");
	CHECK_ERR_GOTO (handle_info_reply (&status) != 0, "Error on info reply");
	CHECK_ERR_GOTO (mutex_unlock (&mgmt_lock), "Cannot unlock mutex");
	return status;
error:
	mutex_error (&mgmt_lock);
	return -1;
}

static int check_hci_status (void)
{
	int hci_sock = -1;
	static struct hci_dev_info di;

	hci_sock = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_NONBLOCK, BTPROTO_HCI);
	CHECK_ERR_GOTO (hci_sock < 0, "Cannot init hci socket");
	if (ioctl(hci_sock, HCIGETDEVINFO, (void *)&di) != 0)
	{
		PRINT_DEBUG ("HCI interface is dead.");
		CLOSE (hci_sock);
		return -1;
	}

	if (hci_test_bit(HCI_UP, &di.flags))
	{
		PRINT_DEBUG ("HCI interface is up");
		CLOSE (hci_sock);
		return 1;
	}
	else
	{
		PRINT_DEBUG ("HCI interface is down");
		CLOSE (hci_sock);
		return 0;
	}
	CLOSE (hci_sock);
	return 0;
error:
	CLOSE (hci_sock);
	return -1;
}

int mgmt_hci_enable (void)
{
	int state = -1;
	int retry = 0;

	retry = 0;
	PRINT_INFO ("Enable HCI.. (%d)", retry);
	while (check_hci_status () == 0 && retry < 5)
	{
		CHECK_WARN (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state) != 0, "Cannot set cancel state");
		CHECK_INFO (mgmt_set_simple_setting (0x00, MGMT_OP_SET_POWERED) != 0, "Cannot set power");
		// enable dual mode !
		CHECK_INFO (mgmt_set_simple_setting (0x01, MGMT_OP_SET_LE) != 0, "Cannot set le");
		CHECK_INFO (mgmt_set_simple_setting (0x01, MGMT_OP_SET_BREDR) != 0, "Cannot set bredr");
		CHECK_INFO (mgmt_set_simple_setting (0x01, MGMT_OP_SET_CONNECTABLE) != 0, "Cannot set connectable");
		CHECK_INFO (mgmt_set_simple_setting (0x01, MGMT_OP_SET_BONDABLE) != 0, "Cannot set bondable");
		CHECK_INFO (mgmt_set_discoverable (0x01, 0x00) != 0, "Cannot set discoverable");
		CHECK_INFO (mgmt_set_local_name ("VERY_LONG_BLE_NAME") != 0, "Cannot set local name");
		// mgmt doc -> 0x02 should be the preferred mode of operation when implementing peripheral mode.
		CHECK_INFO (mgmt_set_simple_setting (0x02, MGMT_OP_SET_ADVERTISING) != 0, "Cannot set advertising");
		CHECK_INFO (mgmt_set_simple_setting (0x01, MGMT_OP_SET_POWERED) != 0, "Cannot set high speed");
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
		retry++;
	}

	CHECK_ERR_GOTO (check_hci_status () == 0, "Cannot enable HCI");
	PRINT_INFO ("Enable HCI.. Done.");
	return 0;
error:
	if (state != -1)
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
	return 1;
}

int mgmt_hci_disable (void)
{
	int retry = 0;
	int state = -1;

	PRINT_INFO ("Disable HCI (%d)..", retry);
	while (check_hci_status () == 1 && retry < 5)
	{
		CHECK_WARN (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state) != 0, "Cannot set cancel state");
		CHECK_INFO (mgmt_set_simple_setting (0x00, MGMT_OP_SET_CONNECTABLE) != 0, "Cannot set advertising");
		CHECK_INFO (mgmt_set_simple_setting (0x00, MGMT_OP_SET_POWERED) != 0, "Cannot set power");
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
		retry++;
	}

	CHECK_ERR_GOTO (check_hci_status () == 1, "Cannot disable HCI");
	PRINT_INFO ("Disable HCI.. Done.");
	return 0;
error:
	if (state != -1)
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
	return 1;
}

int main (void)
{
	int retry = 0;

	mgmt_initialization ();

	while (retry < 100)
	{
		CHECK_WARN (mgmt_hci_disable () != 0, "Cannot disable hci");
		sleep (3);
		CHECK_WARN (mgmt_hci_enable () != 0, "Cannot enable hci");
		sleep (1);
		CHECK_ERR_GOTO (mgmt_check_status () != 0, "Invalid hci status");
		sleep (1);
		retry++;
	}

	PRINT_DEBUG ("All done with success!");
error:
	mgmt_clean ();
}
