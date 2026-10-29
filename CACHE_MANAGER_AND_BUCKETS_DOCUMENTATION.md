# AI Generated Documentation

**Disclaimer**: This documentation was generated using AI and may contain inaccuracies or omissions. It should always be viewed alongside the source code it documents.

# Cache Manager and Bucket Architecture

This document explains the cache and bucket subsystem used by d4 to memoize formula buckets for connected components. The goal of this subsystem is to avoid recomputing the same subformula when the same set of variables appears again during solving.

The code is spread over:

- `src/caching/`
- `src/caching/cnf/`
- `src/problem/`
- `src/specs/`

The most important idea is: a bucket is a serialized representation of a CNF fragment associated with a variable component. The cache stores these buckets by hashing their data and reuses them when the same component is revisited.

---

## 1. High-level architecture

The subsystem is layered in three levels:

1. Memory and storage helpers
   - `BucketAllocator`
   - `DataInfo`
   - `CachedBucket`
   - `TmpEntry`

2. Bucket builders / encoders
   - `BucketManager`
   - `BucketManagerCnf`
   - `BucketManagerCnfCl`
   - `BucketManagerCnfSym`
   - `BucketManagerCnfIndex`
   - `BucketManagerCnfCombi`
   - `BucketInConstruction`
   - `BucketSortInfo`

3. Cache lookup and eviction
   - `CacheManager`
   - `CacheNoCollision`
   - `CacheList`
   - `CacheListLRU`
   - `CacheCleaningManager`
   - `CacheCleaningExpectation`
   - `CacheCleaningNone`

The central flow is:

- A component is identified as a set of variables.
- A `BucketManager` serializes that component into a `CachedBucket`.
- A `CacheManager` hashes the bucket and checks whether an equivalent one already exists.
- If found, the cached bucket is reused.
- If not found, the newly built bucket is inserted into the hash table.
- Optional reduction/eviction policies may remove older entries when cache memory pressure becomes high.

---

## 2. Data representation: `DataInfo`

File: `src/caching/DataInfo.hpp`

### Purpose
`DataInfo` is a compact metadata container attached to every cached bucket. It stores enough information to know:

- how many bytes of raw bucket data are stored,
- how many variables belong to the bucket,
- and some extra packed bits for future use.

### Attribute
- `uint64_t info1;`

This is a bit-packed value.

The comments in the class explain the intended layout:

- lower 21 bits: variable count (`nbVar`)
- next 21 bits: size of data (`szData`)
- upper bits: reserved metadata (`free` / `nbBitFormula` / `nbBitVar`)

### Functions

#### `DataInfo()`
Default constructor. Initializes `info1` to zero.

#### `DataInfo(unsigned szData, unsigned nbVar, unsigned nbBitVar, unsigned nbBitFormula)`
Constructs a packed metadata value. In practice the code uses the default `DataInfo` values and then sets the data through `szData(...)` when a bucket is stored.

#### `operator==`
Compares two metadata values by comparing `info1`.

This is essential because the cache checks equality of raw bucket headers before comparing the actual body bytes.

#### `szData()`
Returns the packed data-size field from `info1`.

#### `nbVar()`
Returns the packed variable-count field.

#### `szData(unsigned sz)`
Writes the `szData` field back into `info1` while preserving the rest of the bits.

It uses a mask to avoid clobbering the non-size fields.

#### `reset()`
Sets the metadata to zero.

#### `printData(...)`
Debug helper to print raw bytes in the bucket.

### Why it matters
The cache does not need the full bucket object to decide if two buckets are probably identical. It first compares headers, then raw memory. This minimizes comparison cost and keeps the cache fast.

---

## 3. The actual bucket object: `CachedBucket<T>`

File: `src/caching/CachedBucket.hpp`

### Purpose
`CachedBucket` is the runtime object stored in the cache. It wraps the serialized data and metadata for a formula bucket.

### Attributes
- `char *data;` — raw bytes of the serialized formula bucket
- `DataInfo header;` — compact metadata describing the bucket
- `T fc;` — value associated with this bucket in the solver or cache layer

### Constructor
`CachedBucket()` initializes:

- `data = NULL`
- `header.szData(0)`

This means an empty uninitialized bucket is safe to reset and reuse.

### Functions

#### `set(char *d, DataInfo &dnew)`
Sets raw data pointer and header together.

#### `lockedBucket(T v)`
Stores the solver result/value associated with the bucket, such as a computed subresult.

#### `getInfo()`
Returns `header.info1` as a 64-bit integer.

