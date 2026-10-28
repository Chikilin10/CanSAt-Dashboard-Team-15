#!/usr/bin/env python3
"""Prueba automatica del backend en C: simula un vuelo y verifica fases, alturas, despliegue e impacto.
Uso:  cd backend/c && make test"""
import json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
BIN = os.path.join(HERE, "..", "c", "build", "cansat_backend")

def run(lines):
    out = subprocess.run([BIN], input="\n".join(lines) + "\n", capture_output=True, text=True).stdout
    return [json.loads(l) for l in out.splitlines()]

sim = subprocess.run([sys.executable, os.path.join(HERE, "sim_flight.py"), "--fast"], capture_output=True, text=True).stdout.splitlines()
rows = run(sim)
fails = []
def check(name, ok, detail=""):
    print(("  OK   " if ok else "  FALLA"), name, detail)
    if not ok: fails.append(name)

d = [r["derived"] for r in rows]
phases = []
for x in d:
    if not phases or phases[-1] != x["phase"]: phases.append(x["phase"])
check("fases en orden", phases == ["ground", "ascent", "freefall", "autogyro", "landed"], str(phases))
check("altura maxima ~450 m", 440 <= d[-1]["alt_max"] <= 460, f'{d[-1]["alt_max"]}')
check("rotor desplegado entre 190 y 250 m", 190 <= d[-1]["deploy_alt"] <= 250, f'{d[-1]["deploy_alt"]}')
auto = [x["descent_rate"] for x in d if x["phase"] == "autogyro" and x["alt"] > 30 and x["descent_rate"] is not None]
stable = auto[len(auto)//2:]
check("autogiro estabilizado en 8-12 m/s", all(8 <= v <= 12 for v in stable), f'{min(stable)}..{max(stable)}')
check("impacto detectado (>4 g)", d[-1]["impact_g"] is not None and d[-1]["impact_g"] > 4, f'{d[-1]["impact_g"]} g')
check("transmite >=10 s tras aterrizar", d[-1]["post_landing_s"] is not None and d[-1]["post_landing_s"] >= 10, f'{d[-1]["post_landing_s"]} s')
check("sin alturas negativas absurdas", min(x["alt"] for x in d) > -5)

# Lineas corruptas no deben tumbar el programa ni producir paquetes
bad = run(["basura", "T,1,2", "T,1,100,5000,20,0,0,9.8,0,0,0,9,0,,,1,,,0,-60", sim[0]])
check("descarta lineas invalidas", len(bad) == 1, f"{len(bad)} paquete(s) validos de 4 lineas")

# Reinicio de la placa EN VUELO: no debe perder la calibracion
mid = [i for i, x in enumerate(d) if x["phase"] == "freefall"][3]
rebooted = sim[:mid] + [",".join(["T", str(j), str(j * 500)] + sim[mid + j].split(",")[3:]) for j in range(0, 12)]
r2 = run(rebooted)
alts = [x["derived"]["alt"] for x in r2[-10:]]
check("reinicio en vuelo conserva la altura", all(a > 150 for a in alts), f"alt tras reinicio: {min(alts)}..{max(alts)}")

print("\nRESULTADO:", "TODO BIEN" if not fails else f"{len(fails)} falla(s): {fails}")
sys.exit(1 if fails else 0)
