"""Carga telemetria.csv a la tabla telemetria.
Uso:  python importar_csv.py telemetria.csv
"""
import sys, csv
import psycopg2

DB = dict(host="localhost", port=5432, dbname="cansat",
          user="postgres", password="elingeleo04")   

archivo = sys.argv[1] if len(sys.argv) > 1 else "telemetria.csv"

conn = psycopg2.connect(**DB)
n = 0
with conn, conn.cursor() as cur, open(archivo, newline="", encoding="utf-8") as f:
    for r in csv.DictReader(f):
        cur.execute(
            """INSERT INTO telemetria
               (hora_pc, id, t_ms, alt_m, temp_c, pres_hpa, lat, lon, rssi, snr, cruda)
               VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)""",
            (r["hora_pc"], int(r["id"]), int(r["t_ms"]), float(r["alt_m"]),
             float(r["temp_c"]), float(r["pres_hpa"]), float(r["lat"]),
             float(r["lon"]), float(r["rssi"]), float(r["snr"]), "csv"),
        )
        n += 1
print(f"Importadas {n} filas")