#### `szData(int s)`
Sets the size of raw data.

#### `szData()`
Returns the stored size.

#### `nbVar()`
Returns how many variables are covered by the bucket, as recorded in the header.

#### `reset()`
Resets the header to zero.

#### `display()`
Prints the bucket header and raw bytes; useful for debugging.

#### `getDataInfo()`
Returns the metadata object itself.

#### `sameHeader(CachedBucket<T> &b)`
Compares the header values.

### Why it matters
The cached bucket is the bridge between:

- the formula serialization layer,
- the hashing layer,
- and the solver value associated with subresult caching.

---

## 4. Temporary cache lookup result: `TmpEntry<T>`

File: `src/caching/TmpEntry.hpp`

### Purpose
A temporary structure used during cache lookup: it carries the bucket, hash value, and whether a hit was found.

### Attributes
- `CachedBucket<T> e;` — candidate bucket
- `unsigned int hashValue;` — hash used for lookup
- `bool defined;` — whether a valid cached entry was found

### Constructors
- default constructor sets `defined = false`
- value constructor copies the bucket, hash, and hit flag

### Functions

#### `getCachedBucket()`
Returns the bucket stored in the temporary result.

#### `getValue()`
Returns `e.fc`, i.e. the cached value attached to the bucket.

### Why it matters
This object is used to tell the rest of the solver: “I checked the cache; here is the bucket, and whether it already existed.”

---

## 5. `BucketAllocator`: memory management for cached buckets

Files:

- `src/caching/BucketAllocator.hpp`
- `src/caching/BucketAllocator.cpp`

### Purpose
This allocator does not use a normal `new/delete` per bucket. It keeps large memory pages and reuses free regions to reduce fragmentation and allocation overhead.

### Attributes
- `std::vector<char *> m_allocateData;` — all allocated pages
- `char *m_data;` — current active page
- `unsigned long m_sizeFirstPage;` — first allocation size
- `unsigned long m_sizeAdditionalPage;` — size of additional pages
- `unsigned long m_sizeData;` — current page size
- `unsigned long m_posInData;` — current writing position inside active page
- `std::vector<std::vector<char *>> m_freeSpace;` — free memory blocks by size
- `unsigned long int m_allMemory;` — total memory reserved
- `unsigned long int m_freeMemory;` — total free space
- `unsigned long int m_usedMemory;` — currently in use
- `bool isInit;` — whether allocator has been initialized
- `bool cleanup;` — whether cleanup is enabled
- `bool m_consumedMemory;` — tracks if memory has been expanded beyond initial page

### Functions

#### `init(sizeFirstPage, sizeAdditionalPage)`
Creates the first page and initializes counters. It is only done once.

This is called from the bucket-manager constructors through `BucketManagerCnf`.

#### `getArray(size)`
Allocates a block of raw memory for a serialized bucket.

It does the following:

- tries to reuse a free block of exactly the requested size;
- otherwise reuses a larger block by splitting it;
- otherwise allocates a new page if the current page is full;
- updates `m_usedMemory`.

This allocator is intentionally specialized for bucket serialization, where many buckets have similar sizes and may be freed and reused.

#### `releaseMemory(m, size)`
Releases a block back to the allocator.

It may:

- return the memory to the end of the current active page if it matches the last allocation,
- or store it in `m_freeSpace[size]` for future reuse.

### Why it matters
The cache stores prefixes and subformula encodings in memory; `BucketAllocator` is the low-level memory manager keeping these allocations efficient.

---

## 6. `BucketManager`: factory and abstract interface

File: `src/caching/BucketManager.hpp`

### Purpose
This is the factory and common interface for building bucket encoders based on the configuration.

### Attributes
- `BucketAllocator *m_bucketAllocator;` — memory allocator used for serialized buckets
- `CachedBucket<T> m_bucket;` — reusable bucket object used by the manager
- `CacheManager<T> *m_cache;` — cache attached to this bucket manager

### Factory method
#### `makeBucketManager(config, cache, s, out)`
This selects a representation strategy based on the configuration:

- `cache_clause_representation == "clause"` → `BucketManagerCnfCl<T>`
- `cache_clause_representation == "sym"` → `BucketManagerCnfSym<T>`
- `cache_clause_representation == "index"` → `BucketManagerCnfIndex<T>`
- `cache_clause_representation == "combi"` → `BucketManagerCnfCombi<T>`

It also reads `cache_store_strategy` and converts strings such as:

