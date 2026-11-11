# tundra-libblueice

Library of tundra-blueice, developed for Tundra OS, works on any Linux. It builds
and parses the alert line that every Tundra tool reports to the guardian:

```
alert 111/<level> <action> by <package> on <object> ts <epoch.ms> reply <options> info case <case> [<key> <value>]... -- <text>
```

Pure functions over buffers of the caller: no allocation, no input or output,
no global state. Writing the line to `stderr` is up to each program.

## Usage

```c
#include <tundra/blueice.h>

struct blueice_alert a;
char line[BLUEICE_LINE_MAX];

blueice_alert_init(&a, "tundra-blueice-arp", "D", 8, "gateway impersonation");
blueice_alert_add(&a, "gw", "10.77.0.1");
if (blueice_alert_format(line, sizeof(line), &a, BLUEICE_STOP, "enp8s0",
                         now_ms) == 0)
	fprintf(stderr, "%s\n", line);
```

Link with `-ltundra-blueice`.

| Function | What it does |
|----------|--------------|
| `blueice_alert_init` | Starts an alert: emitter, case, level (1-10) and text |
| `blueice_alert_add` | Appends `<key> <value>` to `info`, encoding the value |
| `blueice_alert_format` | Writes the line, without the newline |
| `blueice_alert_parse` | Reads level, action, `by`, `on`, `reply` and `case` of a line |
| `blueice_encode` | Writes a value as one token |
| `blueice_action_name`, `blueice_reply` | Name of an action and the replies it accepts |
| `blueice_version` | Version of the library |

Every function that can fail returns `0` or `-1`.

## Rules of the format

- **Values from outside are encoded**: every byte outside `[A-Za-z0-9._:@,/-]`
  becomes `%XX`, so a value can never add a field or start another line. An
  empty value is refused.
- **Emitter, case and keys** are written as they are and must already be tokens
  of that set, of at most 64 bytes.
- **The text** after `--` is fixed by the program, without control bytes, and
  nobody parses it.
- **A failed step spoils the alert**: a value that does not fit or cannot be
  encoded makes `blueice_alert_format` refuse it, instead of reporting half of
  it.
- **Replies**: `force,disconnect` for `stop`; `ack,disconnect` for the rest.
- **Parsing** ignores unknown keys and refuses a repeated or missing `by`, `on`,
  `reply` or `case`, a level outside 1-10 and a token over 64 bytes.

## Build and install

```
make
make check
make install
```

Installs `libtundra-blueice.so.0`, the `libtundra-blueice.so` link and
`<tundra/blueice.h>`. `make check` runs the unit tests with ASan and UBSan, a
program linked to the shared library and the list of exported symbols
(`tests/abi/symbols`).

The soname changes when a released function or struct does.

## License

GPL-3.0-or-later.
