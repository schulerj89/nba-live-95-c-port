#include "nba_period_entry_prefix.h"

void nba_period_entry_prefix_reset(NbaPeriodEntryPrefixState *s) {
    s->bounce_091c=0;s->activity_0948=0;s->release_094a=0;s->field_0962=0;
    s->event_0964=0;s->field_0966=0;s->field_096a=0;s->contact_09bc=0;
    s->field_097c=0;s->shot_value_094c=0;s->field_09d0=0;s->field_0a02=0;s->field_0a04=0;
    s->object_4015=0x0822;s->object_401b=0xffff;s->field_1864=0;
    /* DCE2-DD13 touches only these twelve words in each100-byte actor.
     * In particular, queue cursors becomeFFFF; queue contents are not erased. */
    for(unsigned i=0;i<10;++i) {
        NbaPeriodEntryPrefixActor *a=&s->actors[i];
        a->upper_30=0;a->lower_32=0;a->base_38=0;
        a->upper_accumulator_42=0;a->lower_accumulator_44=0;
        a->upper_phase_3a=0;a->lower_phase_3c=0;a->upper_lock_46=0;a->lower_lock_48=0;
        a->flags_28=0;a->upper_queue_18=0xffff;a->lower_queue_1a=0xffff;
    }
    s->busy_09b4=0;s->event_0964=0;s->contact_09bc=0;s->whistle_09b6=0;
    s->context_4713=0;s->context_4793=0;s->context_4741=0;s->context_47c1=0;
}

bool nba_period_entry_prefix_clock(const NbaPeriodEntryPrefixTables *t,NbaPeriodEntryPrefixState *s) {
    if(!s)return false;
    /* DD30/DD33 is unsigned carry: all period words>=4 read the OT table.
     * DD35-DD3E updates0A0C itself, then DD41-DD44 copies it to0928. */
    if(s->period_0926>=4) {
        if(!t||s->quarter_option_17b1>=4)return false;
        s->selected_clock_0a0c=t->overtime_clock[s->quarter_option_17b1];
    }
    s->clock_0928=s->selected_clock_0a0c;return true;
}

void nba_period_entry_prefix_table(NbaPeriodEntryPrefixState *s) {
    s->shot_clock_092c=0x05a0;s->field_0994=0x05a0;s->field_0996=1;
    if(s->period_0926==2) {
        /* DD64/DD67 and DD71/DD74 are EORFFFF+INC, not a host coordinate
         * normalization: zero and8000 retain their wrapped negation result. */
        s->anchor_46f5=(uint16_t)(0u-s->anchor_46f5);
        s->anchor_4775=(uint16_t)(0u-s->anchor_4775);
    }
    s->scratch_b6=s->anchor_46f5;s->owner_093e=0xffff;s->assistance_09c0=0xffff;
    s->cursor_9a=0x34d3;s->list_head_34d1=0;
    /* Original continuing-period87:9797->8C86->8CA6 calls DCA6, bypassing
     * new-game DA18/DA3F's clear. Keep09BA and09B0/09B2 exactly as carried. */
}

bool nba_period_entry_prefix(const NbaPeriodEntryPrefixTables *t,NbaPeriodEntryPrefixState *s) {
    if(!s||(s->period_0926>=4&&(!t||s->quarter_option_17b1>=4)))return false;
    nba_period_entry_prefix_reset(s);
    (void)nba_period_entry_prefix_clock(t,s);
    nba_period_entry_prefix_table(s);return true;
}