- `not-binary` → `NB`
- `not-touched` → `NT`
- default → `ALL`

### Virtual interface
#### `storeFormula(std::vector<Var> &component, CachedBucket<T> &b) = 0;`
This is the pure virtual method every concrete bucket manager must implement. It serializes a component into the bucket object `b`.

### Utility methods
#### `nbOctetToEncodeInt(unsigned int v)`
Returns how many bytes are required to encode a nonnegative integer.

Used to decide how many bits/bytes to allocate while serializing variable indices and clause indices.

#### `collectBucket(std::vector<Var> &component)`
Creates a bucket for a component by calling `storeFormula()`. It then returns a pointer to the internal `m_bucket`.

#### `usedMemory()`
Delegates to the allocator and returns currently used memory.

#### `getComsumedMemory()`
Exposes whether the allocator has consumed more than the initial page.

#### `reinitComsumedMemory()`
Resets the consumption flag.

#### `releaseMemory(char *m, unsigned size)`
Free a serialized bucket block through the allocator.

#### `remainingMemory()`
Reports how much allocator memory is still available.

### Why it matters
This class does not encode formulas itself; it selects a concrete CNF-specific strategy and gives a common interface to the cache.

---

## 7. `BucketManagerCnf`: base CNF-specific bucket manager

File: `src/caching/cnf/BucketManagerCnf.hpp`

### Purpose
This is the common base for all CNF bucket encodings. It contains the knowledge of the CNF problem structure: number of variables, clauses, max clause length, mode of storage, and the spec manager used to inspect clauses.

### Attributes
- `SpecManagerCnf &m_specManager;` — access to CNF clause metadata and occurrence lists
- `ModeStore m_modeStore;` — store policy (`ALL`, `NB`, `NT`)
- `unsigned m_nbClauseCnf;` — number of clauses
- `unsigned m_nbVarCnf;` — number of variables
- `unsigned m_maxSizeClause;` — max clause length
- `std::vector<bool> m_varInComponent;` — marks whether a variable belongs to current component
- `std::vector<int> m_idxClauses;` — temporary list of clause ids for a component

### Constructor
Initializes:

- cache pointer,
- allocator,
- store mode,
- CNF counts,
- component marker vectors,
- allocator initialization.

### `isKeptClause(int idx)`
Determines whether a clause is retained under the current store strategy:

- `NT`: keep only if `getNbUnsat(idx)` is true
- `NB`: keep only clauses with size > 2
- `ALL`: keep everything

This is central because some cache policies intentionally skip binary clauses or unsatisfied clauses.

### `collectIdActiveClauses(std::vector<Var> &component, std::vector<unsigned> &idxClauses)`
This builds the index list of useful clauses belonging to the current component.

It does:

- ask the spec manager for current clauses in the component,
- optionally filter to non-binary clauses,
- remove clauses rejected by `isKeptClause()`.

### Why it matters
This is the first stage of bucket creation: identifying which clauses are relevant before any serialization occurs.

---

## 8. `BucketSortInfo`: information about a sorted clause bucket

File: `src/caching/cnf/BucketSortInfo.hpp`

### Purpose
This structure tracks a bucket segment during distribution building.

### Attributes
- `unsigned start;` — begin offset in the sorted distribution array
- `unsigned end;` — end offset
- `unsigned counter;` — how many clauses are currently in this bucket
- `unsigned redirected;` — if a bucket is split, this points to the redirected bucket

### Constructors
- default constructor
- `BucketSortInfo(unsigned init)`
- `BucketSortInfo(unsigned s, unsigned e)`

### `display()`
Prints the start/end range for debugging.

### `reset(unsigned s, unsigned e)`
Resets the structure to a new range.

### Why it matters
When building compact clause distributions, a single literal may contribute to multiple clause groups. `BucketSortInfo` keeps track of where those groups start and end while they are being split and merged.

---

## 9. `BucketInConstruction`: temporary work array for bucket assembly

File: `src/caching/cnf/BucketInConstruction.hpp`

### Purpose
This structure acts as the working storage used while the bucket is being built from a component.

### Attributes
- `unsigned *distrib;` — sorted distribution of clauses by literal or by size
- `unsigned *shiftedIndexClause;` — remapped clause indices after redundant clause removal
- `unsigned *shiftedSizeClause;` — compressed clause size map
- `unsigned *sizeClauses;` — how many literals each clause contributes
- `unsigned *distribDiffSize;` — number of clauses for each clause-size class
- `bool *markedAsRedundant;` — marks clauses that are removed as duplicate/redundant
- `unsigned nbClauseInDistrib;` — active clause count in distribution
- `unsigned sizeDistrib;` — current payload size in the distribution
- `unsigned capacityDistrib;` — maximum capacity for the temporary arrays
- `unsigned maxSizeClause;` — largest clause length in this distribution

