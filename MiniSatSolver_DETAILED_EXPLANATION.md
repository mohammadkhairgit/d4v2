# AI Generated Documentation

**Disclaimer**: This documentation was generated using AI and may contain inaccuracies or omissions. It should always be viewed alongside the source code it documents.

# MiniSat Solver Detailed Explanation

This document explains the embedded MiniSat `Solver` class defined in [src/solvers/cnf/minisat/Solver.hpp](src/solvers/cnf/minisat/Solver.hpp). It is a standalone description of the class, its fields, its public API, and the internal helper methods that are directly relevant to the wrapper layer used by d4.

The class is the SAT engine behind the CNF backend. It stores the clause database, the current assignment trail, the heuristic scores, the assumption stack, and the current model. d4 never uses this class as a generic black box; instead, `WrapperMinisat` drives it through a narrower but carefully managed interface.

## 1. What the solver does

`minisat::Solver` implements a CDCL SAT solver.

At a practical level, it is responsible for:

1. Storing the CNF formula as problem clauses and learnt clauses.
2. Maintaining the current partial assignment.
3. Propagating unit clauses through watched literals.
4. Making branch decisions using variable activity and polarity heuristics.
5. Tracking assumptions and solving under those assumptions.
6. Learning clauses from conflicts and backtracking to earlier decision levels.
7. Producing a satisfying model when the formula is SAT.

The solver is stateful. Every solve call depends on the current internal assignment, current assumptions, heuristic state, and clause database.

## 2. Supporting types from `SolverTypes.hpp`

The solver relies on the following MiniSat types.

### `Var`

An integer variable identifier.

Important invariant:

* Variables are indexed from `0` to `nVars() - 1`.
* The solver treats variables as array indices, not as symbolic objects.

### `Lit`

A literal encoded compactly as one integer `x`.

Key helpers:

* `mkLit(var, sign)` builds a literal.
* `sign(l)` returns whether the literal is negated.
* `var(l)` returns the underlying variable.
* `~l` flips the sign.

### `lbool`

A three-valued truth type.

Values used by the solver:

* `l_True`
* `l_False`
* `l_Undef`

### `CRef`

A reference into the clause allocator.

This is how the solver stores clauses compactly without moving them around every time the internal database changes.

### `Clause`

The actual clause object stored behind a `CRef`.

Clauses are attached to watched-literal lists, can be learnt or original, and may store additional metadata such as activity or abstraction.

### `vec<T>`

MiniSat's own dynamic vector type.

It is used everywhere inside the solver instead of `std::vector`.

## 3. Core class role

The solver is the object that makes all SAT decisions.

The wrapper layer asks it to:

* build variables and clauses,
* solve under assumptions,
* rebuild its internal focus for a connected component,
* collect propagated units,
* expose the current model,
* and maintain heuristic information.

If the solver state is inconsistent, every higher-level result becomes unreliable. That is why the wrapper carefully keeps its assumption mirror and model validity flag synchronized with this object.

## 4. Important data members

### `std::ostream *cert`

Optional certificate output stream.

If non-null, the solver can write proof-like information when clauses are inserted through certificate-aware paths.

### `bool ok`

Global consistency flag.

Meaning:

* `true` means the solver state is still potentially satisfiable.
* `false` means the solver has already derived a contradiction and no further normal use of the state is valid.

This flag is checked by many internal operations and is one of the most important invariants in the solver.

### `vec<CRef> clauses`

The original problem clauses.

These are the clauses loaded from the CNF input. They remain part of the permanent clause database unless removed by simplification or garbage collection.

### `vec<CRef> learnts`

Learnt clauses derived from conflicts during search.

Learnt clauses are the main mechanism by which CDCL prunes the search tree.

### `vec<CRef> phantoms`

Temporary or auxiliary clauses used by the solver's phantom-mode machinery.

These are not part of the normal CNF input flow, but the solver keeps them so it can add and later remove special-purpose constraints.

### `vec<double> scoreActivity`

Conflict occurrence score per variable.

This is separate from ordinary branching activity and is exposed through the wrapper as conflict count information.

### `double cla_inc`

Increment used when bumping clause activity.

MiniSat maintains a rolling bump value rather than directly increasing all scores by fixed amounts.

### `vec<double> activity`

Branching activity score per variable.

This is the main heuristic signal used by the variable-order heap.

### `double var_inc`

Increment used when bumping variable activity.

### `OccLists<Lit, vec<Watcher>, WatcherDeleted> watches`

The watched-literal database.

