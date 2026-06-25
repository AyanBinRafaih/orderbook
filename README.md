# Lock-Free Order Book

This is a high-performance limit order book engine in C++20, built with including a lock-free Single-Producer-Single-Consumer (SPSC) queue and a single-threaded matching engine. The orderbook is built from scratch including all necessary parts. It also includes statistical analysis of latency distributions, including the effect of various optimisations (currently in development). 

## Implementation Details

### Build System
The system uses CMake build with C++20, `-Wall -Wextra -Wpedantic`, and `-O3 -march=native` in Release, a ThreadSanatizer to help catch data races in debugging, `ccache` integration for faster rebuilds and it pulls in `GoogleTest` and `Google Benchmark`.

### Data Structures
- **`Order`**: a cache-line-aligned (to 64 bytes) struct which includes:
  - order ID
  - price
  - quantity
  - timestamp
  - side (buy or sell)
  - type
    
- **`Trade`**: record of a completed match between a buy and sell order.
- **`Timer`**: Used to measure performance of the orderbook when matching occurs between orders, and includes:
  - a `rdtsc`-based nanosecond timer
  - `lfence` serialization
  - initial calibration against `CLOCK_MONOTONIC_RAW` (using the median of 5 samples) to convert cycles to nanoseconds (built and measured on MacOS)

### Order Book
A baseline matching engine (using mutex protection):
- `std::map<double, PriceLevel>`:
  - bid/ask price levels
  - bids are sorted in descending order
  - asks are sorted in ascending order

- `std::list<Order>`:
  - for handling orders per price level
  - prioritised using time of arrival (FIFO principle)
    
- `std::unordered_map<order_id, iterator>`:
  - iterator allows very fast order lookup and cancellation

The order book also manages all types of matchings, including exact fills and partial-matches (on both the incoming side and resting side in the book), dealing with remainders.

**Testing** 
11 unit tests are deployed as a correctness check for the orderbook's functionalities:
- Creating a new order
- Lone bid which has no match in the orderbook
- Line ask with no match from the book
- Successful matching (when the buy crosses the spread value, which is defined as Price_Ask - Price_Bid)
- Successful half-matching for 2 cases: (i) buy < resting ask, (ii) buy > resting ask
- Price-time priority decisions for 2 bids at the same price (the bid that arrived first is prioritised first)
- Buy matches through 2 asks in the orderbook
- Cancelling orders for 3 cases (i) order exists in the book, (ii) order does not exist in the book, (iii) multiple orders with shared prices
- Checking if total bought = total sold

## Directory Structure

```
orderbook/
├── include/          # headers (order.h, order_book.h, timer.h, ...)
├── src/              # Implementation (order_book.cpp, timer.cpp, main.cpp)
├── tests/            # GoogleTest unit tests
├── bench/            # Google Benchmark harnesses
├── analysis/         # Python load generation & statistical analysis
├── docs/plots/       # Generated latency distribution plots
└── .github/workflows/ # CI configuration
```

## To Build

Clone this repository:
 
```bash
git clone https://github.com/YOUR_USERNAME/orderbook.git
cd orderbook
```

 
Install the pre-requisites (skip if you already have them installed):

 
```bash
# Xcode Command Line Tools — provides clang++, make, git
xcode-select --install
 
# Homebrew — package manager
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
 
# CMake
brew install cmake
 
# ccache (speeds up rebuilds)
brew install ccache
 
# Python + analysis libraries 
brew install python@3.11
pip3 install numpy scipy matplotlib pandas
```
 
Verify versions:
 
```bash
clang++ --version   # need Apple Clang 14+ for full C++20 support
cmake --version      # need 3.20+
```
 
Build the project using Cmake:
 
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

Run the unit tests:
 
```bash
ctest --output-on-failure
```

## Currently in Development

Some of the features are still in development which include a lock-free SPSC queue, matching engine threading and statistical analysis.
