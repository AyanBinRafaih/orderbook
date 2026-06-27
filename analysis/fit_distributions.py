import numpy as np
import pandas as pd
from scipy import stats
import sys

def compute_stats(latencies: np.ndarray, label: str) -> dict:
    print(f"\n{label}")
    result = {
        'count': len(latencies),
        'mean': np.mean(latencies),
        'median': np.median(latencies),
        'std': np.std(latencies),
        'p50': np.percentile(latencies, 50),
        'p90': np.percentile(latencies, 90),
        'p95': np.percentile(latencies, 95),
        'p99': np.percentile(latencies, 99),
        'p99_9': np.percentile(latencies, 99.9),
        'p99_99': np.percentile(latencies, 99.99),
        'max': np.max(latencies),
    }

    print(f"Count: {result['count']:>12,}")
    print(f"Mean: {result['mean']:>12.2f} ns")
    print(f"Median: {result['median']:>12.2f} ns")
    print(f"Std Dev: {result['std']:>12.2f} ns")
    print(f"P50: {result['p50']:>12.2f} ns")
    print(f"P90: {result['p90']:>12.2f} ns")
    print(f"P95: {result['p95']:>12.2f} ns")
    print(f"P99: {result['p99']:>12.2f} ns")
    print(f"P99.9: {result['p99_9']:>12.2f} ns")
    print(f"P99.99: {result['p99_99']:>12.2f} ns")
    print(f"Max: {result['max']:>12.2f} ns")
    print(f"P99.9/P50 ratio: {result['p99_9']/result['p50']:.1f}x")

    return result

def fit_lognormal(latencies: np.ndarray):
    shape, loc, scale = stats.lognorm.fit(latencies, floc=0)
    mu    = np.log(scale)
    sigma = shape

    ks_stat, ks_pval = stats.kstest(
        latencies,
        lambda x: stats.lognorm.cdf(x, shape, loc=0, scale=scale)
    )

    print(f"\nLog-Normal Fit (MLE):")
    print(f"mu (log-mean): {mu:.4f}")
    print(f"sigma (log-std): {sigma:.4f}")
    print(f"KS statistic: {ks_stat:.6f}")
    print(f"KS p-value: {ks_pval:.6f}")

    return shape, loc, scale, ks_stat

def fit_weibull(latencies: np.ndarray):
    c, loc, scale = stats.weibull_min.fit(latencies, floc=0)
    ks_stat, ks_pval = stats.kstest(
        latencies,
        lambda x: stats.weibull_min.cdf(x, c, loc=0, scale=scale)
    )
    print(f"\nWeibull Fit (MLE):")
    print(f"shape (k): {c:.4f}")
    print(f"scale (lambda): {scale:.4f}")
    print(f"KS statistic: {ks_stat:.6f}")
    return c, loc, scale, ks_stat

def fit_gpdtail(latencies: np.ndarray, threshold_pct: float = 95.0):
    threshold = np.percentile(latencies, threshold_pct)
    tail = latencies[latencies > threshold] - threshold

    xi, loc, beta = stats.genpareto.fit(tail, floc=0)
    ks_stat, _ = stats.kstest(
        tail,
        lambda x: stats.genpareto.cdf(x, xi, loc=0, scale=beta)
    )

    print(f"\nGeneralized Pareto (tail above P{threshold_pct:.0f} = {threshold:.1f} ns):")
    print(f"tail samples: {len(tail):,}")
    print(f"xi (shape): {xi:.4f}  ({'heavy' if xi > 0 else 'light'} tail)")
    print(f"beta (scale): {beta:.4f}")
    print(f"KS statistic: {ks_stat:.6f}")
    return xi, beta, threshold

if __name__ == '__main__':
    filepath = sys.argv[1] if len(sys.argv) > 1 else 'analysis/data/latency_1m.csv'
    df = pd.read_csv(filepath)

    for op, label in [('I', 'Insert (no match)'), ('M', 'Match'), ('C', 'Cancel')]:
        subset = df[df['operation'] == op]['latency_ns'].values
        if len(subset) == 0:
            continue
        stats_dict = compute_stats(subset, label)
        fit_lognormal(subset)
        fit_weibull(subset)
        fit_gpdtail(subset)
        print()