#!/bin/python3

import sys
import subprocess

def spawn_workers(worker_binary_path: str, n_workers: int, heartbeat_interval: int):
    base_port=8100
    for i in range(n_workers):
        try:
            port = base_port+i
            worker_args = [str(port), str(heartbeat_interval)]
            subprocess.Popen([worker_binary_path] + worker_args)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python spawn_workers.py <num_workers> <heartbeat_interval> <worker_binary_path>")
        sys.exit(1)

    n_workers = int(sys.argv[1])
    heartbeat_interval = int(sys.argv[2])
    worker_binary_path = sys.argv[3]
    spawn_workers(worker_binary_path, n_workers, heartbeat_interval)
