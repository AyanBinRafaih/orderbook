#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include "matching_engine.h"
#include "timer.h"

// Read binary order file written by generate_load.py (each order is 34 bytes)
std::vector<Order> load_orders(const char* filepath) {
    std::ifstream f(filepath, std::ios::binary);
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", filepath);
        exit(1);
    }

    struct __attribute__((packed)) DiskOrder {
        uint64_t order_id;
        double   price;
        uint64_t quantity;
        uint64_t timestamp_ns;
        uint8_t  side;
        uint8_t  type;
    };
    static_assert(sizeof(DiskOrder) == 34);

    std::vector<Order> orders;
    DiskOrder disk_order;
    while (f.read(reinterpret_cast<char*>(&disk_order), sizeof(DiskOrder))) {
        Order o{};
        o.order_id     = disk_order.order_id;
        o.price        = disk_order.price;
        o.quantity     = disk_order.quantity;
        o.timestamp_ns = disk_order.timestamp_ns;
        o.side         = static_cast<Side>(disk_order.side);
        o.type         = static_cast<OrderType>(disk_order.type);
        orders.push_back(o);
    }
    return orders;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <order_file.bin> [output_latency.csv]\n", argv[0]);
        return 1;
    }

    const char* order_file   = argv[1];
    const char* latency_file = argc >= 3 ? argv[2] : "analysis/data/latency.csv";

    printf("Loading orders from %s...\n", order_file);
    auto orders = load_orders(order_file);
    printf("Loaded %zu orders\n", orders.size());

    MatchingEngine engine;

    printf("Warming up...\n");
    engine.set_record_latency(false);
    engine.start();
    size_t warmup_count = std::min(orders.size(), size_t(100'000));
    for (size_t i = 0; i < warmup_count; ++i) {
        while (!engine.submit(orders[i])) {
            __asm__ volatile ("pause");
        }
    }
    engine.stop();

    MatchingEngine engine2;
    engine2.set_record_latency(true);
    engine2.start();

    printf("Running benchmark...\n");
    for (const auto& order : orders) {
        while (!engine2.submit(order)) {
            __asm__ volatile ("pause");
        }
    }
    engine2.stop();

    const auto& samples = engine2.latency_samples();
    const auto& trades  = engine2.trades();
    double cpns = timer::calibrate_cycles_per_ns();

    printf("Processed %zu orders, generated %zu trades\n", orders.size(), trades.size());
    printf("Recorded %zu latency samples\n", samples.size());

    FILE* out = fopen(latency_file, "w");
    if (!out) { 
        fprintf(stderr, "Cannot write %s\n", latency_file); 
        return 1; 
    }
    fprintf(out, "latency_ns,operation\n");
    for (const auto& s : samples) {
        fprintf(out, "%.2f,%c\n", timer::cycles_to_ns(s.cycles, cpns), s.operation);
    }
    fclose(out);
    printf("Latency data written to %s\n", latency_file);

    return 0;
}