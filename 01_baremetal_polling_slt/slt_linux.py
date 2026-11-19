import serial, time, csv
from datetime import datetime

ser = serial.Serial("/dev/ttyACM0", 115200, timeout=0.5)
time.sleep(0.2)
ser.reset_input_buffer()

vectors = [(b"1", "Green LED Toggled!"), (b"2", "Blue LED Toggled!"), (b"3", "Red LED Toggled!"), (b"0", "All LEDs OFF!")]
pass_cnt, fail_cnt, latencies, rows = 0, 0, [], []

for cycle in range(1, 51):
    for cmd, exp in vectors:
        t0 = time.perf_counter()
        ser.write(cmd)
        resp = ser.readline().decode("utf-8", errors="ignore").strip()
        lat = (time.perf_counter() - t0) * 1000
        latencies.append(lat)
        ok = (resp == exp)
        pass_cnt += ok
        fail_cnt += (not ok)
        rows.append([cycle, cmd.decode(), exp, resp, f"{lat:.3f}", "PASS" if ok else "FAIL"])

ser.close()
print(f"[LINUX SLT SUMMARY] Total: {pass_cnt+fail_cnt} | PASS: {pass_cnt} | FAIL: {fail_cnt} | Yield: {pass_cnt/(pass_cnt+fail_cnt)*100:.2f}% | Avg Latency: {sum(latencies)/len(latencies):.3f}ms")

fname = f"linux_slt_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
with open(fname, "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["Cycle", "Cmd", "Expected", "Actual", "Latency_ms", "Status"])
    w.writerows(rows)
print(f"[INFO] Saved to {fname}")
