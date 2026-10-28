#ifndef CANSAT_H
#define CANSAT_H

#include <math.h>

/* Valores "nulos" se representan con NAN y salen como null en el JSON. */

typedef struct {
    long   packet;
    double t;            /* segundos (el CanSat manda milisegundos) */
    double pressure;     /* hPa */
    double temp;         /* C */
    double ax, ay, az;   /* m/s^2 */
    double gx, gy, gz;   /* deg/s */
    double volt;         /* V */
    int    rotor;        /* 0/1 */
    double sec_dist;     /* m, NAN si no hay dato */
    double sec_bearing;  /* grados, NAN si no hay dato */
    double apk;          /* pico de aceleracion desde el paquete anterior, en g */
    double lat, lon;     /* NAN si el GPS no tiene fix */
    int    sats;
    int    rssi;
} Raw;

typedef enum { PH_GROUND, PH_ASCENT, PH_FREEFALL, PH_AUTOGYRO, PH_LANDED } Phase;

typedef struct {
    double alt, vel_v, alt_max;
    Phase  phase;
    double deploy_alt;      /* NAN hasta que se despliega el rotor */
    double descent_rate;    /* NAN fuera de la fase de autogiro */
    int    descent_ok;      /* -1 = null, 0 = fuera de rango, 1 = en rango */
    double impact_g;        /* NAN hasta aterrizar */
    double post_landing_s;  /* NAN hasta aterrizar */
} Derived;

/* ---- Parametros de mision (ajustables) ---- */
#define CAL_SAMPLES     10     /* paquetes en tierra para calibrar la presion base */
#define VEL_WINDOW_S    1.0    /* ventana para calcular velocidad vertical */
#define ALT_FILTER      0.35   /* filtro exponencial de altitud (0..1) */
#define ASCENT_ALT_M    10.0
#define ASCENT_VEL_MS   5.0
#define FALL_VEL_MS    -2.0
#define LAND_ALT_M      8.0
#define LAND_VEL_MS     1.5
#define LAND_HOLD_S     1.0
#define IMPACT_ZONE_M   25.0   /* se busca el pico de g por debajo de esta altura */
#define DESCENT_MIN     8.0
#define DESCENT_MAX     12.0

#define HIST 64

typedef struct {
    int    calib_n;
    double p0_sum, p0;
    int    n;                 /* muestras en el historial */
    int    head;
    double h_t[HIST], h_alt[HIST];
    double alt_f;
    int    has_alt;
    double alt_max;
    Phase  phase;
    double deploy_alt;
    int    rotor_prev;
    double still_since;       /* -1 si no esta quieto */
    double landed_at;
    double peak_g;
    double last_t;
    long   last_packet;
} Tracker;

void tracker_init(Tracker *k);
void tracker_update(Tracker *k, const Raw *r, Derived *d);
int  parse_line(const char *line, Raw *r);          /* 0 = ok, -1 = invalido */
void print_json(const Raw *r, const Derived *d);

#endif