For each literal, this stores the clauses that are watching that literal. It is the data structure that makes unit propagation efficient.

### `vec<lbool> assigns`

The current partial assignment.

Each variable maps to one of `l_True`, `l_False`, or `l_Undef`.

### `vec<char> polarity`

Preferred branching polarity per variable.

The solver can branch using this preference or the preference can be overridden by the wrapper.

### `vec<char> decision`

Whether each variable is eligible for branching.

Some variables may be excluded from decision heuristics.

### `vec<Lit> trail`

The assignment stack.

This contains every literal assigned so far, in the order they were assigned.

### `vec<int> trail_lim`

Decision level separators in the trail.

Each entry marks the starting index of a decision level in `trail`.

### `vec<VarData> vardata`

Per-variable reason and decision level.

This tells the solver why a variable was assigned and at what level.

### `int qhead`

Propagation queue head.

This is the index in `trail` up to which propagation has already been processed.

### `int simpDB_assigns` and `int64_t simpDB_props`

Simplification bookkeeping.

They track how much top-level assignment / propagation work has happened since the last simplification step.

### `vec<Lit> assumptions`

Current assumption stack.

This is what `solveWithAssumptions()` uses as the active context for search.

### `Heap<VarOrderLt> order_heap`

Priority queue of decision variables ordered by activity.

This is the solver's main branching queue.

### `double progress_estimate`

Heuristic estimate of search progress.

### `bool remove_satisfied`

Whether satisfied clauses should be removed aggressively during simplification.

### `ClauseAllocator ca`

Clause storage allocator.

This is where clause objects live.

### `vec<char> seen`, `vec<Lit> analyze_stack`, `vec<Lit> analyze_toclear`, `vec<Lit> add_tmp`

Temporary work vectors used by propagation, analysis, and clause addition.

They reduce allocation overhead and support repeated inner-loop operations.

### `double max_learnts`, `double learntsize_adjust_confl`, `int learntsize_adjust_cnt`

Learnt-clause database reduction controls.

These fields help decide when to shrink the learnt database.

### `int idxReasonFinal`, `int idxClausesCpt`

Auxiliary counters used by the solver and certificate / reasoning paths.

### `int64_t conflict_budget`, `int64_t propagation_budget`, `bool asynch_interrupt`

Resource-control fields.

These allow bounded solving or asynchronous interruption.

### `bool phantomMode`, `Lit phantomLit`

Special-mode fields for phantom clause handling.

### `bool reversePolarity`

Flag to invert solver polarity preference.

### `int limTrailNbSatUns`, `vec<vec<int>> occGtThree`, `bool occGtThreeInit`

Occurrence-tracking fields used by the solver's specialized bookkeeping for larger clauses.

### `vec<Var> problemVariable`

The current variable subset considered relevant for the solver's focused rebuild logic.

### `vec<int> occHeapSz`, `int currentTotalVar`, `vec<Var> topVariables`, `vec<Var> noSymVariables`

Additional internal bookkeeping for occurrence and variable-class management.

### `bool needModel`

Whether the solver should preserve or compute a satisfying model for the last solve.

This is set through `setNeedModel(bool)`.

### `vec<vec<int>> occDomainConstraints`, `vec<vec<Lit>> listOfDomainConstraints`

Domain-constraint bookkeeping used by solver extensions in this code base.

## 5. Constructor and destructor

### `Solver(std::ostream *certif = NULL)`

Constructs a new solver instance.

At construction time the solver initializes its internal vectors, counters, and databases, but it does not yet contain the user's CNF formula. That must be added later through `addClause(...)` and `newVar()`.

### `virtual ~Solver()`

Releases solver-owned memory.

The clause allocator, internal vectors, and temporary structures are cleaned up here.

## 6. Problem loading and clause insertion

### `Var newVar(bool polarity = true, bool dvar = true)`

Adds a new variable to the solver.

#### Parameters

* `polarity`: initial preferred polarity.
* `dvar`: whether the variable is eligible for branching.

#### What it does

1. Extends all variable-sized arrays.
2. Initializes the new variable's assignment, polarity, and decision eligibility.
3. Inserts the variable into the order heap if it is a decision variable.

#### Why it matters

The solver needs a consistent array slot for every variable before any clause can refer to it.

### `bool addClause(const vec<Lit> &ps)`

Adds a clause to the solver using a temporary copy.

This is the normal clause insertion path from the public API.

### `bool addEmptyClause()`

Adds the empty clause.

This immediately makes the solver contradictory.

### `bool addClause(Lit p)` / `addClause(Lit p, Lit q)` / `addClause(Lit p, Lit q, Lit r)`

