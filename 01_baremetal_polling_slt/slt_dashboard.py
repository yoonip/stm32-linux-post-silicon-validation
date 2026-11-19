import streamlit as st
import serial, time, subprocess, os
import pandas as pd

st.set_page_config(page_title="Post-Silicon Validation Dashboard", layout="wide")
st.title("STM32F767ZI - Linux Kernel - MIPI-CSI Dashboard")

port = st.sidebar.text_input("UART Port", value="/dev/ttyACM0")
baud = st.sidebar.selectbox("Baudrate", (115200, 9600), index=0)
cycles = st.sidebar.slider("SLT Cycles", 5, 200, 30)

def send_cmd(cmd):
    with serial.Serial(port, baud, timeout=0.5) as s:
        time.sleep(0.05); s.reset_input_buffer()
        t0 = time.perf_counter(); s.write(cmd)
        return s.readline().decode("utf-8", errors="ignore").strip(), (time.perf_counter() - t0) * 1000

t1, t2, t3 = st.tabs(["1. STM32 UART/DMA SLT", "2. OV5647 MIPI-CSI Jitter", "3. Kernel GPIO -> EXTI3 Trigger"])

with t1:
    c1, c2, c3, c4 = st.columns(4)
    if c1.button("Green (PB0)"): r, l = send_cmd(b"1"); st.toast(f"{r} ({l:.2f}ms)")
    if c2.button("Blue (PB7)"): r, l = send_cmd(b"2"); st.toast(f"{r} ({l:.2f}ms)")
    if c3.button("Red (PB14)"): r, l = send_cmd(b"3"); st.toast(f"{r} ({l:.2f}ms)")
    if c4.button("All OFF"): r, l = send_cmd(b"0"); st.toast(f"{r} ({l:.2f}ms)")
    if st.button("START SLT TEST", type="primary"):
        vecs = [(b"1","DMA+IRQ: Green Toggled!"),(b"2","DMA+IRQ: Blue Toggled!"),(b"3","DMA+IRQ: Red Toggled!"),(b"0","DMA+IRQ: All OFF!")]
        recs = []
        with serial.Serial(port, baud, timeout=0.5) as s:
            time.sleep(0.2); s.reset_input_buffer()
            for c in range(1, cycles + 1):
                for cmd, exp in vecs:
                    t0 = time.perf_counter(); s.write(cmd)
                    resp = s.readline().decode("utf-8", errors="ignore").strip()
                    recs.append({"Step": len(recs)+1, "Cmd": cmd.decode(), "Actual": resp, "Latency_ms": round((time.perf_counter()-t0)*1000, 3), "Status": "PASS" if resp==exp else "FAIL"})
        df = pd.DataFrame(recs)
        m1, m2, m3 = st.columns(3)
        m1.metric("Yield", f"{(df['Status']=='PASS').mean()*100:.2f}%")
        m2.metric("Avg Latency", f"{df['Latency_ms'].mean():.3f} ms")
        m3.metric("Max Latency", f"{df['Latency_ms'].max():.3f} ms")
        st.line_chart(df.set_index("Step")["Latency_ms"])
with t2:
    dur = st.slider("Capture Duration (ms)", 1000, 5000, 3500, 500)
    if st.button("RUN CAMERA JITTER ANALYSIS"):
        subprocess.run(["rpicam-vid", "-n", "-t", str(dur), "--save-pts", "timestamps.txt", "-o", "/dev/null"], stderr=subprocess.DEVNULL)
        with open("timestamps.txt") as f: ts = [float(x) for x in f if not x.startswith("#")]
        deltas = [ts[i]-ts[i-1] for i in range(1, len(ts))]
        avg_dt = sum(deltas)/len(deltas)
        k1, k2, k3 = st.columns(3)
        k1.metric("FPS", f"{1000.0/avg_dt:.2f}")
        k2.metric("Worst Jitter", f"{max(abs(max(deltas)-avg_dt), abs(avg_dt-min(deltas))):.3f} ms")
        k3.metric("Drops (>50ms)", f"{sum(1 for d in deltas if d>50.0)}")
        st.line_chart(pd.DataFrame({"Frame": range(1, len(deltas)+1), "Delta_ms": deltas}).set_index("Frame")["Delta_ms"])

with t3:
    st.info("Wiring: Raspberry Pi GPIO17 (Pin 11) <-> STM32 A0 (PA3), GND <-> GND")
    if os.path.exists("/dev/slt_gpio"):
        if st.button("TRIGGER KERNEL GPIO17 PULSE -> STM32 EXTI3", type="primary"):
            with serial.Serial(port, baud, timeout=0.5) as s:
                time.sleep(0.05); s.reset_input_buffer()
                open("/dev/slt_gpio", "w").write("1"); time.sleep(0.02); open("/dev/slt_gpio", "w").write("0")
                st.success(f"STM32 EXTI3 DMA Response: {s.readline().decode().strip()}")
