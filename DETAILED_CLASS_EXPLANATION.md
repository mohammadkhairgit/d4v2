# AI Generated Documentation

**Disclaimer**: This documentation was generated using AI and may contain inaccuracies or omissions. It should always be viewed alongside the source code it documents.

# Extremely Detailed Class Explanation

This document provides precise, exhaustive explanations of five critical classes in the d4 CNF compiler.

---

## 1. ProblemManagerCnf

**Location:** `src/problem/cnf/ProblemManagerCnf.hpp` and `.cpp`

**Role:** Stores and manages the static CNF formula, its metadata, and structural properties. This is the "problem description" layer—it does NOT change during recursion.

### Fields

#### `std::vector<std::vector<Lit>> m_clauses`
- **Exact meaning:** The entire CNF formula as a list of clauses.
- **Concrete example:** If the CNF is `(x1 ∨ ¬x2) ∧ (¬x1 ∨ x3) ∧ (x2 ∨ x3)`, then:
  - `m_clauses[0] = [Lit(1, false), Lit(2, true)]` (x1 ∨ ¬x2)
  - `m_clauses[1] = [Lit(1, true), Lit(3, false)]` (¬x1 ∨ x3)
  - `m_clauses[2] = [Lit(2, false), Lit(3, false)]` (x2 ∨ x3)
- **When modified:** Only during initialization (via `getConditionedFormula`), which creates a NEW ProblemManagerCnf object.
- **Not modified during:** Main recursion in DpllStyleMethod.

#### `std::vector<std::vector<Lit>> m_learnt`
- **Exact meaning:** Learned clauses, typically used in incremental solving or preprocessing.
- **Concrete example:** If the SAT solver derives that `(x1 ∨ x3)` must be true, it might add that as a learned clause.
- **Current usage:** Mostly reserved; not actively used in the main dDNNF compilation path.
- **Initialized by:** `initSolver(ProblemManager &p, std::vector<std::vector<Lit>>& learnt)` if preprocessing added clauses.

#### `unsigned m_nbVar` (inherited from ProblemManager)
- **Exact meaning:** Total count of variables, numbered 1 to m_nbVar (variable 0 is unused).
- **Concrete example:** If there are 100 variables, `m_nbVar = 100`.
- **Critical invariant:** All clauses use variables in the range [1, m_nbVar].

#### `std::vector<double> m_weightLit` (inherited)
- **Exact meaning:** Weight of each literal for weighted model counting.
- **Array layout:** Size is `2 * (m_nbVar + 1)`. Index into this array using `Lit::intern()`.
  - `m_weightLit[Lit(v, false).intern()] = m_weightLit[2*v]` is the weight of the positive literal.
  - `m_weightLit[Lit(v, true).intern()] = m_weightLit[2*v + 1]` is the weight of the negative literal.
- **Concrete example:** If variable 5 has weights `m_weightLit[10] = 2.0` and `m_weightLit[11] = 3.0`:
  - Every model where x5 is true contributes a factor of 2.0.
  - Every model where x5 is false contributes a factor of 3.0.
- **For unweighted:** All weights are 1.0.

#### `std::vector<double> m_weightVar` (inherited)
- **Exact meaning:** Precomputed sum of the two literal weights for each variable.
- **Exact computation:** `m_weightVar[v] = m_weightLit[2*v] + m_weightLit[2*v + 1]`.
- **Purpose:** Optimization; saves recalculating this sum every time.
- **Concrete example:** If `m_weightLit[10] = 2.0` and `m_weightLit[11] = 3.0`, then `m_weightVar[5] = 5.0`.

#### `std::vector<Var> m_selected` (inherited)
- **Exact meaning:** Variables that are "projected" (also called "selected").
- **Concrete example:** If we do projected model counting and want to count models only for variables {3, 5, 7}, then `m_selected = [3, 5, 7]`.
- **What it means:** After `normalize()` is called, this becomes a prefix: `m_selected = [1, 2, 3, ...]` (first k variables).
- **Why renamed to "selected":** In the dDNNF context, these are the "decision-relevant" variables—the ones that appear as explicit nodes in the compiled structure.
- **If empty:** Then ALL variables are projected (classic model counting, not projected).

#### `std::vector<Var> m_maxVar` and `std::vector<Var> m_indVar` (inherited)
- **Exact meaning:** Auxiliary variable classification from preprocessing. Not actively used in basic dDNNF compilation.
- **Purpose:** For other compilation modes or preprocessing passes.

#### `std::vector<Lit> m_gmap` (inherited)
- **Exact meaning:** Global literal mapping, used for equivalence and substitution during normalization.
- **When used:** After `getConditionedFormula`, if variable renumbering happens.

#### `unsigned m_nbFreeVars` (inherited)
- **Exact meaning:** Count of variables that are not constrained by any clause.
- **Concrete example:** If variable 42 appears in no clause, it contributes to free variables.
- **Implication:** Free variables increase the model count by a factor of 2 each (unweighted) or their weight sum (weighted).

