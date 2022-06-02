#ifndef NACHOS_COREMAP__HH
#define NACHOS_COREMAP__HH
#include "lib/utility.hh"

/// A “bitmap” -- an array of bits, each of which can be independently set,
/// cleared, and tested.
///
/// Most useful for managing the allocation of the elements of an array --
/// for instance, disk sectors, or main memory pages.
///
/// Each bit represents whether the corresponding sector or page is in use
/// or free.
class Coremap {
public:
    /// Initialize a Coremap with `nitems` pairs; all pairs  are cleared.
    ///
    /// * `nitems` is the number of items in the coremap.
    Coremap(unsigned nitems);

    /// Uninitialize a coremap.
    ~Coremap();

    /// Set the “nth” entry.
    void Mark(unsigned phys, unsigned vpn, unsigned pid);

    /// Clear the “nth” entry.
    void Clear(unsigned phys);

    bool Test(unsigned phys);

    /// Return the index of a empty pair, and as a side effect, set the bit.
    ///
    /// If no pairs are clear, return (-1,-1).
    int Find(unsigned vpn, unsigned pid);

    unsigned CountClear();

    int *GetOwner(unsigned phys);

    void Print();

private:
    /// Number of entries in the bitmap.
    unsigned numEntries;

    /// Pair storage.
    int **map;
};

#endif
