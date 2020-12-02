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

#define __DIR__ "MGMT"

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
int mgmt_clean (void);
int mgmt_set_power (uint8_t val);
int mgmt_get_power (void);

#endif /* !SIMPLE_MGMT_H_ */
