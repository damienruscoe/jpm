# Order Book Analytics & Feature Extraction Roadmap

This document serves as the implementation specification and conceptual blueprint for upgrading our Level 3 (L3) deterministic order book engine. Transitioning from L2 (price-aggregated) to L3 (order-entity tracking) allows us to analyze individual participant intent, queue topology, and real-time microstructure signals.

The features detailed below are organized into a phased implementation roadmap. Each metric includes a description of its operational significance, mathematical derivation, and concrete usage patterns for both real-time visualization and down-stream quantitative execution strategies.

---

## Phase 1: Core Queue Topology & Microstructure Fundamentals
*Focus: Extracting basic structural insights made accessible by tracking discrete order entities.*

### 1. Relative Queue Position & True Estimated Time to Fill (ETTF)
* **Reasoning:** Traditional L2 execution engines can only approximate queue position using flat volume aggregation. In an L3 book, tracking exact priority order handles allows an execution router to calculate the exact volume standing between our order and the matching engine's execution boundary.
* **Implementation:** When an order is placed, store its unique identifier alongside its execution precedence. For all incoming `Cancel` or `Modify` events at that price tier, check the target order's priority:
  $$\text{If } \text{Priority}_{\text{event}} < \text{Priority}_{\text{our\_order}}, \quad \text{Queue\_Weight} \leftarrow \text{Queue\_Weight} - \text{Size}_{\text{event}}$$
  If the event occurs behind our order, ignore it. ETTF is updated by dividing remaining `Queue_Weight` by the moving average of aggressive trade throughput.
* **Usage:** Front-end rendering displays a dynamic progress bar for active orders. Execution strategies use this to optimize passive placement and dynamically adjust alpha parameters based on real-time fulfillment probability.

### 2. Queue Concentration & Fragmentation Index (HHI)
* **Reasoning:** A price tier composed of a single massive order behaves differently under stress than a tier composed of fifty small retail orders. Large individual orders signify institutional commitment or market-maker anchors, while high fragmentation indicates a fragile consensus prone to rapid cascading holes.
* **Implementation:** Apply the Herfindahl-Hirschman Index (HHI) across the array of individual orders resting at a specific price level $p$:
  $$\text{HHI}_p = \sum_{i=1}^{n} \left( \frac{v_i}{V_p} \right)^2$$
  where $v_i$ is the volume of individual order $i$, and $V_p$ is the total aggregated volume at price level $p$. The index ranges from $1/n$ (perfectly fragmented) to $1.0$ (single concentrated entity).
* **Usage:** Overlay an HHI color intensity gradient directly onto the order book depth visualizer. Automated strategies exploit this by leaning passive orders against high-concentration tiers while shorting fragmented tiers on velocity shifts.

### 3. Multi-Horizon Structural Order Flow Imbalance (L3-OFI)
* **Reasoning:** Standard Level 2 OFI aggregates all net volume adjustments across ticks, obscuring the underlying mechanism. Level 3 OFI splits state transitions into deterministic buckets to differentiate between active liquidity consumption and passive order cancellations.
* **Implementation:** For every inbound message, calculate the discrete delta contribution:
  $$\Delta \text{OFI}_t = \mathbf{1}_{\{\text{NewOrder}\}} \cdot v - \mathbf{1}_{\{\text{Cancel/ModifyDown}\}} \cdot v - \mathbf{1}_{\{\text{TradeFill}\}} \cdot v$$
  Maintain this net imbalance value across continuous sliding circular arrays mapping 10-tick, 100-tick, and 1000-tick horizons.
* **Usage:** Display as a real-time multi-line oscillator under the main depth component. Downstream models utilize the multi-horizon array as a baseline predictive feature for short-term price directionality.

### 4. Deterministic Market Impact & Slippage Simulator
* **Reasoning:** Static slippage estimation relies on historical transaction cost modeling. Real-time L3 simulation walks the live order linked-list to calculate the exact execution cost of a market order before it is dispatched.
* **Implementation:** Construct a non-destructive iteration pointer over the active order lists starting at the top-of-book. For a hypothetical trade size $Q$:
  $$\text{Effective Price} = \frac{1}{Q} \sum_{i=1}^{m} p_i \cdot \min(v_i, Q_{\text{remaining}})$$
  $$\text{Slippage} = |\text{Effective Price} - \text{Mid Price}|$$
