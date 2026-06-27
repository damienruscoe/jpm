
To assemble a comprehensive master index of quantitative trading signals, we must look beyond basic retail indicators and classify the structural micro-alphas, mathematical operators, and cross-asset variables used across high-frequency trading (HFT) desks, statistical arbitrage funds, and systematic macro managers.
The catalog below outlines **201 distinct signals** used across the institutional financial trading spectrum.
### Classification Legends
 * **Data Requirement (Level):**
   * **L1:** Top-of-Book only (Inside Bid, Inside Ask, Last Trade Price/Volume).
   * **L2:** Aggregated Depth (Price-level volumes up to N tiers).
   * **L3:** Individual Order Entities (Discrete order IDs, exact queue priority positions, individual order lifespans).
   * **ALT:** Alternative Data / Fundamental Data / Cross-Asset Feeds.
 * **Primary Inputs:** Time (T), Price (P), Quantity/Volume (Q), Order Lifespan (\tau), Queue Position (\kappa).
 * **Institutional Status:** **Core** (Foundational/Ubiquitous), **Advanced** (Standard Alpha Research), **Niche/Exotic** (Highly structural or context-specific).
## 1. Pure L3 Microstructure & Queue Topology Signals (1–25)
| Implemented | # | Signal Name | Data Level | Primary Inputs | Institutional Status | Description |
|-------------|---|---|---|---|---|---|
| [ ] | 1 | **Relative Queue Position (RQP)** | L3 | Q, \kappa | Core | Tracks the exact volume standing between a specific order ID and the execution boundary. |
| [ ] | 2 | **Estimated Time to Fill (ETTF)** | L3 | T, Q, \kappa | Core | Divides RQP by the rolling localized aggressive trade matching velocity. |
| [ ] | 3 | **Queue Fragmentation Index (HHI)** | L3 | Q | Advanced | Applies the Herfindahl-Hirschman Index to resting orders to see if a tier is a single block or retail noise. |
| [ ] | 4 | **Deterministic Iceberg Identifier** | L3 | T, P, Q | Advanced | Flags instant, identical passive lot replacement within a sub-millisecond latency window following a sweep. |
| [ ] | 5 | **Order Lifespan Decay (OLD)** | L3 | T, \tau | Advanced | Moving median of the delta between order creation and cancellation at specific tick offsets. |
| [ ] | 6 | **Fleet-Footed Cancellation Ratio** | L3 | T, P, Q | Advanced | Volume ratio of cancellations occurring when the inside market shifts within 1 tick of the target order. |
| [ ] | 7 | **Queue Migration Velocity Vector** | L3 | T, P, Q | Exotic | Directional speed of order nodes adjusting their price coordinates without losing tracking identity. |
| [ ] | 8 | **Head-to-Tail Sweep Latency** | L3 | T | Exotic | Time delta between the execution of the first order node and the final node in a single matching engine packet. |
| [ ] | 9 | **L3 Order Flow Imbalance (OFI)** | L3 | Q | Core | Discretizes flow updates into explicit Add, Cancel, Modify, and Fill buckets down to the individual node. |
| [ ] | 10 | **Order Size Roundness Index** | L3 | Q | Advanced | Measures the ratio of clean institutional block sizes (e.g., modulo 100/500) to erratic retail shapes. |
| [ ] | 11 | **Cross-Tier Pull-Pressure Match** | L3 | T, P, Q | Exotic | Detects rapid cancellations on the ask side matched with identical volume insertions on the bid side within \delta t. |
| [ ] | 12 | **Passive Run-Length Chaser** | L3 | T, P | Advanced | Counts sequential modify steps where an algorithm adjusts its price by exactly 1 tick to stay ahead. |
| [ ] | 13 | **Queue Concentration Asymmetry** | L3 | Q, \kappa | Advanced | Measures structural divergence between the HHI profiles of the buy side versus the sell side. |
| [ ] | 14 | **Resting Order Age Heat Slope** | L3 | T | Advanced | Volume-weighted age profile of resting liquidity, mapping the baseline friction of sticky versus transient flow. |
| [ ] | 15 | **Phantom Liquidity Exponent** | L3 | T, Q, \tau | Exotic | Statistical measure of queue tiers backed by high cancel-to-fill rates over rolling sub-second windows. |
| [ ] | 16 | **Queue Step-Out Elasticity** | L3 | T, P, Q | Exotic | Speed at which individual market-maker entities back away (delete positions) during volatile aggressive bursts. |
| [ ] | 17 | **Aggressive Trade Concentration** | L3 | Q | Advanced | Evaluates whether incoming market orders are draining inventory from single large nodes or clearing multiple small ones. |
| [ ] | 18 | **Order Modification Velocity** | L3 | T | Core | The aggregate frequency of order adjustments relative to new entries within a specific ticker queue. |
| [ ] | 19 | **Order Size Clustering Divergence** | L3 | Q | Exotic | Tracks anomalies when non-standard, specific algorithmic lot sizes appear simultaneously across tiers. |
| [ ] | 20 | **Inside Queue Replenishment Rate** | L3 | T, Q | Core | The sub-millisecond return speed of passive liquidity immediately after the inside level is completely cleared. |
| [ ] | 21 | **Passive Order Priority Alpha** | L3 | \kappa | Exotic | Calculates the edge gained or lost by moving up or down the queue relative to structural tick spreads. |
| [ ] | 22 | **Order Level Lifetime Skew** | L3 | \tau | Advanced | Compares the average life expectancy of buy orders versus sell orders at deep price offsets. |
| [ ] | 23 | **Spoofing Signature Coefficient** | L3 | T, P, Q, \tau | Exotic | Real-time score evaluating if large size nodes match the specific profile of known non-executing algorithmic overlays. |
| [ ] | 24 | **Queue Density Sharpness** | L3 | Q, \kappa | Advanced | The rate of change in order counts relative to total cumulative volume as you dig deeper into a specific price level. |
| [ ] | 25 | **L3 Flow Volatility Index** | L3 | T, Q | Advanced | Standard deviation of individual node updates, capturing high-frequency structural noise. |
## 2. Level 2 Book Topology & Depth Analytics (26–60)
| Implemented | # | Signal Name | Data Level | Primary Inputs | Institutional Status | Description |
|-------------|---|---|---|---|---|---|
| [ ] | 26 | **L2 Order Flow Imbalance (OFI)** | L2 | P, Q | Core | Measures net changes in aggregated bid/ask sizes across the top N levels between ticks. |
| [ ] | 27 | **Volume Imbalance (VOI)** | L2 | Q | Core | Tracks the net change in volume at the inside bid and ask, isolating active depth shifts. |
| [X] | 28 | **Micro-Price** | L2 | P, Q | Core | Mid-price weighted by the ratio of bid/ask liquidity to capture short-term directional pressure. |
| [ ] | 29 | **Book Depth Skewness (Top 5)** | L2 | Q | Core | Ratio of aggregate buy depth to aggregate sell depth across the front five price levels. |
| [ ] | 30 | **Tail Liquidity Decay Exponent (\alpha)** | L2 | P, Q | Advanced | Power-law distribution coefficient tracking how quickly depth thins out as price moves away from mid. |
| [ ] | 31 | **Multi-Level Coherence Index** | L2 | Q | Exotic | Primary eigenvalue of the rolling correlation matrix of depth changes across different price levels. |
| [X] | 32 | **Spread Compression Elasticity** | L2 | T, P | Core | High-precision time metric recording how long the bid-ask gap stays widened after a deep sweep. |
| [ ] | 33 | **Inventory-Induced Asymmetry** | L2 | P, Q | Advanced | Distance divergence between the raw mid-price and the center of volume mass of the book. |
| [ ] | 34 | **Slippage Cost Curve Gradient** | L2 | P, Q | Core | Real-time slope derivative representing cost-per-lot as a simulated market order walks the L2 book. |
| [ ] | 35 | **Depth Convexity Index** | L2 | P, Q | Advanced | Measures the second derivative of cumulative depth to locate structural liquidity walls. |
| [ ] | 36 | **Inside Level Volumetric Imbalance** | L2 | Q | Core | Percentage calculation of (\text{Bid}_1 - \text{Ask}_1) / (\text{Bid}_1 + \text{Ask}_1). |
| [ ] | 37 | **Book Density Kurtosis** | L2 | P, Q | Exotic | Evaluates the heavy-tailed nature of volume distribution across the extended L2 matrix. |
| [ ] | 38 | **Bid-Ask Spread Variance** | L2 | P | Core | Moving standard deviation of the difference between the inside ask and inside bid. |
| [ ] | 39 | **Deep Book Cushion Ratio** | L2 | Q | Advanced | The volume ratio of levels 6–20 compared to levels 1–5, capturing macro-structural backstops. |
| [ ] | 40 | **Level-to-Level Liquidity Gaps** | L2 | P | Core | Tracks empty price ticks inside the active order book structure (holes in the market matrix). |
| [X] | 41 | **Weighted Mid-Price Curvature** | L2 | P, Q | Advanced | Polynomial curve fitting across multiple levels to identify non-linear pricing pressure. |
| [ ] | 42 | **Bid/Ask Level Cross-Correlation** | L2 | T, Q | Advanced | Rolling synchronization index measuring whether asks fill or vanish when bids expand. |
| [ ] | 43 | **Total Book Capitalization (Notional Value)** | L2 | P, Q | Core | Summation of P \times Q across the entire visible multi-level depth table. |
| [ ] | 44 | **Order Book Entropy** | L2 | Q | Exotic | Shannon entropy metric calculating the randomness or structural dispersion of volume across tiers. |
| [ ] | 45 | **Resting Depth Half-Life** | L2 | T, Q | Advanced | Time metric tracking how long an aggregated block of volume remains constant before a structural change. |
| [ ] | 46 | **Level-Step Liquidity Resistance** | L2 | P, Q | Core | Total volume required to move the current mid-price by exactly X basis points. |
| [ ] | 47 | **Bid/Ask Distance Asymmetry** | L2 | P | Core | Measures if the distance from the mid-price to level 5 ask matches the distance to level 5 bid. |
| [ ] | 48 | **Inside Spread Jump Probability** | L2 | P, Q | Exotic | Markov-chain state indicator evaluating the likelihood of a multi-tick price gap on the next execution. |
| [ ] | 49 | **Cumulative Volume Delta (CVD)** | L2 | Q | Core | Running summation of aggressive volume buys minus aggressive volume sells. |
| [ ] | 50 | **Book Horizon Saturation Index** | L2 | Q | Advanced | Evaluates whether volume additions are expanding at the inside market or at the deep boundary. |
| [ ] | 51 | **Top-of-Book Size Momentum** | L2 | T, Q | Core | The rate of change of volume sitting at the inside bid and ask quotes over a rolling window. |
| [ ] | 52 | **Spread Hull Moving Average** | L2 | T, P | Advanced | Low-lag moving average of the bid-ask spread to smooth micro-oscillations without signal delay. |
| [ ] | 53 | **Depth Absorption Ratio** | L2 | Q | Core | Volume consumed via aggressive trades divided by the simultaneous replacement volume at that tier. |
| [ ] | 54 | **Level 2 Layer Velocity** | L2 | T, P | Advanced | Tracks the physical speed (updates per millisecond) of different depth tiers. |
| [ ] | 55 | **Asymmetric Cancel Pressure** | L2 | Q | Core | Measures if cancellations are happening at a faster rate on one side of the book over the other. |
| [ ] | 56 | **Deep Level Replenishment Bias** | L2 | Q | Exotic | Tracks if passive market makers prefer reloading bids over asks at deep out-of-the-money offsets. |
| [ ] | 57 | **Spread Mean-Reversion Speed** | L2 | T, P | Advanced | The statistical velocity at which a wide spread snaps back to its historical structural mode. |
| [ ] | 58 | **Book Volume Concentration Gini** | L2 | Q | Advanced | Employs the Gini coefficient to evaluate the inequality of volume distribution across book levels. |
| [ ] | 59 | **Inside Depth Compression Ratio** | L2 | Q | Core | Ratio of inside depth to total book depth, identifying when liquidity is concentrated at the edge. |
| [ ] | 60 | **Order Book Cross-Sectional Variance** | L2 | Q | Exotic | Statistical variance of the volume profiles evaluated across distinct time snapshots. |
## 3. High-Frequency Tick-Level Trade Execution Signals (61–95)
| Implemented | # | Signal Name | Data Level | Primary Inputs | Institutional Status | Description |
|-------------|---|---|---|---|---|---|
| [ ] | 61 | **Volume-Synchronized Probability of Toxicity (VPIN)** | L1 | P, Q | Core | Measures order flow toxicity by checking volume imbalances within standardized volume buckets. |
| [ ] | 62 | **Tick-Based Last Trade Price (LTP) Delta** | L1 | P | Core | Direct tick-to-tick directional difference of consecutive trade executions. |
| [ ] | 63 | **Aggressor Side (Lee-Ready Algorithm)** | L1 | P | Core | Classifies trades as buy- or sell-initiated based on relation to the mid-price and prior price ticks. |
| [ ] | 64 | **Trade Size Acceleration** | L1 | T, Q | Core | Rate of change of executed contract volumes per unit of time or tick bucket. |
| [X] | 65.1 | **Rolling Volume-Weighted Average Price (VWAP)** | L1 | P, Q | Core | Provides a session-wide volume-weighted average price, acting as an objective execution performance anchor and historical benchmark rather than a localized momentum signal. |
| [ ] | 65.2 | **Rolling Volume-Weighted Average Price (VWAP)** | L1 | P, Q | Core | Cumulative notional value divided by cumulative volume over a rolling window. |
| [X] | 66.1 | **Exponentially Weighted VWAP (EMA VWAP)** | L1 | P, Q | Core | A highly responsive, volume-weighted rolling average price utilizing exponential decay; serves as a real-time proxy for short-term aggressive liquidity flow pressure. |
| [ ] | 66.2 | **Exponentially Weighted VWAP (EMA VWAP)** | L1 | P, Q | Core | Applies an exponential smoothing factor to the VWAP tracking calculation. |
| [ ] | 67 | **Time-Weighted Average Price (TWAP) Dev** | L1 | T, P | Core | The current spot price distance from the linear time-weighted average benchmark. |
| [ ] | 68 | **Trade-to-Quote Frequency Ratio** | L1 | T | Advanced | Ratio of trade prints to order book quote changes, capturing matching engine trade efficiency. |
| [ ] | 69 | **Aggressive Block Trade Detector** | L1 | Q | Core | Flags individual executions that exceed a high statistical threshold of standard lot sizes. |
| [ ] | 70 | **Tick Run Length (Directional Persistence)** | L1 | P | Core | Counts consecutive trades executing with positive or negative price momentum. |
| [ ] | 71 | **Trade Volume Concentration Index** | L1 | Q | Advanced | Measures if daily volume is driven by a few large prints or many small, distributed fills. |
| [ ] | 72 | **Sub-Second Realized Volatility** | L1 | T, P | Core | Standard deviation of log returns calculated over millisecond or tick intervals. |
| [ ] | 73 | **Aggressive Buy/Sell Lot Ratio** | L1 | Q | Core | Ratio of buy-initiated trade sizes to sell-initiated trade sizes over a rolling window. |
| [ ] | 74 | **Trade Density Per Second** | L1 | T | Core | The count of discrete trade prints executing within a rolling 1000ms window. |
| [ ] | 75 | **Flash Sweep Volumetric Footprint** | L1 | P, Q | Advanced | Measures total volume consumed when a single trade sequence wipes out multiple price tiers. |
| [ ] | 76 | **Inter-Trade Arrival Time Variance** | L1 | T | Advanced | Tracking variance in trade timing; clustering shifts indicate institutional execution activity. |
| [ ] | 77 | **Price Impact Per Lot (Kyle's Lambda)** | L1 | P, Q | Advanced | Regression slope of price change relative to executed trade volume, gauging market thinness. |
| [ ] | 78 | **Trade-Mid Divergence Delta** | L1 | P | Core | The instantaneous delta between a trade's execution price and the prevailing mid-price. |
| [ ] | 79 | **Microsecond Drift Exponent** | L1 | T, P | Advanced | Hurst exponent calculated over ticks to classify market states as mean-reverting or trending. |
| [ ] | 80 | **Passive Trade Fill Velocity** | L1 | T, Q | Advanced | Speed at which passive orders are filled by market orders at the inside touch. |
| [ ] | 81 | **Aggressive Sweep Decay Time** | L1 | T, P | Exotic | Time required for price to recover to pre-sweep levels after a multi-level execution event. |
| [ ] | 82 | **Notional Volume Throughput Acceleration** | L1 | T, P, Q | Core | Derivative of the cumulative dollar volume flowing through the matching engine. |
| [ ] | 83 | **Trade-Sized Z-Score** | L1 | Q | Core | Standard score of the current trade volume relative to its rolling historical mean and variance. |
| [ ] | 84 | **Inside Touch Trade Exhaustion** | L1 | Q | Advanced | Flags when aggressive trades slow down while approaching a major resting depth wall. |
| [ ] | 85 | **Tick-Level Autocorrelation Index** | L1 | P | Advanced | Measures statistical correlation between the directions of consecutive trade updates. |
| [ ] | 86 | **Trade Size vs Depth Cushion Ratio** | L1 | Q | Core | Compares incoming trade size to total volume at the target tier to evaluate consumption depth. |
| [ ] | 87 | **Aggressive Order Slicing Signature** | L1 | T, Q | Advanced | Detects uniform volume signatures spaced at fixed time intervals, identifying algorithmic execution. |
| [ ] | 88 | **Realized Micro-Skewness** | L1 | P | Advanced | Third statistical moment of tick returns, identifying directional asymmetry over ultra-short frames. |
| [ ] | 89 | **Trade Volume Clustering Index** | L1 | T, Q | Exotic | Applies point-process models (like Hawkes processes) to measure self-exciting trade clusters. |
| [ ] | 90 | **Effective-to-Realized Spread Ratio** | L1 | P | Advanced | Compares immediate transaction execution costs against price changes over short horizons. |
| [ ] | 91 | **Post-Trade Bid-Ask Spread Drift** | L1 | P | Core | Measures whether the spread expands or contracts in the ticks following a large trade block. |
| [ ] | 92 | **Aggressive Volume Weighted Drift** | L1 | P, Q | Core | Measures price directional drift, weighting each price change by the trade's active size. |
| [ ] | 93 | **Tick Drawdown Velocity** | L1 | T, P | Core | Speed and depth of consecutive down-ticks before a positive tick occurs. |
| [ ] | 94 | **Inside Market Trade Penetration** | L1 | Q | Advanced | Measures how deeply into the inside tier volume an aggressive trade prints before halting. |
| [ ] | 95 | **High-Frequency Return Realized Kurtosis** | L1 | P | Advanced | Measures the fourth moment of tick returns to evaluate high-frequency gap or jump risk. |
## 4. Momentum, Mean Reversion & Technical Moving Averages (96–135)
| Implemented | # | Signal Name | Data Level | Primary Inputs | Institutional Status | Description |
|-------------|---|---|---|---|---|---|
| [ ] | 96 | **Simple Moving Average (SMA) Crossover** | L1 | P | Core | Standard crossover of short-horizon price averages versus long-horizon price averages. |
| [X] | 97.1 | **Exponential Moving Average (EMA)** | L1 | P | Core | A trade-count-based exponential moving average of execution prices; functions as a low-latency price trend baseline for calculating divergence and mean-reversion signals. |
| [ ] | 97.2 | **Exponential Moving Average (EMA) Divergence** | L1 | P | Core | Measures the gap between spot price and its exponentially smoothed historical average. |
| [ ] | 98 | **Moving Average Convergence Divergence (MACD)** | L1 | P | Core | Difference between two distinct EMAs, typically smoothed by a secondary signal line. |
| [ ] | 99 | **Relative Strength Index (RSI)** | L1 | P | Core | Momentum oscillator measuring the speed and change of price movements over defined lookbacks. |
| [ ] | 100 | **Bollinger Band Width & Z-Score** | L1 | P | Core | Measures price distance from moving averages normalized by rolling standard deviations. |
| [ ] | 101 | **Average True Range (ATR) Volatility** | L1 | P | Core | Measures asset volatility by looking at the decomposition of high, low, and closing ranges. |
| [ ] | 102 | **Stochastic Oscillator (\%K / \%D)** | L1 | P | Core | Compares a closing price to its price range over a specified historical period. |
| [ ] | 103 | **Commodity Channel Index (CCI)** | L1 | P | Core | Tracks the current price deviation relative to its average statistical distance. |
| [ ] | 104 | **Rate of Change (ROC) Momentum** | L1 | P | Core | Pure percentage calculation of current price relative to price N periods ago. |
| [ ] | 105 | **Directional Movement Index (DMI / ADX)** | L1 | P | Core | Evaluates the overall strength and expansion of a trend, independent of its direction. |
| [ ] | 106 | **Hull Moving Average (HMA) Derivative** | L1 | P | Advanced | First derivative of the HMA, optimized to minimize delay while eliminating curvature noise. |
| [ ] | 107 | **Kaufman's Adaptive Moving Average (KAMA)** | L1 | T, P | Advanced | Dynamically adjusts its smoothing coefficient based on the market's noise-to-signal ratio. |
| [ ] | 108 | **Chande Momentum Oscillator (CMO)** | L1 | P | Advanced | Modified RSI using data in both numerator and denominator to capture raw momentum. |
| [ ] | 109 | **Donchian Channel Breakout Delta** | L1 | P | Core | Instantly measures price proximity to the N-period high or low boundary. |
| [ ] | 110 | **Keltner Channel Squeeze Index** | L1 | P | Advanced | Measures the structural alignment of Bollinger Bands compressing inside Keltner Channels. |
| [ ] | 111 | **Parabolic SAR Acceleration Delta** | L1 | P | Core | Trailing stop-and-reverse indicator tracking trailing momentum step factors. |
| [ ] | 112 | **Trix Oscillator** | L1 | P | Core | Triple exponentially smoothed moving average momentum oscillator. |
| [ ] | 113 | **Arsh Target Velocity Indicator** | L1 | P, Q | Exotic | Time-decayed momentum tracking index weighting distance by trade velocity. |
| [ ] | 114 | **Vortex Indicator (VI+/VI-) Crossing** | L1 | P | Advanced | Captures trend intersection points by comparing the distance between consecutive highs/lows. |
| [ ] | 115 | **Linear Regression Slope Feature** | L1 | T, P | Core | Calculates the mathematical slope of the best-fit line across historical close data. |
| [ ] | 116 | **Detrended Price Oscillator (DPO)** | L1 | P | Advanced | Strips out long-term cycles from price data to isolate short-term overbought/oversold states. |
| [ ] | 117 | **Money Flow Index (MFI)** | L1 | P, Q | Core | Volume-weighted relative strength index measuring buying and selling pressure. |
| [ ] | 118 | **Accumulation/Distribution Line Slope** | L1 | P, Q | Core | Tracks cumulative volume flows relative to where a bar closes within its range. |
| [ ] | 119 | **Chaikin Oscillator** | L1 | P, Q | Core | Applies MACD principles directly to the Accumulation/Distribution line state. |
| [ ] | 120 | **Volume Price Trend (VPT) Indicator** | L1 | P, Q | Core | Adds or subtracts volume from a running total based on daily percentage price updates. |
| [ ] | 121 | **On-Balance Volume (OBV) Momentum** | L1 | P, Q | Core | Running total of trading volume that increments on up-days and decrements on down-days. |
| [ ] | 122 | **Ease of Movement (EOM) Vector** | L1 | P, Q | Core | Evaluates how easily a price can move up or down based on volume efficiency configurations. |
| [ ] | 123 | **Coppock Curve Momentum Matrix** | L1 | P | Advanced | Smoothed momentum indicator tracking long-term cyclical shifts. |
| [ ] | 124 | **True Strength Index (TSI)** | L1 | P | Advanced | Double-smoothed momentum indicator that limits erratic price ripples. |
| [ ] | 125 | **Fisher Transform Market Polarizer** | L1 | P | Advanced | Converts price data into a Gaussian normal distribution to clearly isolate turning points. |
| [ ] | 126 | **Ultimate Oscillator Profile** | L1 | P | Core | Combines three distinct lookback horizons using a weighted short-, medium-, and long-term calculation. |
| [ ] | 127 | **Williams \%R Momentum Threshold** | L1 | P | Core | Inverse scale oscillator tracking high-to-close placement inside a defined lookup window. |
| [ ] | 128 | **Arnoon Indicator (Aroon Up / Aroon Down)** | L1 | T | Core | Measures time elapsed since the asset recorded its highest high or lowest low over N intervals. |
| [ ] | 129 | **Elder Ray Index (Bull/Bear Power)** | L1 | P | Advanced | Estimates buying and selling pressure by comparing extreme price points to an EMA. |
| [ ] | 130 | **Center of Gravity Oscillator** | L1 | P | Exotic | Filters price moves by determining the center of mass within a finite data window. |
| [ ] | 131 | ** Arnaud Legoux Moving Average (ALMA)** | L1 | P | Advanced | Uses a shifted Gaussian filter to reduce lag while controlling response smoothness. |
| [ ] | 132 | **Zero-Lag Exponential Moving Average** | L1 | P | Advanced | Strips lag out of an EMA by tracking and adding prior data offsets back into the calculation. |
| [ ] | 133 | **Mass Index Trend Reversal** | L1 | P | Advanced | Examines high-low ranges over time to detect trend reversals based on range expansion. |
| [ ] | 134 | **Schaff Trend Cycle (STC)** | L1 | P | Advanced | Combines MACD with a secondary stochastic calculation to speed up cycle trend detection. |
| [ ] | 135 | **Volumetric Price Target Distance** | L1 | P, Q | Core | Distance between the spot price and the volume-node center of a daily distribution profile. |
## 5. Statistical Arbitrage, Cointegration & Cross-Asset Signals (136–165)
| Implemented | # | Signal Name | Data Level | Primary Inputs | Institutional Status | Description |
|-------------|---|---|---|---|---|---|
| [ ] | 136 | **Pairs Trading Z-Score Residual** | ALT | P | Core | Tracks price divergence from the historical cointegration vector of two highly correlated assets. |
| [ ] | 137 | **Cross-Asset Lead-Lag Correlation Index** | ALT | T, P | Advanced | Cross-correlation calculation identifying lag delays between a primary market leader and secondary follower. |
| [ ] | 138 | **ETF-Underlying Basket NAV Arbitrage Delta** | ALT | P, Q | Core | Real-time tracking of net asset value discrepancies between an ETF and its constituent components. |
| [ ] | 139 | **Spot-Futures Basis Spread** | ALT | P | Core | Absolute price difference between a spot asset and its matching front-month futures derivative contract. |
| [ ] | 140 | **Cross-Exchange Liquidity Arbitrage Index** | ALT | P, Q | Advanced | Compares order book depth across multiple venues to catch pricing discrepancies for identical assets. |
| [ ] | 141 | **Principal Component Analysis (PCA) Residual** | ALT | P | Advanced | Statistical arbitrage feature mapping an asset's price movement against systemic equity risk factors. |
| [ ] | 142 | **Implied Volatility vs Realized Vol Risk Prem** | ALT | P | Advanced | Spread metric tracking options-implied volatility against actual trailing price variance. |
| [ ] | 143 | **Future Calendar Spread Divergence** | ALT | P | Core | Pricing variance between near-month and far-month futures contracts on the same underlying asset. |
| [ ] | 144 | **Cross-Asset Order Flow Imbalance Elasticity** | ALT | Q | Exotic | Measures order book flow updates in one asset following an imbalance shock in a correlated vehicle. |
| [ ] | 145 | **Beta-Adjusted Market Deviation Vector** | ALT | P | Core | An asset's outperformance or underperformance relative to a benchmark index, adjusted for beta. |
| [ ] | 146 | **Corporate Bond-to-Equity Capital Struct Spread** | ALT | P | Exotic | Tracks price divergences between credit protection swaps/bonds and the equity valuation layer. |
| [ ] | 147 | **Commodity Crack/Crush Margin Dislocation** | ALT | P | Advanced | Spread matrix tracking raw commodity inputs against finished processing output values. |
| [ ] | 148 | **ADR-Local Equity Parity Drift** | ALT | P | Core | Valuation divergence between an American Depositary Receipt and its underlying home market listing. |
| [ ] | 149 | **Currency Cross-Rate Triangle Arbitrage** | ALT | P | Core | Instantaneous implied loop rate discrepancy across three interconnected foreign currency books. |
| [ ] | 150 | **Index Arbitrage Program Execution Flow** | ALT | T, Q | Advanced | Monitors index-level baskets for simultaneous automated execution activity. |
| [ ] | 151 | **Cross-Commodity Substitution Ratio** | ALT | P | Advanced | Relative price relationship between competitive commodities (e.g., natural gas vs coal). |
| [ ] | 152 | **Realized Co-Variance Exponent** | ALT | P | Advanced | Multi-asset rolling high-frequency calculation measuring covariance shifts across assets. |
| [ ] | 153 | **Global Depositary Receipt Premium Vector** | ALT | P | Core | Deviation percentage tracking international listings against home market price points. |
| [ ] | 154 | **Options Put-Call Ratio Volatility Skew** | ALT | Q | Core | Volume imbalances between put option placement and call option placement across strike chains. |
| [ ] | 155 | **Warrant Implied Parity Dislocation** | ALT | P | Core | Mispricing between a corporate warrant vehicle and its underlying common equity price. |
| [ ] | 156 | **Yield Curve Term Structure Inversion Delta** | ALT | P | Core | The spread gap between long-term government debt instruments and short-term debt rates. |
| [ ] | 157 | **Cross-Venue Sweep Co-Inbound Match** | ALT | T, P | Exotic | Flags coordinated aggressive execution patterns hitting distinct geographic markets simultaneously. |
| [ ] | 158 | **Asset Return Dispersion Skew** | ALT | P | Advanced | Measures return distribution dispersion across a group of sector peers to identify stock-specific anomalies. |
| [ ] | 159 | **Dual-Class Shares Voting Premium Spread** | ALT | P | Advanced | Price spread tracking Class A voting structures against Class B non-voting share equivalents. |
| [ ] | 160 | **Cross-Asset Volume Covariance Spike** | ALT | Q | Advanced | Sudden spikes in trading volume correlation across otherwise independent asset classes. |
| [ ] | 161 | **Inter-Market Order Book Thinness Ratio** | ALT | Q | Advanced | Compares relative depth availability across primary matching engines vs alternative dark pools. |
| [ ] | 162 | **Sovereign Credit Default Swap (CDS) Imbalance** | ALT | P | Exotic | Evaluates risk changes in sovereign debt protection markets relative to index pricing. |
| [ ] | 163 | **VIX Term Structure Premium Divergence** | ALT | P | Core | Price relationships between spot VIX readings and longer-dated volatility futures. |
| [ ] | 164 | **Options Implied Leverage Surface Gradient** | ALT | P | Exotic | Measures changes in option implied volatility surfaces across varying execution horizons. |
| [ ] | 165 | **Multi-Asset Momentum Convergence** | ALT | P | Advanced | Quantitative rank sorting tracking when multiple correlated sectors lock into a shared directional trend. |
## 6. Macro, Fundamental & Alternative Data Signals (166–201)
| Implemented | # | Signal Name | Data Level | Primary Inputs | Institutional Status | Description |
|-------------|---|---|---|---|---|---|
| [ ] | 166 | **Earnings Surprise Residual (Standardized)** | ALT | P | Core | Measures the difference between reported earnings and consensus analyst expectations. |
| [ ] | 167 | **Short Interest Float Saturation Ratio** | ALT | Q | Core | Total shorted volume divided by the total available share float of a corporate entity. |
| [ ] | 168 | **Insider Transaction Volume Imbalance** | ALT | Q | Core | Ratio of legal insider corporate buying filings to corporate selling filings. |
| [ ] | 169 | **Institutional Ownership Concentration Shift** | ALT | Q | Advanced | Tracks large institutional position accumulation or liquidation trends via mandatory filings. |
| [ ] | 170 | **Real-Time Satellite Port Saturation Index** | ALT | ALT | Exotic | Computer-vision tracking score counting container vessels to estimate supply chain shifts. |
| [ ] | 171 | **LLM Corporate Sentiment Scoring Index** | ALT | ALT | Advanced | Natural Language Processing score evaluating transcripts of corporate earnings calls. |
| [ ] | 172 | **Consumer Geo-Location Retail Footprint Velocity** | ALT | ALT | Exotic | Anonymized cellular location tracking measuring traffic changes inside physical retail properties. |
| [ ] | 173 | **Scraped Digital Inventory Deflation Gradient** | ALT | ALT | Advanced | Direct web scraping of retail sites to monitor discount frequencies and evaluate margin pressure. |
| [ ] | 174 | **Supply Chain Counterparty Stress Index** | ALT | P | Advanced | Aggregated health index of a core company's downstream suppliers and upstream buyers. |
| [ ] | 175 | **Central Bank Tone Shift Distance Matrix** | ALT | ALT | Advanced | Text-distance vector comparing a new monetary policy statement to prior policy text. |
| [ ] | 176 | **Freight/Shipping Cost Index Velocity (BDI)** | ALT | P | Core | Rate of change in global bulk freight shipping costs, capturing global demand shifts. |
| [ ] | 177 | **Patent Filing Innovation Trajectory** | ALT | ALT | Exotic | Metrics tracking a firm's patent filing volume and citation indexing to judge future positioning. |
| [ ] | 178 | **Job Posting Deflation / Expansion Velocity** | ALT | ALT | Advanced | Tracks corporate health and growth by monitoring alterations in open employment listings. |
| [ ] | 179 | **Commodity Inventory Warehouse Drawdowns** | ALT | Q | Core | Physical commodity stock drawdowns or additions reported by global exchanges. |
| [ ] | 180 | **Weather-Model Crop Yield Anomaly Index** | ALT | ALT | Exotic | Satellite and moisture models predicting agricultural harvest yield variations. |
| [ ] | 181 | **Real-Time Consumer Digital Checkout Velocity** | ALT | ALT | Exotic | Credit card transaction aggregators measuring immediate spending shifts against baselines. |
| [ ] | 182 | **Corporate Debt Refinancing Maturity Cliff** | ALT | T | Core | Map of upcoming corporate debt maturity exposures relative to prevailing rate environments. |
| [ ] | 183 | **Analyst Target Price Revision Momentum** | ALT | P | Core | Rolling momentum index tracking sell-side analyst revisions to target prices. |
| [ ] | 184 | **Government Procurement Contract Velocity** | ALT | P | Exotic | Dollar volume tracking new state contract assignments to corporate winners. |
| [ ] | 185 | **Social Media Hype Volumetric Acceleration** | ALT | ALT | Advanced | Sudden spikes in mention counts for specific asset handles across online networks. |
| [ ] | 186 | **Dividend Yield Premium Deviation** | ALT | P | Core | Asset dividend yield performance relative to historical historical industry peer averages. |
| [ ] | 187 | **Free Cash Flow Yield Decoupling Exponent** | ALT | P | Core | Measures price divergence relative to core underlying free cash flow generation. |
| [ ] | 188 | **Enterprise Value to EBITDA Multiple Compression** | ALT | P | Core | Moving evaluation of core cash valuation multiples relative to changes in equity pricing. |
| [ ] | 189 | **Regulatory Enforcement Sentiment Metric** | ALT | ALT | Exotic | Natural Language Processing tracker capturing text adjustments inside regulatory enforcement filings. |
| [ ] | 190 | **Real-Time Power Grid Load Demand Index** | ALT | ALT | Exotic | Industrial manufacturing health proxy measuring electricity consumption rates. |
| [ ] | 191 | **Product Recall Volumetric Severity Index** | ALT | ALT | Core | Evaluates financial exposure based on product units pulled relative to firm size. |
| [ ] | 192 | **Corporate Executive Attrition Frequency** | ALT | T | Exotic | Unusual frequency variations in key leadership team departures, tracking corporate stress. |
| [ ] | 193 | **Free Float Velocity Index** | ALT | Q | Advanced | Tracks alterations in active trading float caused by share buyback initiatives. |
| [ ] | 194 | **Sovereign Tariff Adjustment Imbalance** | ALT | ALT | Exotic | Macro matrix scoring financial impact across industries following tariff shifts. |
| [ ] | 195 | **Consumer App Store Download Velocity** | ALT | ALT | Advanced | Download ranking changes for consumer digital apps, serving as a revenue proxy. |
| [ ] | 196 | **Options Implied Dividend Trajectory** | ALT | P | Advanced | Extracting expected future corporate payouts directly from long-dated option structures. |
| [ ] | 197 | **Macro Capital Expenditure Cutback Indicator** | ALT | P | Core | Industry-wide reductions in capital deployment planning, indicating cyclical shifts. |
| [ ] | 198 | **Credit Card Delinquency Rolling Drift Vector** | ALT | ALT | Core | Tracks macroeconomic consumer health via default velocity aggregators. |
| [ ] | 199 | **Corporate Litigation Volumetric Risk Metric** | ALT | ALT | Exotic | Counts active legal complaints filed against corporate entities across court systems. |
| [ ] | 200 | **Real-Time Flight Tracking Cargo Velocity** | ALT | ALT | Exotic | Automatic Dependent Surveillance-Broadcast (ADS-B) tracking of logistics aircraft. |
| [ ] | 201 | **Dark Pool Liquidity Concentration Shift** | L1 | Q | Advanced | Tracks shifts in alternative dark venue execution sizes relative to lit public markets. |


