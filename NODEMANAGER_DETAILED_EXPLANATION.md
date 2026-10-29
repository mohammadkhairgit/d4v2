# AI Generated Documentation

**Disclaimer**: This documentation was generated using AI and may contain inaccuracies or omissions. It should always be viewed alongside the source code it documents.

# Detailed Explanation: NodeManager and NodeManagerTyped Classes

## Table of Contents
1. [Overview](#overview)
2. [Class Hierarchy](#class-hierarchy)
3. [Template Parameters](#template-parameters)
4. [Member Variables](#member-variables)
5. [Memory Management Strategy](#memory-management-strategy)
6. [Factory Pattern: makeNodeManager()](#factory-pattern-makenodemanager)
7. [NodeManager Base Class](#nodemanager-base-class)
8. [NodeManagerTyped Specialized Class](#nodemanagertypd-specialized-class)
9. [Node Creation Methods](#node-creation-methods)
10. [Node Analysis Methods](#node-analysis-methods)

---

## Overview

The `NodeManager` and `NodeManagerTyped` classes form a **memory management and factory system** for creating and analyzing nodes in a Negation Normal Form (NNF) representation of a CNF formula. The system uses a **page-based memory allocation strategy** to efficiently manage memory for potentially thousands of nodes while avoiding heap fragmentation.

The design employs the **Factory Method Pattern** and **Template Specialization** to:
- Dynamically choose the smallest appropriate variable ID storage type at runtime
- Efficiently allocate memory in bulk pages
- Provide virtual polymorphic interfaces for node operations

---

## Class Hierarchy

```
                    NodeManager<T> (abstract base)
                           ▲
                           │ inherits
                           │
         NodeManagerTyped<T, U> (concrete implementation)
```

### Relationship:
- **NodeManager<T>**: Abstract base class that defines the interface and implements common memory allocation
- **NodeManagerTyped<T, U>**: Concrete template specialization that provides typed implementations of all virtual methods

---

## Template Parameters

### NodeManager<T>
- **T**: The type used to store **model counts** (typically `uint32_t`, `uint64_t`, or arbitrary precision types)
- Example: If counting models, `T` might be `uint64_t` to hold values up to 2^64

### NodeManagerTyped<T, U>
- **T**: Same as NodeManager<T> - the model count type
- **U**: The type used to store **variable indices** within each node's data payload
  - `uint8_t`:  For problems with < 128 variables (1 bit reserved for sign)
  - `uint16_t`: For problems with 128 to 32,767 variables
  - `uint32_t`: For problems with 32,768+ variables
  
**Why separate these?** Variable indices are stored many times in the graph (one per node), so using the smallest type that fits saves significant memory. Model counts are typically used once per operation, so they can be larger without penalty.

---

## Member Variables

### NodeManager<T> (Base Class)

#### `std::vector<uint8_t *> m_memoryPages`
- **Type**: Vector of pointers to raw byte arrays
- **Purpose**: Stores all allocated memory pages
- **How it works**: 
  - Each element is a single "page" of `PAGE_SIZE` bytes (16 MB)
  - When current page fills up, a new page is allocated and added to this vector
  - All pages are deallocated in the destructor via a loop: `delete[] p`
- **Memory overhead**: One pointer per page (~8 bytes per page on 64-bit systems)

#### `unsigned m_posInMemoryPage`
- **Type**: Current position in the active memory page
- **Range**: 0 to PAGE_SIZE
- **Purpose**: Tracks where we are in the current page
- **How it works**:
  - Starts at 0 for a fresh page
  - Incremented after each allocation
  - When it reaches PAGE_SIZE, a new page is allocated and position resets to 0
  - Enables linear allocation within pages (no fragmentation)

#### `uint8_t *m_data`
- **Type**: Pointer to the current active memory page
- **Purpose**: Quick access to the active page
- **How it works**:
  - Points to `m_memoryPages.back()` after each page allocation
  - Avoids repeated vector lookups
  - Updated when a new page is created

#### `unsigned m_globalStamp`
- **Type**: Counter for marking/invalidation
- **Purpose**: Provides a per-operation "timestamp" to avoid revisiting nodes
- **How it works**:
  - Incremented at the start of each traversal operation
  - Each node can store the last stamp it was visited with
  - If node's stamp != m_globalStamp, it hasn't been visited in this operation
  - This is a classic technique to reset visited flags without iterating all nodes
- **Benefit**: O(1) reinitialization instead of O(nodes)

#### `static const unsigned PAGE_SIZE = 1 << 24`
- **Value**: 16,777,216 bytes = 16 MB per page
- **Why this size**:
  - Large enough to amortize allocation overhead
  - Small enough to avoid excessive memory per page
  - Powers of 2 allow fast bit-shifting in calculations

### NodeManagerTyped<T, U> (Derived Class)

#### `TrueNode<T> *trueNode`
- **Type**: Pointer to a special node representing boolean TRUE
- **Purpose**: Singleton constant node for tautologies
- **How it works**:
  - Allocated once in constructor via placement new
  - Reused for all "true" branches without reallocation
  - Reduces memory for common terminal case

#### `FalseNode<T> *falseNode`
- **Type**: Pointer to a special node representing boolean FALSE
- **Purpose**: Singleton constant node for contradictions
- **How it works**:
  - Allocated once in constructor via placement new
  - Reused for all "false" branches without reallocation
  - Reduces memory for common terminal case

---

## Memory Management Strategy

### Core Concept: Page-Based Arena Allocation

Instead of allocating individual nodes with `new` (which causes heap fragmentation), the manager allocates **large contiguous blocks (pages)** and suballocates from them.

```
┌─ Page 0 ──────────────────┐
│ [Node1][Node2][Node3]...  │  16 MB
│ m_posInMemoryPage ──┐     │
└───────────────────────────┘
                          
┌─ Page 1 ──────────────────┐
│ [Node4][Node5]...         │  16 MB
│ m_posInMemoryPage ──┐     │
└───────────────────────────┘

m_memoryPages = [Page0_ptr, Page1_ptr]
```

### Allocation Flow: getMemory(unsigned nbBytes)

```cpp
inline uint8_t *getMemory(unsigned nbBytes) {
    // Check if current page has enough space
    if (m_posInMemoryPage + nbBytes >= PAGE_SIZE) {
        // Current page is full or would overflow
        m_posInMemoryPage = 0;
        m_data = new uint8_t[PAGE_SIZE];
        m_memoryPages.push_back(m_data);
    }
    
    // Calculate the return address
    uint8_t *returnAddr = &m_data[m_posInMemoryPage - nbBytes];
    
    // Update position for next allocation
    m_posInMemoryPage += nbBytes;
    
    return returnAddr;
}
```

**Step-by-step example**:
```
Initial state:
  m_posInMemoryPage = 100
  nbytes = 50
  
Check: 100 + 50 = 150 < PAGE_SIZE (16MB)? YES, no new page needed.

returnAddr = &m_data[100 - 50] = &m_data[50]  // Wait, this is backwards!

Actually, let me reconsider:
  m_posInMemoryPage += nbytes;  // First: m_posInMemoryPage becomes 150
  return &m_data[m_posInMemoryPage - nbytes];  // Then: return &m_data[100]
```

**Note**: The function increments first, then returns the address BEFORE the increment. This allocates memory linearly and contiguously.

### Deallocation

Deallocation is **en masse** at destruction:
```cpp
virtual ~NodeManager() {
    for (auto &p : m_memoryPages)
        delete[] p;
    m_posInMemoryPage = 0;
}
```

**No per-node deallocation**: Individual nodes are never freed. Only entire pages are freed when the NodeManager is destroyed. This is efficient for:
- Avoiding overhead of tracking individual deallocations
- Simplifying cleanup logic
- Ensuring no use-after-free (all nodes exist for the entire NodeManager lifetime)

---

## Factory Pattern: makeNodeManager()

### Signature
```cpp
static NodeManager<T> *makeNodeManager(unsigned nbVar)
```

### Purpose
**Runtime selection of the smallest variable ID storage type** based on the number of variables in the problem.

### How It Works

```cpp
if (nbVar < (1 << 7))      // nbVar < 128
    return new NodeManagerTyped<T, uint8_t>();

if (nbVar < (1 << 15))     // nbVar < 32,768
    return new NodeManagerTyped<T, uint16_t>();

return new NodeManagerTyped<T, uint32_t>();
```

### Why This Matters

Each node contains a data payload with variable indices. If we always used `uint32_t`:

```
1000 nodes × 32 bits per variable reference × multiple refs/node 
  = significant memory waste for small problems
```

By selecting `uint8_t` for small problems:
```
1000 nodes × 8 bits per variable reference × multiple refs/node 
  = 75% memory savings for 100-variable problems
```

### Example Decision Tree

| nbVar | Selected Type | Bytes per ID | Max representable variables |
|-------|---------------|--------------|---------------------------|
| 50    | uint8_t       | 1            | 127 (1 bit reserved for sign) |
| 500   | uint16_t      | 2            | 32,767 |
| 50,000| uint32_t      | 4            | 2,147,483,647 |

---

## NodeManager Base Class

The base class defines the **interface** and **common memory management**.

### Protected Methods

#### `inline uint8_t *getMemory(unsigned nbBytes)`
- **Visibility**: Protected (only subclasses and methods can call)
- **Return**: Pointer to allocated memory block
- **Behavior**: 
  - Linear allocation from current page
  - Automatic page allocation when current page fills
  - See Memory Management Strategy section for details
- **Used by**: All node creation methods in NodeManagerTyped

### Public Interface (Virtual, Pure Abstract)

These methods **must** be implemented by subclasses:

```cpp
virtual Node<T> *makeTrueNode() = 0;
virtual Node<T> *makeFalseNode() = 0;
virtual Node<T> *makeDecomposableAndNode(Node<T> **sons, unsigned size) = 0;
virtual Node<T> *makeBinaryDeterministicOrNode(
    DataBranch<Node<T> *> &left, 
    DataBranch<Node<T> *> &right) = 0;
virtual Node<T> *makeUnaryNode(DataBranch<Node<T> *> &branch) = 0;
virtual T computeNbModels(Node<T> *node, 
                         std::vector<ValueVar> &fixedValue,
                         ProblemManager &problem) = 0;
virtual bool isSAT(Node<T> *node, 
                  std::vector<ValueVar> &fixedValue) = 0;
virtual void printNNF(Node<T> *node, std::ostream &out) = 0;
virtual void deallocate(Node<T> *node) = 0;
```

### Factory Static Method

```cpp
static NodeManager<T> *makeNodeManager(unsigned nbVar)
```

Creates and returns the appropriate concrete implementation based on nbVar.

---

## NodeManagerTyped Specialized Class

This is the **concrete implementation** that all node operations actually use.

### Constructor: NodeManagerTyped()

```cpp
NodeManagerTyped() {
    m_data = new uint8_t[PAGE_SIZE];
    m_memoryPages.push_back(m_data);
    m_posInMemoryPage = 0;
    m_globalStamp = 0;

    // Allocate TrueNode
    unsigned memoryNeeded = sizeof(TrueNode<T>);
    uint8_t *data = NodeManager<T>::getMemory(memoryNeeded);
    trueNode = new (data) TrueNode<T>();

    // Allocate FalseNode
    memoryNeeded = sizeof(FalseNode<T>);
    data = NodeManager<T>::getMemory(memoryNeeded);
    falseNode = new (data) FalseNode<T>();
}
```

**Step-by-step**:
1. **Allocate first page**: 16 MB raw memory
2. **Add to pages vector**: Store for later deallocation
3. **Initialize position**: Start at beginning of page
4. **Initialize stamp**: Ready for traversals
5. **Create TrueNode**:
   - Calculate bytes needed for a TrueNode<T> object
   - Call getMemory() to allocate within the page
   - Use placement new to construct object at that address
   - Store pointer for future reuse
6. **Create FalseNode**: Same process
7. **Result**: Manager is ready to create more nodes, with two sentinel nodes pre-allocated

### Node Access Methods

#### `inline Node<T> *makeTrueNode()`
```cpp
inline Node<T> *makeTrueNode() { return trueNode; }
```
- **Complexity**: O(1)
- **Purpose**: Return the cached TrueNode singleton
- **Note**: No allocation occurs; just returns stored pointer

#### `inline Node<T> *makeFalseNode()`
```cpp
inline Node<T> *makeFalseNode() { return falseNode; }
```
- **Complexity**: O(1)
- **Purpose**: Return the cached FalseNode singleton
- **Note**: No allocation occurs; just returns stored pointer

---

## Node Creation Methods

These methods create new nodes of various types. The pattern is always:
1. Calculate bytes needed
2. Call getMemory() to allocate
3. Use placement new to construct
4. Return pointer

### makeBinaryDeterministicOrNode()

```cpp
Node<T> *makeBinaryDeterministicOrNode(
    DataBranch<Node<T> *> &left,
    DataBranch<Node<T> *> &right) {
    
    unsigned memoryNeeded =
        sizeof(BinaryDeterministicOrNode<T, U>) +
        (left.sumFreeUnit() + right.sumFreeUnit()) * sizeof(U);

    uint8_t *data = NodeManager<T>::getMemory(memoryNeeded);
    return new (data) BinaryDeterministicOrNode<T, U>(left, right);
}
```

**How it calculates memory**:
- **Fixed part**: `sizeof(BinaryDeterministicOrNode<T, U>)` - the object header/metadata
- **Variable part**: `(left.sumFreeUnit() + right.sumFreeUnit()) * sizeof(U)` - inline array of variable indices
  - `sumFreeUnit()` returns count of free variables in a branch
  - Each variable index is stored as type U (uint8_t, uint16_t, or uint32_t)
  - All stored inline in the node, not separately allocated

**Example with concrete types**:
```
Assume U = uint8_t (1 byte per variable)
left has 5 free variables
right has 3 free variables

memoryNeeded = sizeof(BinaryDeterministicOrNode<T, uint8_t>) + (5+3) * 1
             = 64 bytes (estimated header) + 8 bytes
             = 72 bytes total

Allocated memory layout:
[BinaryDeterministicOrNode header][var indices array - 8 bytes]
```

### makeUnaryNode()

```cpp
Node<T> *makeUnaryNode(DataBranch<Node<T> *> &branch) {
    unsigned memoryNeeded =
        sizeof(UnaryNode<T, U>) + sizeof(U) * branch.sumFreeUnit();
    uint8_t *data = NodeManager<T>::getMemory(memoryNeeded);
    return new (data) UnaryNode<T, U>(branch);
}
```

**Similar to binary but simpler**:
- Single branch instead of left/right
- Variable array size: `sizeof(U) * branch.sumFreeUnit()`
- Used for negations and other unary operations

### makeDecomposableAndNode()

```cpp
inline Node<T> *makeDecomposableAndNode(Node<T> **sons, unsigned size) {
    if (size == 1) return *sons;  // Optimization: no AND with 1 son

    unsigned memoryNeeded =
        sizeof(DecomposableAndNode<T, U>) + size * sizeof(Node<T> *);
    uint8_t *data = NodeManager<T>::getMemory(memoryNeeded);
    return new (data) DecomposableAndNode<T, U>(size, sons);
}
```

**Peculiarity of AND nodes**:
- **Special case**: If only one son, return that son directly instead of wrapping
- **Rationale**: `A AND A = A`, so avoid creating unnecessary nodes
- **Memory layout**:
  - Fixed header
  - Inline array of `size` pointers to child nodes
  - Each pointer is 8 bytes on 64-bit systems

**Example**:
```
size = 3 children
memoryNeeded = sizeof(DecomposableAndNode header) + 3 * sizeof(Node<T> *)
             = ~40 bytes (header) + 3 * 8 bytes (pointers)
             = 64 bytes
```

---

## Node Analysis Methods

These methods traverse the NNF graph and perform computations. They all use a **virtual function dispatch pattern**.

### Pattern: Function Pointer Array Dispatch

All analysis methods follow this pattern:

```cpp
1. Create array of function pointers: func[TypeNode::count]
2. Populate array with appropriate static methods from node classes
3. Increment m_globalStamp (new traversal)
4. Call: func[node->header.typeNode](parameters)
5. Return result
```

**Why this pattern?**
- Avoids virtual function overhead in tight loops
- Allows static methods to be inlined
- Cache-friendly - function pointers are local variables
- Memoization via globalStamp prevents revisiting nodes

### computeNbModels()

```cpp
T computeNbModels(Node<T> *node, 
                 std::vector<ValueVar> &fixedValue,
                 ProblemManager &problem) {
    // Create array of function pointers
    T(*func[TypeNode::count])
    (Node<T> * node, T(*t[])(), std::vector<ValueVar> &, ProblemManager &, unsigned);

    // Populate with static methods from each node type
    func[TypeNode::TypeDecAndNode] = DecomposableAndNode<T, U>::computeNbModels;
    func[TypeNode::TypeIteNode] = BinaryDeterministicOrNode<T, U>::computeNbModels;
    func[TypeNode::TypeUnaryNode] = UnaryNode<T, U>::computeNbModels;
    func[TypeNode::TypeFalseNode] = FalseNode<T>::computeNbModels;
    func[TypeNode::TypeTrueNode] = TrueNode<T>::computeNbModels;

    // Increment stamp for memoization
    m_globalStamp++;
    
    // Dispatch to appropriate function and return
    return func[node->header.typeNode](
        node, 
        (T(**)())func,      // Pass function array for recursive calls
        fixedValue, 
        problem,
        m_globalStamp);     // Pass stamp to prevent cycles
}
```

**Purpose**: Count number of satisfying assignments

**Parameters**:
- `node`: Root of NNF graph to analyze
- `fixedValue`: Current partial assignment (which variables are assigned)
- `problem`: ProblemManager with problem data
- Returns: Model count of type T

**How dispatch works**:
1. `node->header.typeNode` is an enum value (0-4) indicating node type
2. `func[typeCode]` selects the appropriate static method
3. Static method executes type-specific logic for counting models

**Memoization**:
- Each node tracks the last `m_globalStamp` it was processed with
- If `node->stamp == m_globalStamp`, return cached result
- Avoids recomputing for shared subgraphs

### isSAT()

```cpp
bool isSAT(Node<T> *node, std::vector<ValueVar> &fixedValue) {
    bool (*func[TypeNode::count])(
        Node<T> * node, 
        bool (*t[])(),
        std::vector<ValueVar> &, 
        unsigned);

    func[TypeNode::TypeDecAndNode] = DecomposableAndNode<T, U>::isSAT;
    func[TypeNode::TypeIteNode] = BinaryDeterministicOrNode<T, U>::isSAT;
    func[TypeNode::TypeUnaryNode] = UnaryNode<T, U>::isSAT;
    func[TypeNode::TypeFalseNode] = FalseNode<T>::isSAT;
    func[TypeNode::TypeTrueNode] = TrueNode<T>::isSAT;

    m_globalStamp++;
    return func[node->header.typeNode](
        node, 
        (bool (**)())func, 
        fixedValue,
        m_globalStamp);
}
```

**Purpose**: Test satisfiability (true = at least one model exists)

**Optimization vs computeNbModels**:
- Returns `bool` instead of counting all models
- Can short-circuit early when SAT is found
- Potentially faster for large model counts

### printNNF()

```cpp
void printNNF(Node<T> *node, std::ostream &out) {
    unsigned (*func[TypeNode::count])(
        Node<T> * node, 
        unsigned (*t[])(),
        std::ostream &, 
        unsigned &, 
        unsigned);
        
    func[TypeNode::TypeDecAndNode] = DecomposableAndNode<T, U>::printNNF;
    func[TypeNode::TypeIteNode] = BinaryDeterministicOrNode<T, U>::printNNF;
    func[TypeNode::TypeUnaryNode] = UnaryNode<T, U>::printNNF;
    func[TypeNode::TypeFalseNode] = FalseNode<T>::printNNF;
    func[TypeNode::TypeTrueNode] = TrueNode<T>::printNNF;

    m_globalStamp++;
    unsigned idx = 1;
    func[node->header.typeNode](node, (unsigned (**)())func, out, idx, m_globalStamp);
}
```

**Purpose**: Convert NNF graph to human-readable string representation

**Parameters**:
- `out`: Output stream (stdout, file, string buffer)
- `idx`: Node numbering counter (passed by reference, incremented by methods)
- `m_globalStamp`: Prevents printing shared subgraphs multiple times

**Output format**: Typical NNF format with node IDs and operators

### deallocate()

```cpp
void deallocate(Node<T> *node) {
    void (*func[TypeNode::count])(
        Node<T> * node, 
        void (*t[])(), 
        unsigned int);

    func[TypeNode::TypeDecAndNode] = DecomposableAndNode<T, U>::deallocate;
    func[TypeNode::TypeIteNode] = BinaryDeterministicOrNode<T, U>::deallocate;
    func[TypeNode::TypeUnaryNode] = UnaryNode<T, U>::deallocate;
    func[TypeNode::TypeFalseNode] = FalseNode<T>::deallocate;
    func[TypeNode::TypeTrueNode] = TrueNode<T>::deallocate;

    m_globalStamp++;
    func[node->header.typeNode](node, (void (**)())func, m_globalStamp);
}
```

**Purpose**: Recursively deallocate member data of a node and its descendants

**Important nuance**:
- Does NOT deallocate the node memory itself (that's handled by page deallocation)
- Does deallocate member variables within each node if they use dynamic memory
- Example: If a node has `std::vector<int> m_data`, this calls `m_data.~vector()` via the static method

**Why separate from NodeManager destructor**:
- Allows selective cleanup of subtrees
- Useful if you want to free intermediate results without destroying entire manager
- Could be used to implement garbage collection of unused subgraphs

---

## Summary: Data Flow Example

Imagine creating a small NNF formula:

```
// Create manager for 50-variable problem
NodeManager<uint64_t> *mgr = NodeManager<uint64_t>::makeNodeManager(50);
// → Returns NodeManagerTyped<uint64_t, uint8_t>

// Get true/false nodes
Node *trueNode = mgr->makeTrueNode();  // O(1) - just returns stored pointer
Node *falseNode = mgr->makeFalseNode();// O(1)

// Create a unary node (for negation)
DataBranch left = ...;  // Some branch with 3 free vars
Node *notNode = mgr->makeUnaryNode(left);
// → getMemory() allocates: sizeof(UnaryNode) + 3*sizeof(uint8_t) bytes
// → Uses placement new to construct at allocated address
// → Returns pointer to new node

// Compute models
uint64_t models = mgr->computeNbModels(notNode, fixedValue, problem);
// → Sets up function array with 5 entries
// → Increments m_globalStamp to 1
// → Dispatches to UnaryNode<uint64_t, uint8_t>::computeNbModels
// → That method recursively calls children, using func array
// → Returns total model count
```

**Memory state after operations**:
```
Page 0 (16 MB):
[TrueNode  ][FalseNode ][UnaryNode  ][...more nodes...][              ]
^pos=0     ^pos=32    ^pos=96     ^pos=position    ^                ^
                                                   current position   PAGE_SIZE
```

When page fills and new allocation needed:
```
Page 0 (16 MB): FULL - will be deleted on NodeManager destruction
Page 1 (16 MB): [NewNode   ][...more...][              ]
                ^pos=0    ^pos=position ^             ^
                                        current pos   PAGE_SIZE

m_memoryPages = [page0_ptr, page1_ptr]
```

---

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| makeTrueNode() | O(1) | Constant cached pointer |
| makeFalseNode() | O(1) | Constant cached pointer |
| makeDecomposableAndNode() | O(size) | Actually O(size) due to pointer copy in constructor |
| makeBinaryDeterministicOrNode() | O(left.freeUnits + right.freeUnits) | Copy free vars to node |
| getMemory() | O(1) amortized | Linear allocation, page allocation O(1) |
| computeNbModels() | O(nodes) | With memoization via globalStamp |
| isSAT() | O(nodes) | May short-circuit early |
| deallocate() | O(nodes) | Recursive cleanup |

---

## Design Patterns Used

1. **Factory Method**: `makeNodeManager()` decides concrete type at runtime
2. **Template Specialization**: Different U types for different problem sizes
3. **Arena Allocation**: Page-based memory to reduce fragmentation
4. **Function Pointer Dispatch**: Virtual-like behavior without vtable overhead
5. **Memoization via Global Timestamp**: Avoid revisiting nodes in traversal
6. **Placement New**: Construct objects in pre-allocated memory
7. **Singleton Pattern**: TrueNode and FalseNode cached for reuse

---

## Key Insights

1. **Memory efficiency**: Page-based allocation with type selection minimizes memory overhead
2. **Cache locality**: Sequential allocation keeps related nodes close in memory
3. **No per-node overhead**: No individual allocation/deallocation means less fragmentation
4. **Flexible model counting**: Type T allows different precision (32-bit, 64-bit, arbitrary)
5. **Fast traversals**: globalStamp memoization prevents redundant computation
6. **Clean separation**: Base class handles memory, derived class handles operations