### Constructor and destructor
`BucketInConstruction(SpecManagerCnf &occM)` allocates all the working arrays based on formula properties.

### `reinit()`
Resets the internal state before starting a new bucket assembly.

### Why it matters
This class performs the “construction-time bookkeeping” that the final bucket encoding requires: sorting, redundancy detection, and index remapping.

---

## 10. `BucketManagerCnfCl`: clause-oriented compressed bucket manager

File: `src/caching/cnf/BucketManagerCnfCl.hpp`

### Purpose
This is the clause-based CNF serialization policy. It builds a compressed representation where the formula is represented by its actual clause distribution and variable ordering.

### Internal fields
- `std::vector<BucketSortInfo> m_vecBucketSortInfo;`
- `int m_unusedBucket;`
- `std::vector<unsigned long int> m_mapVar;`
- `std::vector<int> m_mustUnMark;`
- `std::vector<int> m_markIdx;`
- `std::vector<unsigned> m_idInVecBucket;`
- `BucketInConstruction m_inConstruction;`
- `unsigned *m_memoryPosWrtClauseSize;`
- `unsigned *m_offsetClauses;`

### Constructor
Initializes:

- the base manager,
- the internal construction helpers,
- variable mapping,
- mark arrays,
- clause offset tables,
- and the per-clause memory index arrays.

### Utility: `nbBitUnsigned(unsigned v)`
Computes the number of bits needed to encode a positive integer as a compact bit width. This is used for space-efficient serialization.

### Nested struct: `AllocSizeInfo`
Tracks how much memory is required to store:

- variables,
- literals,
- the formula distribution,
- and the total final bytes.

It has fields:

- `nbBitEltVar`
- `nbByteStoreVar`
- `nbByteStoreFormula`
- `nbBitStoreLit`
- `totalByte`

And a `display()` method for debug output.

### `getIdxBucketSortInfo(BucketInConstruction &inConstruction)`
Obtains an index in the bucket sort info array.

If there is a free bucket available, it reuses it; otherwise it creates a new `BucketSortInfo` entry.

### `pushSorted(unsigned *tab, unsigned pos, unsigned val)`
Inserts a value into a sorted array in-place by shifting larger values to the right.

This is used to maintain sorted clause indices by literal-aware ordering.

### `createDistribWrTLit(const Lit &l, BucketInConstruction &inConstruction)`
This is one of the key functions.

It does the following:

1. reserves room in the distribution for the literal and its size info,
2. selects a bucket for the literal,
3. traverses every clause containing that literal,
4. classifies clauses into new vs already-seen buckets,
5. updates the clause ordering structure,
6. splits redirected buckets when needed,
7. records the literal + size metadata back into the distribution.

This is the crucial step in building a sorted residual CNF structure.

### `collectDistrib(std::vector<Var> &component, BucketInConstruction &inConstruction)`
This assembles the final clause distribution for the component.

It:

- iterates over all variables in the component,
- creates distributions for both positive and negative literals,
- marks redundant clauses,
- removes duplicates or unnecessary clauses,
- reindexes the clauses compactly,
- returns the actual serialized distribution size.

### `initSortBucket(BucketInConstruction &inConstruction)`
Resets the temporary sort state before creating a new bucket.

### `computeNeededBytes(...)`
Computes the exact storage space needed for the bucket by looking at:

- number of variables in the component,
- clause size distribution,
- number of literal entries.

### `addElementInData(...)`
Adds a value into the byte array while packing bits across bytes.

This is low-level bit-packing logic used when serializing integers into compact byte arrays.

### `storeVariables(...)`
Stores the variable IDs using the calculated bit lengths.

### `storeFormula(...)`
This is the public entry point for bucket creation.

It performs the complete process:

1. reset working data structures,
2. build the clause distribution for the component,
3. compute memory requirements,
4. allocate the required raw storage,
5. serialize the variables and clauses,
6. fill `CachedBucket` metadata.

This function is the practical serializer for this strategy.

### Why it matters
This is the most detailed CNF representation in the code. It compresses the formula structure directly based on clause sizes and literal occurrences.

---

## 11. `BucketManagerCnfSym`: symmetric clause representation

