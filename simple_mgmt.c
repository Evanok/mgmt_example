#include "simple_mgmt.h"

static int ble_fd = -1;
static str_mutex_context mgmt_lock;
static str_mutex_context hci_mutex;		/*!< mutex to hci queue */
static hci_enum hci_queue[HCI_QUEUE_SIZE];	/*!< hci queue */
static sem_t hci_sem;				/*!< semephore to process hci queue */

static char* mgmt_reply_codestr (unsigned char code)
{
	switch (le16toh(code))
	{
	case MGMT_EV_CMD_COMPLETE:
		return ("CMD COMPLETE");
		break;
	case MGMT_EV_DEVICE_DISCONNECTED:
		return ("DEVICE DISCONNECTED");
		break;
	case MGMT_EV_DEVICE_CONNECTED:
		return ("DEVICE CONNECTED");
		break;
	case MGMT_EV_CMD_STATUS:
		return ("CMD STATUS");
		break;
	default:
		return ("Unknown");
		break;
	}
}

static char* mgmt_req_codestr (unsigned char code)
{
	switch (le16toh(code))
	{
	case MGMT_OP_SET_CONNECTABLE:
		return ("SET CONNECTABLE");
		break;
	case MGMT_OP_SET_POWERED:
		return ("SET POWER");
		break;
	case MGMT_OP_SET_ADVERTISING:
		return ("SET ADVERTISING");
		break;
	case MGMT_OP_SET_LE:
		return ("SET LE");
		break;
	case MGMT_OP_SET_BREDR:
		return ("SET BREDR");
		break;
	case MGMT_OP_SET_BONDABLE:
		return ("SET BONDABLE");
		break;
	case MGMT_OP_SET_DISCOVERABLE:
		return ("SET DISCOVERABLE");
		break;
	case MGMT_OP_SET_LOCAL_NAME:
		return ("SET LOCAL NAME");
		break;
	case MGMT_OP_READ_INFO:
		return ("READ INFO");
		break;
	default:
		return ("Unknown");
		break;
	}
}

static int dump_mgmt_queue (void)
{
	str_mgmt_basic_cmd_t answer;
	fd_set rset;
	struct timeval tv;
	char buf[MAX_PATH];
	int n = 0;
retry:
	// Wait up to 100 ms.
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

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
		if (n == 0)
		{
			PRINT_DEBUG ("Timeout on select. MGMT queue is empty !");
			return 0;
		}
	}

	CHECK_ERR_GOTO (!FD_ISSET(ble_fd, &rset), "Signal from unexpected fd");
	memset (&answer, 0, sizeof(answer));
	n = read (ble_fd, &buf[0], sizeof(buf));
	CHECK_ERR_GOTO (n <= 0, "Cannot read mgmt answer (%d)", n);
	PRINT_DEBUG ("Read %d bytes", n);
	memcpy (&answer, &buf[0], sizeof(answer));
	PRINT_DEBUG ("Dump op code 0x%x (%s)", le16toh(answer.opcode), mgmt_reply_codestr(le16toh(answer.opcode)));
	goto retry;
error:
	return 1;
}

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
	int attempt = 20;

retry:
	CHECK_INF_GOTO (attempt <= 0, "Reach max attempt. No more retry");

	// Wait up to 500 ms.
	tv.tv_sec = 0;
	tv.tv_usec = 500000;

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
 		if (n == 0)
		{
			PRINT_INFO ("Timeout on select");
			attempt--;
			goto retry;
		}
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

	PRINT_DEBUG ("first op code : 0x%x (%s)", le16toh(answer.opcode), mgmt_reply_codestr(le16toh(answer.opcode)));
	if (le16toh(answer.opcode) == MGMT_EV_CMD_STATUS)
	{
		CHECK_ERR_GOTO (n < offset + (int)sizeof(status), "Not enought data to fill status (%d)", n);
		memcpy (&status, &buf[offset], sizeof(status));
		offset += sizeof(status);
		PRINT_INFO ("Error on reply. Status : 0x%x (%s)", status.status, mgmt_errstr(status.status));
		CHECK_INF_GOTO (status.status == MGMT_STATUS_REJECTED || status.status == MGMT_STATUS_BUSY, "Rejected or Busy status. Skip attempt");
		attempt--;
		goto retry;
	}
	if (le16toh(answer.opcode) != MGMT_EV_CMD_COMPLETE)
	{
		PRINT_DEBUG ("Unexpected op code 0x%x (%s)", le16toh(answer.opcode), mgmt_reply_codestr(le16toh(answer.opcode)));
		goto retry;
	}
	CHECK_ERR_GOTO (n < offset + (int)sizeof(evt), "Not enought data to fill evt (%d)", n);
	memcpy (&evt, &buf[offset], sizeof(evt));
	offset += sizeof(evt);
	PRINT_DEBUG ("second op code : 0x%x (%s)", le16toh(evt.opcode), mgmt_req_codestr(le16toh(evt.opcode)));
	CHECK_INF_GOTO (le16toh(evt.opcode) != op, "Unexpected answer complete event 0x%x", le16toh(evt.opcode));
	PRINT_DEBUG ("Status answer : 0x%x", evt.status);
	CHECK_INF_GOTO (evt.status != 0, "Unexpected status 0x%x on op 0x%x", evt.status, op);
	return 0;
