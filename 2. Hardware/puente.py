import serial, socket, time, os, sys
import psycopg2

# ---------- Configuracion (variables de entorno, con valores por defecto) ----------
PUERTO = os.environ.get("CANSAT_PUERTO", "COM6")          # placa receptora
IP_MAC = os.environ.get("CANSAT_IP_MAC", "10.23.22.36")   # IP de la Mac del backend
PUERTO_TCP = 9000                                         # el de nc en la Mac (NO el 8080)

if "CANSAT_DB_PASS" not in os.environ:
    sys.exit("Falta la variable de entorno CANSAT_DB_PASS (contrasena de PostgreSQL).")

DB = dict(
    host=os.environ.get("CANSAT_DB_HOST", "localhost"),
    port=5432,
    dbname="cansat",
    user="postgres",
    password=os.environ["CANSAT_DB_PASS"],
    connect_timeout=3,
)

ACEL_EN_G = True      # la placa manda g (1.00 en reposo); el backend espera m/s2
G = 9.80665
REINTENTO_DB_S = 5    # cada cuantos segundos reintentar la base si falla
REINTENTO_SOCK_S = 3  # cada cuantos segundos reintentar el backend si falla
# ------------------------------------------------------------------------------------

ser = serial.Serial(PUERTO, 115200, timeout=2)
t0 = time.time()

db_conn = None
db_ultimo_intento = 0.0
sock = None
sock_ultimo_intento = 0.0


# ---------- Base de datos ----------
def db_conectar():
    global db_ultimo_intento
    db_ultimo_intento = time.time()
    try:
        c = psycopg2.connect(**DB)
        c.autocommit = True
        print("Conectado a la base de datos")
        return c
    except Exception as e:
        print("(BD no disponible)", str(e).strip().splitlines()[0])
        return None


def db_guardar(fila):
    global db_conn
    if db_conn is None:
        if time.time() - db_ultimo_intento < REINTENTO_DB_S:
            return
        db_conn = db_conectar()
        if db_conn is None:
            return
    try:
        with db_conn.cursor() as cur:
            cur.execute(
                """INSERT INTO telemetria
                   (id, t_ms, temp_c, pres_hpa, alt_m,
                    ax, ay, az, gx, gy, gz, rssi, snr, cruda)
                   VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)""",
                fila,
            )
    except Exception as e:
        print("(error al guardar en BD)", str(e).strip().splitlines()[0])
        try:
            db_conn.close()
        except Exception:
            pass
        db_conn = None


def fila_para_db(texto):
    """Entrada: id,temp,pres,alt,ax,ay,az,gx,gy,gz,rssi,snr (aceleracion en g)."""
    c = texto.split(",")
    if len(c) != 12:
        return None
    try:
        v = [float(x) for x in c]
    except ValueError:
        return None
    t_ms = int((time.time() - t0) * 1000)
    return (
        int(v[0]), t_ms, v[1], v[2], v[3],
        v[4], v[5], v[6], v[7], v[8], v[9],
        v[10], v[11], texto,
    )


# ---------- Backend (Mac) ----------
def enviar(linea):
    """Manda una linea al backend. Si no hay conexion, reintenta sin bloquear el programa."""
    global sock, sock_ultimo_intento
    if sock is None:
        if time.time() - sock_ultimo_intento < REINTENTO_SOCK_S:
            return
        sock_ultimo_intento = time.time()
        try:
            sock = socket.create_connection((IP_MAC, PUERTO_TCP), timeout=1)
            print("Conectado al backend")
        except OSError:
            print("(sin conexion con el backend)")
            sock = None
            return
    try:
        sock.sendall((linea + "\n").encode())
    except OSError:
        try:
            sock.close()
        except Exception:
            pass
        sock = None


def convertir(texto):
    """Entrada: id,temp,pres,alt,ax,ay,az,gx,gy,gz,rssi,snr
    Salida:  T,seq,t_ms,pres,temp,ax,ay,az,gx,gy,gz,bat,0,,,,,,,rssi"""
    c = texto.split(",")
    if len(c) != 12:
        return None
    try:
        pid = int(c[0])
        temp = float(c[1])
        pres = float(c[2])
        ax, ay, az = float(c[4]), float(c[5]), float(c[6])
        gx, gy, gz = float(c[7]), float(c[8]), float(c[9])
        rssi = float(c[10])
    except ValueError:
        return None

    if ACEL_EN_G:
        ax, ay, az = ax * G, ay * G, az * G

    t_ms = int((time.time() - t0) * 1000)
    campos = [
        pid, t_ms, f"{pres:.2f}", f"{temp:.2f}",
        f"{ax:.2f}", f"{ay:.2f}", f"{az:.2f}",
        f"{gx:.1f}", f"{gy:.1f}", f"{gz:.1f}",
        "",       # bateria (la placa aun no la manda)
        0,
        "", "", "", "", "", "",   # campos 13 a 18 (GPS, satelites, etc.)
        f"{rssi:.0f}",
    ]
    return "T," + ",".join(str(x) for x in campos)


# ---------- Programa principal ----------
db_conn = db_conectar()

while True:
    linea = ser.readline()
    if not linea.strip():
        continue

    texto = linea.decode(errors="ignore").strip()

    if texto.startswith("DATA,"):
        texto = texto[len("DATA,"):]

    if not texto or not texto[0].isdigit():
        print("(ignorado)", texto)
        continue

    # 1) Respaldo en la base, siempre primero
    fila = fila_para_db(texto)
    if fila is not None:
        db_guardar(fila)

    # 2) Envio al dashboard
    paquete = convertir(texto)
    if paquete is None:
        print("(formato inesperado)", texto)
        continue

    print(paquete)
    enviar(paquete)