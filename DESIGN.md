# Matching Engine Architectural Design

## Design Philosophy
The matching engine prioritizes functional correctness and generality over speculative performance optimization. The system is designed to operate reliably across arbitrary market conditions, venue specifications, and order volumes. Performance optimizations are applied selectively on identified hot paths while maintaining system-wide determinism.

## Core Design Decisions
*   **Memory Management:** Utilizes C++17 `std::pmr` (Polymorphic Memory Resource) to manage object lifetimes. `ObjectResource` and `ObjectPool` ensure that order objects and price level nodes are allocated via a controlled pool resource, reducing heap fragmentation and allocation latency.
*   **OrderID Handling:** Implemented `FixedSizeOrderID` (a wrapper around `char[10]`) to replace `std::string`. This removes heap allocation on the hot path and supports transparent lookups (`std::string_view`), eliminating redundant object construction during cancellations and amendments.

## Data Ingestion Pipeline
The engine utilizes a zero-copy, high-performance ingestion pipeline:
*   **Memory-Mapped I/O (`mmfile.hpp`):** Maps input files directly into the process address space, minimizing I/O syscall overhead during data ingestion.
*   **Fast Parsing (`parser.cpp`):** Operates on `std::string_view` to avoid temporary string allocations. Utilizes `std::from_chars` for fast, non-throwing, and allocation-free integer conversion.

## Numeric Precision
*   **Fixed-Point Arithmetic (`fixed_point.hpp`):** Uses a custom `FixedPoint` type for all price calculations. This ensures precise financial arithmetic and prevents the indeterminism and performance penalties associated with floating-point math in matching logic.

## Configurability
The `OrderBook` is implemented as a template class, allowing it to be instantiated with custom types for `OrderID`, `Price`, and `Quantity`. This enables the engine to adapt to venue-specific requirements (e.g., varying precision for prices, different ID formats, or larger quantity types) without requiring core logic modifications.

## Structural Design
The system architecture is centered around high-performance object management and efficient order lookups, utilizing intrusive data structures to ensure minimal memory overhead and fast access times on critical execution paths.

*   **Price Level Management (`std::pmr::map`):** Used to maintain a sorted order book of price levels, ensuring functional correctness without making venue-specific assumptions regarding price tick size or range.
*   **Order Linking (`boost::intrusive::list`):** Used to link `Order` objects directly within the object pool for efficient, cache-friendly management of multiple orders at the same price level.
*   **Order Lookup (`std::unordered_set`):** Maintains a set of order pointers keyed by `FixedSizeOrderID` to provide O(1) average-time lookups for order cancellations and amendments.

## Strengths
*   **Correctness and Robustness:** Functionally sound across arbitrary inputs; handles price-time priority matching without venue-specific constraints.
*   **Memory Safety:** Strict management of object lifetimes via pool-based ownership; eliminated dangling references in lookups.
*   **Low-Latency-Friendly API:** Transparent heterogeneous lookups allow the system to search for orders using `std::string_view` without additional object construction.

## Weaknesses
*   **Cache Locality:** `std::pmr::map` is a node-based, pointer-chasing data structure, resulting in sub-optimal cache utilization in extreme market conditions.
*   **Virtual Call Overhead:** `std::pmr::map` utilizes a polymorphic allocator, which introduces virtual function calls for every memory allocation and deallocation, adding latency to the hot path of order book modifications.
*   **Algorithmic Complexity:** `matchPrice` implements a linear O(N) traversal of price levels.
*   **Memory Overhead:** `ObjectResource` utilizes an `std::unordered_set<T*>` for order lookups, introducing hashing and indirection overhead.

## Potential Improvements
*   **Structure Optimization:** Replace `std::pmr::map` with cache-friendly alternatives (e.g., flat maps or fixed-size price level arrays) once venue specifications (price ranges, max depth) are finalized.
*   **Memory Management:** Transition from `unsynchronized_pool_resource` to `std::pmr::monotonic_buffer_resource` for tighter control over allocation patterns, contingent on defining the maximum order volume.
*   **Order Struct Compactness & Cache-Line Efficiency:** Optimize the `Order` data structure to maximize the number of orders per cache line and reduce memory footprint.
        *   **Manual Bit-Stealing and Packing:**
            *   **Pointer Tagging:** Utilize the upper unused bits of 64-bit memory addresses in intrusive list hooks to store `side_t` or status flags.
            *   **Index Packing:** Utilize compact integer representations to pack multiple logical identifiers and metadata fields into a reduced-size primitive, maximizing data density and minimizing structural overhead.
        *   **Memory Layout Optimization:** Re-sequence struct members to eliminate implicit padding; potentially utilize Template Metaprogramming (TMP) to automate layout compaction.
        *   **Intrusive Indexing:** Replace 64-bit pointer-based intrusive list hooks with compact index-based identifiers to reduce structural overhead.
        *   **Decoupled ID Management:** Optimize `OrderID` storage to reduce struct footprint and improve lookup efficiency:
            *   **External Data Store (Index-based):** Externalize `OrderID` storage entirely by using a compact integer index into an external lookup table. This maximizes `Order` struct density at the cost of potential indirection during ID resolution.
            *   **Inlined Compressed Storage:** Inline a compressed (e.g., bit-packed or radix-encoded) integer representation of the `OrderID` directly within the `Order` struct. This prioritizes cache locality by co-locating the ID with order data, eliminating indirection during hot-path processing.
*   **Functional Expansion:** Implement support for Market Orders and alternative matching algorithms (e.g., Pro-Rata) by refactoring the matching dispatcher.
*   **Benchmarking:** Establish a comprehensive benchmarking suite to quantify latency impacts of current abstractions and validate proposed optimizations.