* **Usage:** Provide an interactive slider in the visualization framework that charts instantaneous execution cost against order size. Trading routers use this to dynamically slice parent orders into optimal child block chunks.

### 5. Order Lifespan & Turnover Velocity
* **Reasoning:** High order insertion rates coupled with immediate cancellations indicate transient, non-firm liquidity. Tracking the historical lifespan of orders at specific distances from the mid-price reveals where genuine risk is being held.
* **Implementation:** Maintain a localized hash map of historical order lifespans. Upon a complete fill or cancellation event, record the delta between the termination timestamp ($t_{\text{term}}$) and creation timestamp ($t_{\text{orig}}$). Compute a rolling median lifespan vector grouped by price offset tiers:
  $$\tau_{\text{median}} = \text{median}(t_{\text{term}} - t_{\text{orig}})$$
* **Usage:** Highlight price tiers with short lifespans ($< 5\text{ms}$) via high-frequency visual flashing, indicating algorithmic churning. Strategies ignore these fleeting zones during structural execution routing.

---

## Phase 2: Behavioral & Participant Classification Analytics
*Focus: Isolating latent intent, identifying algorithmic archetypes, and tracking order lifecycle patterns.*

### 6. Order Age Heat-Maps
* **Reasoning:** Passive orders that sit undisturbed for long durations reflect strong institutional inventory placement or hard structural baseline pricing. Conversely, brand new orders are more likely to be tactical, speculative, or high-frequency noise.
* **Implementation:** For every active order node in the book, expose its insertion timestamp. Calculate the current age $a_i = t_{\text{current}} - t_{\text{insertion}}$. For visualization and state extraction, calculate the volume-weighted average age of each price tier:
  $$\bar{A}_p = \frac{\sum (v_i \cdot a_i)}{V_p}$$
* **Usage:** Render the order book depth chart as a temporal heatmap, shifting from deep cool colors (sticky, ancient liquidity) to bright neon accents (newly flashed liquidity).

### 7. Pseudonymous Algorithmic Profile Clustering
* **Reasoning:** Institutional execution algorithms leave structural signatures in how they manage risk, size orders, and cycle modifications. Identifying these signatures allows us to anticipate their downstream actions.
* **Implementation:** Pass incoming order events through a signature hashing engine that tracks recurring parameter combinations: (Order Size, Target Spread Offset, Delta-Time to modification). Group active order nodes using a high-speed inline categorization look-up based on these signature patterns.
* **Usage:** Color-code discrete order blocks within the book visualizer to separate market makers, retail flow clusters, and predatory high-frequency execution algorithms.

### 8. Queue Refilling & Hidden Iceberg Diagnostic
* **Reasoning:** Participants often use hidden iceberg orders to minimize market impact. By measuring whether a specific price tier fills back up immediately after an aggressive sweep, we can expose the hidden resting size.
* **Implementation:** Monitor price tiers that undergo trade consumption. If a trade executes for volume $V_{\text{trade}}$ equal to the visible top-of-book size $V_{\text{visible}}$, and a new order is instantly inserted at that identical price tier within an ultra-short latency window $\Delta t < \delta$ without the price changing, log an iceberg detection flag:
  $$\text{Iceberg Flag} = \mathbf{1}_{\{\Delta t \le \text{Latency Threshold}\}} \times (V_{\text{new}} \approx V_{\text{visible}})$$
* **Usage:** Annotate the depth visualization with a distinct marker on the affected price level. Strategies can safely trade against these levels, knowing deep passive inventory is backstopping the position.

### 9. Order Size Roundness & Institutional Profiling
* **Reasoning:** Retail participants and simple electronic market makers typically submit erratic or fractional order sizes. Large institutional desks or block-trading algorithms frequently utilize clean, round lot configurations (e.g., 500, 1000, 5000) or structured randomized scales.
* **Implementation:** Compute a running metric representing the distribution profile of order sizes across the book:
  $$\text{Roundness Index} = \frac{\sum_{i=1}^n v_i \cdot [\mathbf{1}_{\{v_i \pmod K == 0\}}]}{V_p}$$
  where $K$ represents standard block size thresholds (e.g., 100 or 500 lots).
* **Usage:** Flag and highlight price levels anchored by highly standardized round volumes to quickly identify institutional walls.

