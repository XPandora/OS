#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

#define MAX_TX_PKTSIZE 1518
#define MAX_RX_PKTSIZE 2048

volatile uint32_t *e1000_base_addr;
struct tx_desc *tx_descs;
#define N_TXDESC (PGSIZE / sizeof(struct tx_desc))
// struct tx_desc tx_descs[N_TXDESC] __attribute__((aligned(16)));
char tx_buf[N_TXDESC][MAX_TX_PKTSIZE];

// challenge!
uint16_t e1000_eeprom_read_register(uint8_t address)
{
	// mark the start bit, and is self-clearing
	base->EERD = (address << 8) | 0x1;
	// wait for the done bit
	while ((base->EERD & 0x10) == 0)
		;

	// return the data bits
	return base->EERD >> 16;
}

int e1000_tx_init()
{
	// Allocate one page for descriptors

	// Initialize all descriptors

	// Set hardward registers
	// Look kern/e1000.h to find useful definations
	// cprintf("global addr: %08x\n", tx_descs);
	// struct tx_desc* tx_descss;
	tx_descs = (struct tx_desc *)page2kva(page_alloc(ALLOC_ZERO));
	cprintf("alloc addr: %08x\n", tx_descs);
	for (int i = 0; i < N_TXDESC; i++)
	{
		tx_descs[i].addr = PADDR(tx_buf[i]);
		tx_descs[i].status |= E1000_TX_STATUS_DD;
	}

	base->TDBAL = PADDR(tx_descs);
	base->TDBAH = 0;
	base->TDLEN = PGSIZE;
	base->TDH = 0;
	base->TDT = 0;
	base->TCTL |= E1000_TCTL_EN;
	base->TCTL |= E1000_TCTL_PSP;
	base->TCTL |= E1000_TCTL_CT_ETHER;
	base->TCTL |= E1000_TCTL_COLD_FULL_DUPLEX;
	base->TIPG = E1000_TIPG_DEFAULT;
	return 0;
}

struct rx_desc *rx_descs;
#define N_RXDESC (PGSIZE / sizeof(struct rx_desc))
char rx_buf[N_RXDESC][MAX_RX_PKTSIZE];

int e1000_rx_init()
{
	// Allocate one page for descriptors

	// Initialize all descriptors
	// You should allocate some pages as receive buffer

	// Set hardward registers
	// Look kern/e1000.h to find useful definations
	uint16_t mac_low_addr = e1000_eeprom_read_register(0);
	uint16_t mac_mid_addr = e1000_eeprom_read_register(1);
	uint16_t mac_high_addr = e1000_eeprom_read_register(2);

	base->RAL = (mac_low_addr & 0xffff) + ((mac_mid_addr << 16) & 0xffff0000);
	base->RAH = mac_high_addr;

	e1000_mac_address[0] = mac_low_addr & 0xff;
	e1000_mac_address[1] = (mac_low_addr >> 8) & 0xff;
	e1000_mac_address[2] = mac_mid_addr & 0xff;
	e1000_mac_address[3] = (mac_mid_addr >> 8) & 0xff;
	e1000_mac_address[4] = mac_high_addr & 0xff;
	e1000_mac_address[5] = (mac_high_addr >> 8) & 0xff;

	for (int i = 0; i < 128; i++)
	{
		base->MTA[i] = 0;
	}

	rx_descs = (struct rx_desc *)page2kva(page_alloc(ALLOC_ZERO));
	for (int i = 0; i < N_RXDESC; i++)
	{
		rx_descs[i].addr = PADDR(rx_buf[i]);
	}

	base->RDBAL = PADDR(rx_descs);
	base->RDBAH = 0;
	base->RDLEN = PGSIZE;
	base->RDH = 0;
	base->RDT = N_RXDESC - 1;
	base->RCTL |= E1000_RCTL_EN;
	base->RCTL |= E1000_RCTL_BSIZE_2048;
	base->RCTL |= E1000_RCTL_SECRC;

	return 0;
}

int pci_e1000_attach(struct pci_func *pcif)
{
	// Enable PCI function
	// Map MMIO region and save the address in 'base;
	pci_func_enable(pcif);
	e1000_base_addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	base = (struct E1000 *)e1000_base_addr;
	// cprintf("STATUS: %08x\n", base->STATUS);

	e1000_tx_init();
	e1000_rx_init();
	return 0;
}

int e1000_tx(const void *buf, uint32_t len)
{
	// Send 'len' bytes in 'buf' to ethernet
	// Hint: buf is a kernel virtual address

	if (buf == NULL || len > MAX_TX_PKTSIZE)
	{
		return -E_INVAL;
	}

	uint32_t tail = base->TDT;
	if ((tx_descs[tail].status & E1000_TX_STATUS_DD) == 0)
	{
		return -E_AGAIN;
	}

	memset(tx_buf[tail], 0, MAX_TX_PKTSIZE);
	memmove(tx_buf[tail], buf, len);

	tx_descs[tail].length = len;
	tx_descs[tail].cmd |= E1000_TX_CMD_RS;
	tx_descs[tail].cmd |= E1000_TX_CMD_EOP;
	tx_descs[tail].status &= ~E1000_TX_STATUS_DD;

	base->TDT = (base->TDT + 1) % N_TXDESC;
	return 0;
}

int e1000_rx(void *buf, uint32_t len)
{
	// Copy one received buffer to buf
	// You could return -E_AGAIN if there is no packet
	// Check whether the buf is large enough to hold
	// the packet
	// Do not forget to reset the decscriptor and
	// give it back to hardware by modifying RDT
	if (buf == NULL || len > MAX_RX_PKTSIZE)
	{
		panic("should not be here\n");
		return -E_INVAL;
	}

	uint32_t tail = base->RDT;
	tail = (tail + 1) % N_RXDESC;
	if ((rx_descs[tail].status & E1000_RX_STATUS_DD) == 0)
	{
		return -E_AGAIN;
	}

	// ... the parameter len is useless
	len = rx_descs[tail].length;

	memmove(buf, rx_buf[tail], len);

	rx_descs[tail].status &= ~E1000_RX_STATUS_DD;
	rx_descs[tail].status &= ~E1000_RX_STATUS_EOP;
	base->RDT = tail;
	return len;
}
