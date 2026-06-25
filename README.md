# Matching Engine

A C++ matching engine designed for reliability and generality across arbitrary market conditions.

## Building and Running

### Running with Docker (Recommended)

Docker provides a consistent, pre-configured environment for building and running the engine.

```bash
# Build and run the engine with default test data
make run

# Run with a custom CSV file
make run CSV=./path/to/your_data.csv
```

### Running Locally

If you prefer to build directly on your system, ensure all dependencies are available. The following packages are required:
- `cmake`
- `clang`, `llvm`, `lld`
- `libgtest-dev`
- `libboost-all-dev`


```bash
# Build
mkdir build && cd build
cmake ..
make

# Run
./matching_engine <path_to_csv>
```

### Running Tests

```bash
# With Docker
make test

# Locally (requires cmake -DENABLE_COVERAGE=ON)
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON ..
make coverage
```

# Codebase Walkthrough

## Main Entry Point (`main.cpp`)
`main.cpp` orchestrates the application flow:
- **Delegation**: It delegates file parsing to the ingestion pipeline and forwards parsed messages to the `OrderBook`.
- **Order Handling**: It invokes `OrderBook` actions (`New`, `Cancel`, `Amend`) and processes resulting state changes.
- **Trade Reporting**: A callback mechanism notifies the application when trades occur for real-time reporting.
- **Finalization**: It concludes by printing the remaining state of the order book.

## Data Ingestion
The parser is designed for efficiency and zero-copy ingestion:
- **Memory Mapping**: Loads input files directly into the process address space using `mmap` (`mmfile.hpp`, `mmfile.cpp`), minimizing I/O overhead.
- **Line Iteration**: A line iterator (`line_view.hpp`) traverses the memory-mapped file.
- **Parsing**: Each line is parsed directly into a `Message` structure (`parser.hpp`, `parser.cpp`) using `std::string_view` to avoid temporary allocations.
- **Error Handling**: Robust error reporting is provided via a functional `Expected` type (`expected.hpp`).

## The Order Book (`OrderBook`)
The `OrderBook` is the central matching logic component:
- **Generality**: It is template-parameterized on `OrderID`, `Price`, and `Quantity` types, allowing adaptation to various venue requirements without modifying core logic.
- **Numeric Precision**: Utilizes the `cnl` (C++ Numeric Library) for `FixedPoint` arithmetic, ensuring precise and deterministic financial calculations.
- **Memory Management**: Uses a custom `ObjectPool` (`object_pool.hpp`) to store order objects, promoting memory locality and cache efficiency, while providing stable object pointers used as identifiers.
- **Efficient Lookups**: Manages orders via `ObjectResource` (`object_resource.hpp`), which provides efficient, transparent lookup capabilities.

## Order Book Structure (`OrderBookSide`)
The `OrderBook` is bifurcated into two sides (bids and asks) managed by `OrderBookSide` (`order_book_side.hpp`):
- **Price Priority**:
    - Bids use `std::greater<Price>` for descending price ordering.
    - Asks use `std::less<Price>` for ascending price ordering.
- **Memory Efficiency**: Implemented with `std::pmr::map`, utilizing an `unsynchronized_pool_resource` for efficient node allocation. Both sides share this pool, maintained by the parent `OrderBook`.

## Price Levels (`PriceLevel`)
`PriceLevel` (`price_level.hpp`) manages the orders resting at a specific price:
- **Order Management**: Orders are stored in an intrusive `boost::intrusive::list`, leveraging stable pointers from the `ObjectPool`.
- **Quantity Tracking**: Maintains an accumulated quantity of the price level, allowing for O(1) checks.
- **Matching Logic**: `matchAgainst` iterates through orders to execute trades until the quantity is exhausted.
- **Event Notification**: Upon execution, the `on_filled` callback notifies the `OrderBook` and the application, ensuring seamless event handling.


# Architectural Strengths