### 10. Fleet-Footed Cancellation Tracking (Ghost Liquidity)
* **Reasoning:** High-frequency quoting systems frequently inject massive size into the book simply to claim queue priority, pulling the orders the exact instant a trade becomes imminent. This is ghost liquidity.
* **Implementation:** Track the ratio of cancellations that occur when the inside market shifts within 1 tick of the target order's level. Calculate the Ghost Liquidity Ratio:
  $$\text{GLR}_p = \frac{\sum v_{\text{canceled}} \text{ when } |p - p_{\text{mid}}| \le 1 \text{ tick}}{\sum v_{\text{inserted}}}$$
* **Usage:** Visually mute or desaturate the volume bars of price tiers heavily populated by high-GLR profiles to ensure traders do not count on false liquidity.

---

## Phase 3: Spatial Topology & Latency Arbitrage Signals
*Focus: Measuring cross-tier propagation speeds, queue migration vectors, and latency friction.*

### 11. Queue Migration & Velocity Vectors
* **Reasoning:** Orders do not just appear; they move. Tracking whether volume is steadily migrating closer to the inside market or retreating to deeper tiers reveals shifts in market urgency.
* **Implementation:** Track individual order ID modifications that shift price levels without loss of queue tracking identity. Compute the directional velocity vector of the book depth:
  $$\vec{V}_{\text{migration}} = \sum_{i=1}^m \Delta p_i \cdot \frac{v_i}{\Delta t}$$
* **Usage:** Render small vector directional arrows next to the aggregated price bars to show if a tier is advancing or retreating.

### 12. Head-to-Tail Execution Latency Diagnostics
* **Reasoning:** Measuring the precise duration it takes for an aggressive trade sweep to propagate from the first matching order node to the trailing order node reveals the systemic queue friction.
* **Implementation:** For any trade sequence matching multiple individual resting orders across the exact same timestamp packet, compute the latency delta between the initial execution node receipt and the trailing execution node prune message:
  $$\Delta \tau_{\text{sweep}} = t_{\text{tail\_execution}} - t_{\text{head\_execution}}$$
* **Usage:** Displayed as a system health status metric. Sudden drops in this latency mark periods of extreme high-frequency matching engine pressure.

### 13. Cancel-to-Fill Volatility Ratio (CFR)
* **Reasoning:** A healthy market environment exhibits stable execution rates relative to modifications. When the ratio of cancels to actual trades spikes exponentially, the order book state is highly unstable and prone to false breakouts.
* **Implementation:** For a given time window, calculate:
  $$\text{CFR} = \frac{\sum \text{Count}(\text{Cancellations})}{\sum \text{Count}(\text{Trade Fills})}$$
* **Usage:** Monitor as a high-level dial or structural warning light on the layout. High CFR warns automated engines to avoid aggressive momentum strategies.

### 14. Spread Compression Elasticity & Resilience
* **Reasoning:** When a large trade completely sweeps the inside market, the spread widens. The speed at which passive market makers step in to close that gap reflects the baseline resilience of the book.
* **Implementation:** Upon a trade that widens the bid-ask spread, instantiate a high-precision hardware timer. Stop the timer the exact microsecond a new passive order arrives to compress the spread back to its structural baseline:
  $$\text{Resilience} = t_{\text{spread\_restored}} - t_{\text{spread\_widened}}$$
* **Usage:** Display as a rolling mean recovery speed. A low resilience score indicates thin liquidity backstops, warning strategies of high gap risk.

### 15. Cross-Tier Pull-Pressure Diagnostics
* **Reasoning:** If a participant pulls a massive order from the ask side and instantly re-injects that exact volume on the bid side, it signals an immediate directional pivot.
* **Implementation:** Monitor the book for large cancellations ($> \text{Threshold}$). For each event, watch the inverse side of the book within a tight time window $\delta t$ for an identical or highly correlated volume insertion:
  $$\text{Pull-Pressure Match} = \mathbf{1}_{\{v_{\text{bid\_add}} \approx v_{\text{ask\_cancel}}\}} \times \mathbf{1}_{\{\Delta t \le \delta t\}}$$
* **Usage:** Flash an alert when massive liquidity blocks dynamically cross from one side of the market to the other.

---