File: `src/caching/cnf/BucketManagerCnfSym.hpp`

### Purpose
This manager stores CNF buckets using a symmetric representation around the literal and the representative literal for each clause.

The functions follow the same overall logic as `BucketManagerCnfCl`, but they include a `repLit` parameter and rely on a different representation scheme.

### Key functions

#### `createDistribWrTLit(const Lit &l, BucketInConstruction &inConstruction, const Lit repLit)`
This is the symmetric version of clause distribution creation. Instead of only considering the current literal, it also tracks a representative literal so that a clause can be assigned to a canonical literal bucket.

#### `collectDistrib(...)`
Same conceptual behavior as `BucketManagerCnfCl`: sort the clause distribution, eliminate redundant clauses, and compact the result.

#### `computeNeededBytes(unsigned nBda, unsigned nbD, unsigned nbEltData, unsigned nbEltDist)`
Calculates the serialized size using the number of bytes needed for variable encoding and distribution metadata.

#### `storeVariables` / `storeDistribInfo` / `storeClauses`
These functions serialize the final representation of:

- variables,
- distribution metadata,
- clause payloads.

#### `getInfoDistributionSize(...)`
Computes the number of distinct clause sizes and the maximum clause size for the component.

#### `storeFormula(...)`
This final method decides how much memory is needed, allocates it, serializes the distribution and clauses, and attaches the result to the output bucket.

### Why it matters
This representation is intended to exploit structural symmetry in the formulas. In practice it is another serialized CNF-layout strategy, allowing the cache to choose different encodings depending on formula shape.

---

## 12. `BucketManagerCnfIndex`: index-based representation

File: `src/caching/cnf/BucketManagerCnfIndex.hpp`

### Purpose
Instead of serializing full clause text, this manager stores:

- the component variables,
- and the list of active clause indexes.

It is a compact index representation, especially useful when the component is large and the clause set is stable.

### Key methods

#### `storeData<U, W>(void *data, std::vector<W> &value)`
Writes a vector of integer-like values into a raw byte block, type-safe for `uint8_t`, `uint16_t`, or `uint32_t`.

#### `storeFormula(std::vector<Var> &component, CachedBucket<T> &b)`
This method:

1. collects active clause ids with `collectIdActiveClauses`,
2. computes required bytes for variable ids and clause ids,
3. allocates memory,
4. stores component variables,
5. stores active clause ids,
6. leaves the final bucket metadata to be completed.

The current implementation contains a `goto fillTheBucket;` branch and a commented-out bucket metadata creation. That indicates this strategy is partially implemented and may still be under construction or intentionally unfinished in this branch.

### Why it matters
This representation is useful when the cache wants to avoid reconstructing clause bodies and instead store references into the CNF structure.

---

## 13. `BucketManagerCnfCombi`: strategy chooser

File: `src/caching/cnf/BucketManagerCnfCombi.hpp`

### Purpose
This class combines multiple CNF encodings and chooses the representation based on the component size.

### Attributes
- `BucketManagerCnfCl<T> *clBucketManager;`
- `BucketManagerCnfCl<T> *clBucketManagerBis;`
- `BucketManagerCnfSym<T> *symBucketManager;`
- `BucketManagerCnfIndex<T> *indexBucketManager;`
- `unsigned m_limitNbVarSym;`
- `unsigned m_limitNbVarIndex;`

### Constructor
It creates all concrete sub-managers and sets thresholds:

- if component size is smaller than `m_limitNbVarSym`, use the symmetric manager,
- if component size is larger than `m_limitNbVarIndex`, use the index manager,
- otherwise use the clause manager.

It also disables cleanup on the allocator while the combined manager is active.

### `storeFormula(...)`
Delegates to one of the sub-managers based on size thresholds.

### Why it matters
This is the adaptive strategy: small components can be represented with a compact symmetric layout, large ones with index storage, and middle-sized components with clause-based representation.

---

## 14. `CacheManager`: base cache logic

File: `src/caching/CacheManager.hpp`

### Purpose
This is the central cache abstraction. It owns:

- the hash-table implementation,
- the bucket manager used to create buckets,
- the cleaning policy used to evict entries,
- statistics about hits and memory usage.

