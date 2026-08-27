#include "nba_shot_launch.h"
#include "nba_gameplay_ball.h"
#include <string.h>

static uint16_t u16(const uint8_t *p) {
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static uint32_t u32(const uint8_t *p) {
    return u16(p) | (uint32_t)u16(p+2) << 16;
}
static const uint16_t table_ranges[5][2] = {
    {0x9EB2,38},{0x9F32,18},{0xA17D,64},{0xA344,144},{0xA4AB,192}
};
static const uint8_t *shot_tables(const NbaAssetPack *assets) {
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_GAMEPLAY_SHOT_TABLES);
    if(!item || !item->data || item->size!=528u) return NULL;
    const uint8_t *data=item->data;
    if(memcmp(data,"NBSHOT1",8) || u32(data+8)!=5) return NULL;
    uint32_t offset=72;
    for(unsigned i=0;i<5;++i) {
        const uint8_t *d=data+12+i*12;
        if(u32(d)!=table_ranges[i][0] || u32(d+4)!=table_ranges[i][1] ||
           u32(d+8)!=offset) return NULL;
        offset+=table_ranges[i][1];
    }
    return data;
}
static bool table_word(const uint8_t *data,uint16_t address,uint16_t *out) {
    for(unsigned i=0;i<5;++i) {
        if(address>=table_ranges[i][0] &&
           (unsigned)address+1u < (unsigned)table_ranges[i][0]+table_ranges[i][1]) {
            *out=u16(data+u32(data+20+i*12)+address-table_ranges[i][0]);
            return true;
        }
    }
    return false;
}
static bool negative_difference(uint16_t a,uint16_t b) {
    return (int16_t)(uint16_t)(a-b)<0;
}
static uint16_t nonnegative(uint16_t value) {
    return (int16_t)value<0 ? 0 : value;
}

/* $86:9ED8-$A10E. Keep wrapped N-flag comparisons and A02A's doubled
 * word index. STZ at 9FC1 does not alter Z: its following BNE is not taken
 * when CMP team_group was equal, so the hot-team case still reaches 9FC5. */
static bool shot_quality(const uint8_t *tables,const NbaShotLaunchInput *in,
                          uint16_t value,int16_t velocity_z,uint16_t *out) {
    uint16_t rating=value==3 ? in->rating_three : in->rating_two;
    bool beyond=in->distance_8c>=in->range_49;
    uint16_t chance=beyond ? (rating>=217 ? 220 : rating>=192 ? 160 : rating>=168 ? 130 : 110)
                            : (rating>=217 ? 230 : rating>=192 ? 192 : rating>=168 ? 153 : 115);
    uint16_t adjust=0;
    if(!table_word(tables,(uint16_t)((in->controller<0 ? 0x9F3E :
            beyond ? 0x9F32 : 0x9F38)+in->difficulty*2u),&adjust)) return false;
    chance=(uint16_t)(chance+(in->controller<0 ? -adjust : adjust));
    uint16_t cursor=0xA4F9,limit;
    do {
        if(!table_word(tables,cursor,&limit)) return false;
        if(negative_difference(in->distance_8c,limit)) break;
        cursor+=4;
    } while(true);
    if(!table_word(tables,cursor+2,&adjust)) return false;
    chance=nonnegative((uint16_t)(chance+adjust));
    if(!in->shot_assistance_17bf && negative_difference(in->modifier_b2,3) && in->controller>=0) {
        uint16_t speed=velocity_z<0 ? (uint16_t)(0u-(uint16_t)velocity_z) : (uint16_t)velocity_z;
        uint16_t tier=(uint16_t)(((uint16_t)(rating-128u)>>5)+1u);
        tier=tier>=in->difficulty ? (uint16_t)(tier-in->difficulty) : 0;
        cursor=(uint16_t)(0xA535+tier*2u);
        uint16_t index=0;
        for(;;cursor+=2,index+=2) {
            if(!table_word(tables,cursor,&limit)) return false;
            if(speed==limit || negative_difference(speed,limit)) break;
        }
        if(!negative_difference(index,11)) index=10;
        if(!table_word(tables,(uint16_t)(0xA555+index*2u),&adjust)) return false;
        chance=nonnegative((uint16_t)(chance+adjust));
    }
    if(!negative_difference(in->modifier_b2,3)) chance=(uint16_t)(chance+(chance>>2));
    uint16_t fatigue=(uint16_t)(((uint16_t)(0x7FFFu-in->stamina_18)>>8)>>1);
    chance=nonnegative((uint16_t)(chance-fatigue));
    if(negative_difference(in->defense_8a,32)) {
        uint16_t penalty=(uint16_t)((uint16_t)(31u-in->defense_8a)*5u);
        if(in->controller>=0) {
            if(!in->difficulty) penalty=0;
            else if(in->difficulty==1) penalty>>=1;
        }
        chance=nonnegative((uint16_t)(chance-penalty));
    }
    if(in->movement_4c>=0x100) chance=(uint16_t)(chance-(chance>>3));
    if((int16_t)in->hot_team_09c0>=0)
        chance=in->hot_team_09c0==in->team_group ? (uint16_t)(chance*2u) : chance>>2;
    if(negative_difference(chance,5)) chance=5;
    else if(!negative_difference(chance,255)) chance=255;
    if(in->clock_0928<300 && (in->period_0926==1 || in->period_0926>=3) &&
       negative_difference(chance,25)) chance=25;
    *out=chance;
    return true;
}

