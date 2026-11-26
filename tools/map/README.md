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
    overlap NAME NAME               two placed objects share bytes

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
  start is left, and only if the run bytes don't contradict it.

The run kind can be repeated: with `REVENGE_MAP_DUMP_DIR` set to a folder, the
harness writes `SNAP.objects` there at each snap, with every object's name,
size and bytes (`src/SDL/map_dump.c`, written by `make_map_dump.py` from the
headers). Unset, it does nothing.

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