#### `bool m_isUnsat` (inherited)
- **Exact meaning:** Flag indicating whether the formula is unsatisfiable.
- **Set to true by:** `getUnsatProblem()` or directly by preprocessing.
- **Used by:** DpllStyleMethod to short-circuit recursion.

### Key Functions

#### `ProblemManagerCnf(std::string &nameFile)`
- **Exact behavior:**
  1. Parses a DIMACS CNF file from `nameFile`.
  2. Calls `ParserDimacs::parse_DIMACS(nameFile, this)` to populate `m_clauses` and `m_weightLit`.
  3. Computes `m_weightVar[i] = m_weightLit[2*i] + m_weightLit[2*i+1]` for each variable.
  4. Calls `normalize()` to ensure selected variables come first.
- **Return:** A fully initialized `ProblemManagerCnf` object.

#### `void normalize()`
- **Exact behavior:**
  1. If `m_selected.size() == m_nbVar` (all variables selected), do nothing.
  2. Otherwise:
     - Build a `marked` array marking which variables are in `m_selected`.
     - Build a `remap` array:
       - First k indices (1 to k) map to selected variables.
       - Indices k+1 to n map to non-selected variables.
     - Rewrite every literal in every clause using the `remap` table.
     - Reset `m_selected = [1, 2, ..., k]` (canonical prefix).
- **Example:**
  - Original: `m_selected = [5, 2]`, `m_nbVar = 4`, clauses = `[(1, 2), (4, 5)]`.
  - After normalization: `m_selected = [1, 2]`, clauses = `[(3, 1), (4, 2)]`.
  - Why: Variable 2 becomes 1, variable 5 becomes 2, variable 1 becomes 3, variable 4 stays 4.
- **Invariant after:** `isSelected(v)` test `v <= m_nbProj` is valid.

#### `void normalizeInner()`
- **Exact behavior:** Sort the literals inside each clause by variable ID (sign ignored).
- **Example:** Clause `[Lit(5, true), Lit(2, false), Lit(8, true)]` becomes `[Lit(2, false), Lit(5, true), Lit(8, true)]`.
- **Purpose:** Deterministic representation for hashing, comparison, and printing.

#### `ProblemManager *getConditionedFormula(std::vector<Lit> &units)`
- **Exact behavior:**
  1. Create a new `ProblemManagerCnf` object `ret` copying this problem.
  2. For each literal in `units`:
     - Add it as a unit clause to `ret`.
     - Mark its variable's value in a local array.
  3. For each original clause:
     - If the clause is satisfied by any literal in `units`, skip it (do NOT add it).
     - Otherwise, iterate the clause and remove any literals whose negation is in `units`.
     - Add the simplified clause (or nothing if it becomes empty—but that would make the formula UNSAT, which is handled separately).
  4. Return `ret`.
- **Concrete example:**
  - Original: `[(x1 ∨ ¬x2), (¬x1 ∨ x3)]`.
  - Units: `[x1]`.
  - Result clauses: `[x1]` (unit), `[x3]` (simplified from ¬x1 ∨ x3).
- **Critical detail:** This function creates a NEW problem object; it does NOT modify the original.
- **Why "Conditioned":** This is precisely the conditioning operation in probability theory: $P(F | L_1 \wedge L_2 \wedge \ldots)$.

---

## 2. SpecManagerCnf

**Location:** `src/specs/cnf/SpecManagerCnf.hpp` and `.cpp`

**Role:** The "current formula state tracker." It maintains the original CNF clauses alongside dynamic state that evolves during recursion. It answers questions like "which variables are still connected?" and "which clauses are currently unsatisfied?"

### Fields

#### `std::vector<std::vector<Lit>> m_clauses`
- **Exact meaning:** A copy of the original CNF clauses from the problem.
- **Critical:** These vectors are NOT rewritten during recursion. They stay fixed.
- **Why kept:** So the spec manager can answer structural questions without rebuilding.

#### `std::vector<int> m_clausesNotBin`
- **Exact meaning:** List of clause indices where each clause has size > 2.
- **Concrete example:** If clauses are `[(x1, ¬x2), (x3, x4, x5), (x1, x2, x3, x4)]`, then `m_clausesNotBin = [1, 2]` (indices of clauses with 3+ literals).
- **Why tracked:** Binary clauses are handled specially in propagation. This list lets us skip binary clauses when we only care about long clauses.
- **Populated in constructor:** By iterating through all clauses and storing indices where `size > 2`.

#### `unsigned m_maxSizeClause`
- **Exact meaning:** The size of the largest clause.
- **Concrete example:** If the largest clause has 7 literals, `m_maxSizeClause = 7`.
- **Purpose:** Informational; used for diagnostics and heuristics.

#### `std::vector<lbool> m_currentValue`
- **Exact meaning:** The current partial assignment of variables during the current recursion.
- **Array layout:** Size is `m_nbVar + 1`. Index by variable number (0 is unused).
- **Values:**
  - `l_Undef` (unassigned)
  - `l_True` (assigned to true)
  - `l_False` (assigned to false)
