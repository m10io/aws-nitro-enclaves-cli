/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _NE_PCI_DEV_H_
#define _NE_PCI_DEV_H_

#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/wait.h>

/* Nitro Enclaves (NE) PCI device identifier */

/**
 * PCI_VENDOR_ID_AMAZON is included in linux/pci_ids.h since kernel 4.18.
 * Define it here to be available for older kernel versions.
 */
#ifndef PCI_VENDOR_ID_AMAZON
#define PCI_VENDOR_ID_AMAZON (0x1d0f)
#endif
#define PCI_DEVICE_ID_NE (0xe4c1)
#define PCI_BAR_NE (0x03)

/* Device registers */

/**
 * (1 byte) Register to notify the device that the driver is using it
 * (Read/Write).
 */
#define NE_REG_ENABLE (0x0000)

#define DEV_DISABLE (0x00)
#define DEV_ENABLE (0x01)
#define DEV_ENABLE_MASK (0xff)

/*
 * TODO: Add feature negotiation bitmap (device exposes 32 bits of features,
 * driver selects 32 bits of features).
 */
/* (2 bytes) Register to select the device run-time version (Read/Write). */
#define NE_REG_VERSION (0x0002)

#define DEV_VERSION (0x0001)
#define DEV_VERSION_MASK (0xffff)

/**
 * (4 bytes) Register to notify the device what command was requested
 * (Write-Only).
 */
#define NE_REG_COMMAND (0x0004)

/**
 * (4 bytes) Register to notify the driver that a reply or a device event
 * is available (Read-Only):
 * - Lower half  - command reply counter
 * - Higher half - out-of-band device event counter
 */
#define NE_REG_REPLY_PENDING (0x000c)

#define REPLY_MASK (0x0000ffff)
#define EVENT_MASK (0xffff0000)

/* (240 bytes) Buffer for sending the command request payload (Read/Write). */
#define NE_REG_SEND_DATA (0x0010)

/* (240 bytes) Buffer for receiving the command reply payload (Read-Only). */
#define NE_REG_RECV_DATA (0x0100)

/* Device MMIO buffer sizes */

/* TODO: Update the buffer size value in the Nitro Enclaves device. */
/* 240 bytes for send / recv buffer. */
#define NE_SEND_DATA_SIZE (240)
#define NE_RECV_DATA_SIZE (240)


/* MSI-X interrupt vectors */

/* Communication vector */
#define NE_VEC_COMM (0)

/* Out-of-band rescan vector */
#define NE_VEC_RESCAN (1)

/* Device command types. */
enum ne_pci_dev_cmd_type {
	INVALID_CMD = 0,
	ENCLAVE_START = 1,
	ENCLAVE_GET_SLOT = 2,
	ENCLAVE_STOP = 3,
	SLOT_ALLOC = 4,
	SLOT_FREE = 5,
	SLOT_ADD_MEM = 6,
	SLOT_ADD_VCPU = 7,
	SLOT_COUNT = 8,
	NEXT_SLOT = 9,
	SLOT_INFO = 10,
	SLOT_ADD_BULK_VCPUS = 11,
	SHUTDOWN = 12,
	MAX_CMD,
};

/* Device commands - payload structure for requests and replies. */

/*
 * TODO: Add flags field for each data structure, as first field and u64.
 * Have a union in a general request / reply data structure, including all
 * command types.
 */

struct enclave_start_req {
	/* Slot unique id mapped to the enclave to start. */
	u64 slot_uid;
	/**
	 * Context ID (CID) for the enclave vsock device.
	 * If 0, CID is autogenerated.
	 */
	u64 enclave_cid;
	/* TODO: Update to be u64. */
	/* Flags for the enclave to start with (e.g. debug mode). */
	u16 flags;
} __attribute__ ((__packed__));

struct enclave_start_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
	/**
	 * Context ID (CID) for the enclave vsock device.
	 * If 0, CID is autogenerated.
	 */
	u64 enclave_cid;
	/**
	 * Token used by user space to send the enclave image to the enclave
	 * VMM endpoint via vsock.
	 */
	u64 vsock_loader_token;
} __attribute__ ((__packed__));

struct enclave_get_slot_req {
	/* Context ID (CID) for the enclave vsock device. */
	u64 enclave_cid;
} __attribute__ ((__packed__));

struct enclave_get_slot_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct enclave_stop_req {
	/* Slot unique id mapped to the enclave to stop. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct enclave_stop_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_alloc_req {
	/* In order to avoid weird sizeof edge cases. */
	u8 unused;
} __attribute__ ((__packed__));

struct slot_alloc_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
	/**
	 * The maximum number of memory regions that can be handled by
	 * the lower levels.
	 */
	u64 mem_regions;
} __attribute__ ((__packed__));

struct slot_free_req {
	/* Slot unique id mapped to the slot to free. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_free_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_add_mem_req {
	/* Slot unique id mapped to the slot to add the memory region to. */
	u64 slot_uid;
	/* Physical address of the memory region to add to the slot. */
	u64 paddr;
	/* Memory size, in bytes, of the memory region to add to the slot. */
	u64 size;
} __attribute__ ((__packed__));

struct slot_add_mem_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_add_vcpu_req {
	/* Slot unique id mapped to the slot to add the vCPU to. */
	u64 slot_uid;
	/* vCPU ID of the CPU to add to the enclave. */
	u32 vcpu_id;
} __attribute__ ((__packed__));