/* $86:A1BD-$A294. Preserve all 16 fractional bits until the ROM's signed
 * eight-bit shift; signed division truncates toward zero, not floor. */
static bool launch_velocity(const uint8_t *tables,uint32_t dx,uint32_t dy,
                             NbaShotLaunchState *s) {
    uint16_t distance=nba_gameplay_hoop_distance((int16_t)(dx>>16),(int16_t)(dy>>16));
    uint16_t threshold,duration,cursor=0xA4AB;
    for(;;cursor+=6) {
        if(cursor>=0xA4F9 || !table_word(tables,cursor,&threshold)) return false;
        if(distance<threshold) break;
    }
    if(!table_word(tables,cursor+2,&duration) || !duration) return false;
    int32_t sx=(int32_t)(int16_t)(dx>>16)*256+(uint8_t)(dx>>8);
    int32_t sy=(int32_t)(int16_t)(dy>>16)*256+(uint8_t)(dy>>8);
    uint16_t dz_hi=(uint16_t)(80u-s->z);
    uint16_t dz_lo=(uint16_t)(0u-s->z_fraction);
    int32_t sz=(int32_t)(int16_t)dz_hi*256+(dz_lo>>8);
    s->velocity_x=(int16_t)(sx/duration);
    s->velocity_y=(int16_t)(sy/duration);
    uint32_t sum=(uint16_t)(sz/duration)+12u*duration;
    s->velocity_z=(int16_t)(uint16_t)(sum+24u+(sum>>16));
    return true;
}

static uint32_t fixed(uint16_t fraction,uint16_t integer) {
    return (uint32_t)integer<<16 | fraction;
}

/* Complete shared launch, excluding inline data: $86:9D6E-$9EB1,
 * $86:9ED8-$9F31, $86:9F44-$A17C, $86:A1BD-$A343, $86:A3D4-$A476.
 * Entries $86:9D6E and $86:9DA6 differ ONLY in the ordinary
 * facing snap/upper-pose installation. No host animation frame or guessed
 * make probability is used. Scratch registers/stack preservation are not
 * gameplay state; the complete persistent outputs are represented below. */
