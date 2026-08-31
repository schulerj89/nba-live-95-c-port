#include "nba_bootstrap_oam_prefix.h"

#include <string.h>

/* Keep the accepted prefix immutable. Including its implementation gives this
 * child access to the same private clock/bus owner without copying state at
 * the boundary. Build this file instead of separately compiling nmi.c. */
#include "nba_bootstrap_nmi.c"

typedef struct {
    uint32_t pc, address;
    uint8_t expected, kind;
    bool last, source_read;
} OamCycle;

static const OamCycle oam_cycles[] = {
    {0x808184,0x808184,0x8f,NBA_CODEC_READ,false,false},
    {0x808184,0x808185,0x03,NBA_CODEC_READ,false,false},
    {0x808184,0x808186,0x21,NBA_CODEC_READ,false,false},
    {0x808184,0x808187,0x00,NBA_CODEC_READ,false,false},
    {0x808184,0x002103,0x00,NBA_CODEC_WRITE,true,false},
    {0x808188,0x808188,0xaf,NBA_CODEC_READ,false,false},
    {0x808188,0x808189,0xfe,NBA_CODEC_READ,false,false},
    {0x808188,0x80818a,0x08,NBA_CODEC_READ,false,false},
    {0x808188,0x80818b,0x00,NBA_CODEC_READ,false,false},
    {0x808188,0x0008fe,0x00,NBA_CODEC_READ,true,true},
    {0x80818c,0x80818c,0xd0,NBA_CODEC_READ,false,false},
    {0x80818c,0x80818d,0x1a,NBA_CODEC_READ,true,false},
};

static bool resume_prefix(NbaBootstrapOamPrefix *owner)
{
    NbaBootstrap *s=&owner->prefix.machine.core;
    NbaCodecWorkEntry *r=&s->cpu.work.registers;
    if(s->status!=NBA_BOOT_CPU_SOURCE||s->boundary_pc!=0x808184u||
       s->cpu.boundary_pc!=0x808184u||s->cpu.program_bank!=0x80u||
       s->cpu.emulation||(r->status&0x30u)!=0x20u)
        return false;
    /* The previous generated runner has no pending bus cycle at its explicit
     * source boundary. Keep every carried register and hardware byte. */
    if(s->cpu.work.pending_index<s->cpu.work.pending_count)return false;
    owner->continuation_active=true;owner->source_stage=0;owner->bus_index=0;
    s->status=NBA_BOOT_RUNNING;s->boundary_pc=0;
    return true;
}

static void complete_instruction(NbaBootstrapOamPrefix *owner,uint32_t pc)
{
    NbaBootstrap *s=&owner->prefix.machine.core;
    NbaCodecWorkEntry *r=&s->cpu.work.registers;
    s->cpu.work.instructions++;
    if(pc==0x808188u){
        r->value=(uint16_t)((r->value&0xff00u)|owner->loaded_value);
        r->status=(uint8_t)((r->status&0x7du)|
            (owner->loaded_value==0?2u:0u)|(owner->loaded_value&0x80u));
    }
    owner->source_stage++;
    if(owner->source_stage==3){
        s->status=NBA_BOOT_CPU_SOURCE;s->boundary_pc=0x80818eu;
        s->cpu.boundary_pc=0x80818eu;owner->continuation_active=false;
    }
}

static bool oam_write(NbaBootstrapOamPrefix *owner,uint8_t value)
{
    NbaBootstrapFill *fill=&owner->prefix.machine;
    NbaBootstrap *s=&fill->core;
    if(!fill_cpu_write(fill,0x2103u,value))return false;
    /* PPU Write2103 reloads the internal byte address from2102/2103 and
     * publishes bit7 as priority rotation. The write supersedes unrepresented
     * earlier OAM increment history, so no captured pre-address is needed. */
    owner->oam_ram_address=(uint16_t)(s->io[0x102]|((uint16_t)(value&1u)<<8));
    owner->oam_internal_address=(uint16_t)(owner->oam_ram_address<<1);
    owner->oam_priority_rotation=(value&0x80u)!=0;
    return true;
}

static bool continuation_step(NbaBootstrapOamPrefix *owner,
                              NbaBootstrapObserver observer,void *context)
{
    NbaBootstrap *s=&owner->prefix.machine.core;
    NbaCodecWorkEntry *r=&s->cpu.work.registers;
    const OamCycle *c;uint8_t value;unsigned duration;uint64_t sample;
    if(owner->bus_index>=sizeof(oam_cycles)/sizeof(oam_cycles[0]))
        return stop(s,NBA_BOOT_BAD_SOURCE,0x80818eu);
    c=&oam_cycles[owner->bus_index];value=c->kind==NBA_CODEC_WRITE?(uint8_t)r->value:0;
    if(owner->bus_index==0||oam_cycles[owner->bus_index-1].last)
        event(s,observer,context,NBA_BOOT_EVENT_CPU_ENTRY,c->pc,0,0,0,false,s->clock.master);
    duration=speed(s,c->address);nmi_cpu_cycle(s,false);
    if(c->kind==NBA_CODEC_READ){
        if(!clocks(s,duration-4u,observer,context)||!cpu_read(s,c->address,&value))return false;
        sample=s->clock.master;if(!clocks(s,4u,observer,context))return false;
        if(c->source_read)owner->loaded_value=value;
        else if(value!=c->expected)return stop(s,NBA_BOOT_BAD_SOURCE,c->pc);
    }else{
        if(!clocks(s,duration,observer,context)||!oam_write(owner,value))return false;
        sample=s->clock.master;
    }
    event(s,observer,context,NBA_BOOT_EVENT_CPU,c->pc,c->address,value,c->kind,c->last,sample);
    s->cpu_cycles++;owner->bus_index++;
    if(c->last)complete_instruction(owner,c->pc);
    return s->status==NBA_BOOT_RUNNING;
}

bool nba_bootstrap_oam_prefix_power_on(NbaBootstrapOamPrefix *owner,
    const uint8_t *rom,size_t size,NbaBootstrapProfile profile)
{
    if(!owner)return false;memset(owner,0,sizeof(*owner));
    return nba_bootstrap_nmi_power_on(&owner->prefix,rom,size,profile);
}

bool nba_bootstrap_oam_prefix_step(NbaBootstrapOamPrefix *owner,
    NbaBootstrapObserver observer,void *context)
{
    NbaBootstrap *s;
    if(!owner)return false;s=&owner->prefix.machine.core;
    if(!owner->continuation_active){
        if(s->status==NBA_BOOT_RUNNING&&nba_bootstrap_nmi_step(&owner->prefix,observer,context))return true;
        if(!resume_prefix(owner))return false;
    }
    return continuation_step(owner,observer,context);
}
