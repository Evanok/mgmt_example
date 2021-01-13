/**
**

** \file simple_mgmt.h
** \brief define header for simple mgmt
** \author Arthur LAMBERT
** \date 25/11/2020
**
**/

#ifndef SIMPLE_MGMT_H_
# define SIMPLE_MGMT_H_

# include <sys/types.h>
# include <bluetooth/bluetooth.h>
# include <bluetooth/hci.h>
# include "mgmt.h"
# include "mutex.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/ioctl.h>

# include "log.h"
# include "macro.h"

#define HCI_QUEUE_SIZE 10

typedef enum
{
	HCI_INITIAL	= -1,
	HCI_ENABLE	= 0,
	HCI_DISABLE	= 1,
	HCI_CHECK	= 2,
} hci_enum;

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
static inline int hci_test_bit(int nr, void *addr)
{
	return *((uint32_t *) addr + (nr >> 5)) & (1 << (nr & 31));
}

typedef struct __attribute__((__packed__))
{
    uint16_t opcode;
    uint16_t index;
    uint16_t length;
    uint8_t enable;
} str_mgmt_simple_cmd_t;

typedef struct __attribute__((__packed__))
{
	uint16_t opcode;
	uint16_t index;
	uint16_t length;
	uint8_t enable;
	uint16_t timeout;
} str_mgmt_discov_cmd_t;

typedef struct __attribute__((__packed__))
{
    uint16_t opcode;
    uint16_t index;
    uint16_t length;
} str_mgmt_basic_cmd_t;

typedef struct __attribute__((__packed__))
{
    uint16_t opcode;
    uint16_t index;
    uint16_t length;
    struct mgmt_ev_local_name_changed mgmt_name;
} str_mgmt_name_cmd_t;

int mgmt_initialiation (void);
int mgmt_set_simple_setting (uint8_t enable, int op);
int mgmt_set_discoverable(uint8_t enable, uint16_t timeout);
int mgmt_set_local_name(const char* name);
int mgmt_check_status(int quiet);

int hal_push_event_hci (hci_enum event);

#endif /* !SIMPLE_MGMT_H_ */