struct slot_add_vcpu_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_count_req {
	/* In order to avoid weird sizeof edge cases. */
	u8 unused;
} __attribute__ ((__packed__));

struct slot_count_reply {
	s64 rc;
	/* Number of active slots. */
	u64 slot_count;
} __attribute__ ((__packed__));

struct next_slot_req {
	/* Slot unique id of the next slot in the iteration. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct next_slot_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_info_req {
	/* Slot unique id mapped to the slot to get information about. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct slot_info_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
	/* Context ID (CID) for the enclave vsock device. */
	u64 enclave_cid;
	/**
	 * The maximum number of memory regions that can be handled by
	 * the lower levels.
	 */
	u64 mem_regions;
	/* Enclave memory size. */
	u64 mem_size;
	/* Enclave vCPU count. */
	u64 nr_vcpus;
	/* TODO: Update to be u64. */
	/* Enclave flags that were setup at enclave start time. */
	u16 flags;
	/* The current state of the enclave. */
	u16 state;
} __attribute__ ((__packed__));

struct slot_add_bulk_vcpus_req {
	/* Slot unique id mapped to the slot to add vCPUs to. */
	u64 slot_uid;
	/* Number of vCPUs to add to the slot. */
	u64 nr_vcpus;
} __attribute__ ((__packed__));

struct slot_add_bulk_vcpus_reply {
	s64 rc;
	/* Slot unique id mapped to the enclave. */
	u64 slot_uid;
} __attribute__ ((__packed__));

struct shutdown_req {
	/* In order to avoid weird sizeof edge cases. */
	u8 unused;
} __attribute__ ((__packed__));

struct shutdown_reply {
	s64 rc;
} __attribute__ ((__packed__));

/*
 * TODO: Have a union in a general reply data structure, including all
 * command types.
 */
struct ne_pci_dev_cmd_reply {
	s32 rc;
	/* Valid for all commands except SLOT_COUNT and SHUTDOWN. */
	u64 slot_uid;
	/* Valid for ENCLAVE_START command. */
	u64 enclave_cid;
	/* Valid for ENCLAVE_START command. */
	u64 vsock_loader_token;
	/* Valid for SLOT_COUNT command. */
	u64 slot_count;
	/* Valid for SLOT_ALLOC and SLOT_INFO commands. */
	u64 mem_regions;
	/* Valid for SLOT_INFO command. */
	u64 mem_size;
	/* Valid for SLOT_INFO command. */
	u64 nr_vcpus;
	/* TODO: Update to be u64. */
	/* Valid for SLOT_INFO command. */
	u16 flags;
	/* Valid for SLOT_INFO command. */
	u16 state;
} __attribute__ ((__packed__));


/**
 * Used for polling the PCI device for replies as a fallback in cases where
 * MSI-X fails initialization or is not available.
 */
struct ne_pci_poll {
	/**
	 * The end time of the hrtimer for polling the PCI device for replies
	 * after which the hrtimer is stopped.
	 */
	ktime_t end_time;
	/**
	 * The previous reply of the PCI command.
	 *
	 * A value change in the NE_REG_REPLY_PENDING PCI register is
	 * used to determine when a reply is received.
	 */
	u32 prev_reply;
	/* Variable used to determine if a reply is received. */
	bool recv_reply;
	/* The wait queue needed for handling received replies. */
	wait_queue_head_t reply_q;
	/* Fallback timer for polling the PCI device for replies. */
	struct hrtimer timer;
};

/* Nitro Enclaves (NE) PCI device. */
struct ne_pci_dev {
	/* Variable set if a reply has been sent by the PCI device. */
	bool cmd_reply_available;
	/* Wait queue for handling command reply from the PCI device. */
	wait_queue_head_t cmd_reply_wait_q;
	/* List of the enclaves managed by the PCI device. */
	struct list_head enclaves_list;
	/* Lock for accessing the list of enclaves. */
	struct mutex enclaves_list_lock;
	/**
	 * Work queue for handling rescan events triggered by the Nitro
	 * Hypervisor which require enclave state rescaning and propagation
	 * to the enclave process.
	 */
	struct workqueue_struct *event_wq;
	/* MMIO region of the PCI device. */
	void __iomem *iomem_base;
	/* Work item for every received rescan event. */
	struct work_struct notify_work;
	/* Lock for accessing the PCI dev MMIO space. */
	struct mutex pci_dev_lock;
	/* Used for polling if MSI-X is not enabled. */
	struct ne_pci_poll poll;
};

/**
 * do_request - Submit command request to the PCI device based on the command
 * type and retrieve the associated reply.
 *
 * This function uses the ne_pci_dev lock to handle one command at a time.
 *
 * @pdev: PCI device to send the command to and receive the reply from.
 * @cmd_type: command type of the request sent to the PCI device.
 * @cmd_request: command request payload.
 * @cmd_request_size: size of the command request payload.
 * @cmd_reply: command reply payload.
 * @cmd_reply_size: size of the command reply payload.
 *
 * @returns: 0 on success, negative return value on failure.
 */
int do_request(struct pci_dev *pdev, enum ne_pci_dev_cmd_type cmd_type,
	       void *cmd_request, size_t cmd_request_size,
	       struct ne_pci_dev_cmd_reply *cmd_reply, size_t cmd_reply_size);

struct pci_dev *ne_get_pci_dev(void);

int ne_pci_dev_init(void);

void ne_pci_dev_uninit(void);

#endif /* _NE_PCI_DEV_H_ */