### Attributes
- `bool verb;` — verbose mode flag
- `unsigned long m_nbEntry;` — current number of stored buckets
- `unsigned long m_nbPositiveHit;` — number of successful cache hits
- `unsigned long m_nbNegativeHit;` — number of cache misses or failed comparisons
- `unsigned minAffectedHitCache;` — lower bound for affected cache values
- `double sumAffectedHitCache;` — aggregate hit statistics
- `unsigned m_nbInitVar;` — initial variable count
- `unsigned long int m_nbCreationBucket;` — number of bucket creations
- `unsigned long int m_sumDataSize;` — total serialized bucket memory
- `unsigned int m_limitVarCached;` — current maximum variable count allowed in cache
- `std::ostream m_out;` — stream used for logging
- `HashString hashMethod;` — hashing object used to compute bucket hash
- `BucketManager<T> *m_bucketManager;` — serializer and bucket factory
- `CacheCleaningManager<T> *m_cacheCleaningManager;` — eviction policy manager

### Constructor
Initializes:

- output stream copy,
- counters,
- the cache limit based on total variable count,
- the cleaning manager,
- the bucket manager.

### Factory method
#### `makeCacheManager(config, nbVar, specs, out)`
Creates a specific cache backend:

- `no-collision` → `CacheNoCollision<T>`
- `list` → `CacheList<T>`

The factory does not currently build `CacheListLRU` via the normal route. It is a legacy implementation and is not the active default path.

### Virtual methods
These must be implemented by concrete cache types:

- `pushInHashTable(CachedBucket<T> &cb, unsigned int hashValue, T val)`
- `bucketAlreadyExist(CachedBucket<T> &cb, unsigned hashValue)`
- `initHashTable(unsigned maxVar)`
- `removeEntry(std::function<bool(CachedBucket<T> &c)> test)`

### Important helper methods

#### `usedMemory()`
Returns the memory used by the bucket manager allocator.

#### `getLimitVarCached()` / `setLimitVarCache(...)`
Control the eviction threshold for bucket sizes.

#### `isActivated(unsigned nbVar)`
Returns whether a bucket of a given component size is allowed to participate in caching.

#### `nbCreationBucket()` / `sumDataSize()`
Expose creation and memory statistics.

#### `getNbPositiveHit()` / `getNbNegativeHit()`
Return total cached-hit statistics.

#### `getNbEntry()`
Returns entry count.

#### `decrementNbEntry()`
Decrements the tally after a removal.

#### `getBucketManager()`
Returns the active bucket manager.

#### `releaseMemory(char *data, int size)`
Forwards memory release to the bucket manager allocator.

#### `printCacheInformation(std::ostream &out)`
Prints statistics about hits and eviction policy.

#### `computeHash(CachedBucket<T> &bucket)`
Calculates the hash using `HashString` over the serialized bucket data and metadata.

#### `addInCache(TmpEntry<T> &cb, T val)`
Pushes a newly created bucket into the hash table.

#### `searchInCache(std::vector<Var> &varConnected)`
This is the most central method of the class.

It:

1. checks if the cache is under memory pressure and triggers cleaning if needed,
2. collects the bucket for the current component using the bucket manager,
3. computes its hash,
4. probes the hash table using the hash,
5. updates cache statistics via the cleaning manager,
6. if found, returns a `TmpEntry<T>` marked as a hit,
7. otherwise returns a `TmpEntry<T>` containing the newly built bucket flagged as not found.

This is the method the rest of the solver uses to ask “is this component already memoized?”

#### `releaseMemory(CachedBucket<T> &formulaBucket)`
Releases a bucket’s raw memory when the formula is no longer needed.

#### `setInfoFormula(unsigned mVar)`
Records the formula variable count and the minimum affected cache size.

### Why it matters
This is the real orchestrator of the cache subsystem. The concrete cache classes implement the storage strategy, but this class defines the generic lookup and bookkeeping behavior used everywhere.

---

## 15. `CacheNoCollision<T>`

File: `src/caching/CacheNoCollision.hpp`

### Purpose
This cache uses a single large hash table with one entry per hash bucket (`SIZE_HASH = 22041997`). It assumes collisions are rare enough that a single slot per hash modulo is sufficient, hence the name “no-collision”.

### Attributes
- `const unsigned SIZE_HASH = 22041997;`
- `std::vector<CachedBucket<T>> hashTable;`

### Constructor
Calls the base constructor and initializes the table with `initHashTable(nbVar)`.

### `pushInHashTable(CachedBucket<T> &cb, unsigned int hashValue, T val)`
This stores a bucket into the chosen hash slot.

It does:

