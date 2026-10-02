#!/usr/bin/env python3
"""Add the keys a shipped JSON file has and a deployed copy lacks.

Usage: add_missing_keys.py SHIPPED DEPLOYED [--dry-run] [--set-if KEY OLD NEW]

Never overwrites a value: for every key already in DEPLOYED (at any depth) the
deployed value wins. Objects are merged recursively; a key whose value is an
object in both files gets its missing children added. Lists and scalars are
never touched. Existing key order is kept; new keys are appended to the object
they belong to.

--set-if KEY OLD NEW (top level only, repeatable): if DEPLOYED[KEY] equals OLD
exactly, set it to NEW. Anything else the user typed is kept. It exists for one
reason: a default the user asked to change (easyHoursCaged 12 -> 6).

Prints one line per change ("+ path" added, "~ path old -> new" changed) and a
summary. Writes nothing with --dry-run, or when there is nothing to change.
The deployed file is re-serialised with 2-space indent when it changes, so hand
formatting (blank lines) is normalised on that write; a backup is the caller's job.
Exit codes: 0 ok, 2 a file does not parse, 3 usage.
"""
import json
import sys


def load(path):
    with open(path, encoding="utf-8-sig") as f:
        return json.load(f)


def add_missing(shipped, deployed, path, added):
    for key, value in shipped.items():
        here = path + [key]
        if key not in deployed:
            deployed[key] = value
            added.append(".".join(here))
        elif isinstance(value, dict) and isinstance(deployed[key], dict):
            add_missing(value, deployed[key], here, added)


def main(argv):
    args = argv[1:]
    dry = "--dry-run" in args
    args = [a for a in args if a != "--dry-run"]
    set_if = []
    while "--set-if" in args:
        i = args.index("--set-if")
        if i + 3 >= len(args):
            print("usage: --set-if KEY OLD NEW", file=sys.stderr)
            return 3
        set_if.append((args[i + 1], args[i + 2], args[i + 3]))
        del args[i:i + 4]
    if len(args) != 2:
        print(__doc__, file=sys.stderr)
        return 3
    shipped_path, deployed_path = args

    try:
        shipped = load(shipped_path)
    except (OSError, ValueError) as e:
        print("cannot read shipped file %s: %s" % (shipped_path, e), file=sys.stderr)
        return 2
    try:
        deployed = load(deployed_path)
    except FileNotFoundError:
        print("  (deployed file missing: the shipped file would be copied whole)")
        if not dry:
            with open(deployed_path, "w", encoding="utf-8", newline="\n") as f:
                json.dump(shipped, f, indent=2, ensure_ascii=False)
                f.write("\n")
        return 0
    except (OSError, ValueError) as e:
        print("cannot parse deployed file %s: %s" % (deployed_path, e), file=sys.stderr)
        return 2

    added = []
    add_missing(shipped, deployed, [], added)
    for a in added:
        print("  + " + a)

    changed = []
    for key, old, new in set_if:
        cur = deployed.get(key)
        is_old = (not isinstance(cur, bool)) and isinstance(cur, (int, float)) and cur == float(old)
        if key not in deployed:
            print("  = %s: absent, takes the shipped value (no --set-if needed)" % key)
        elif is_old:
            deployed[key] = json.loads(new)
            changed.append(key)
            print("  ~ %s: %s -> %s (still the old default)" % (key, cur, new))
        else:
            print("  = %s: %r kept (not the old default %s)" % (key, cur, old))

    if not added and not changed:
        print("  nothing to change")
        return 0
    print("  %d key(s) added, %d value(s) changed" % (len(added), len(changed)))
    if not dry:
        tmp = deployed_path + ".tmp"
        with open(tmp, "w", encoding="utf-8", newline="\n") as f:
            json.dump(deployed, f, indent=2, ensure_ascii=False)
            f.write("\n")
        import os
        os.replace(tmp, deployed_path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
