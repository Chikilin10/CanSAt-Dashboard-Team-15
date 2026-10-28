#include <stdio.h>
#include <string.h>
#include "cansat.h"

/*
 * Lee lineas CSV crudas por stdin y escribe una linea JSON por paquete en stdout.
 * Comando especial por stdin: "RESET" reinicia calibracion y fases.
 * Los mensajes de diagnostico van a stderr para no ensuciar el JSON.
 */
int main(void) {
    Tracker k;
    Raw r;
    Derived d;
    char line[512];
    long bad = 0;

    tracker_init(&k);
    setvbuf(stdout, NULL, _IOLBF, 0);

    while (fgets(line, sizeof line, stdin)) {
        if (strncmp(line, "RESET", 5) == 0) {
            tracker_init(&k);
            fprintf(stderr, "[c] tracker reiniciado\n");
            continue;
        }
        if (parse_line(line, &r) != 0) {
            fprintf(stderr, "[c] linea descartada (%ld): %.60s\n", ++bad, line);
            continue;
        }
        tracker_update(&k, &r, &d);
        print_json(&r, &d);
    }
    return 0;
}