- compute the slot index using `hashValue % SIZE_HASH`,
- free the old entry if a bucket is already present,
- replace the slot with the new bucket,
- attach the solver value with `lockedBucket(val)`,
- initialize per-bucket cleanup counts,
- update statistics (`m_nbCreationBucket`, `m_sumDataSize`, `m_nbEntry`).

### `bucketAlreadyExist(...)`
This is the actual hit test.

It checks:

- same slot,
- same header via `cb.sameHeader(cbi)`,
- same raw byte content via `memcmp(refData, cbi.data, cbi.szData())`.

If equal, it increments positive hits and returns the cached bucket.

Otherwise, it increments negative hits and returns `NULL`.

### `initHashTable(unsigned maxVar)`
Clears and resizes the table to the fixed hash size and sets formula info.

### `removeEntry(...)`
Removes buckets that satisfy a predicate from the table.

It calls `releaseMemory` and resets entries to empty state.

### Why it matters
This is the simplest and fastest implementation when the hash table is large enough to make collisions unlikely. It is also easier to reason about because each slot stores only one bucket.

---

## 16. `CacheList<T>`

File: `src/caching/CacheList.hpp`

### Purpose
This cache uses a hash table of vectors. Each bucket index holds a list of buckets that collided on the same modulo class.

### Attributes
- `const unsigned SIZE_HASH = 999331;`
- `std::vector<std::vector<CachedBucket<T>>> hashTable;`

### Constructor
Initializes the vector-of-vectors table.

### `pushInHashTable(...)`
Adds the new bucket to the collision list of the selected hash slot.

It then locks the solver value in the final `CachedBucket` and updates creation statistics.

### `bucketAlreadyExist(...)`
Checks every bucket in the collision list for the same header and same raw bytes.

This is a linear search inside a collision list, which is slower than the no-collision strategy but more robust when the hash space is smaller or collisions are common.

### `createAndStoreBucket(...)`
A convenience helper to create and store a bucket for some set of variables.

### `initHashTable(...)`
Resizes each bucket slot to a vector container.

### `removeEntry(...)`
Removes matching buckets from each collision list while preserving the order of remaining entries.

### Why it matters
This strategy tolerates more collisions because each slot can store multiple buckets. It is the classic collision-chaining design.

---

## 17. `CacheListLRU<T>`

File: `src/caching/CacheListLRU.hpp`

### Purpose
This is a more advanced, time-based cache policy with usage tracking. It is a legacy/experimental LRU-style cache.

### Internal node structure
```cpp
struct Node {
  CachedBucket<T> bucket;
  size_t last_use;
};
```

Each bucket entry stores when it was last used.

### Attributes
- `std::vector<std::vector<Node>> hashTable;`
- `size_t counter;` — global monotonic access counter
- `size_t max_mem;` — memory limit before eviction starts

### `pushInHashTable(...)`
Adds the bucket to the collision list and records its `last_use` as `counter`.

If the manager exceeds `max_mem`, it computes a median last-use cutoff and removes entries that are older than that threshold.

### `bucketAlreadyExist(...)`
As with the list cache, it linearly scans the collision list. On a hit, it updates `last_use` to `counter`.

### `initHashTable(...)`
Initializes the slot vectors.

### `removeEntry(...)`
Removes matching buckets while preserving other entries.

### Why it matters
This is a weaker, heuristic eviction scheme. It is not the primary active strategy in the current factory and is better understood as an experimental or older design.

---

## 18. Cache cleaning and eviction: `CacheCleaningManager`

File: `src/caching/CacheCleaningManager.hpp`

### Purpose
This class is responsible for deciding when the cache should reduce its contents and which entries should be discarded.

### Abstract API
The base class defines:

- `initCountCachedBucket(CachedBucket<T> *cb)`
- `updateCountCachedBucket(CachedBucket<T> *cb, int nbVar)`
- `reduceCache()`
- `printCleaningInfo(std::ostream &out)`

### Factory method
`makeCacheCleaningManager(config, cache, nbVar, out)` selects:

- `expectation` → `CacheCleaningExpectation<T>`
- otherwise → `CacheCleaningNone<T>`

### `releaseMemory(char *data, int size)`
A convenience method delegating the actual free operation to the bucket manager.

### Why it matters
The cache may be allowed to grow only to a certain point. The cleaning manager decides when to trim it based on hit rates and component sizes.

---

## 19. `CacheCleaningExpectation<T>`

File: `src/caching/CacheCleaningExpectation.hpp`

### Purpose
This policy tries to adapt the cache limit by looking at the ratio of positive and negative hits for buckets of a given variable count.

