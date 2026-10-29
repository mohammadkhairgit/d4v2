# XORiented d-DNNF Compilation Implementation

This document is a concise guide to the implementation added to d4 for compiling formulas that contain both ordinary CNF [clauses](#clause) and alternative (exactly-one) [clauses](#clause). It focuses on the significant source changes, how the new [path](#path) is selected, and which classes were modified or newly added.

## Scope of the changes

The implementation adds a mixed-formula [path](#path) while preserving the original CNF [path](#path). The main changes and their corresponding classes are:

* a common clause abstraction for different clause types: [`ClauseType`](src/problem/allClauses/ClauseType.hpp) 
* parsing, storage, and use of alternative-clause input across multiple classes
* mixed problem ([`ProblemManagerAll`](src/problem/allClauses/ProblemManagerAll.hpp)) and specification managers ([`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp))
* clause branching and clause-scoring heuristics: [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp), [`ProjDpllStyleMethod`](src/methods/ProjDpllStyleMethod.hpp), [`ClauseScoringMethod`](src/heuristics/ClauseScoringMethod.hpp)
* support for non-binary deterministic OR dDNNF-nodes in the generated dDNNF: [`DeterministicOrNode`](src/methods/nnf/DeterministicOrNode.hpp)
* adaptations to solvers, caching, scoring, and factory/manager constructors so they can create objects of the new mixed classes

The original CNF managers and heuristics remain available. The mixed [path](#path) is selected **only** when an alternative input (`alternative-input`) file is provided ([example](#example)).

## Modification categories

### New abstractions

These are the central types and classes introduced for the mixed formulas or subjected to the most significant changes:

* [`ClauseType`](src/problem/allClauses/ClauseType.hpp): [`CNFClause`](src/problem/allClauses/ClauseType.hpp) and [`AlternativeClause`](src/problem/allClauses/ClauseType.hpp)
* [`ClauseScoringMethod`](src/heuristics/ClauseScoringMethod.hpp) and its alternative-clause scoring implementations
* [`ProblemManagerAll`](src/problem/allClauses/ProblemManagerAll.hpp) and [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp)
* the non-binary [`DeterministicOrNode`](src/methods/nnf/DeterministicOrNode.hpp)
* [`BucketManagerAll`](src/caching/allClauses/BucketManagerAll.hpp), [`BucketManagerAllCl`](src/caching/allClauses/BucketManagerAllCl.hpp)
* [`ScoringMethodMomAll`](src/heuristics/allClauses/ScoringMethodMomAll.hpp)

### New Goals

These are the classes in which the new alternative-clause branching functionality is implemented:

* [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp) and [`ProjDpllStyleMethod`](src/methods/ProjDpllStyleMethod.hpp)

### Extended existing classes/files

These are supporting classes that were extended either to support mixed formulas or to provide a [path](#path) for using the new mixed-formula classes:

* [`ParserDimacs`](src/problem/cnf/ParserDimacs.hpp)
* [`WrapperMinisat`](src/solvers/cnf/WrapperMinisat.hpp)
* [`BucketInConstruction`](src/caching/cnf/BucketInConstruction.hpp)
* [`CountingOperation`](src/methods/CountingOperation.hpp) and [`DecisionDNNFOperation`](src/methods/DecisionDNNFOperation.hpp)
* [`Node`](src/methods/nnf/Node.hpp), [`NodeManager`](src/methods/nnf/NodeManager.hpp), [`MethodManager`](src/methods/MethodManager.hpp), [`ProblemManager`](src/problem/ProblemManager.hpp), and [`SpecManager`](src/specs/SpecManager.hpp).

### Configuration and path selection

`alternative-input` selects the [path](#path) that uses the mixed parser and managers. `clause-scoring` selects the policy used when choosing an alternative clause for branching. When no alternative input is supplied, the regular CNF [path](#path) is used and clause scoring is disabled.

**Note:** Preprocessing and partitioning arguments should still be set to `"none"` because they are not implemented for the mixed-formula case. 

**Original implementation bug:** The shortened d4 argument form for partitioning is broken in the implementation, and preprocessing may have a similar issue.

### Tests

Unit tests exist for the classes [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp), [`ProblemManagerAll`](src/problem/allClauses/ProblemManagerAll.hpp), and [`ParserDimacs`](src/problem/cnf/ParserDimacs.hpp).

## Clause representation

### `ClauseType`

[`ClauseType`](src/problem/allClauses/ClauseType.hpp) is the common interface and state holder for mixed formula [clauses](#clause). It stores:

* the clause literals
* the number of currently satisfied and unsatisfied literals
* the number of non-projected variables and decided non-projected variables
* a watcher literal

The abstraction provides operations shared by both clause kinds:

* clause conversion to CNF
* satisfaction checks using either a partial assignment or tracked counters for quick evaluation
* tracking of the watcher and clause-local assignment counters
* detection of remaining projected variables and whether a clause is still relevant for branching
* cloning and literal access
* literal remapping and inner normalization

`ClauseKind` distinguishes clause types like `Cnf` and `Alternative`.

**Note:** The caller is responsible for maintaining the assignment counters. Currently, [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp) keeps them synchronized with the assignments made during recursive calls in [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp) by using `preUpdate` and `postUpdate`.

**Shortcomings:**

* Naming disjunctive OR as `Cnf`.
* The watcher is currently selected by other classes ([`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp)) rather than internally.
* The Name ClauseKind or ClauseType might be a little unfitting because the term "Clause" is often related to only Cnf constraints.

### `CNFClause`

[`CNFClause`](src/problem/allClauses/ClauseType.hpp) preserves ordinary disjunctive-clause semantics. It is satisfied when at least one literal is true. Its CNF encoding/transformation is the original literal list, so no auxiliary variables are required.

### `AlternativeClause`

[`AlternativeClause`](src/problem/allClauses/ClauseType.hpp) represents an exactly-one constraint. It is satisfied only when one literal is true and all other literals are false assigning a literal does not by itself satisfy the clause.

**Note:** The formula is converted to CNF when required. For now, this is only used by the wrapper to pass the clauses to the solver. Small clauses (`< 6`) use pairwise at-most-one constraints, while larger clauses use a sequential auxiliary-variable encoding. The auxiliary variables belong to the solver encoding and are not added to any other class.

## Parsing and problem management

### `ParserDimacs`

The existing DIMACS parser ([`ParserDimacs`](src/problem/cnf/ParserDimacs.hpp)) continues to read the main CNF input. The added alternative parser reads integer literal lists terminated by `0`, using the same underlying input mechanism as the original CNF parser. Parsing is separated from clause-object construction so the same raw literal data can be used to build the appropriate clause type based on the input file argument from which it was parsed.

**Note:** The program exits with an error if the alternative file contains an empty clause (`0`) or if a variable has a larger index than the number of variables declared in the CNF DIMACS file.

### `ProblemManagerAll`

[`ProblemManagerAll`](src/problem/allClauses/ProblemManagerAll.hpp) stores clauses as owning `std::unique_ptr<ClauseType>` objects instead of storing only vectors of literals. Its semantics and syntax are largely similar to those of [`ProblemManagerCnf`](src/problem/cnf/ProblemManagerCnf.hpp). It:

* constructs CNF clause objects from the main input path
* constructs alternative clause objects from the `alternative-input` path
* deep-copies clause and learnt-clause containers when required
* normalizes and remaps literals through the clause abstraction
* creates conditioned and unsatisfiable problem instances while preserving
  clause kinds
* reports mixed-formula statistics and exposes the typed clause containers.

This manager is selected by [`ProblemManager`](src/problem/ProblemManager.hpp)::makeProblemManager when `alternative-input` is non-empty and the input type is `cnf` or `dimacs`.

**Note:** The most significant, although relatively minor, difference from the original implementation is that the constructor receiving a [`ProblemManager`](src/problem/ProblemManager.hpp) parameter also copies the clauses. This is not done in the original [`ProblemManagerCnf`](src/problem/cnf/ProblemManagerCnf.hpp) implementation.

**Shortcoming:**

* To prevent the [original](#original_implementation) CNF implementation from always using preprocessing, a new constructor was added that also copies the clauses. This creates a difference between the two implementations: the new implementation has one constructor that always copies the clauses, while the [original](#original_implementation) CNF implementation has two.

## Mixed specification management

### `SpecManagerAll`

[`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp) is the mixed counterpart of [`SpecManagerCnf`](src/specs/cnf/SpecManagerCnf.hpp). It preserves d4's component-discovery workflow while storing and updating [`ClauseType`](src/problem/allClauses/ClauseType.hpp) objects. Its occurrence table (`m_occurrence`) still uses literals as indices and stores arrays of clause indices as values. Unsatisfied clauses are also tracked by `m_clausesByType`, which provides fast access for the clause-scoring classes.

The manager maintains:

* clause objects and active-clause maps (`m_clausesByType`) separated by kind
* literal occurrence lists for binary and non-binary clauses (`m_occurrence`)
* the current partial assignment
* temporary watcher information
* variables occurring in non-CNF clauses, which identifies variables that cannot become pure literals [while solving](#while_solving)
* temporary union-find and component-discovery state.

### Assignment updates

`preUpdate` and `postUpdate` update clause counters, occurrence lists (`m_occurrence`), and watchers as assignments are pushed and removed. Unlike the CNF-only [path](#path), an alternative clause can change status when a literal is assigned true as well as when it is assigned false. Therefore, both assignment signs are considered when updating clause state.

A clause is removed from active occurrence processing (`m_occurrence`) and deactivated in `m_clausesByType` when it becomes satisfied, then re-added/reactivated when it becomes unsatisfied. The manager also tracks non-CNF occurrences separately (`m_varCurrentOccurrenceInNonCnfClause`) because an alternative clause is recorded in `m_occurrence` only under the same literal polarity that appears in the clause, even though their cnf equivalent formula will have them in both polarity. For example, an alternative clause containing `1` is recorded in `m_occurrence[1]`, but not in `m_occurrence[not 1]`; a clause containing `not 2` is recorded in `m_occurrence[not 2]`, but not in `m_occurrence[2]`.

`getCurrentClausesByKind` exposes active, unsatisfied clauses that intersect a current connected component. Clause scoring uses this operation to consider alternative clauses only when they can be branched on.

**Note:** This polarity-specific occurrence representation should be considered when reasoning about pure literals, unit propagation, and other occurrence-based operations. For example, the solver receives the CNF-transformed formula and does not use this representation directly, avoiding potential issues such as incorrect unit-clause detection. The mixed path currently does not use CNF-only preprocessing. If preprocessing is extended to the mixed representation, it must also account for the fact that the CNF encoding can contain more clauses than the original mixed formula and that encoded clause indices do not necessarily correspond directly to original clause indices.

## Clause scoring and branching

### `ClauseScoringMethod`

[`ClauseScoringMethod`](src/heuristics/ClauseScoringMethod.hpp) is the factory and common interface for selecting a clause. It receives the mixed specification manager and the existing variable scoring method. Its main operations are:

* compute a score for one clause
* find the best alternative clause in the current connected component
* choose a scoring method based on the `clause-scoring` argument

`selectClause` can either accept any available clause or require the clause score to beat the best available variable score. This allows the method to choose between ordinary variable branching and clause branching.

The [`ClauseScoringMethodNone`](src/heuristics/clauseScoring/alternative/ClauseScoringMethodNone.hpp) class never selects a clause, so the search always falls back to literal/variable branching.

**Note:** The best clause found in the `selectClause` function is only used/branched_on in [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp) if the function return true.

**Shortcoming:**

* Instead of having the condition of a clause score beating the best literal score in selectClause it should have been done in getBestScoreClause as that would have allowed more flexibility. Furthermore, currently there is no d4 argument for setting this `selectClause` function parameter.

### `ClauseScoringMethodLiteralMetric`

This base class ([`ClauseScoringMethodLiteralMetric`](src/heuristics/clauseScoring/alternative/ClauseScoringMethodLiteralMetric.hpp)) derives clause scores from the existing literal-scoring heuristic. Assigned literals and literals excluded by projected-variable mode are skipped during scoring. It considers the branches that can still be created from the current partial assignment and prefers a higher clause comparison score, using fewer branches as a tie-breaker.

The available policies are:

* `literal-sum`: sums literal scores across the possible branches
* `literal-sum-over-branch-avg`: sums the scores of all possible branches and divides by the branch count
* `literal-min`: uses the minimum score among the undecided non-projected literals in the clause
* `literal-max`: uses the maximum score among the undecided non-projected literals in the clause
* `branch-min`: selects the least-scored possible branch, allowing the clause with the highest minimal branch score to be chosen
* `branch-max`: selects according to the best-scored possible branch, allowing the clause with the highest maximal branch score to be chosen

**Note:** The `clauseCompareScore` value in the `ClauseScores` struct is used to compare clauses, while `literalCompareScore` is returned to compare the winning clause against the best variable choice. This separation exists because some metrics may compare clauses correctly but may not be meaningfully directly comparable with a literal score. For the currently implemented metrics, the two values are always the same.

### `DpllStyleMethod`

When alternative input is enabled, [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp) creates a clause-scoring object and consults it while computing a decision node. If a clause is selected, the method creates branches corresponding to the still-decidable literals of the alternative. The set of literals eligible for branching is determined before the branches are created so that literals that become learned units during branching are not incorrectly skipped.

**Shortcoming:** Branching over a clause could be generalized further to make the addition of other clause types easier.

### `ProjDpllStyleMethod`

The projected variant ([`ProjDpllStyleMethod`](src/methods/ProjDpllStyleMethod.hpp)) applies the same clause-selection logic while preserving projected-model semantics. For an alternative clause with projected (undecidable) literals **and** an undecided non-projected variable, it adds a branch that sets all non-projected (decidable) literals to false while leaving the undecidable literals available for subsequent processing. This single branch covers the all projected branches associated with the projected (undecidable) literals.

## Solver and NNF integration

### `WrapperMinisat`

The MiniSat wrapper ([`WrapperMinisat`](src/solvers/cnf/WrapperMinisat.hpp)) passes the CNF encodings generated by [`ClauseType`](src/problem/allClauses/ClauseType.hpp) to the solver. This avoids implementing a separate solver for the mixed-clause case. Auxiliary variables introduced by an alternative-clause encoding exist only inside the solver but not inside the MiniSat wrapper. d4's branching and model handling continue to use only original variables and assumptions.

**Note:** Code that needs to inspect solver variables directly must account for this distinction, because solver-only auxiliary variables are not present in the mixed classes.

### `DeterministicOrNode`

[`DeterministicOrNode`](src/methods/nnf/DeterministicOrNode.hpp) stores a variable number of deterministic branches and their explicit unit/free-variable data in one compact allocation. It supports the same lifecycle operations as the other NNF node types:

* recursive deallocation
* model counting
* satisfiability checks under a fixed assignment
* NNF printing

[`Node`](src/methods/nnf/Node.hpp) and [`NodeManager`](src/methods/nnf/NodeManager.hpp) register this node type and allocate its branch data storage. The operation classes were extended to handle the results of non-binary deterministic OR branching.

**Note:** This is a minor implementation detail: `DeterministicOrNode` uses `U *data`, while the original implementation uses `U data[0]`.

## Caching and heuristic paths

Mixed clauses require clause-aware versions of components that previously assumed vectors of CNF literals:

* [`BucketManagerAll`](src/caching/allClauses/BucketManagerAll.hpp) and [`BucketInConstruction`](src/caching/cnf/BucketInConstruction.hpp) operate with [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp)
* [`BucketManagerAllCl`](src/caching/allClauses/BucketManagerAllCl.hpp) preserves clause type in cached representations
* [`ScoringMethodMomAll`](src/heuristics/allClauses/ScoringMethodMomAll.hpp) reads occurrence information from the mixed manager but is otherwise identical to the original.
* [`MethodManager`](src/methods/MethodManager.hpp), [`ProblemManager`](src/problem/ProblemManager.hpp), and [`SpecManager`](src/specs/SpecManager.hpp) select mixed factories when alternative input is active.

Preprocessing currently supports only CNF, so it is not applied to mixed problems. The method-construction [path](#path) supplies the required default state for unsupported preprocessing data instead of passing a mixed formula through a CNF-only preprocessor.

**Run Example<span id="example"></span>:** `d4 --input /path/cnf.dimacs --alternative-input /path/cnf.alt --clause-scoring literal-sum --occurrence-manager dynamic --input-type cnf --dump-ddnnf /path/output.ddnnf --scoring-method mom --phase-heuristic false --cache-activated true -m ddnnf-compiler --partitioning-heuristic none --preproc none`
* --partitioning-heuristic none, --preproc none, and --phase-heuristic false are the current safe settings for the mixed implementation. These options are not fully supported by the alternative-clause path and should be kept as shown.
* --occurrence-manager dynamic, --input-type cnf, and --scoring-method mom are the currently supported choices for this implementation. The code assumes this configuration for the mixed path, even though --input-type could later be generalized to describe the mixed-formula case explicitly.
* All remaining arguments can be set according to the desired execution mode and input/output requirements.

## Known limitations and extension points

* Clause watchers and assignment counters are maintained incrementally and must remain synchronized with `preUpdate`/`postUpdate`; this limitation also exists in the original implementation.
* Adding further clause types (for example, exactly-*n* clauses) would require:
  * a parser function, a path argument for the new clause file, and a modification to the [`ProblemManagerAll`](src/problem/allClauses/ProblemManagerAll.hpp) constructor that uses the parser
  * an implementation of [`ClauseType`](src/problem/allClauses/ClauseType.hpp)
  * corresponding changes to [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp) and [`ProjDpllStyleMethod`](src/methods/ProjDpllStyleMethod.hpp) if the new clause should support dedicated branching.
* Currently, all unsatisfied clauses are reevaluated at each recursive call. This could be optimized by storing the score of each clause in the scoring class and updating only the clauses whose literal scores changed during the previous recursive call, although this would not be trivial. For example, the stored score would also need to be restored when the recursive call returns, similarly to `preUpdate`/`postUpdate`.
* If implementing preprocessing and partitioning support for the new representation is not desired, a simpler approach could be considered:
  * Add another argument for the alternative file or use `alternative-input` but that would require more refactoring.
  * Use a DIMACS file containing both the OR clauses and the CNF-transformed alternative clauses. Pass this file through d4’s `input` argument without setting `alternative-input`. Alternatively, use the `toCnf` function in [`ClauseType`](src/problem/allClauses/ClauseType.hpp), although this would require additional refactoring.
  * Add a [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp) member to the `proj`/`DpllStyleMethod` classes, and call its functions alongside those of the regular [`SpecManager`](src/specs/SpecManager.hpp).(Duplicate functions call)
  * Use [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp) exclusively for clause scoring and branching.
  * As a result, the original CNF [path](#path) would be used throughout the full implementation, while [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp) would be used only for branching and scoring.

**Original Implementation Remarks:**
* The original [`SpecManagerCnf`](src/specs/cnf/SpecManagerCnf.hpp) provides `getVecIdxClause` and `getVecIdxClauseBin`, which return only clauses that were initially binary, rather than all clauses that are currently binary. The mixed [`SpecManagerAll`](src/specs/allClauses/SpecManagerAll.hpp) follows the same behavior; this should be considered when using these functions.
* In [`WrapperMinisat`](src/solvers/cnf/WrapperMinisat.hpp), `m_isInAssumption` is updated only by `pushAssumption` and `popAssumption`. It is also used by `isInAssumption` when branching on variables. If assumptions are pushed only for branching decisions and not for unit literals, this can produce redundant nodes. However, this does not occur in the current implementation because connected components returned by `SpecManager` contain only undecided variables, keeping the relevant branches safe. This behavior should be considered when modifying the implementation.
* Another observation regarding proj/DpllStyleMethod is that `if (m_solver->isInAssumption(l))` and `else if (m_solver->isInAssumption(~l))` can never be true for the same reason of how m_isInAssumption is updated.


## Terms and definitions

* <span id="path">A **path** is the sequence of execution steps selected by the provided arguments. For example, the arguments determine which concrete classes are created by managers or factories.</span>
* A **non-projected** variable is equivalent to a decidable variable.
* <span id="while_solving">**"While solving":** In the current implementation, 'solving' consequently causes the [`ProjDpllStyleMethod`](src/methods/ProjDpllStyleMethod.hpp), [`DpllStyleMethod`](src/methods/DpllStyleMethod.hpp) and [`WrapperMinisat`](src/solvers/cnf/WrapperMinisat.hpp) to immediately assign all variable of an alternative clause a value (true or false) when a literal becomes true or when all but one literal becomes false. This prevents a literal occurring in an alternative clause from becoming pure. Without unit propagation, a pure literal could occur in a non-CNF clause.</span>
* <span id="clause">**Clause:** In this document and implementation, a clause is a set of literals combined by an operation. The current implementation supports two operations: disjunctive OR and alternative (exactly-one constraint).</span>

## <span id="file_placements">Source Files</span>

* `ClauseKind`, `ClauseType`, `CNFClause`, `AlternativeClause`: [`src/problem/allClauses/ClauseType.hpp`](src/problem/allClauses/ClauseType.hpp)
* `ParserDimacs`: [`src/problem/cnf/ParserDimacs.hpp`](src/problem/cnf/ParserDimacs.hpp)
* `ProblemManager`: [`src/problem/ProblemManager.hpp`](src/problem/ProblemManager.hpp)
* `ProblemManagerCnf`, `ProblemManagerAll`: [`src/problem/cnf/ProblemManagerCnf.hpp`](src/problem/cnf/ProblemManagerCnf.hpp), [`src/problem/allClauses/ProblemManagerAll.hpp`](src/problem/allClauses/ProblemManagerAll.hpp)
* `SpecManager`: [`src/specs/SpecManager.hpp`](src/specs/SpecManager.hpp)
* `SpecManagerCnf`, `SpecManagerAll`: [`src/specs/cnf/SpecManagerCnf.hpp`](src/specs/cnf/SpecManagerCnf.hpp), [`src/specs/allClauses/SpecManagerAll.hpp`](src/specs/allClauses/SpecManagerAll.hpp)
* `ClauseScoringMethod`: [`src/heuristics/ClauseScoringMethod.hpp`](src/heuristics/ClauseScoringMethod.hpp)
* `ClauseScoringMethodLiteralMetric`, `ClauseScoringMethodLiteralSum`, `ClauseScoringMethodLiteralSumOverBranchAvg`, `ClauseScoringMethodLiteralMin`, `ClauseScoringMethodBranchMin`, `ClauseScoringMethodLiteralMax`, `ClauseScoringMethodBranchMax`: [`src/heuristics/clauseScoring/alternative/ClauseScoringMethodLiteralMetric.hpp`](src/heuristics/clauseScoring/alternative/ClauseScoringMethodLiteralMetric.hpp)
* `ClauseScoringMethodNone`: [`src/heuristics/clauseScoring/alternative/ClauseScoringMethodNone.hpp`](src/heuristics/clauseScoring/alternative/ClauseScoringMethodNone.hpp)
* `ScoringMethodMomAll`: [`src/heuristics/allClauses/ScoringMethodMomAll.hpp`](src/heuristics/allClauses/ScoringMethodMomAll.hpp)
* `DpllStyleMethod`: [`src/methods/DpllStyleMethod.hpp`](src/methods/DpllStyleMethod.hpp)
* `ProjDpllStyleMethod`: [`src/methods/ProjDpllStyleMethod.hpp`](src/methods/ProjDpllStyleMethod.hpp)
* `CountingOperation`: [`src/methods/CountingOperation.hpp`](src/methods/CountingOperation.hpp)
* `DecisionDNNFOperation`: [`src/methods/DecisionDNNFOperation.hpp`](src/methods/DecisionDNNFOperation.hpp)
* `Node`: [`src/methods/nnf/Node.hpp`](src/methods/nnf/Node.hpp)
* `NodeManager`: [`src/methods/nnf/NodeManager.hpp`](src/methods/nnf/NodeManager.hpp)
* `DeterministicOrNode`: [`src/methods/nnf/DeterministicOrNode.hpp`](src/methods/nnf/DeterministicOrNode.hpp)
* `MethodManager`: [`src/methods/MethodManager.hpp`](src/methods/MethodManager.hpp)
* `BucketInConstruction`: [`src/caching/cnf/BucketInConstruction.hpp`](src/caching/cnf/BucketInConstruction.hpp)
* `BucketManagerAll`, `BucketManagerAllCl`: [`src/caching/allClauses/BucketManagerAll.hpp`](src/caching/allClauses/BucketManagerAll.hpp), [`src/caching/allClauses/BucketManagerAllCl.hpp`](src/caching/allClauses/BucketManagerAllCl.hpp)
* `WrapperMinisat`: [`src/solvers/cnf/WrapperMinisat.hpp`](src/solvers/cnf/WrapperMinisat.hpp)