Convenience overloads for unit, binary, and ternary clauses.

These methods build a temporary vector and delegate to `addClause_(...)`.

### `bool addClause_(vec<Lit> &ps)`

Adds a clause without creating an additional internal copy.

This is the lower-level insertion routine used by the convenience wrappers.

### Internal clause insertion helpers

The actual insertion path may:

* simplify the clause,
* check for satisfied literals,
* attach watched literals,
* mark the solver inconsistent if an empty clause is added,
* and update clause / literal statistics.

## 7. Solving API

### `bool simplify()`

Removes already satisfied clauses and performs simplification steps.

Typical use:

* after many top-level assignments,
* before a heavier solve,
* or when the solver wants to clean up the database.

### `bool solveWithAssumptions()`

Solves using the current content of `assumptions`.

This is the main entry point used by the wrapper after it has prepared assumptions and, if needed, a focused variable subset.

### `bool solveWithAssumptions(vec<Lit> &assums, bool model = false)`

Solves under a supplied assumption list.

#### What it does

1. Backtracks to zero.
2. Replaces the current assumption stack.
3. Temporarily sets model preservation according to `model`.
4. Runs the main solve.
5. Restores the previous model-setting and returns to zero level.

### `bool solveWithAssumptions(vec<Lit> &assums, vec<Lit> &unit, bool model = false)`

Solves under supplied assumptions and also returns the unit literals implied by those assumptions.

This overload is especially useful when the caller wants both SAT status and the forced literals produced by propagation.

### `bool propagateAssumption()`

Propagates assumptions without a full search.

### `bool solve(const vec<Lit> &assumps)`

Solves under a fixed assumption vector.

This is a convenience overload that copies the assumptions and runs search.

### `lbool solveLimited(const vec<Lit> &assumps)` and `lbool solveLimited(const vec<Lit> &assumps, int nbConflict)`

Runs a bounded solve with resource limits.

These methods return a tri-state result because the search may be interrupted before it proves SAT or UNSAT.

### `bool solve()`

Solves without assumptions.

This clears the assumption stack and performs a standard search.

### `bool solve(Lit p)`, `bool solve(Lit p, Lit q)`, `bool solve(Lit p, Lit q, Lit r)`

Convenience overloads for one, two, or three assumptions.

### `bool okay() const`

Returns whether the solver is still in a valid, non-contradictory state.

## 8. State access and heuristic configuration

### `void setPolarity(Var v, bool b)`

Sets the preferred polarity for variable `v`.

### `void setDecisionVar(Var v, bool b)`

Marks whether variable `v` may be selected by the branching heuristic.

If a variable becomes eligible, it is inserted into the order heap.

### `lbool value(Var x) const`

Returns the current assignment of variable `x`.

### `lbool value(Lit p) const`

Returns the current assignment of literal `p`.

### `lbool modelValue(Var x) const`

Returns the value of variable `x` in the last computed model.

### `lbool modelValue(Lit p) const`

Returns the model value of literal `p`.

### `int nAssigns() const`

Returns the number of currently assigned literals.

### `int nClauses() const`

Returns the number of original clauses.

### `const Clause &getClause(int i) const`

Returns the `i`-th original clause.

### `int nLearnts() const`

Returns the number of learnt clauses.

### `int nVars() const`

Returns the number of variables.

### `int nFreeVars() const`

Returns the number of free variables, computed from decision-variable count and top-level assignments.

## 9. Resource controls

### `void setConfBudget(int64_t x)`

Sets a conflict budget relative to the current number of conflicts.

### `void setPropBudget(int64_t x)`

Sets a propagation budget relative to the current number of propagations.

### `void budgetOff()`

Disables both budgets.

### `void interrupt()` / `void clearInterrupt()`

Sets or clears the asynchronous interrupt flag.

### `bool withinBudget() const`

Returns whether solving is still allowed under the current interruption and budget limits.

## 10. Memory management and statistics

### `virtual void garbageCollect()`

Reclaims unused clause memory.

### `void checkGarbage(double gf)` / `void checkGarbage()`

Checks whether the wasted memory fraction is above threshold and triggers garbage collection if needed.

### Statistics fields

The solver stores counters such as:

* `solves`
* `starts`
* `decisions`
* `rnd_decisions`
* `propagations`
* `conflicts`
* `dec_vars`
* `clauses_literals`
* `learnts_literals`
* `max_literals`
* `tot_literals`

These are used for diagnostics, performance tracking, and in some cases heuristic tuning.

