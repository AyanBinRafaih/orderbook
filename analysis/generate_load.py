import struct
import random
import argparse
import os

ORDER_FORMAT = '<QdQQBB' 
ORDER_SIZE   = struct.calcsize(ORDER_FORMAT)
assert ORDER_SIZE == 34, f"Expected 34, got {ORDER_SIZE}"

def generate_orders(n_orders, mid_price=100.0, tick_size=0.01,
                    cancel_frac=0.15, market_frac=0.05, seed=42):
    random.seed(seed)
    orders = []
    active_order_ids = []
    order_id = 1
    timestamp_ns = 1_000_000_000  

    for _ in range(n_orders):
        r = random.random()

        if r < cancel_frac and active_order_ids:
            target_id = random.choice(active_order_ids)
            active_order_ids.remove(target_id)
            orders.append((target_id, 0.0, 0, timestamp_ns, 0, 2))

        elif r < cancel_frac + market_frac:
            side = random.randint(0, 1)
            qty  = random.randint(1, 100) * 10
            orders.append((order_id, 0.0, qty, timestamp_ns, side, 1))
            order_id += 1
        else:
            side = random.randint(0, 1)
            offset = random.gauss(0, 10) * tick_size
            price  = round(max(tick_size, mid_price + offset), 2)
            qty    = random.randint(1, 100) * 10
            orders.append((order_id, price, qty, timestamp_ns, side, 0))
            active_order_ids.append(order_id)
            order_id += 1

        timestamp_ns += random.randint(100, 10_000)

    return orders

def write_orders(orders, filepath):
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'wb') as f:
        for o in orders:
            order_id, price, qty, ts_ns, side, otype = o
            f.write(struct.pack(ORDER_FORMAT, order_id, price, qty, ts_ns, side, otype))
    print(f"Wrote {len(orders)} orders to {filepath} ({os.path.getsize(filepath)} bytes)")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--n', type=int, default=1_000_000, help='Number of orders')
    parser.add_argument('--out', type=str, default='analysis/data/orders.bin')
    parser.add_argument('--seed', type=int, default=42)
    args = parser.parse_args()

    orders = generate_orders(args.n, seed=args.seed)
    write_orders(orders, args.out)