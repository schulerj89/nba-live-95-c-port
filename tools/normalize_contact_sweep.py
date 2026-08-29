import argparse
import json
from pathlib import Path

import verify_ball_contact_sweep_vectors as ball
import verify_player_contact_sweep_vectors as player


def eligible_player(vector):
    before, after = player.memory(vector['entry']), player.memory(vector['exit'])
    return (not player.native_ball_or_event_changed(before, after) and
            not player.native_nested_animation_only(before, after) and
            not player.native_nested_rng_only(before, after))


def eligible_ball(vector):
    before, after = player.memory(vector['entry']), player.memory(vector['exit'])
    return (player.native_ball_or_event_changed(before, after) and
            player.word(before, 0x0936) != 0x81 and
            any(a != b for index, (a, b) in enumerate(zip(
                ball.row(before), ball.row(after))) if index not in (0, 4)))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--kind', choices=('player', 'ball'), required=True)
    parser.add_argument('--vectors', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    raw = [json.loads(line) for line in Path(args.vectors).open() if line.strip()]
    if args.kind == 'player':
        eligible = [vector for vector in raw if eligible_player(vector)]
        changed = [vector for vector in eligible if
                   player.row(player.memory(vector['entry'])) !=
                   player.row(player.memory(vector['exit']))]
        unchanged = [vector for vector in eligible if vector not in changed]
        step = max(1, len(unchanged) // 20)
        selected = changed + unchanged[::step][:20]
        calls = []
        for vector in selected:
            before = player.memory(vector['entry'])
            after = player.memory(vector['exit'])
            player.normalize_nested_receiver_animation(before, after)
            calls.append({'input': before.hex(), 'expected': player.row(after),
                          'changed': vector in changed})
    else:
        selected = [vector for vector in raw if eligible_ball(vector)]
        calls = []
        for vector in selected:
            before, after = player.memory(vector['entry']), player.memory(vector['exit'])
            calls.append({'input': before.hex(), 'expected': ball.row(after),
                          'acquisition': ball.row(before)[2] != ball.row(after)[2]})
    Path(args.output).write_text(json.dumps({'calls': calls}, separators=(',', ':')))
    print(f'[CONTACT NORMALIZE] kind={args.kind} calls={len(calls)}')


if __name__ == '__main__':
    main()