error:
	return 1;
}

static int handle_info_reply (int* status, int quiet)
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
	int attempt = 20;
	static int try = 0;

	PRINT_DEBUG ("Check hci0 status..");
retry:
	CHECK_INF_GOTO (attempt <= 0, "Reach max attempt. No more retry");

	// Wait up to 100 ms.
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

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
 		if (n == 0)
		{
			PRINT_INFO ("Timeout on select");
			attempt--;
			goto retry;
		}
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
		PRINT_DEBUG ("Unexpected op code 0x%x (%s)", le16toh(answer.opcode), mgmt_reply_codestr(le16toh(answer.opcode)));
		// only want to increment attempt on code different from connect/disconnect
		if (le16toh(answer.opcode) != MGMT_EV_DEVICE_DISCONNECTED && le16toh(answer.opcode) != MGMT_EV_DEVICE_CONNECTED)
			attempt--;
		goto retry;
	}
	CHECK_ERR_GOTO (n < offset + (int)sizeof(evt), "Not enought data to fill evt (%d)", n);
	memcpy (&evt, &buf[offset], sizeof(evt));
	offset += sizeof(evt);
	if (le16toh(evt.opcode) != MGMT_OP_READ_INFO)
	{
		PRINT_DEBUG ("Unexpected answer code 0x%x (%s)", le16toh(evt.opcode), mgmt_req_codestr(le16toh(evt.opcode)));
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
		if (!quiet)
		{
			PRINT_WARN ("HCI BTC mode is not enabled");
			if (try < 5)
			{
				CHECK_WARN (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
				CHECK_WARN (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
				try++;
			}
		}
		*status = 1;
	}
	else if ((current & MGMT_SETTING_LE) == 0)
	{
		if (!quiet)
		{
			PRINT_WARN ("HCI LE mode is not enabled");
			if (try < 5)
			{
				CHECK_WARN (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
				CHECK_WARN (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
				try++;
			}
		}
		*status = 1;
	}
	else if ((current & MGMT_SETTING_POWERED) == 0)
	{
		if (!quiet)
		{
			PRINT_WARN ("HCI Powered is not enabled");
			if (try < 5)
			{
				CHECK_WARN (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
				CHECK_WARN (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
				try++;
			}
		}
		*status = 1;
	}
	else if ((current & MGMT_SETTING_DISCOVERABLE) == 0)
	{
		if (!quiet)
		{
			PRINT_WARN ("HCI Discoverable is not enabled");
			if (try < 5)
			{
				CHECK_WARN (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
				CHECK_WARN (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
				try++;
			}
		}
		*status = 1;
	}
	else if ((current & MGMT_SETTING_ADVERTISING) == 0)
	{
		if (!quiet)
		{
			PRINT_WARN ("HCI Advertising is not enabled");
			if (try < 5)
			{
				CHECK_WARN (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
				CHECK_WARN (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
				try++;
			}
		}
		*status = 1;
	}
	else if ((current & MGMT_SETTING_CONNECTABLE) == 0)
	{
		if (!quiet)
		{
			PRINT_WARN ("HCI Connectable mode is not enabled");
			if (try < 5)
			{
				CHECK_WARN (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
				CHECK_WARN (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
				try++;
			}
		}
		*status = 1;
	}
	return 0;
error:
	return 1;
}

int mgmt_set_simple_setting(uint8_t enable, int op)
{
	str_mgmt_simple_cmd_t cmd;
	size_t pksz = sizeof(cmd);
	int n = 0;

	CHECK_ERR_GOTO (mutex_lock (&mgmt_lock), "Cannot lock mutex");
	dump_mgmt_queue ();
	PRINT_DEBUG ("Set op  0x%x (%s) with value : 0x%x", op, mgmt_req_codestr(op), enable);
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
	dump_mgmt_queue ();
	PRINT_DEBUG ("Set op 0x%x (%s) with value : 0x%x/0x%x", MGMT_OP_SET_DISCOVERABLE, mgmt_req_codestr(MGMT_OP_SET_DISCOVERABLE), enable, timeout);
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
	dump_mgmt_queue ();
	PRINT_DEBUG ("Set op 0x%x (%s)", MGMT_OP_SET_LOCAL_NAME, mgmt_req_codestr(MGMT_OP_SET_LOCAL_NAME));
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

int mgmt_check_status(int quiet)
{
	str_mgmt_basic_cmd_t cmd;
	size_t pksz = sizeof(cmd);
	ssize_t n;
	int status = -1;

	CHECK_ERR_GOTO (mutex_lock (&mgmt_lock), "Cannot lock mutex");
	CHECK_ERR_GOTO (ble_fd < 0, "Invalid socket");
	dump_mgmt_queue ();
	cmd.opcode = htole16(MGMT_OP_READ_INFO);
	cmd.index = htole16(0); // hci0
	cmd.length = htole16(0);

	n = send(ble_fd, &cmd, pksz, 0);
	CHECK_ERR_GOTO (n < 0, "Cannot send mgmt data");
	CHECK_ERR_GOTO (handle_info_reply (&status, quiet) != 0, "Error on info reply");
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

int hal_push_event_hci (hci_enum event)
{
	int nb_elem = -1;
	static int rear_queue = 0;

	PRINT_DEBUG ("Push hci event : %d", event);
	CHECK_ERR_GOTO (sem_getvalue (&hci_sem, &nb_elem) != 0, "Cannot get sem value");
	if (nb_elem >= HCI_QUEUE_SIZE)
	{
		PRINT_ERR_GOTO ("HCI Queue is full : %d elem", nb_elem);
	}

	CHECK_ERR_GOTO (mutex_lock (&hci_mutex) != 0, "Cannot lock mutex");
	hci_queue[rear_queue] = event;
	rear_queue = (rear_queue + 1) % HCI_QUEUE_SIZE;
	CHECK_ERR_GOTO (sem_post(&hci_sem) != 0, "Cannot sem_post event");
	CHECK_ERR_GOTO (mutex_unlock (&hci_mutex) != 0, "Cannot unlock mutex");

	return 0;
error:
	mutex_error (&hci_mutex);
	return 1;
}

int mgmt_hci_enable (void)
{
	int state = -1;
	int retry = 0;
	long int begin, end;

	begin = time (NULL);
	retry = 0;
	while (mgmt_check_status (1) != 0 && retry < 5)
	{
		if (retry != 0) sleep (1); // want delay between retry
		PRINT_INFO ("Enable HCI.. (%d)", retry++);
		CHECK_WARN (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state) != 0, "Cannot set cancel state");
		if (check_hci_status () == 1)
		{
			if (mgmt_set_simple_setting (0x00, MGMT_OP_SET_POWERED) != 0)
			{
				PRINT_INFO ("Cannot set power");
				continue;
			}
		}
		if (mgmt_set_simple_setting (0x01, MGMT_OP_SET_LE) != 0)
		{
			PRINT_INFO ("Cannot set le");
			continue;
		}
		if (mgmt_set_simple_setting (0x01, MGMT_OP_SET_BREDR) != 0)
		{
			PRINT_INFO ("Cannot set bredr");
			continue;
		}
		if (mgmt_set_simple_setting (0x01, MGMT_OP_SET_CONNECTABLE) != 0)
		{
			PRINT_INFO ("Cannot set connectable");
			continue;
		}
		if (mgmt_set_simple_setting (0x00, MGMT_OP_SET_BONDABLE) != 0)
		{
			PRINT_INFO ("Cannot set bondable");
			continue;
		}
		if (mgmt_set_discoverable (0x01, 0x00) != 0)
		{
			PRINT_INFO ("Cannot set discoverable");
			continue;
		}

		CHECK_INFO (mgmt_set_local_name ("MGMT-TEST") != 0, "Cannot set local name");
		if (mgmt_set_simple_setting (0x02, MGMT_OP_SET_ADVERTISING) != 0)
		{
			PRINT_INFO ("Cannot set advertising");
			continue;
		}
		if (mgmt_set_simple_setting (0x01, MGMT_OP_SET_POWERED) != 0)
		{
			PRINT_INFO ("Cannot set power");
			continue;
		}
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
		retry++;
	}

	end = time (NULL);
	if (mgmt_check_status (0) != 0)
	{
		PRINT_WARN ("Cannot enable HCI (%ld seconds).", end - begin);
		return 1;
	}
	if (retry == 0)
	{
		PRINT_INFO ("Enable HCI already done");
	}
	else
	{
		PRINT_INFO ("Enable HCI.. Done (%ld seconds).", end - begin);
	}
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
	long int begin, end;

	begin = time (NULL);
	while (check_hci_status () == 1 && retry < 5)
	{
		if (retry != 0) sleep (1); // want delay between retry
		PRINT_INFO ("Disable HCI (%d)..", retry);
		CHECK_WARN (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state) != 0, "Cannot set cancel state");
		CHECK_INFO (mgmt_set_simple_setting (0x00, MGMT_OP_SET_CONNECTABLE) != 0, "Cannot set connectable");
		CHECK_INFO (mgmt_set_simple_setting (0x00, MGMT_OP_SET_POWERED) != 0, "Cannot set power");
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
		retry++;
	}

	end = time (NULL);
	CHECK_ERR_GOTO (check_hci_status () == 1, "Cannot disable HCI (%ld seconds)", end - begin);
	if (retry == 0)
	{
		PRINT_INFO ("Disable HCI already done");
	}
	else
	{
		PRINT_INFO ("Disable HCI.. Done (%ld seconds).", end - begin);
	}
	return 0;
error:
	if (state != -1)
		CHECK_WARN (pthread_setcancelstate(state, NULL) != 0, "Cannot set cancel state");
	return 1;
}

void* hci_thread (void *arg __attribute__((__unused__)))
{
	hci_enum e = -1;
	int front_queue = 0;
	int current_hci_status = 0; // 0 -> disable, 1 -> enable

	CHECK_ERR_GOTO (mutex_init(&mgmt_lock) != 0, "Cannot init mgmt mutex");
	while (1)
	{
		CHECK_ERR_GOTO (sem_wait_nointr(&hci_sem) != 0, "Cannot wait hci event");
		e = hci_queue[front_queue];
		front_queue = (front_queue + 1) % HCI_QUEUE_SIZE;

		PRINT_DEBUG ("Receive new hci event : %d", e);
		if (e == HCI_ENABLE)
		{
			if (current_hci_status == 0)
			{
				if (mgmt_hci_enable () == 0) current_hci_status = 1;
			}
			else
			{
				PRINT_DEBUG ("Ignore hci request. hci already enabled");
			}
		}
		else if (e == HCI_DISABLE)
		{
			if (current_hci_status == 1)
			{
				if (mgmt_hci_disable () == 0) current_hci_status = 0;
			}
			else
			{
				PRINT_DEBUG ("Ignore hci request. hci already disabled");
			}
		}
		else if (e == HCI_CHECK)
		{
			CHECK_WARN (mgmt_check_status (0) == -1, "Cannot check hci status");
		}
		else
		{
			PRINT_WARN ("Unknown hci event : %d", e);
		}
	}

	CHECK_WARN (mutex_destroy (&mgmt_lock) != 0, "Cannot destroy mgmt mutex");
	pthread_exit (NULL);
error:
	CHECK_WARN (mutex_destroy (&mgmt_lock) != 0, "Cannot destroy mgmt mutex");
	pthread_exit (NULL);
}

int mgmt_initialization (void)
{
	struct sockaddr_hci addr;
	pthread_t t_hci;

	CHECK_ERR_GOTO (sem_init(&hci_sem, 0, 0) != 0, "Cannot init semaphore");
	CHECK_ERR_GOTO (mutex_init(&hci_mutex) != 0, "Cannot init hci mutex");

	PRINT_INFO ("Initialize MGMT..");
	// start hci thread
	if (pthread_create (&t_hci, NULL, hci_thread, NULL))
		PRINT_ERR_GOTO ("error on pthread_create for hci");
	if (pthread_detach (t_hci)!= 0)
		PRINT_ERR_GOTO ("error on pthread_detach for hci");

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
	CHECK_WARN (mutex_destroy (&hci_mutex) != 0, "Cannot destroy hci mutex");
	CHECK_WARN (sem_destroy (&hci_sem) != 0, "Cannot destroy semaphore");
	return 0;
}

int main (void)
{
	int retry = 0;

	mgmt_initialization ();

	while (retry < 20)
	{
		CHECK_ERR_GOTO (hal_push_event_hci (HCI_DISABLE) != 0, "Cannot send hci event");
		sleep (1);
		CHECK_ERR_GOTO (hal_push_event_hci (HCI_ENABLE) != 0, "Cannot send hci event");
		sleep (1);
		CHECK_ERR_GOTO (hal_push_event_hci (HCI_CHECK) != 0, "Cannot send hci event");
		sleep (1);
		retry++;
	}

	PRINT_DEBUG ("All done with success!");
error:
	mgmt_clean ();
}
