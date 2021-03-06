#include "gfx_1.3.h"

#include "core/plugin.h"
#include "core/rdram.h"

#define SP_INTERRUPT    0x1
#define SI_INTERRUPT    0x2
#define AI_INTERRUPT    0x4
#define VI_INTERRUPT    0x8
#define PI_INTERRUPT    0x10
#define DP_INTERRUPT    0x20

extern GFX_INFO gfx;

static uint32_t rdram_size;
static uint8_t* rdram_hidden_bits;

static bool is_valid_ptr(void *ptr, uint32_t bytes)
{
    SIZE_T dwSize;
    MEMORY_BASIC_INFORMATION meminfo;
    if (!ptr) {
        return false;
    }
    memset(&meminfo, 0x00, sizeof(meminfo));
    dwSize = VirtualQuery(ptr, &meminfo, sizeof(meminfo));
    if (!dwSize) {
        return false;
    }
    if (MEM_COMMIT != meminfo.State) {
        return false;
    }
    if (!(meminfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        return false;
    }
    if (bytes > meminfo.RegionSize) {
        return false;
    }
    if ((uint64_t)((char*)ptr - (char*)meminfo.BaseAddress) > (uint64_t)(meminfo.RegionSize - bytes)) {
        return false;
    }
    return true;
}

void plugin_init(void)
{
    // Zilmar plugins can't know how much RDRAM is allocated, so use a Win32 hack
    // to detect it
    rdram_size = is_valid_ptr(&gfx.RDRAM[0x7f0000], 16) ? 0x800000 : 0x400000;

    // Zilmar plugins also can't access the hidden bits, so allocate it on our own
    rdram_hidden_bits = malloc(rdram_size);
    memset(rdram_hidden_bits, 3, rdram_size);
}

void plugin_sync_dp(void)
{
    *gfx.MI_INTR_REG |= DP_INTERRUPT;
    gfx.CheckInterrupts();
}

uint32_t** plugin_get_dp_registers(void)
{
    // HACK: this only works because the ordering of registers in GFX_INFO is
    // the same as in dp_register
    return (uint32_t**)&gfx.DPC_START_REG;
}

uint32_t** plugin_get_vi_registers(void)
{
    // HACK: this only works because the ordering of registers in GFX_INFO is
    // the same as in vi_register
    return (uint32_t**)&gfx.VI_STATUS_REG;
}

uint8_t* plugin_get_rdram(void)
{
    return gfx.RDRAM;
}

uint8_t* plugin_get_rdram_hidden(void)
{
    return rdram_hidden_bits;
}

uint32_t plugin_get_rdram_size(void)
{
    return rdram_size;
}

uint8_t* plugin_get_dmem(void)
{
    return gfx.DMEM;
}

uint8_t* plugin_get_rom_header(void)
{
    return gfx.HEADER;
}

void plugin_close(void)
{
    if (rdram_hidden_bits) {
        free(rdram_hidden_bits);
        rdram_hidden_bits = NULL;
    }
}
