Matching Engine Coding Assessment 

CONFIDENTIAL \- FOR CANDIDATE USE ONLY 

This assessment is proprietary and confidential. Please do not share, post online, or distribute this  document or your solution. By proceeding with this assessment, you agree to keep all materials  confidential. 

Problem 

You are required to implement a matching engine as part of the exchange simulator in a regression  testing platform. 

Background 

The order matching engine accepts buy and sell orders from clients and matches them up with  price/time priority, which means orders with better price would get matched first and orders at the  same price are matched according to the sequence they enter the matching engine. 

Requirements 

The matching engine will accept a text file of order new/amend/cancel messages as input. 

Every time an order is created/amended and matches with another order, a trade will be produced  and a message shall be printed to standard output. 

We assume the following rules: • When an order crosses the opposite limit (e.g. if we receive Buy @  10 on an existing best Sell @ 10 or 9), a trade will be generated • If the order was fully filled, it should  be removed from the matching engine order book. If the order was partially filled, the quantity  remaining on the order book should be updated 

The program should be able to: 

a) Read a sequence of messages in a text file. Create your C++ program from scratch,  using **C++17/20, Boost/STL as needed**.  

b) Given a sequence of messages, as defined above, construct an in-memory representation of the  current state of the order book. You will need to design your own dataset to test your code.  
c) When two orders cross, print the crossed quantity and price to console (including which side was  the aggressor) 

d) On exit, print out a human-readable representation of the book down to the 5th level for each  product id. 

e) Handle various exceptions with invalid input message and print an error log to standard output.  Cases include but not limited to: • Invalid input message format (missing columns/extra columns) •  Duplicated order ids (for new order) • Cancel/amend message with invalid order id (no  corresponding order) • Negative, missing, or out-of-bounds prices/quantities • Order ID length  exceeding 10 characters • Side changes on amend (not allowed) 

f) Please optimize your solution for latency/throughput whenever possible 

Messages 

The valid format of messages is as follows: 

\<exchange\_ticker\>, \<request\_type\>, \<order\_id\>, \<side\>, \<quantity\>, \<price\> Example: 101,N,A001,B,1000,3.2 

• **exchange\_ticker** \= unique product identifier on the exchange, positive integer • **request\_type** \= N (new), C (cancel), A (amend) 

• **order\_id** \= unique alphanumeric string (max 10 characters) to identify each order; also used to  reference existing orders for cancel/amend. Allowed characters: \[A-Za-z0-9-\] 

• **side** \= B (buy), S (sell) 

• **quantity** \= positive integer indicating maximum quantity to buy/sell 

• **price** \= double indicating max price at which to buy/min price to sell 

Amendment Rules 

When an order is amended: 

• **Reduce-only amendments** (price unchanged, quantity reduced) **preserve time priority** 

• **Price changes or quantity increases** cause the order to **lose time priority** (treated as cancel \+  new order at back of queue) 

• **Side changes are NOT allowed** (should be rejected with error message)  
Example 

This is an example of message sequence that the matching engine has to deal with: 

```
101,N,A001,S,3000,6.8
101,N,A002,S,1000,6.9
101,N,A003,B,2000,6.7
101,N,A004,B,1000,6.8 // crosses with order A001 with price 6.8
101,A,A003,B,2000,6.9 // after amending A003 crosses with A001 and both fully  filled
102,N,A005,S,2000,10.2
102,N,A006,B,2000,10.1
102,N,A007,B,2000,10.1
102,C,A006,B,2000,10.1 // A006 will be removed from order book
```

Expected Output 

(Your format can differ from below) 

```
Trade: 101 A004 1000 6.8, AggrSide=B 
Trade: 101 A001 1000 6.8, AggrSide=B 
Trade: 101 A003 2000 6.8, AggrSide=B 
Trade: 101 A001 2000 6.8, AggrSide=B 
<on exit> 
Ticker: 101 
OrderId: A002 Side: Sell Quantity: 1000 Price: 6.9 
Ticker: 102 
OrderId: A005 Side: Sell Quantity: 2000 Price: 10.2 
OrderId: A007 Side: Buy Quantity: 2000 Price: 10.1
```

**Note:** AggrSide indicates which side was the aggressor (B=buyer lifted offer, S=seller hit bid). The  aggressor's order determines the trade price. 

Deliverables 

1\. **Source code:** C++ implementation with clear structure  
2\. **Build instructions:** Makefile or compile command (e.g., g++ \-std=c++17 \-O2 \-o  matching\_engine main.cpp) 

3\. **Test data:** Your own test file(s) demonstrating edge cases you handle (e.g., partial fills,  priority preservation, lazy deletion, etc.) 

4\. **Brief README:** • How to build and run • Key design decisions (data structures, algorithms) •  Performance optimizations applied • Known limitations or assumptions 

What We're Evaluating 

• **Correctness:** Proper price-time priority matching, amendment rules 

• **Code quality:** Clean structure, appropriate abstractions, error handling 

• **Performance awareness:** Efficient algorithms and data structures (O(1) operations preferred) • **Completeness:** Handles all requirements and edge cases 

• **Testing:** Quality and coverage of test cases 

• **Documentation:** Clear README explaining design decisions 

Submission 

**Expected time:** 2-3 hours 

Please submit all source files, build instructions, test data, and README. 

**Good luck\!**