- **Concrete example:** After assigning x3 = true and x7 = false:
  - `m_currentValue[3] = l_True`
  - `m_currentValue[7] = l_False`
  - `m_currentValue[5] = l_Undef` (still unassigned)
- **Who updates it:** `SpecManagerCnfDyn::preUpdate()` sets values, and `postUpdate()` unsets them.
- **Lifetime:** Resets to all `l_Undef` at the start of each independent component.

#### `std::vector<SpecClauseInfo> m_infoClauses`
- **Exact meaning:** Per-clause metadata.
- **SpecClauseInfo structure:**
  ```cpp
  struct SpecClauseInfo {
    unsigned nbSat;      // How many literals in this clause are TRUE?
    unsigned nbUnsat;    // How many literals in this clause are FALSE?
    Lit watcher;         // An arbitrary unassigned literal (for fast checking)
  };
  ```
- **Concrete example:** Clause `(x1 ∨ ¬x2 ∨ x3)` with x1=true, x2=true, x3=undef:
  - `nbSat = 1` (x1 is true)
  - `nbUnsat = 1` (¬x2 is false)
  - `watcher = Lit(3, false)` (x3 is unassigned, so it's a good witness)
- **nbSat semantics:** If `nbSat > 0`, the clause is SATISFIED (needs at least one true literal).
- **nbUnsat semantics:** If `nbUnsat == clause.size()`, the clause is FALSIFIED (all literals are false).
- **watcher semantics:** A fast way to check if a clause is still relevant. If the watcher is unassigned, the clause might still become relevant.
- **Updated by:** `preUpdate()` and `postUpdate()` as assignments change.

#### `std::vector<bool> m_inCurrentComponent`
- **Exact meaning:** Temporary marking array used during `computeConnectedComponent()`.
- **Array layout:** Size `m_nbVar + 1`, indexed by variable.
- **Usage:** Set to true for variables in the current component being analyzed, then reset to false after analysis.
- **Concrete example:** During union-find traversal, if we're analyzing component C1 containing {3, 5, 7}, we set `m_inCurrentComponent[3] = m_inCurrentComponent[5] = m_inCurrentComponent[7] = true`.
- **Reused:** Between multiple calls to avoid repeated allocation.

#### `std::vector<DataOccurrence> m_occurrence`
- **Exact meaning:** Occurrence lists for each literal.
- **Array layout:** Size is `2 * (m_nbVar + 1)` (one entry per literal, accounting for both signs).
  - Index by `Lit(v, sign).intern()`.
- **DataOccurrence structure:**
  ```cpp
  struct DataOccurrence {
    int *bin;        // Pointer to start of binary clause indices
    unsigned nbBin;  // Count of binary clauses
    int *notBin;     // Pointer to start of non-binary clause indices
    unsigned nbNotBin; // Count of non-binary clauses
  };
  ```
- **Concrete example:** For literal x3 (positive), `m_occurrence[Lit(3, false).intern()]` contains:
  - Indices of all binary clauses containing x3
  - Indices of all non-binary clauses containing x3
- **Why separated:** Binary vs. non-binary clause handling is different in propagation.
- **Memory layout:** All indices are stored in a single allocated buffer `m_dataOccurrenceMemory`, with pointers carved out during construction.

#### `int *m_dataOccurrenceMemory`
- **Exact meaning:** Single large heap allocation holding all occurrence indices.
- **Total size:** Sum of all clause sizes (counted during constructor).
- **Reason:** Cache efficiency; all occurrence data is in one contiguous block.
- **Freed in:** Destructor.

#### `std::vector<bool> m_markView`
- **Exact meaning:** Temporary marking array for union-find during connected-component analysis.
- **Array layout:** Size `m_clauses.size()`, indexed by clause index.
- **Usage:** Marks which clauses have been visited during connected-component traversal.
- **Reset:** Cleared after each `computeConnectedComponent()` call.

#### `std::vector<int> m_mustUnMark`
- **Exact meaning:** List of clause indices that were marked during connected-component analysis.
- **Concrete example:** If during traversal we mark clauses {0, 3, 7, 12}, then `m_mustUnMark = [0, 3, 7, 12]`.
- **Purpose:** Efficient cleanup. Instead of iterating all clauses to unmark, we iterate only the marked ones.

#### `std::vector<Var> m_tmpVecVar`
- **Exact meaning:** Temporary working vector.
- **Used by:** Various algorithms (e.g., connected-component traversal) to avoid repeated allocations.

#### `std::vector<int> m_idxComponent`
- **Exact meaning:** Maps variables to their component index.
- **Array layout:** Size `m_nbVar + 1`.
- **Concrete example:** If x3, x5, x7 are in component 0, and x1, x2 are in component 1:
  - `m_idxComponent[3] = m_idxComponent[5] = m_idxComponent[7] = 0`
  - `m_idxComponent[1] = m_idxComponent[2] = 1`

#### `std::vector<InfoCluster> m_infoCluster`
- **Exact meaning:** Union-find structure for connected-component detection.
- **Array layout:** Size `m_nbVar + m_clauses.size() + 1`.
  - Indices 0 to m_nbVar represent variables.
  - Indices m_nbVar+1 to m_nbVar+m_clauses.size() represent clauses.
- **InfoCluster structure:**
  ```cpp
  struct InfoCluster {
    Var parent;      // Union-find parent
    unsigned size;   // Size of the component
    int pos;         // Position in output array (-1 = not finalized)
  };
  ```
- **parent semantics:** Variable v has parent `m_infoCluster[v].parent`. If `parent == v`, then v is a root.
- **size semantics:** Only meaningful at root; tells how many variables are in this component.
- **pos semantics:** After union-find completes, `pos` stores the index in the output component array. Reset to -1 after each call.

#### `double cleaness`
- **Exact meaning:** Informational; related to clause quality metrics (not actively used in basic compilation).

#### `unsigned m_nbProj` (inherited)
- **Exact meaning:** Number of projected (selected) variables.
- **Concrete example:** If there are 100 variables and 20 are projected, `m_nbProj = 20`.
- **Used by:** `isSelected(v)` test: `return v <= m_nbProj`.

#### `unsigned m_nbVar` (inherited)
- **Exact meaning:** Total number of variables.

### Key Functions

#### `SpecManagerCnf(ProblemManager &p)`
**Constructor. This is executed once at the start of DpllStyleMethod.**

- **Step 1: Copy clauses**
  - Dynamic cast `p` to `ProblemManagerCnf`.
  - Store reference to its clauses: `m_clauses = pcnf.getClauses()`.
  - If cast fails, error.

- **Step 2: Build occurrence lists**
  - For each clause index i:
    - If `m_clauses[i].size() > 2`, append i to `m_clausesNotBin`.
    - For each literal in the clause:
      - Append clause index i to the temporary occurrence list for that literal.
    - Track the maximum clause size.
  - Count total literal appearances: sum of all clause sizes. This is the size of `m_dataOccurrenceMemory`.

- **Step 3: Allocate and layout occurrence memory**
  - Allocate `m_dataOccurrenceMemory` to hold all clause indices.
  - For each literal:
    - Separate its clause list into binary and non-binary.
    - Store binary indices first, then non-binary indices.
    - Set `m_occurrence[lit].bin` to point to binary start.
    - Set `m_occurrence[lit].notBin` to point to non-binary start.

- **Step 4: Initialize per-variable state**
  - `m_currentValue.resize(m_nbVar + 1, l_Undef)` — all unassigned.
  - `m_inCurrentComponent.resize(m_nbVar + 1, false)` — no variables in component yet.
  - `m_idxComponent.resize(m_nbVar + 1, 0)` — all in component 0 initially.

- **Step 5: Initialize per-clause state**
  - `m_infoClauses.resize(m_clauses.size())` — one per clause.
  - For each clause, set `m_infoClauses[i].watcher = m_clauses[i][0]` (first literal as watcher).
  - `m_infoClauses[i].nbSat = 0`, `nbUnsat = 0` (initially, no assignments).

- **Step 6: Initialize union-find structure**
  - `m_infoCluster.resize(m_nbVar + m_clauses.size() + 1)` with default `{0, 0, -1}`.

- **Step 7: Copy projection info**
  - `m_nbProj = p.getNbSelectedVar()`.

#### `int computeConnectedComponent(std::vector<std::vector<Var>> &varCo, std::vector<Var> &setOfVar, std::vector<Var> &freeVar)`

**This is one of the most important functions. It partitions unassigned variables into independent subproblems using union-find.**

- **Input:**
  - `setOfVar`: The variables we want to decompose (typically all variables or a current component).
  - `varCo` (output): Will be populated with one vector per component.
  - `freeVar` (output): Variables in `setOfVar` that appear in no clause.

- **Algorithm:**

  1. **Initialize union-find for input variables:**
     ```
     for each v in setOfVar:
       m_infoCluster[v].parent = v
       m_infoCluster[v].size = 1
     ```

  2. **Build components by traversing clauses:**
     ```
     for each unassigned variable v in setOfVar:
       rootV = v
       for each literal l = v or l = ¬v:
         for each clause index idx in occurrence[l]:
           if not marked[idx]:
             mark[idx] = true
             union(rootV, idx) — connect v with the clause
           else:
             rootW = find(idx)  — find what v this clause connects to
             if rootV != rootW:
               union(rootV, rootW)  — merge two roots
     ```
     The key insight: If a clause contains literals from variables v1 and v2, those variables are in the same component.

  3. **Extract components:**
     ```
     for each unassigned v in setOfVar:
       rootV = find(v)
       if v is its own root and size == 1:
         add v to freeVar  — no clauses touch this variable
       else:
         add v to component[rootV]
     ```

  4. **Cleanup:**
     - Clear all marks.
     - Reset `pos` to -1 for all roots.

- **Concrete example:**
  - Variables: {1, 2, 3, 4, 5}
  - Clauses: `[(x1, x2), (x2, x3), (x4)]`
  - After traversal:
    - x1, x2, x3 are connected (via the clauses).
    - x4 is alone (single-literal clause).
    - x5 is free (appears in no clause).
  - Output: `varCo = [[1, 2, 3], [4]]`, `freeVar = [5]`.

#### `void preUpdate(std::vector<Lit> &lits)` (from SpecManagerCnfDyn)

**Called when new literals are assigned. Updates the clause metadata to reflect the new partial assignment.**

**Input:** `lits` — newly assigned literals.

**Algorithm:**

1. **For each newly assigned literal l:**
   - Set `m_currentValue[l.var()] = (l.sign() ? l_False : l_True)`.
   
2. **For each clause containing l (positive satisfaction):**
   - Increment `m_infoClauses[clause].nbSat` — clause is now closer to being satisfied.
   - For each literal in the clause:
     - If that literal is still unassigned, remove it from the occurrence list (optimization; it's no longer a free literal in this clause).

3. **For each clause containing ¬l (negative satisfaction):**
   - Increment `m_infoClauses[clause].nbUnsat` — clause has another false literal.
   - If the watcher for this clause is ¬l, mark the clause for watcher re-assignment.

4. **Re-assign watchers:**
   - For each marked clause:
     - If it's not already satisfied, find an unassigned literal and set it as the new watcher.

**Concrete example:**
- Clause: `(x1 ∨ ¬x2 ∨ x3)`
- Assign x1 = true:
  - `nbSat++` (now 1)
  - If x2 or x3 are unassigned, they're removed from occurrence.
- Assign x2 = false (which makes ¬x2 true):
  - `nbSat++` (now 2, clause is definitely satisfied)
  - Remove all unassigned literals from occurrence.
- Assign x3 = true (complementary, makes x3 true):
  - No effect; clause is already satisfied.

#### `void postUpdate(std::vector<Lit> &lits)` (from SpecManagerCnfDyn)

**Called when backtracking. Undoes the changes made by preUpdate in reverse order.**

**Algorithm:**

1. **Iterate lits in reverse:**
   - For each literal l (in reverse order):
     - Decrement `nbSat` and `nbUnsat` for affected clauses.
     - Re-add literals to occurrence lists.
     - Set `m_currentValue[l.var()] = l_Undef`.

**Purpose:** Restore the state so a different branch can be tried.

---

## 3. Solver (MiniSat)

**Location:** `src/solvers/cnf/minisat/Solver.hpp`

**Role:** The low-level SAT engine. This is the actual DPLL/CDCL solver that decides satisfiability and propagates units.

### Critical Fields

#### `vec<Lit> trail`
- **Exact meaning:** The assignment stack. Every literal assigned (by decision or propagation) is pushed here in order.
- **Concrete example:** If we decide x3=true, then propagate x1=false, then decide x5=true:
  - `trail = [Lit(3, false), Lit(1, true), Lit(5, false)]`
- **Why important:** This is the complete history of decisions and propagations.
- **Not popped individually:** Only reset via `cancelUntil(level)`.

#### `vec<int> trail_lim`
- **Exact meaning:** Marks the trail position at each decision level.
- **Concrete example:**
  - Decision level 0: `trail_lim = []` (empty; no decisions yet)
  - After first decision: `trail_lim = [0]` (trail position before this decision)
  - After second decision: `trail_lim = [0, 2]` (trail positions before each decision)
- **Used by:** `cancelUntil(level)` to backtrack to a specific decision level.

#### `vec<lbool> assigns`
- **Exact meaning:** Current value of each variable.
- **Array layout:** Size `nVars()`. Index by variable id.
- **Values:** `l_True`, `l_False`, or `l_Undef`.
- **Concrete example:** After assigning x3=true and x5=false:
  - `assigns[3] = l_True`
  - `assigns[5] = l_False`
- **Relationship to trail:** The `assigns` array is the fast lookup. The `trail` is the sequence.

#### `vec<CRef> clauses`
- **Exact meaning:** Original problem clauses (as clause references).
- **Why not `vec<vec<Lit>>`:** MiniSat uses a single allocator for all clauses to optimize memory layout.

#### `vec<CRef> learnts`
- **Exact meaning:** Learned clauses (derived during conflict analysis).
- **Built by:** `analyze()` when a conflict occurs.

#### `OccLists<Lit, vec<Watcher>, WatcherDeleted> watches`
- **Exact meaning:** Two-literal watching. For each literal, a list of clauses watching that literal.
- **Concrete example:** If clause `(x1 ∨ ¬x2 ∨ x3)` is watching x1 and x2:
  - `watches[Lit(1, false)]` contains a `Watcher` pointing to this clause.
  - `watches[Lit(2, true)]` contains a `Watcher` pointing to this clause.
- **Purpose:** Fast propagation. When x1 is assigned true, we only look at clauses in `watches[Lit(1, false)]`.

#### `vec<double> activity`
- **Exact meaning:** Variable activity scores for the decision heuristic.
- **Semantics:** Higher activity = more likely to make decisions on.
- **Updated by:** `varBumpActivity()` when a variable appears in a conflict.
- **Decayed by:** `varDecayActivity()` periodically.

#### `vec<double> scoreActivity`
- **Exact meaning:** How many times each variable appeared in a conflict.
- **Semantics:** This is used by d4's heuristics to guide partitioning and branching.

#### `vec<Lit> assumptions`
- **Exact meaning:** The current set of assumptions.
- **Concrete example:** If we want to solve under assumptions {x3, ¬x5}, then `assumptions = [Lit(3, false), Lit(5, true)]`.
- **Used by:** `solveWithAssumptions()` to enforce these as top-level facts.

### Key Functions

#### `bool solveWithAssumptions(vec<Lit> &assums, vec<Lit> &unit, bool model = false)`

**Main solving entry point called by DpllStyleMethod.**

- **Exact behavior:**
  1. Set `assumptions = assums`.
  2. Call `solve_()` to run the main DPLL loop.
  3. If satisfiable:
     - If `model == true`, copy the model to `this->model`.
     - Call `whichAreUnits(setOfVar, unit)` (wrapper function) to extract unit literals.
     - Return true.
  4. If unsatisfiable:
     - Return false.

- **Unit extraction:** The `unit` vector is filled by examining the trail and finding literals at decision level 0.

#### `bool propagateAssumption()`

**Performs unit propagation on the assumption stack without search.**

- **Exact behavior:**
  1. For each assumption not yet in the trail:
     - Enqueue it.
     - Call `propagate()` to forward propagate.
     - If conflict, return false.
  2. If no conflict, return true.

#### `lbool search(int nof_conflicts)`

**Core DPLL search with conflict-driven clause learning.**

- **Exact behavior:**
  1. Loop until we reach the conflict budget:
     - Pick an unassigned variable using `pickBranchLit()`.
     - Push it as a decision.
     - Propagate.
     - If conflict:
       - Analyze the conflict to derive a learned clause.
       - Backtrack appropriately.
  2. If all variables assigned or propagation complete, return `l_True` (satisfiable).
  3. If timeout, return `l_Undef`.

#### `CRef propagate()`

**Boolean constraint propagation (BCP).**

- **Exact behavior:**
  1. Iterate through the trail, starting from `qhead` (the last propagated literal).
  2. For each literal l just set to true:
     - Look at clauses watching `¬l`.
     - For each such clause:
       - If the clause is satisfied (one of its watched literals is true), skip.
       - Otherwise, find a new literal to watch.
       - If no new literal found, the other watched literal must be true (unit).
       - If no literal is true, conflict!
  3. If all propagation succeeds, return `CRef_Undef` (no conflict).
  4. If conflict found, return the conflicting clause reference.

---

## 4. WrapperMinisat

**Location:** `src/solvers/cnf/WrapperMinisat.hpp` and `.cpp`

**Role:** Adapter layer between d4's abstract interface (`WrapperSolver`) and the concrete MiniSat solver.

### Fields

#### `minisat::Solver s`
- **Exact meaning:** The embedded MiniSat solver instance.
- **Lifetime:** Lives as long as the WrapperMinisat object.

#### `minisat::vec<minisat::Var> m_setOfVar_m`
- **Exact meaning:** Temporary vector for MiniSat variable representation.
- **Used by:** `solve(std::vector<Var> &setOfVar)` to convert d4's variable list to MiniSat format.
- **Why separate:** MiniSat uses its own `vec<>` template; we need a temporary to interface.

#### `std::vector<Lit> m_assumption`
- **Exact meaning:** Mirror of the current assumption stack in d4 types.
- **Dual:** There's also `s.assumptions` in MiniSat types.
- **Why both:** DpllStyleMethod works with d4 types; the wrapper maintains both for conversion.

#### `std::vector<lbool> m_model`
- **Exact meaning:** The last satisfying model in d4 types.
- **Dual:** `s.model` is the MiniSat version.
- **Converted from:** `s.model` after each solve via `getModel()`.

#### `bool m_activeModel`
- **Exact meaning:** Whether the current `m_model` is still valid.
- **When true:** The model reflects the current set of assumptions.
- **When false:** Must re-solve to get a fresh model.
- **Set to false when:** Backtracking or assumptions change.
- **Set to true when:** After a successful solve with model requested.

#### `bool m_needModel`
- **Exact meaning:** Whether the caller wants the model preserved.
- **Set by:** `setNeedModel(bool b)`.
- **Used by:** `pushAssumption()` to decide whether to maintain `m_activeModel`.

#### `std::vector<char> m_isInAssumption` (inherited from WrapperSolver)
- **Exact meaning:** Per-variable state indicating whether the variable is in the current assumption and with what polarity.
- **Array layout:** Size `m_nbVar + 1`.
- **Values:**
  - `0` — not in assumption.
  - `1` — assumed positive (variable = true).
  - `2` — assumed negative (variable = false).
- **Used by:** `isInAssumption(Lit l)` to quickly check if an assumption is active.

### Key Functions

#### `void initSolver(ProblemManager &p)`

**Initializes the solver with a problem.**

- **Exact behavior:**
  1. Dynamic cast `p` to `ProblemManagerCnf`.
  2. Ensure MiniSat has enough variables:
     ```cpp
     while ((unsigned)s.nVars() <= pcnf.getNbVar())
       s.newVar();
     ```
  3. Add all clauses from the problem:
     ```cpp
     for (auto &cl : clauses) {
       minisat::vec<minisat::Lit> lits;
       for (auto &l : cl)
         lits.push(minisat::mkLit(l.var(), l.sign()));
       s.addClause(lits);
     }
     ```
  4. Initialize model vector and assumption tracking:
     ```cpp
     m_model.resize(pcnf.getNbVar() + 1, l_Undef);
     m_isInAssumption.resize(p.getNbVar() + 1, 0);
     ```

#### `bool solve(std::vector<Var> &setOfVar)`

**Solve under the current assumption, focusing on a subset of variables.**

- **Exact behavior:**
  1. Convert `setOfVar` to MiniSat format in `m_setOfVar_m`.
  2. Call `s.rebuildWithConnectedComponent(m_setOfVar_m)` to optimize the solver's internal state.
  3. Call `s.solveWithAssumptions()` (MiniSat internal) to solve.
  4. Cache result in `m_activeModel`.
  5. Return satisfiability result.

#### `bool solve()`

**Solve under current assumptions without variable focus.**

- **Exact behavior:**
  1. Call `s.rebuildWithAllVar()`.
  2. Call `s.solveWithAssumptions()`.
  3. Cache and return result.

#### `void pushAssumption(Lit l)`

**Add a literal to the assumption stack.**

- **Exact behavior:**
  1. Mark in MiniSat's assumption vector: `s.assumptions.push(minisat::mkLit(l.var(), l.sign()))`.
  2. Mirror in d4 vector: `m_assumption.push_back(l)`.
  3. Update assumption tracking:
     ```cpp
     m_isInAssumption[l.var()] = 1 + l.sign();
     ```
  4. If model is active:
     - Check if the new assumption is consistent with the current model.
     - If not, mark model as inactive: `m_activeModel = false`.

#### `void popAssumption(unsigned count)`

**Remove the last `count` assumptions.**

- **Exact behavior:**
  1. For each assumption being removed:
     - Clear the assumption-tracking bit: `m_isInAssumption[lit.var()] = 0`.
  2. Resize `m_assumption` and `s.assumptions` to remove the literals.
  3. Call `s.cancelUntil(new_level)` to backtrack the MiniSat trail.

#### `std::vector<Lit> &getAssumption()`

**Return the current assumption list.**

- **Exact behavior:** Return reference to `m_assumption`.

#### `void setAssumption(std::vector<Lit> &assums)`

**Replace the entire assumption stack.**

- **Exact behavior:**
  1. Pop all existing assumptions.
  2. For each new assumption, push it.

#### `bool decideAndComputeUnit(Lit l, std::vector<Lit> &units)`

**Try assigning a literal and extract propagated units.**

- **Exact behavior:**
  1. If `l` is already assigned:
     - If consistent, add to units and return true.
     - If conflicting, return false.
  2. Otherwise:
     - Remember current trail size: `posTrail`.
     - Create new decision level.
     - Enqueue `l` unchecked.
     - Call `s.propagate()`.
     - If conflict:
       - Analyze to extract conflicting clause.
       - Cancel back.
       - Return false (assignment causes conflict).
     - Otherwise:
       - Collect all literals added to trail after posTrail as units.
       - Cancel back.
       - Return true.

#### `void whichAreUnits(std::vector<Var> &component, std::vector<Lit> &units)`

**Extract current unit literals from the trail.**

- **Exact behavior:**
  1. Iterate the MiniSat trail.
  2. For each literal at decision level 0 (unit):
     - If its variable is in `component`, add it to `units`.

#### `std::vector<lbool> &getModel()`

**Return the current model (converting from MiniSat format).**

- **Exact behavior:**
  1. Copy `s.model` to `m_model` with tri-value conversion.
  2. Return `m_model`.

---

## 5. DecisionDNNFOperation

**Location:** `src/methods/DecisionDNNFOperation.hpp`

**Role:** The output backend. Turns the DPLL recursion tree into a decision-DNNF, handling node creation, combining branches, and final output.

### Fields

#### `ProblemManager *m_problem`
- **Exact meaning:** Pointer to the problem, used for weights when computing model counts.
- **Accessed by:** `manageResult()` and `count()` to get weights.

#### `NodeManager<T> *m_nodeManager`
- **Exact meaning:** Factory and manager for DNNF nodes.
- **Created with:** `specs->getNbVariable() + 1` slots.
- **Manages:** Node allocation, caching, and compilation.

#### `WrapperSolver *m_solver`
- **Exact meaning:** Pointer to the solver, used in `manageTop()`.
- **Why stored:** To call `whichAreUnits()` when a component is a tautology.

### Key Functions

#### `U createTop()` and `U createBottom()`
- **Exact meaning:** Create true and false nodes.
- **Semantics:**
  - `createTop()`: No decisions needed; the subformula is a tautology.
  - `createBottom()`: The subformula is unsatisfiable.
- **Return:** Handles to the nodes (U is typically `Node<T>*`).

#### `U manageDeterministOr(DataBranch<U> *elts, unsigned size)`

**Combine two branches of a decision node with OR.**

- **Input:**
  - `elts[0]` and `elts[1]`: The two branches (true and false assignments).
  - Each `DataBranch` contains:
    - The compiled subformula `d`.
    - `unitLits`: Unit literals in that branch.
    - `freeVars`: Free variables in that branch.

- **Exact behavior:**
  1. Call `m_nodeManager->makeBinaryDeterministicOrNode(elts[0], elts[1])`.
  2. This creates a dDNNF OR node combining the two branches.
  3. The "deterministic" part means: exactly one branch can be true (DNNF property).

- **Semantics:** If we decided on variable v:
  - `elts[0]` is the compiled formula under v = true.
  - `elts[1]` is the compiled formula under v = false.
  - Result is: `(v ∧ elts[0].d) ∨ (¬v ∧ elts[1].d)` (with unit and free constraints attached).

#### `U manageDecomposableAnd(U *elts, unsigned size)`

**Combine independent components with AND.**

- **Input:**
  - `elts[0..size-1]`: Compiled subformulas for each independent component.
  - `size`: Number of components.

- **Exact behavior:**
  1. Call `m_nodeManager->makeDecomposableAndNode(elts, size)`.
  2. This creates a dDNNF AND node combining the components.
  3. The "decomposable" part means: no variable appears in multiple components (DNNF property).

- **Semantics:** Result is simply the conjunction of all component compilations: `elts[0] ∧ elts[1] ∧ ... ∧ elts[size-1]`.

#### `U manageTop(std::vector<Var> &component)`

**Handle the case where a component is a tautology.**

- **Exact behavior:**
  1. Call `m_solver->whichAreUnits(component, b.unitLits)` to extract units.
  2. Create a true node: `b.d = m_nodeManager->makeTrueNode()`.
  3. If there are unit literals:
     - Package them as `DataBranch<U> b` with `d`, `unitLits`, `freeVars`.
     - Create a unary node: `return m_nodeManager->makeUnaryNode(b)`.
  4. Otherwise, just return the true node.

- **Why needed:** Even though the formula is tautological, unit propagation may have implied literals.

#### `U manageBranch(DataBranch<U> &e)`

**Wrap a single branch result.**

- **Exact behavior:**
  1. Call `m_nodeManager->makeUnaryNode(e)`.
  2. This wraps the branch with its unit and free constraints.

- **Used by:** `compute()` to wrap the final result.

#### `void manageResult(U &result, Config &config, std::ostream &out)`

**Output the final compiled result.**

- **Branches:**
  1. **If `config.dump_ddnnf` is set:**
     - Open file.
     - Call `m_nodeManager->printNNF(result, outFile)`.
     - Close file.

  2. **Else if `config.query` is set (batch queries):**
     - Parse query file.
     - For each query:
       - Build `fixedValue` array encoding the query literals.
       - If query is counting: Call `m_nodeManager->computeNbModels(result, fixedValue, *m_problem)` and print.
       - If query is decision: Call `m_nodeManager->isSAT(result, fixedValue)` and print SAT/UNS.
       - Reset `fixedValue`.

  3. **Otherwise (default model count):**
     - Initialize `fixedValue` as all unassigned.
     - Call `m_nodeManager->computeNbModels(result, fixedValue, *m_problem)`.
     - Print the count.

  4. **Cleanup:**
     - Call `m_nodeManager->deallocate(result)` to free the tree.

#### `T count(U &root)` and `T count(U &root, std::vector<Lit> &assum)`

**Count models in the compiled structure.**

- **Exact behavior:**
  1. Initialize `fixedValue` vector (all unassigned for first version, or fixed by assumptions for second).
  2. Call `m_nodeManager->computeNbModels(root, fixedValue, *m_problem)`.
  3. Return the result.

- **Semantics:** Traverses the dDNNF tree, accumulating model counts using the arithmetic in template parameter `T` (e.g., `mpz_class` for arbitrary precision, or `double` for floating point).

---

## Summary of Data Flow

1. **ProblemManagerCnf** loads the CNF and normalizes it (projected variables first).
2. **SpecManagerCnf** mirrors the clauses and builds occurrence lists. It overlays `m_currentValue` and clause metadata to track the current subproblem.
3. **DpllStyleMethod** orchestrates:
   - Uses `m_solver` to check satisfiability and extract units.
   - Calls `m_specs->preUpdate()` to update clause metadata.
   - Calls `m_specs->computeConnectedComponent()` to split into independent subproblems.
   - Branches on variables and recursively solves each subproblem.
   - Calls `m_specs->postUpdate()` to undo changes when backtracking.
4. **WrapperMinisat** handles the solver calls, maintaining the assumption stack and model cache.
5. **Solver** (MiniSat) does the actual SAT solving under assumptions.
6. **DecisionDNNFOperation** constructs the dDNNF tree by combining recursive results.

