// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 *
 * Derived from MIPS:
 * Copyright (C) 1994 - 2003, 06, 07 by Ralf Baechle (ralf@linux-mips.org)
 * Copyright (C) 2007 MIPS Technologies, Inc.
 */
#include <linux/export.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/syscalls.h>

#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <asm/cpu-features.h>
#include <asm/dma.h>
#include <asm/loongarchregs.h>
#include <asm/processor.h>
#include <asm/setup.h>

const static int icache_lsize = 16;
const static int icache_sets = 256;
const static int icache_ways = 4;
const static int dcache_lsize = 16;
const static int dcache_sets = 256;
const static int dcache_ways = 4;

static inline void local_flush_icache_all(void) {
	unsigned int set, way;
	for (set = 0; set < icache_sets; set++) {
		for(way = 0; way < icache_ways; way++) {
			cache_op(8, set * icache_lsize + way);
		}
    }
}

static inline void local_flush_dcache_all(void) {
	unsigned int set, way;
	for (set = 0; set < dcache_sets; set++) {
		for(way = 0; way < dcache_ways; way++) {
			cache_op(9, set * dcache_lsize + way);
		}
    }
}

void local_flush_cache_all(void) {
	local_flush_dcache_all();
	local_flush_icache_all();
}

void local_flush_icache_range(unsigned long start, unsigned long end)
{
	asm volatile ("\tibar 0\n"::);
	local_flush_cache_all();
}

static void la32_dma_cache_wback_inv(unsigned long addr, unsigned long size);

void __update_cache(unsigned long address, pte_t pte)
{
	struct page *page;
	unsigned long pfn, addr;

	pfn = pte_pfn(pte);
	if (unlikely(!pfn_valid(pfn)))
		return;
	page = pfn_to_page(pfn);
	if (Page_dcache_dirty(page)) {
		if (PageHighMem(page))
			addr = (unsigned long)kmap_atomic(page);
		else
			addr = (unsigned long)page_address(page);

		la32_dma_cache_wback_inv(addr, 4096);

		if (PageHighMem(page))
			kunmap_atomic((void *)addr);

		ClearPageDcacheDirty(page);
	}
}

void cache_error_setup(void)
{
	extern char __weak except_vec_cex;
	set_merr_handler(0x0, &except_vec_cex, 0x80);
}

static unsigned long icache_size __read_mostly;
static unsigned long dcache_size __read_mostly;

static char *way_string[] = { NULL, "direct mapped", "2-way",
	"3-way", "4-way", "5-way", "6-way", "7-way", "8-way",
	"9-way", "10-way", "11-way", "12-way",
	"13-way", "14-way", "15-way", "16-way",
};

#ifdef CONFIG_32BIT

/* DMA cache operations. */
void (*_dma_cache_wback_inv)(unsigned long start, unsigned long size);
void (*_dma_cache_wback)(unsigned long start, unsigned long size);
void (*_dma_cache_inv)(unsigned long start, unsigned long size);

static void la32_dma_cache_wback_inv(unsigned long addr, unsigned long size)
 {
     /* Catch bad driver code */
     BUG_ON(size == 0);

     if (size >= dcache_size) {
         local_flush_dcache_all();
     } else {
         blast_dcache_range(addr, addr + size);
     }
}

static void la32_dma_cache_inv(unsigned long addr, unsigned long size)
{
    /* Catch bad driver code */
    BUG_ON(size == 0);

    if (size >= dcache_size) {
        local_flush_dcache_all();
    } else {
        unsigned long lsize = cpu_dcache_line_size();
        unsigned long almask = ~(lsize - 1);

        cache_op(Hit_Writeback_Inv_D, addr & almask);
        cache_op(Hit_Writeback_Inv_D, (addr + size - 1)  & almask);
        blast_inv_dcache_range(addr, addr + size);
    }

}

#endif

static void probe_pcache(void)
{
	struct cpuinfo_loongarch *c = &current_cpu_data;

	c->icache.linesz = icache_lsize;
	c->icache.sets = icache_sets;
	c->icache.ways = icache_ways;
	c->icache.waysize = c->icache.sets * c->icache.linesz;
	icache_size = c->icache.waysize * c->icache.ways;

    c->dcache.linesz = dcache_lsize;
	c->dcache.sets = dcache_sets;
	c->dcache.ways = dcache_ways;
	c->dcache.waysize = c->dcache.sets * c->dcache.linesz;
	dcache_size = c->dcache.waysize * c->dcache.ways;

	c->options |= LOONGARCH_CPU_PREFETCH;

	pr_info("Primary instruction cache %ldkB, %s, %s, linesize %d bytes.\n",
		icache_size >> 10, way_string[c->icache.ways], "VIPT", c->icache.linesz);

	pr_info("Primary data cache %ldkB, %s, %s, %s, linesize %d bytes\n",
		dcache_size >> 10, way_string[c->dcache.ways], "VIPT", "no aliases", c->dcache.linesz);
#ifdef CONFIG_32BIT
    _dma_cache_wback_inv    = la32_dma_cache_wback_inv;
    _dma_cache_wback    = la32_dma_cache_wback_inv;
    _dma_cache_inv      = la32_dma_cache_inv;
#endif

}

void cpu_cache_init(void)
{
	probe_pcache();
	shm_align_mask = PAGE_SIZE - 1;
}
