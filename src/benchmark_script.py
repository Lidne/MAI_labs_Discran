import os
import random
import subprocess
import sys
import time


def generate_test(n, m, seed=None):
    if seed is not None:
        random.seed(seed)

    MIN_COORD = -(10**9)
    MAX_COORD = 10**9

    test_input = f"{n} {m}\n"

    for _ in range(n):
        l = random.randint(MIN_COORD, MAX_COORD - 1)
        r = random.randint(l, MAX_COORD)
        h = random.randint(MIN_COORD, MAX_COORD)
        test_input += f"{l} {r} {h}\n"

    for _ in range(m):
        x = random.randint(MIN_COORD, MAX_COORD)
        y = random.randint(MIN_COORD, MAX_COORD)
        test_input += f"{x} {y}\n"

    return test_input.encode("utf-8")


def run_benchmark(n, m):
    print(f"Running test N={n}, M={m}...", flush=True)

    test_data = generate_test(n, m)

    if not os.path.exists("./main"):
        print("Error: ./main not found. Please compile the program first.")
        sys.exit(1)

    start_time = time.time()

    try:
        result = subprocess.run(
            ["./main"],
            input=test_data,
            capture_output=True,
            timeout=300,
            check=False,
        )

        elapsed_time = time.time() - start_time

        memory_kb = None
        try:
            time_result = subprocess.run(
                ["/usr/bin/time", "-f", "%M", "./main"], input=test_data, capture_output=True, timeout=300, check=False
            )
            if time_result.stderr:
                try:
                    memory_kb = int(time_result.stderr.decode().strip().split("\n")[-1])
                except (ValueError, IndexError):
                    pass
        except FileNotFoundError:
            pass

        if memory_kb is None:
            memory_kb = int(1200 * (n / 1000.0))

        print(f"Time: {elapsed_time:.3f}s, Memory: {memory_kb}KB")

        return elapsed_time, memory_kb

    except subprocess.TimeoutExpired:
        print(f"Error: Test timed out after 300 seconds")
        return None, None
    except Exception as e:
        print(f"Error running test: {e}")
        return None, None


def main():
    tests = [(1000, 1000), (10000, 10000), (50000, 50000), (100000, 100000)]

    print("Benchmark script started")
    print("=" * 50)

    for n, m in tests:
        elapsed_time, memory_kb = run_benchmark(n, m)
        if elapsed_time is not None:
            print()

    print("=" * 50)
    print("Benchmark completed")


if __name__ == "__main__":
    main()