## 11. Main internal methods

These methods implement the CDCL engine.

### `void insertVarOrder(Var x)`

Inserts a variable into the decision heap if it is eligible and active.

### `Lit pickBranchLit()`

Returns the next branching literal.

This is the solver's internal branching choice mechanism.

### `void newDecisionLevel()`

Starts a new decision level by pushing the current trail size to `trail_lim`.

### `void uncheckedEnqueue(Lit p, CRef from = CRef_Undef)`

Assigns a literal without testing consistency.

This is a core building block of propagation and assumption handling.

### `bool enqueue(Lit p, CRef from = CRef_Undef)`

Safe version of assignment.

It returns false if the literal contradicts the current state.

### `CRef propagate()`

Runs watched-literal propagation.

This is the heart of Boolean Constraint Propagation.

#### What it does

1. Walk through newly assigned literals.
2. For each literal, inspect clauses watching its negation.
3. Try to find another watch or detect that the clause is unit / conflicting.
4. Enqueue forced literals.
5. Return `CRef_Undef` if no conflict occurs, or a conflicting clause reference otherwise.

### `void cancelUntil(int level)`

Backtracks to the requested decision level.

It undoes assignments above that level and restores the corresponding solver state.

### `void analyze(CRef confl, vec<Lit> &out_learnt, int &out_btlevel)`

Performs conflict analysis and produces a learned clause.

### `void analyzeLastUIP(CRef confl, vec<Lit> &out_learnt, int &out_btlevel)`

Performs conflict analysis with a last-UIP target.

This is the variant the wrapper uses in the single-literal decision test.

### `void analyzeFinal(Lit p, vec<Lit> &out_conflict)`

Builds a final conflict clause for UNSAT reasoning under assumptions.

### `bool litRedundant(Lit p, uint32_t abstract_levels)`

Helper used by conflict analysis to detect redundant literals.

### `lbool search(int nof_conflicts)`

Runs the inner search loop for a bounded number of conflicts.

### `lbool solve_(bool rebuildHeap = true, int nbConflict = 0)`

The main solving engine.

This is where the solver alternates between propagation, decisions, conflict analysis, learning, and backtracking.

### `void reduceDB()`

Reduces the learnt database.

### `void removeSatisfied(vec<CRef> &cs)`

Removes satisfied clauses from a clause list.

### `void rebuildOrderHeap()`

Rebuilds the branching heap from the current set of decision variables and their activity.

## 12. Activity management

### `void varDecayActivity()`

Decays variable activity by scaling the bump increment.

### `void varBumpActivity(Var v, double inc)`

Increases the activity of a variable by `inc` and reorders the heap if needed.

### `void varBumpActivity(Var v)`

Convenience overload that uses the current bump amount.

### `void claDecayActivity()`

Decays clause activity by scaling the clause bump increment.

### `void claBumpActivity(Clause &c)`

Increases a clause's activity.

Activity management matters because it biases the solver toward variables and clauses that have recently participated in conflicts.

## 13. Clause database operations

### `void attachClause(CRef cr)`

Attaches a clause to the watcher lists.

### `void detachClause(CRef cr, bool strict = false)`

Detaches a clause from watcher lists.

### `void removeClause(CRef cr, bool strict = false)`

Detaches and frees a clause.

### `void removeNotAttachedClause(CRef cr)`

Removes a clause that is not currently attached.

### `bool locked(const Clause &c) const`

Returns whether a clause is currently a reason for some implication.

### `bool satisfied(const Clause &c) const`

Returns whether a clause is satisfied under the current assignment.

### `void relocAll(ClauseAllocator &to)`

Relocates all clauses to a different allocator.

This is used during garbage collection or allocator resizing.

## 14. Miscellaneous solver methods

### `int decisionLevel() const`

Returns the current decision level.

### `uint32_t abstractLevel(Var x) const`

Returns a compact abstraction of the decision level of `x`.

### `CRef reason(Var x) const`

Returns the clause reference that justified the assignment of `x`.

### `int level(Var x) const`

Returns the decision level of `x`.

### `double progressEstimate() const`

Returns a heuristic estimate of search progress.

### `bool withinBudget() const`

Already covered in the resource control section, but it is also part of the solver's general misc API because many internal loops test it.

## 15. Convenience and helper utilities

### `static inline double drand(double &seed)` / `static inline int irand(double &seed, int size)`

Random-number helpers used by stochastic parts of the solver.

### `inline void setNeedModel(bool b)`

Stores whether model maintenance is required for subsequent solve calls.