The design philosophy centres on achieving superior performance without compromising clarity or structural integrity. A fundamental success of this architecture is its approach to memory management. By leveraging C++17 Polymorphic Memory Resources, specifically utilizing `ObjectPool` and `ObjectResource` backed by `std::pmr::unsynchronized_pool_resource`, the system exercises control over the lifecycle of order objects and price level nodes. This design significantly minimizes heap fragmentation and reduces the latency typically associated with memory allocation, ensuring that order data remains stable, cache-friendly, and immediately accessible during the matching process.

The data ingestion pipeline is built to focus on high throughput. The engine employs memory-mapped I/O via `mmfile` to map input files directly into the process address space, which effectively bypasses conventional I/O syscall overheads. This mechanism is complemented by a high-performance parser designed to operate directly on `std::string_view` to avoid temporary string allocations. By utilizing `std::from_chars` for integer conversion, the parser achieves allocation-free, non-throwing parsing.

Financial accuracy is maintained through a custom `FixedPoint` arithmetic implementation. This approach ensures precise financial calculations and eliminates the indeterminism and performance degradation associated with floating-point math within matching logic. Furthermore, the `OrderBook` is implemented as a configurable template class. This design allows for adaptation to varied venue-specific requirements, such as differing price precision or alternative ID formats, without necessitating structural modifications to the core matching logic.

The structural efficiency of the OrderBook is aided by the use of data structures designed to optimize the execution path. Intrusive data structures are central to this design, where `boost::intrusive::list` facilitates cache-friendly management of order chains within the object pool. The `ObjectPool` itself provides stable pointers for order objects, which is a key requirement for effective intrusive container usage. Additionally, the system implements a O(1) lookup with a std::unordered_set for order modifications and cancellations. The API is inherently low-latency friendly, supporting transparent heterogeneous lookups using `std::string_view`, which avoids redundant object construction on the hot path. Finally, the use of `std::pmr::map` ensures that price levels are maintained in a sorted order book, upholding price-time priority matching rules consistently.

# Technical Weaknesses and Opportunities for Improvement

While the current architecture is robust, several areas have been identified where optimization could further reduce latency, particularly under extreme market conditions.

The `std::pmr::map` implementation of OrderBookSide utilizes polymorphic allocators that introduce virtual function call overhead for every node allocation and deallocation. This was a deliberate choice as I thought choosing an object pool of dense map nodes may offset the polymorphic dispatch overhead. I would like to investigate a method of achieving a pool based allocator without the polymorphic overhead. This will need to be supported with benchmarks.

The current `std::pmr::map` allocator is implemented with std::pmr::unsynchronized_pool_resource which can allocate when its memory is exhaused, causing jitter on the hot path. If the maximum number of orders is known then this could be replaced with a more tightly controlled `std::pmr::monotonic_buffer_resource` presents an opportunity to mitigate this overhead.

The reliance on `std::pmr::map` for price level management introduces pointer-chasing behaviour that can adversely affect cache locality. Transitioning to more cache-friendly alternatives, such as flat maps or fixed-size arrays once the specific venue constraints regarding price ranges and depth are defined, could significantly improve data access times. Maybe a heap could be used to increase the efficiency of the finding applicible trades while running the matching engine which will increase the access to the best price level to O(1) with the trade-off of more expensive modification and cancellation, as these operations need to find an arbitary price level.

There is significant potential to further optimize the `Order` struct to maximize cache-line efficiency. Opportunities include re-sequencing struct members to eliminate implicit padding, utilizing manual bit-stealing (such as pointer tagging within intrusive list hooks) to store metadata like order sides or status flags, and replacing pointer-based intrusive list hooks with compact index-based identifiers. Optimizing `OrderID` storage also remains a viable path forward; strategies could include externalizing `OrderID` storage into a compact integer index table or inlining compressed integer representations of identifiers directly within the `Order` struct to eliminate indirection during hot-path processing.

These potential enhancements should be validated by establishing a comprehensive benchmarking suite to rigorously quantify their impact on system latency.
