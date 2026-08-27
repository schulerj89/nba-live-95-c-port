"""Natural CPU shot-selection and persistent made-run integration checks."""
from analyze_shot_selection import reason


def verify(rows):
    serial=0;special=[];made=0
    for i,row in enumerate(rows):
        state=row['shot_selection'];event=state['input']
        if state['serial']!=serial:
            if state['serial']!=serial+1:raise AssertionError('missing selector event')
            serial+=1
            if (reason(event)=='special')!=(event[6]==17):
                raise AssertionError('natural selector input/output mismatch')
            if event[6]==17:special.append((i,event[7]))
        if not i:continue
        prior=rows[i-1];old=prior['shot_selection'];match=row['match']
        scores=[match['score_left_raw'],match['score_right_raw']]
        before=[prior['match']['score_left_raw'],prior['match']['score_right_raw']]
        expected=list(old['made_run'])
        if scores!=before:
            shooter=match['shot_actor_raw_09c8']
            if not 0<=shooter<10:raise AssertionError('made shot missing shooter')
            expected[shooter]=(expected[shooter]+1)&65535
            first=5 if shooter<5 else 0
            expected[first:first+5]=[0]*5;made+=1
        if state['made_run']!=expected:raise AssertionError('made-run update/reset timing')
    if not special:raise AssertionError('unforced CPU run never selected mode 17')
    for index,actor in special:
        start=rows[index];pose=start['actors'][actor]['animation']
        if pose not in (0x14,0x15) or start['ball']['owner']!=actor:
            raise AssertionError('special startup pose/owner')
        release=next((j for j in range(index+1,min(index+120,len(rows)))
                      if rows[j]['ball']['owner']!=actor),None)
        if release is None:raise AssertionError('natural special shot never released')
        before,after=rows[release-1],rows[release]
        if before['actors'][actor]['raw']['animation_rom']['upper_phase_3a']<3 or \
           after['ball']['owner']!=-1 or after['ball']['state']!=5 or \
           after['actors'][actor]['animation']!=pose or \
           after['match']['shot_actor_raw_09c8']!=actor or after['match']['shot_value_raw']!=2:
            raise AssertionError('natural special release contract')
        if not any(r['actors'][actor]['z']>0 for r in rows[index:release]):
            raise AssertionError('natural special jump missing')
        print(f'[NATURAL SPECIAL] actor={actor} selected={start["frame"]} released={after["frame"]} pose={pose:02x}')
    print(f'[SHOT STATE TRACE] {serial} selectors, {made} made-run updates, {len(special)} unforced specials')