bool nba_shot_launch(const NbaAssetPack *assets,const NbaShotLaunchInput *in,
                      NbaShotLaunchState *state) {
    const uint8_t *tables=shot_tables(assets);
    if(!tables || !in || !state || in->difficulty>2) return false;
    NbaShotLaunchState s=*state;
    if(!in->special_entry) {
        s.facing=nba_shot_action_release_facing(in->actor_x,in->actor_y,in->basket_x);
        uint16_t request=0x17;
        if(!nba_player_animation_command(assets,&s.actor.animation,
                NBA_ANIMATION_INSTALL_UPPER,&request,in->boosted,in->alternate_lower)) return false;
    }
    s.roster_low=in->roster_low; s.roster_bank=in->roster_bank;
    s.last_owner=s.display_shooter=s.owner;
    s.attempt_latch=1; s.owner=s.dead_0966=0xFFFF;
    s.dead_096c=s.height_0968=s.bounce_0920=s.inner_veto=0;
    s.actor.bounce_count=0; /* same $0920 in the action wrapper */
    s.live_state=1; s.timeout_0930=0x708;
    s.value=s.display_value=s.initial_value=in->free_throw_0978 ? 1u : 2u;
    if(!negative_difference((uint16_t)(in->assist_clock_47-in->clock_0928),120))
        s.assist_43=s.assist_45=0xFFFF;
    s.contact_inhibit=20; s.ball_record=0x3EEB;
    uint32_t dx=fixed(in->basket_fraction,(uint16_t)in->basket_x)-fixed(s.x_fraction,s.x);
    uint32_t dy=0u-fixed(s.y_fraction,s.y);
    /* $86:A561-$A5AF runs even during a free throw; only writes beyond arc. */
    if(nba_gameplay_shot_value(false,in->origin_x,in->origin_y,in->basket_x>=0)==3)
        s.value=s.display_value=3;
    s.chance=0; s.miss_index=0xFFFF;
    if(in->controller>=0 && in->shot_control_17c3) {
        /* Manual shot control bypasses both quality and first FT aim. */
    } else if(in->free_throw_0978) {
        s.x_fraction=s.y_fraction=s.z_fraction=0;
        uint16_t aim=in->aim_0982;
        if(aim>=56) aim=(uint16_t)(111u-aim);
        aim=(uint16_t)(aim-27u);
        if(!negative_difference(aim,9)) aim=9;
        else if(negative_difference(aim,(uint16_t)-9)) aim=(uint16_t)-9;
        uint16_t offset;
        if(!table_word(tables,(uint16_t)(0x9EB2+(uint16_t)(aim+9u)*2u),&offset)) return false;
        dy+=(uint32_t)offset<<16;
    } else {
        if(!shot_quality(tables,in,s.value,s.actor.velocity_z,&s.chance)) return false;
        if((nba_gameplay_rng_next(&s.rng)&255u)>=s.chance) {
            s.inner_veto=1;
            s.miss_index=nba_gameplay_rng_next(&s.rng)&15u;
            uint16_t mx,my;
            if(!table_word(tables,(uint16_t)(0xA17D+s.miss_index*4u),&mx) ||
               !table_word(tables,(uint16_t)(0xA17F+s.miss_index*4u),&my)) return false;
            if(in->basket_x>=0) { mx=(uint16_t)(0u-mx); my=(uint16_t)(0u-my); }
            dx+=(uint32_t)mx<<16;
            dy+=(uint32_t)my<<16;
        }
    }
    if(!launch_velocity(tables,dx,dy,&s)) return false;
    if(in->free_throw_0978) {
        if(in->controller<0) {
            uint16_t rating=in->rating_free>=128 ? in->rating_free-128u : 0;
            uint16_t chance;
            if(!table_word(tables,(uint16_t)(0xA344+(rating>>4)*2u),&chance)) return false;
            bool miss=(nba_gameplay_rng_next(&s.rng)&255u)>=chance;
            uint16_t variant=nba_gameplay_rng_next(&s.rng)&3u;
            if(in->alternate_lower) variant+=4;
            uint16_t address=(uint16_t)((miss ? 0xA394 : 0xA354)+variant*8u);
            uint16_t vx,vy,vz;
            if(!table_word(tables,address,&vx) || !table_word(tables,address+2,&vy) ||
               !table_word(tables,address+4,&vz)) return false;
            s.velocity_x=(int16_t)(in->actor_x<0 ? (uint16_t)(0u-vx) : vx);
            s.velocity_y=(int16_t)vy; s.velocity_z=(int16_t)vz;
        } else {
            uint16_t power=in->power_0980;
            if(power>=56) power=(uint16_t)(111u-power);
            uint16_t offset=(uint16_t)(50u-power*2u);
            uint16_t vz=(uint16_t)(offset+(uint16_t)s.velocity_z);
            if(negative_difference(vz,(uint16_t)-46)) vz=(uint16_t)-46;
            s.velocity_z=(int16_t)vz;
            if((int16_t)offset>=0) {
                uint16_t horizontal=(uint16_t)(offset*8u);
                if((int16_t)s.x<0) horizontal=(uint16_t)(0u-horizontal);
                s.velocity_x=(int16_t)(uint16_t)((uint16_t)s.velocity_x+horizontal);
            }
            bool aim_good=in->aim_0982>=15 && in->aim_0982<96 &&
                          (in->aim_0982<40 || in->aim_0982>=70);
            bool power_good=in->power_0980>=15 && in->power_0980<96 &&
                            (in->power_0980<40 || in->power_0980>=70);
            if(!aim_good || !power_good) s.inner_veto=1;
        }
    }
    /* $86:9CDB-$9D06, $86:9D0C-$9D17, $86:9D20-$9D4D,
     * $86:9D53-$9D5E, $86:9D67-$9D6D: A=0 attempt paths only.
     * Made-shot statistics are separate branches, not launch side effects. */
    unsigned index=s.value==1 ? 4u : 0u;
    ++s.player_stats[index];
    if(s.value!=1 && s.value!=2) ++s.player_stats[2];
    if(in->controller>=0) {
        ++s.controller_stats[index];
        if(s.value!=1 && s.value!=2) ++s.controller_stats[2];
    }
    s.actor.mode=11; /* caller, not launch, owns timer/flags cleanup */
    *state=s;
    return true;
}