## Phase 4: Statistical Micro-Alphas & Inventory Imbalance
*Focus: Calculating predictive mathematical indicators for automated systematic trading strategies.*

### 16. Localized Skew & Micro-Price Curvature
* **Reasoning:** Standard micro-price assumes a linear relationship between bid/ask sizes. L3 curvature captures higher-order distribution variations across multiple price levels simultaneously.
* **Implementation:** Calculate the localized volume skew using weightings from the top 5 deep book levels:
  $$\text{Skew}_{\text{L3}} = \frac{\sum_{k=1}^5 w_k \cdot (v_{\text{bid}, k} - v_{\text{ask}, k})}{\sum_{k=1}^5 w_k \cdot (v_{\text{bid}, k} + v_{\text{ask}, k})}$$
  where $w_k = 1/k$ (harmonic distance decay parameter).
* **Usage:** Serves as the primary short-term pricing center line in visualization, acting as a high-frequency fair value anchor.

### 17. Multi-Level Queue Coherence Index
* **Reasoning:** In an orderly, highly liquid market, price tiers move in tandem. If the first 3 levels are expanding in size while level 4 and 5 are collapsing, the underlying order flow is structurally incoherent, signaling an imminent regime shift.
* **Implementation:** Compute the rolling correlation matrix of depth adjustments across the top 5 levels. The Coherence Index is defined as the primary eigenvalue of this cross-level correlation matrix.
* **Usage:** Display as a single structural health coefficient. Low coherence triggers a defensive mode inside passive market-making strategies.

### 18. Tail Liquidity Decay Coefficient
* **Reasoning:** The depth profile of an asset can decay exponentially, linearly, or flatly as you step away from the mid-price. Tracking this decay curve shape shows how protected the book is against massive multi-tier market sweeps.
* **Implementation:** Fit a power-law distribution to the aggregated volume across the first 10 price levels away from the inside market:
  $$V_x = C \cdot x^{-\alpha}$$
  Extract the structural decay exponent $\alpha$ via high-speed log-linear regression.
* **Usage:** Plot the fitted power-law curve overlaying the extended depth chart to visually illustrate structural support thickness.

### 19. Passive Order Modification Clustering (Run Lengths)
* **Reasoning:** High-frequency execution strategies often adjust their quotes by shifting them up or down 1 tick continuously to capture priority. Tracking these sequential runs tells us whether an algorithm is aggressively chasing the market.
* **Implementation:** For a specific order profile track, count sequential `Modify` instructions that maintain identical size but iteratively increment or decrement price steps without pauses:
  $$\text{Run Length} = \sum \mathbf{1}_{\{\Delta p = \pm 1 \text{ tick}, \, \Delta t < \epsilon\}}$$
* **Usage:** Highlight chasing behavior on the UI to warn manual traders against attempting to front-run the chasing algorithm.

### 20. Inventory-Induced Asymmetry Flags
* **Reasoning:** Market makers facing inventory imbalances will skew their quoting behavior, widening one side of their spread while making the other side highly dense to attract inventory-balancing trades.
* **Implementation:** Measure the structural imbalance in distance between the mid-price and the structural volume centers on both sides of the book:
  $$\text{Asymmetry} = \left| \frac{\sum (p_{\text{ask},i} \cdot v_i)}{\sum v_i} - p_{\text{mid}} \right| - \left| p_{\text{mid}} - \frac{\sum (p_{\text{bid},i} \cdot v_i)}{\sum v_i} \right|$$
* **Usage:** Displays as an asymmetrical shift on the depth layout, showing which side of the market is actively absorbing inventory pressure.

---

## Phased Implementation Schedule

```
  Phase 1: Core Topology   ==================> [Weeks 1-3]
  Phase 2: Behavioral      =========================> [Weeks 4-6]
  Phase 3: Spatial Tech    ===============================> [Weeks 7-9]
  Phase 4: Micro-Alphas    =====================================> [Weeks 10-12]
```

### Next Steps for Implementation
1. **Zero-Copy Memory Layout:** Ensure all metrics in Phase 1 hooks directly into the existing L3 linked-list nodes without introducing additional allocations or heap overhead.
2. **Ring Buffer Scopes:** Implement fixed-capacity circular buffers for the multi-horizon metrics to guarantee a strict $O(1)$ write path cost.
