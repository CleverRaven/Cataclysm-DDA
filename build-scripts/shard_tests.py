import argparse
import os
import subprocess
import sys


def get_tests(test_bin, cata_test_opts):
    if not os.path.exists(test_bin) and os.path.exists(test_bin + ".exe"):
        test_bin += ".exe"

    cmd = [test_bin] + cata_test_opts + ["--list-test-names-only"]
    try:
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        )
        if not result.stdout.strip():
            print(
                f"Empty stdout from {test_bin}. "
                f"Return code: {result.returncode}",
                file=sys.stderr
            )
        tests = [t.strip() for t in result.stdout.splitlines() if t.strip()]
        return tests
    except Exception as e:
        print(f"Error running {test_bin}: {e}", file=sys.stderr)
        return []


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", required=True)
    parser.add_argument("--opts", default="", help="Space-separated options")
    parser.add_argument("--shards", type=int, required=True)
    args = parser.parse_args()

    if args.shards < 1:
        parser.error("--shards must be at least 1")

    opts_list = args.opts.split() if args.opts else []
    tests = get_tests(args.bin, opts_list)
    if not tests:
        sys.exit(1)

    known_slow = {
        "overmap_terrain_coverage": 290,
        "effective_vs_actual_damage_per_second": 240,
        "accuracy_increases_success": 238,
        "starting_items": 167,
        "overmap_generation_is_deterministic": 33,
        "Proportional_armor_material_resistances": 32,
        "default_overmap_generation_always_succeeds": 24,
        "monsters_appear_on_map_as_expected": 22,
        "Melee_coverage_vs_melee_damage": 22,
        "Ranged_coverage_vs_bullet": 21,
    }

    shard_weights = [0.0] * args.shards
    shard_lists = [[] for _ in range(args.shards)]

    test_durations = []
    for t in tests:
        if t in known_slow:
            test_durations.append((t, known_slow[t]))
        else:
            test_durations.append((t, 0.5))  # Average fast test

    test_durations.sort(key=lambda x: x[1], reverse=True)

    for t, dur in test_durations:
        min_idx = shard_weights.index(min(shard_weights))
        shard_weights[min_idx] += dur
        shard_lists[min_idx].append(t)

    for i in range(args.shards):
        filename = f"shard_{i}.txt"
        with open(filename, "w") as f:
            for t in shard_lists[i]:
                f.write(t + "\n")
        print(filename)


if __name__ == "__main__":
    main()