### Attributes
- `const double INC_THRESHOD = 0.05;`
- `const double INIT_THRESHOD = 0;`
- `unsigned m_nbReduceCall;` — number of reductions
- `unsigned long m_nbRemoveEntry;` — number of removed entries
- `unsigned long m_nbPositiveHit;` — positive hit count
- `unsigned long m_nbNegativeHit;` — negative hit count
- `int m_nbVar;`
- `double m_threshold;`
- `std::vector<StatVarSizeCache> m_statVar;`

### `StatVarSizeCache`
This structure tracks for each variable-count bucket size:

- `negative`
- `positive`
- `number`

### `initCountCachedBucket(CachedBucket<T> *cb)`
Increments the number of buckets stored for the given variable count.

### `updateCountCachedBucket(CachedBucket<T> *cb, int nbVar)`
Updates the positive/negative counters depending on whether a lookup succeeded.

### `reduceCache()`
This is the main algorithm.

It does:

1. computes a dynamic limit based on the current cache size limit,
2. scans variable-size classes from large to small,
3. computes a positive/negative ratio,
4. uses a threshold to decide when a class is too expensive to keep,
5. halves the counters of the class to age them,
6. adjusts `m_cache->setLimitVarCache(limit)`,
7. removes all cached buckets whose number of variables is above the new limit.

This means the cache becomes more aggressive as it sees lots of misses for larger buckets.

### `printCleaningInfo(...)`
Prints reduction-call statistics.

### Why it matters
This is the adaptive eviction heuristic: it tries to keep the cache focused on variable sets that are actually re-used.

---

## 20. `CacheCleaningNone<T>`

File: `src/caching/CacheCleaningNone.hpp`

### Purpose
A no-op cleaner: the cache never evicts entries automatically.

### Methods
All methods are empty:

- `initCountCachedBucket(...)` — do nothing
- `updateCountCachedBucket(...)` — do nothing
- `reduceCache()` — do nothing
- `printCleaningInfo(...)` — do nothing

### Why it matters
This is the simplest non-adaptive policy and is used when the chosen config disables cache reduction.

---

## 21. Runtime flow: how a cache lookup really works

The workflow is best understood through `CacheManager::searchInCache()`.

Pseudo-flow:

1. If cache memory is marked as consumed, trigger cleanup.
2. Reinitialize the consumed-memory flag.
3. Ask the bucket manager to build a bucket for the set of variables in the current component.
4. Compute hash of the bucket contents.
5. Probe the cache backend (`CacheNoCollision`, `CacheList`, etc.) for an equivalent bucket.
6. Update cleaning stats with the result of the hit/miss.
7. If hit: return the cached bucket and mark it as a hit.
8. If miss: return the new bucket as a temporary entry and let the caller insert it into the cache.

This is exactly how the caching layer avoids repeated recomputation of connected-component subformulas.

---

## 22. Why the cache exists in this solver

The solver works on formula components. The same variable subset may recur many times during decomposition, elimination, and propagation. Without memoization, the solver would repeatedly reconstruct and serialize the same CNF fragments.

The cache stores these subproblems by means of:

- variable set identity,
- formula serialization,
- hashing,
- equality check on both header and raw bytes.

This means the solver can quickly answer whether a previously computed subformula already exists and reuse its result instead of recomputing it.

---

## 23. Summary of the main design ideas

1. `BucketAllocator` manages raw memory for cached formula blocks.
2. `DataInfo` packs metadata about bucket size and variable count.
3. `CachedBucket` is the runtime cache record.
4. `BucketManager` creates serialized bucket encodings for component formulas.
5. `BucketManagerCnf*` implementations choose a representation strategy for CNF buckets.
6. `CacheManager` performs hash-based lookup and stores the result.
7. `CacheNoCollision` and `CacheList` are concrete storage backends.
8. `CacheCleaning*` decides whether old entries should be removed to maintain memory pressure.

The combination of these classes gives the solver a fast and flexible memoization system for repeated CNF component computations.

---

## 24. Practical interpretation

From a software engineering standpoint, this design separates responsibilities cleanly:

- allocation and memory reuse: `BucketAllocator`
- metadata and runtime representation: `DataInfo`, `CachedBucket`
- formula serialization to a byte stream: `BucketManager*`
- hash-based cache storage: `CacheManager` + derived classes
- cache eviction policy: `CacheCleaningManager` + derived classes

That separation is exactly what makes the subsystem reusable, configurable, and adaptable to different CNF encodings and memory budgets.
