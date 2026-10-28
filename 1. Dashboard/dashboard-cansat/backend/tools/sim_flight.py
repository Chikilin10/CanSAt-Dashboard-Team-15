#!/usr/bin/env python3
"""Simula la radio de tierra: imprime lineas CSV como las del CanSat real.

Uso:
  python3 sim_flight.py            # tiempo real, 2 paquetes por segundo
  python3 sim_flight.py --fast     # sin pausas (para pruebas)
"""
import random, sys, time

HZ = 2.0
dt = 1.0 / HZ
fast = "--fast" in sys.argv

def pressure(alt):
    return 1013.25 * (1 - alt / 44330.0) ** 5.255

alt, vel, t, n = 0.0, 0.0, 0.0, 0
phase, rotor, landed_at, impact = "ground", 0, None, False
hover_until = 0

while True:
    if phase == "ground" and t > 6:
        phase = "ascent"
    elif phase == "ascent":                      # el dron sube a ~4 m/s
        vel = 4.0; alt += vel * dt
        if alt >= 450: phase, hover_until = "hover", t + 8
    elif phase == "hover":                       # el dron se estabiliza y suelta el CanSat
        vel = 0.0
        if t >= hover_until: phase = "freefall"
    elif phase == "freefall":
        vel = max(-35.0, vel - 9.8 * dt); alt += vel * dt
        if alt <= 210: phase, rotor = "autogyro", 1
    elif phase == "autogyro":
        vel += (-10.0 - vel) * 0.25; alt += vel * dt
        if alt <= 0:
            alt, vel, phase, landed_at, impact = 0.0, 0.0, "landed", t, True
    elif phase == "landed" and t - landed_at > 15:
        break

    az = 0.3 if phase == "freefall" else 9.8
    apk = 6.5 + random.random() if impact else 1.0 + random.random() * 0.1
    impact = False
    fields = [
        "T", n, int(t * 1000),
        f"{pressure(alt) + random.gauss(0, 0.05):.2f}",
        f"{25 - 0.0065 * alt + random.gauss(0, 0.1):.2f}",
        f"{random.gauss(0, 0.2):.2f}", f"{random.gauss(0, 0.2):.2f}", f"{az + random.gauss(0, 0.2):.2f}",
        f"{random.gauss(0, 3):.1f}", f"{random.gauss(0, 3):.1f}", f"{random.gauss(0, 3):.1f}",
        f"{9.3 - n * 0.0006:.2f}", rotor,
        "4.2" if phase == "landed" else "", "135" if phase == "landed" else "",
        f"{apk:.2f}", "25.651234", "-100.289456", 8, -62,
    ]
    print(",".join(str(x) for x in fields), flush=True)
    n += 1; t += dt
    if not fast: time.sleep(dt)
