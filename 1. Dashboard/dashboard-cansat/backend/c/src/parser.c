#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cansat.h"

/*
 * Linea esperada (ver docs/protocol.md):
 * T,packet,ms,pressure,temp,ax,ay,az,gx,gy,gz,volt,rotor,sec_dist,sec_bearing,apk,lat,lon,sats,rssi
 * Un campo vacio significa "sin dato" (NAN).
 */
#define MAX_FIELDS 32

static double num(char **f, int nf, int i) {
    if (i >= nf || f[i][0] == '\0') return NAN;
    char *end;
    double v = strtod(f[i], &end);
    return (end == f[i]) ? NAN : v;
}

int parse_line(const char *line, Raw *r) {
    char buf[512];
    strncpy(buf, line, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0';

    char *f[MAX_FIELDS];
    int nf = 0;
    char *p = buf;
    while (nf < MAX_FIELDS) {          /* split que conserva campos vacios */
        f[nf++] = p;
        char *c = strchr(p, ',');
        if (!c) break;
        *c = '\0';
        p = c + 1;
    }

    if (nf < 13 || strcmp(f[0], "T") != 0) return -1;

    double packet = num(f, nf, 1), ms = num(f, nf, 2);
    r->pressure = num(f, nf, 3);
    if (isnan(packet) || isnan(ms) || isnan(r->pressure)) return -1;
    if (r->pressure < 300.0 || r->pressure > 1100.0) return -1;   /* dato corrupto */

    r->packet = (long)packet;
    r->t = ms / 1000.0;
    r->temp = num(f, nf, 4);
    r->ax = num(f, nf, 5); r->ay = num(f, nf, 6); r->az = num(f, nf, 7);
    r->gx = num(f, nf, 8); r->gy = num(f, nf, 9); r->gz = num(f, nf, 10);
    r->volt = num(f, nf, 11);
    r->rotor = (int)num(f, nf, 12) != 0;
    r->sec_dist = num(f, nf, 13);
    r->sec_bearing = num(f, nf, 14);
    r->apk = num(f, nf, 15);
    r->lat = num(f, nf, 16);
    r->lon = num(f, nf, 17);
    double s = num(f, nf, 18), rs = num(f, nf, 19);
    r->sats = isnan(s) ? 0 : (int)s;
    r->rssi = isnan(rs) ? 0 : (int)rs;
    return 0;
}

static void pnum(const char *key, double v, int dec, int *first) {
    printf("%s\"%s\":", *first ? "" : ",", key);
    *first = 0;
    if (isnan(v) || isinf(v)) printf("null");
    else {
        double scale = 1.0;
        for (int i = 0; i < dec; i++) scale *= 10.0;
        if (fabs(v) < 0.5 / scale) v = 0.0;      /* evita imprimir -0.0 */
        printf("%.*f", dec, v);
    }
}

static void pint(const char *key, long v, int *first) {
    printf("%s\"%s\":%ld", *first ? "" : ",", key, v);
    *first = 0;
}

static const char *PHASES[] = { "ground", "ascent", "freefall", "autogyro", "landed" };

void print_json(const Raw *r, const Derived *d) {
    int f = 1;
    printf("{\"raw\":{");
    pint("packet", r->packet, &f);
    pnum("t", r->t, 2, &f);
    pnum("pressure", r->pressure, 2, &f);
    pnum("temp", r->temp, 2, &f);
    pnum("ax", r->ax, 2, &f); pnum("ay", r->ay, 2, &f); pnum("az", r->az, 2, &f);
    pnum("gx", r->gx, 1, &f); pnum("gy", r->gy, 1, &f); pnum("gz", r->gz, 1, &f);
    pnum("volt", r->volt, 2, &f);
    printf(",\"rotor_deployed\":%s", r->rotor ? "true" : "false");
    pnum("sec_dist", r->sec_dist, 1, &f);
    pnum("sec_bearing", r->sec_bearing, 0, &f);
    pnum("lat", r->lat, 6, &f); pnum("lon", r->lon, 6, &f);
    pint("sats", r->sats, &f);
    pint("rssi", r->rssi, &f);
    printf("},\"derived\":{");
    f = 1;
    pnum("alt", d->alt, 1, &f);
    pnum("vel_v", d->vel_v, 1, &f);
    pnum("alt_max", d->alt_max, 1, &f);
    printf(",\"phase\":\"%s\"", PHASES[d->phase]);
    pnum("deploy_alt", d->deploy_alt, 1, &f);
    pnum("descent_rate", d->descent_rate, 1, &f);
    printf(",\"descent_ok\":%s", d->descent_ok < 0 ? "null" : d->descent_ok ? "true" : "false");
    pnum("impact_g", d->impact_g, 1, &f);
    pnum("post_landing_s", d->post_landing_s, 1, &f);
    printf("}}\n");
    fflush(stdout);
}
