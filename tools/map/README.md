# The map of the original's memory

`data_map.txt` gives each data object the port's headers declare its place in
the original's memory: DOS 1.00's `REVENGE.EXE`, its image loaded at 0800h,
its data segment 3A6A. It holds names, addresses and sizes only. It is the
ground the shared-memory check stands on (one object where the original has
one), and the typed state will build on it.

## Lines

    place NAME SEG:OFF SIZE KIND    NAME is at SEG:OFF, SIZE bytes (hex)
    host NAME SIZE REASON           the port's own: no place of its own in the original
    unplaced NAME SIZE REASON       not placed yet, and why
    overlap NAME NAME [NOTE]        two placed objects share bytes

SIZE is the original's size: a host pointer counts as the far pointer it
stands for (4 bytes), and an import piece has the piece's size.

`check_map.py` (ctest `map.data_map`) fails when a declared object has no
line, or a line names no declared object. It also fails when two placed
objects share a byte with no overlap line naming the pair, or when
`src/SDL/map_dump.c` is out of date.

## How a place was found (KIND)

- `table`: a row of `tests/test_initial_data_image.c`. The source states the
  address, and the test checks the object's start against the image.
- `import`: an import piece's offset (`src/Support/original_import_pieces.txt`).
  A pointed text sits where its pointer points.
- `annotation`: the port's reference annotations name the object with its
  address. Where these and the table both place an object, they agree.
- `run`: the object's bytes, written by the port at every snap of the trace
  scripts, equal the original's memory at that place in the same snaps
  (the REVSNAP captures), and at no other place in the segment. Held out
  against the objects placed otherwise, every unique match was right.
- `procedure`: the original's procedures that use the object. The DS
  operands of each procedure, less those of objects already placed, bound
  where it can start. The object must not overlap placed objects and must
  match the image's bytes if it starts with any. It is placed only when one
  start is left, an instruction reaches that first byte, and the run bytes
  don't contradict it.

The run kind can be repeated: with `REVENGE_MAP_DUMP_DIR` set to a folder, the
harness writes `SNAP.objects` there at each snap, with every object's name,
size and bytes (`src/SDL/map_dump.c`, written by `make_map_dump.py` from the
headers). Unset, it does nothing.

`check_dump.py` (ctest `map.dump_checks`) uses the dump the other way: each
line of `dump_checks.txt` names a script, a snap and a span of the data
segment, and the run must hold there what the image holds at a given place.
It reads the span from the objects the map places on it, so a check stays
the same when the port's objects are merged or renamed.

## Not placed yet

The reasons, and what places them:

- `zero at every snap`: no run evidence yet; a script that reaches the object
  (training, the overview).
- `host pointer`: its bytes can't be compared; the typed state.
- `its bytes match nowhere`: far pointers whose segments differ by where each
  program sits, or values that differ.
- `several places match`: ambiguous evidence.
- `text constant`: the port's own short strings.
- `conflict`: the procedure bound and the run bytes disagree. These are the
  first things to look at.

## Overlaps

Each `overlap` line is a place the port has two objects where the original has
one, or an object larger than the original's. Each is looked at in turn: an
alias to merge, a size to correct, or a table the original lays over another.
A line without a note is one byte in the original, and `check_dump.py`'s
`agree` lines hold the two objects to the same bytes at every snap. A note
says why they may differ for now (a host pointer, a size still to settle).

## Where it stands

At the end of the whole-program check, the map has 763 objects:

- 428 placed: 153 by the image test's table, 147 by the runs, 65 by
  annotations, 44 by import pieces and 19 by procedures.
- 5 host: the import's own state and the resource file lengths.
- 330 unplaced: 161 zero at every snap, 100 host pointers, 45 whose bytes
  match nowhere and 24 with several places that match. None is in conflict,
  and no text constant is left.

The 10 overlap lines are all noted, and none is a defect:

- 6 are host pointers. Five path pointers lie over the cached-file
  descriptors (each set once at start, only read after), and the campaign
  message table lies over the port's own pointers to the same texts.
- 4 are constant tables the original lays over each other: the AdLib
  register lists, the two pairs of line step tables, and the missile damage
  bonus over the hit location selector's tail.

Every object the port had larger than the original's is now the original's
size. Where an index comes from data, `REVENGE_INDEX` (`revenge/debug.h`)
guards it: an index past the table stops the program with a fault that names
the table.

The unplaced objects are left for the typed state. The scripts that would
reach the zero ones (training, the overview map) are listed in the trace
README.
