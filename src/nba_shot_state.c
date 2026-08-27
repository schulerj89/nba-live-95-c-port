#include "nba_shot_state.h"
#include <string.h>

static uint16_t word(const uint8_t *p) {return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static const uint8_t *tables(const NbaAssetPack *assets) {
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_FATIGUE_TABLES);
    if(!item || !item->data || item->size!=88 || memcmp(item->data,"NBFAT1\0",8))return NULL;
    return item->data;
}

bool nba_shot_state_assets_valid(const NbaAssetPack *assets) {return tables(assets)!=NULL;}

/* $85:A081-$A0B7: a made attempt increments the shooter; opposing actors
 * lose BOTH run counters. Actor +6E, not array-side inference, selects them.
 * $85:A0B8-$A0EA: CPU Assistance uses PRE-basket scores and remaining clock.
 * Preserve the asymmetric original boundary: left trails by >=3, right >=2.
 * This is trailing-team assistance, not a generic hot-player mechanic. */
bool nba_shot_momentum_make(NbaShotMomentum *s,uint16_t shooter,
    uint16_t enabled,uint16_t clock,uint16_t left,uint16_t right) {
    if(!s || shooter>=10)return false;
    ++s->made_run[shooter];
    unsigned first=s->team_group[shooter]==0 ? 5u : 0u;
    for(unsigned i=first;i<first+5;++i)s->made_run[i]=s->defensive_run[i]=0;
    s->assistance_team=0xFFFF;
    if(enabled && clock<0x1C20 && left!=right) {
        uint16_t difference=(uint16_t)(left-right);
        if(left<right) {if(difference<0xFFFE)s->assistance_team=0;}
        else if(difference>=2)s->assistance_team=5;
    }
    return true;
}

/* $86:DD80-$DD88: period setup also releases ownership. */
void nba_shot_momentum_reset(NbaShotMomentum *s) {s->owner=s->assistance_team=0xFFFF;}

/* $86:DA49-$DA60 initializes every roster statistics record, not ten actors. */
void nba_shot_stamina_init(NbaShotFatigue *s) {
    for(unsigned i=0;i<24;++i)s->stamina[i]=0x7FFF;
}

/* $87:8DF3-$8DF8 separately primes the first fatigue update. */
void nba_shot_fatigue_timer_init(NbaShotFatigue *s) {s->timer=1000;}

/* $87:985D-$987D: 16-bit sum then unsigned clamp; preserve wrap behavior. */
void nba_shot_stamina_grant(NbaShotFatigue *s,uint16_t amount) {
    for(unsigned i=0;i<24;++i) {
        uint16_t sum=(uint16_t)(s->stamina[i]+amount);
        s->stamina[i]=sum>=0x7FFF ? 0x7FFF : sum;
    }
}

/* $86:8468-$8495: earlier shift/min calculation is overwritten by the
 * unconditional LDA #1000 at 847A. The actual grant is always 4096. */
void nba_shot_stamina_fixed_grant(NbaShotFatigue *s) {nba_shot_stamina_grant(s,0x1000);}

/* $87:996A-$99C2: recovery applies to all 24 records even with fatigue OFF.
 * The final clamp tests N, unlike the unsigned timeout-grant clamp above. */
bool nba_shot_stamina_recover(const NbaAssetPack *assets,NbaShotFatigue *s) {
    const uint8_t *data=tables(assets);
    if(!s || !data || s->quarter>=4)return false;
    for(unsigned i=0;i<24;++i)if(s->rating[i]<3 || s->rating[i]>10)return false;
    for(unsigned i=0;i<24;++i) {
        unsigned offset=24+s->quarter*16+(s->rating[i]-3)*2;
        uint16_t amount=(uint16_t)(word(data+offset)*2u);
        uint16_t sum=(uint16_t)(s->stamina[i]+amount);
        s->stamina[i]=(int16_t)sum<0 ? 0x7FFF : sum;
    }
    return true;
}

/* $87:98EA-$9969, reached via $87:8EF3-$8EF6 before actor updates. */
bool nba_shot_fatigue_step(const NbaAssetPack *assets,NbaShotFatigue *s) {
    if(!s)return false;
    if((int16_t)(uint16_t)(s->live_state-0x80u)>=0 || s->timer<60)return true;
    const uint8_t *data=tables(assets);
    if(!data || s->quarter>=4)return false;
    NbaShotFatigue next=*s;
    next.timer=0;
    for(unsigned i=0;i<10;++i) {
        unsigned slot=next.active_roster[i];
        if(slot>=24)return false;
        if(next.enabled) {
            uint16_t drain=word(data+8+next.quarter*2+(next.boost[i] ? 8 : 0));
            next.stamina[slot]=next.stamina[slot]>=drain ? (uint16_t)(next.stamina[slot]-drain) : 0;
        }
        ++next.playing_seconds[slot];
    }
    if(!nba_shot_stamina_recover(assets,&next))return false;
    *s=next;return true;
}

/* $85:EDC6-$EE3D: independent 60-Hz clock/fatigue writer. Clock-stop and
 * post-score run-clock branches are distinct; neither uses actor cadence. */
void nba_shot_clock_step(NbaShotClock *s) {
    bool dead=s->live_state==0x82;
    if(dead) {
        if(!s->dead_clock_enabled)return;
        uint16_t threshold=(int16_t)(uint16_t)(s->period-3u)<0 ? 0xE10u : 0x1C20u;
        if(s->clock<threshold)return;
    } else {
        if((int16_t)(uint16_t)(s->live_state-0x80u)>=0)return;
        s->dead_clock_enabled=0;
        if(s->shot_clock==0x5A0)s->shot_clock_mirror=0x5A0;
    }
    if(!s->clock)return;
    if((int16_t)(uint16_t)(s->fatigue_timer-60u)<0)++s->fatigue_timer;
    --s->clock;++s->elapsed_clock;
    if(dead)return;
    --s->flight_timer;
    if(s->shot_clock_enabled){--s->shot_clock;++s->elapsed_shot_clock;}
}