### `inline bool litTrueInLastModel(Lit l)`

Checks whether a literal is true in the last model.

This is used heavily by the wrapper when it wants to preserve or validate a model across assumption changes.

### `inline void rebuildWithAllVar()`

Rebuilds the solver focus to include all currently unassigned variables.

The method also refreshes `problemVariable` and the heap membership stamp.

### `inline void rebuildWithConnectedComponent(vec<Var> &v)`

Rebuilds the solver focus to the specific connected component `v`.

This is the component-focused variant used by the wrapper's restricted solve path.

### `inline void refillAssums()`

Restores assumptions after a decision-level reset.

It walks the assumption stack and reenacts the solver trail until the current level matches the assumption depth.

### `inline void backTrack()`

Backtracks one decision level.

### `inline void computeLitPropagate(Lit l, vec<Lit> &vp)`

Tests a literal under a temporary decision level and returns the full propagation closure.

### `inline bool oneIsTrue(vec<Lit> &vc)`

Returns whether at least one literal in a vector is currently true.

### `inline void rebuildTrail(vec<Lit> &areUnit)`

Replays unit literals into the trail and propagates them.

### `inline bool clauseIsSAT(Clause &c)`

Checks whether a clause is already satisfied.

### `void collectUnit(vec<Var> &setOfVar, vec<Lit> &unitsLit, Lit dec = lit_Undef)`

Collects unit literals from a variable set.

### `inline bool isAssigned(Var v)`

Returns whether variable `v` currently has a value.

### `inline Lit litAssigned(Var v)`

Returns the literal representing the current assignment of `v`, or `lit_Undef` if unassigned.

### `inline bool getIsTautologie()`

Returns the solver's tautology flag.

### Phantom-clause helpers

The following helpers manage solver-internal phantom constraints:

* `startPhantomMode()`
* `removePhantomTrace()`
* `removeLearnt()`
* `removePhantomClausesFromLearnt()`
* `removePhantomTrail()`
* `removePhantom()`
* `cancelUntilOldZeroLevelTrail(int szt)`
* `addPhantomClause(vec<Lit> &ps)`

These methods support solver extensions that temporarily add and later remove special clauses or assignments.

## 16. What the wrapper uses most

The d4 wrapper does not need the entire solver API equally. The most important calls are:

* `newVar`
* `addClause`
* `solveWithAssumptions`
* `propagateAssumption`
* `rebuildWithAllVar`
* `rebuildWithConnectedComponent`
* `newDecisionLevel`
* `uncheckedEnqueue`
* `propagate`
* `cancelUntil`
* `isAssigned`
* `litAssigned`
* `litTrueInLastModel`
* `modelValue`
* `setNeedModel`
* `setPolarity`
* `setDecisionVar`
* `activity`
* `scoreActivity`
* `polarity`
* `trail`
* `assumptions`

These are the operations that make the solver usable as a controlled backend rather than a standalone black box.

## 17. Solver invariants

The solver assumes several internal invariants:

1. `assigns`, `trail`, and `vardata` must agree on current variable state.
2. `trail_lim` must correctly delimit decision levels.
3. Every attached clause must be consistent with watcher lists.
4. `ok` must remain true unless the solver has derived contradiction.
5. `assumptions` must be aligned with the trail when solving under assumptions.
6. The heap must only contain decision variables that are eligible and current.

Violating these invariants can corrupt search, propagation, or model extraction.

## 18. Typical call flow

A normal solve cycle looks like this:

1. The caller creates variables with `newVar()`.
2. The caller adds all problem clauses with `addClause(...)`.
3. The wrapper sets model preservation if needed with `setNeedModel(true)`.
4. The wrapper or caller installs assumptions.
5. The solver rebuilds focus using `rebuildWithAllVar()` or `rebuildWithConnectedComponent(...)`.
6. `solveWithAssumptions()` launches the CDCL search.
7. During search, `propagate()` and `analyzeLastUIP(...)` handle conflicts and learned clauses.
8. On success, `model` stores a satisfying assignment.
9. On backtracking or query changes, `cancelUntil(...)`, `restart`, or assumption manipulation restore the desired state.

## 19. Relation to `WrapperMinisat`

`WrapperMinisat` is a thin but stateful adapter over this solver.

The wrapper is responsible for:

* converting d4 literals and variables to MiniSat form,
* keeping a mirror of the assumption stack,
* exposing the model in d4 types,
* and managing component-focused rebuilds.

The solver itself performs the actual SAT reasoning.

That is why a correct understanding of the wrapper always depends on understanding this class first.