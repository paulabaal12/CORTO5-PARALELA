#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

#define NUM_CARROS 5
#define NUM_VUELTAS 3
#define NUM_SECTORES 4
#define CIRCUITO "Monza, Italia" 

const char *nombres[NUM_CARROS]  = {"Hamilton", "Verstappen", "Leclerc", "Norris", "Sainz"};
const char *equipos[NUM_CARROS]  = {"Ferrari",  "Red Bull",   "Ferrari", "McLaren","Williams"};
const char *climas_posibles[]    = {"Soleado", "Nublado", "Lluvia"};

int main(void) {

    // --- Declaraciones compartidas/estado de la simulación ---
    int  tiempos[NUM_CARROS][NUM_VUELTAS][NUM_SECTORES]; // compartida
    int  tiempo_total[NUM_CARROS] = {0};                 // compartida
    int  mejor_vuelta_global = 9999;                     // reducción(min)
    int  mejor_vuelta_carro[NUM_CARROS];                 // compartida
    int  posiciones[NUM_CARROS];                         // compartida
    char clima_vuelta[NUM_VUELTAS][20];                  // compartida
    int  tiempo_base = 25;                               // firstprivate
    float promedio_vueltas = 0.0f;                       // salida
    int  carro_mejor_vuelta = -1;                        // post-reducción
    int  num_vuelta_mejor  = -1;                         // post-reducción

    // --- Variables para usar en cláusulas private ---
    int i, j, k;
    int tiempo_sector;
    int tiempo_vuelta;
    int tiempo_total_piloto;

    srand((unsigned int)time(NULL));

    printf("\n========================================\n");
    printf("  ¡Comienza la carrera de F1 en %s!\n", CIRCUITO);
    printf("========================================\n\n");

    // Inicializar matriz de tiempos y mejor vuelta de cada carro
    for (i = 0; i < NUM_CARROS; i++) {
        for (j = 0; j < NUM_VUELTAS; j++) {
            for (k = 0; k < NUM_SECTORES; k++) {
                tiempos[i][j][k] = 0;
            }
        }
        mejor_vuelta_carro[i] = 9999;
    }

    // ============================================================
    // Fase 1: Calcular tiempos por sector (parallel for)
    // private(i,j,k,tiempo_sector), firstprivate(tiempo_base), shared(tiempos)
    // ============================================================
    #pragma omp parallel for private(i, j, k, tiempo_sector) firstprivate(tiempo_base) shared(tiempos)
    for (i = 0; i < NUM_CARROS; i++) {
        unsigned int seed = (unsigned int)time(NULL) + (unsigned int)(i * 10007 + omp_get_thread_num());
        for (j = 0; j < NUM_VUELTAS; j++) {
            for (k = 0; k < NUM_SECTORES; k++) {
                seed = seed * 1103515245u + 12345u;
                int variacion   = (int)((seed >> 16) % 7); // 0..6
                tiempo_sector   = tiempo_base + variacion;
                tiempos[i][j][k]= tiempo_sector;
            }
        }
    }

    // ======================================================================
    // Fase 2: Calcular tiempo total por carro y mejor vuelta (parallel for)
    // private(j,k,tiempo_vuelta,tiempo_total_piloto), shared(tiempos,tiempo_total,mejor_vuelta_carro)
    // reduction(min:mejor_vuelta_global)
    // ======================================================================
    #pragma omp parallel for private(j, k, tiempo_vuelta, tiempo_total_piloto) \
        shared(tiempos, tiempo_total, mejor_vuelta_carro) reduction(min:mejor_vuelta_global)
    for (i = 0; i < NUM_CARROS; i++) {
        tiempo_total_piloto = 0;
        for (j = 0; j < NUM_VUELTAS; j++) {
            tiempo_vuelta = 0;
            for (k = 0; k < NUM_SECTORES; k++) {
                tiempo_vuelta += tiempos[i][j][k];
            }
            if (tiempo_vuelta < mejor_vuelta_carro[i]) {
                mejor_vuelta_carro[i] = tiempo_vuelta;
            }
            // La reducción se queda con el valor mínimo global
            if (tiempo_vuelta < mejor_vuelta_global) {
                mejor_vuelta_global = tiempo_vuelta;
            }
            tiempo_total_piloto += tiempo_vuelta;
        }
        tiempo_total[i] = tiempo_total_piloto;
    }

    // Promedio de las mejores vueltas
    int suma_vueltas = 0;
    #pragma omp parallel for reduction(+:suma_vueltas)
    for (i = 0; i < NUM_CARROS; i++) {
        suma_vueltas += mejor_vuelta_carro[i];
    }
    promedio_vueltas = (float)suma_vueltas / (float)NUM_CARROS;

    // Resolver quién hizo la mejor vuelta global y en qué vuelta (post-reducción)
    for (i = 0; i < NUM_CARROS; i++) {
        for (j = 0; j < NUM_VUELTAS; j++) {
            int tv = 0;
            for (k = 0; k < NUM_SECTORES; k++) tv += tiempos[i][j][k];
            if (tv == mejor_vuelta_global) {
                carro_mejor_vuelta = i;
                num_vuelta_mejor   = j;
                i = NUM_CARROS; // romper ambos bucles
                break;
            }
        }
    }

    // =================================================================================
    // Secciones paralelas independientes: estadísticas, posiciones y clima
    // Se agregaron regiones críticas para evitar intercalado de prints entre secciones
    // =================================================================================
    #pragma omp parallel sections shared(posiciones, tiempo_total, mejor_vuelta_carro, nombres, equipos, clima_vuelta, mejor_vuelta_global, carro_mejor_vuelta, num_vuelta_mejor, promedio_vueltas)
    {
        // Sección 1: Estadísticas de la carrera
        #pragma omp section
        {
            #pragma omp critical
            {
                printf("Seccion 1: Estadisticas\n");
                printf("  Mejor vuelta global: %d segundos (Piloto: %s, Vuelta %d)\n",
                    mejor_vuelta_global,
                    (carro_mejor_vuelta >= 0 ? nombres[carro_mejor_vuelta] : "N/A"),
                    (num_vuelta_mejor  >= 0 ? num_vuelta_mejor + 1 : 0));
                printf("  Promedio de mejores vueltas: %.1f segundos\n", promedio_vueltas);
                printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
            }
        }

        // Sección 2: Calcular y mostrar posiciones finales
        #pragma omp section
        {
            // Inicializa posiciones
            for (int p = 0; p < NUM_CARROS; p++) posiciones[p] = p;

            // Ordena por tiempo_total (bubble sort simple)
            for (int a = 0; a < NUM_CARROS - 1; a++) {
                for (int b = 0; b < NUM_CARROS - 1 - a; b++) {
                    if (tiempo_total[posiciones[b]] > tiempo_total[posiciones[b + 1]]) {
                        int tmp = posiciones[b];
                        posiciones[b] = posiciones[b + 1];
                        posiciones[b + 1] = tmp;
                    }
                }
            }

            #pragma omp critical
            {
                printf("Seccion 2: Posiciones finales\n");
                for (int r = 0; r < NUM_CARROS; r++) {
                    int idx = posiciones[r];
                    printf("  Lugar %d: %s (%s) - Tiempo total: %d seg, Mejor vuelta: %d seg\n",
                        r + 1, nombres[idx], equipos[idx], tiempo_total[idx], mejor_vuelta_carro[idx]);
                }
                printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
            }
        }

        // Sección 3: Actualizar clima por vuelta y mostrar
        #pragma omp section
        {
            unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)omp_get_thread_num();
            for (int v = 0; v < NUM_VUELTAS; v++) {
                seed = seed * 1103515245u + 12345u;
                int clima_idx = (int)((seed >> 16) % 3); // 0..2
                snprintf(clima_vuelta[v], sizeof(clima_vuelta[v]), "%s", climas_posibles[clima_idx]);
            }

            #pragma omp critical
            {
                printf("Seccion 3: Clima por vuelta\n");
                for (int v = 0; v < NUM_VUELTAS; v++) {
                    printf("  Vuelta %d: %s\n", v + 1, clima_vuelta[v]);
                }
                printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
            }
        }
    }

    // Tabla resumen final de la carrera
    printf("\n---------------------------------------------\n");
    printf("Piloto      | Escuderia     | Tiempo Total | Mejor Vuelta\n");
    printf("---------------------------------------------\n");
    for (i = 0; i < NUM_CARROS; i++) {
        printf("%-11s | %-10s |   %4d seg   |   %3d seg\n",
            nombres[i], equipos[i], tiempo_total[i], mejor_vuelta_carro[i]);
    }
    printf("---------------------------------------------\n");

    // Podio final y ganador
    printf("\nPodio final en %s:\n", CIRCUITO);
    for (i = 0; i < NUM_CARROS; i++) {
        int idx = posiciones[i];
        printf("  Lugar %d: %s (%s) - Tiempo total: %d seg, Mejor vuelta: %d seg\n",
            i + 1, nombres[idx], equipos[idx], tiempo_total[idx], mejor_vuelta_carro[idx]);
    }
    printf("\nGanador: %s\n", nombres[posiciones[0]]);

    // Mostrar la mejor vuelta global
    printf("\nMejor vuelta global: %d segundos (Piloto: %s, Vuelta %d)\n",
        mejor_vuelta_global,
        (carro_mejor_vuelta >= 0 ? nombres[carro_mejor_vuelta] : "N/A"),
        (num_vuelta_mejor  >= 0 ? num_vuelta_mejor + 1 : 0));

    printf("\n========================================\n");
    printf("   ¡Fin de la carrera en %s!\n", CIRCUITO);
    printf("========================================\n");

    return 0;
}