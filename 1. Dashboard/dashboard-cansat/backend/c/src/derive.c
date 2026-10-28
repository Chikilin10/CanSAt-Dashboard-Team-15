#include <string.h>
#include "cansat.h"

void tracker_init(Tracker *k) {
    memset(k, 0, sizeof *k);
    k->phase = PH_GROUND;
    k->still_since = -1.0;
    k->deploy_alt = NAN;
    k->landed_at = NAN;
    k->last_packet = -1;
}

/* Altitud barometrica relativa a la presion de la plataforma (atmosfera estandar). */
static double pressure_to_alt(double p, double p0) {
    return 44330.0 * (1.0 - pow(p / p0, 1.0 / 5.255));
}

/* Velocidad vertical = pendiente de la altitud filtrada en ~1 s. */
static double vertical_speed(Tracker *k, double t, double alt) {
    k->h_t[k->head] = t;
    k->h_alt[k->head] = alt;
    k->head = (k->head + 1) % HIST;
    if (k->n < HIST) k->n++;

    int newest = (k->head + HIST - 1) % HIST;
    int pick = -1;
    for (int i = 1; i < k->n; i++) {                 /* de la mas nueva hacia atras */
        int idx = (newest + HIST - i) % HIST;
        pick = idx;
        if (t - k->h_t[idx] >= VEL_WINDOW_S) break;
    }
    if (pick < 0) return 0.0;
    double dt = t - k->h_t[pick];
    return dt < 0.2 ? 0.0 : (alt - k->h_alt[pick]) / dt;
}

void tracker_update(Tracker *k, const Raw *r, Derived *d) {
    /* El contador o el tiempo retroceden = la placa se reinicio.
     *  - En tierra (antes de despegar o ya aterrizado): es un vuelo nuevo -> se reinicia todo.
     *  - En vuelo: el firmware conserva su referencia de altura (ver cansat.ino), asi que aqui tambien
     *    se conserva la calibracion; solo se descarta el historial de tiempo para no calcular una
     *    velocidad con tiempos que retrocedieron. */
    if (k->last_packet >= 0 && (r->packet < k->last_packet || r->t < k->last_t)) {
        if (k->phase == PH_GROUND || k->phase == PH_LANDED) tracker_init(k);
        else { k->n = 0; k->head = 0; k->still_since = -1.0; }
    }
    k->last_packet = r->packet;
    k->last_t = r->t;

    d->alt = d->vel_v = d->alt_max = 0.0;
    d->deploy_alt = d->descent_rate = d->impact_g = d->post_landing_s = NAN;
    d->descent_ok = -1;
    d->phase = PH_GROUND;

    /* 1) Calibracion en tierra */
    if (k->calib_n < CAL_SAMPLES) {
        k->p0_sum += r->pressure;
        k->calib_n++;
        if (k->calib_n == CAL_SAMPLES) k->p0 = k->p0_sum / CAL_SAMPLES;
        return;
    }

    /* 2) Altitud filtrada y velocidad vertical */
    double raw_alt = pressure_to_alt(r->pressure, k->p0);
    if (!k->has_alt) { k->alt_f = raw_alt; k->has_alt = 1; }
    else k->alt_f += ALT_FILTER * (raw_alt - k->alt_f);
    double alt = k->alt_f;
    double vel = vertical_speed(k, r->t, alt);
    if (alt > k->alt_max) k->alt_max = alt;

    /* 3) Pico de g cerca del suelo (el CanSat manda el maximo entre paquetes) */
    if (k->phase >= PH_FREEFALL && alt < IMPACT_ZONE_M) {
        double g = !isnan(r->apk) ? r->apk
                 : sqrt(r->ax*r->ax + r->ay*r->ay + r->az*r->az) / 9.80665;
        if (g > k->peak_g) k->peak_g = g;
    }

    /* 4) Maquina de fases */
    if (r->rotor && !k->rotor_prev && k->phase != PH_LANDED) k->deploy_alt = alt;
    k->rotor_prev = r->rotor;

    switch (k->phase) {
    case PH_GROUND:
        if (alt > ASCENT_ALT_M || vel > ASCENT_VEL_MS) k->phase = PH_ASCENT;
        break;
    case PH_ASCENT:
        if (r->rotor) k->phase = PH_AUTOGYRO;
        else if (vel < FALL_VEL_MS && k->alt_max > 30.0) k->phase = PH_FREEFALL;
        break;
    case PH_FREEFALL:
        if (r->rotor) k->phase = PH_AUTOGYRO;
        break;
    default: break;
    }

    if (k->phase == PH_FREEFALL || k->phase == PH_AUTOGYRO) {
        if (alt < LAND_ALT_M && fabs(vel) < LAND_VEL_MS) {
            if (k->still_since < 0) k->still_since = r->t;
            if (r->t - k->still_since >= LAND_HOLD_S) {
                k->phase = PH_LANDED;
                k->landed_at = k->still_since;   /* fecha el aterrizaje al inicio de la quietud */
            }
        } else k->still_since = -1.0;
    }

    /* 5) Salida */
    d->alt = alt;
    d->vel_v = vel;
    d->alt_max = k->alt_max;
    d->phase = k->phase;
    d->deploy_alt = k->deploy_alt;
    if (k->phase == PH_AUTOGYRO) {
        d->descent_rate = -vel;
        d->descent_ok = (d->descent_rate >= DESCENT_MIN && d->descent_rate <= DESCENT_MAX);
    }
    if (k->phase == PH_LANDED) {
        d->impact_g = k->peak_g;
        d->post_landing_s = r->t - k->landed_at;
    }
}
