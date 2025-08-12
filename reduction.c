
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

#define NUM_CARROS 5      // Número de carros en la carrera
#define NUM_VUELTAS 3     // Número de vueltas
#define NUM_SECTORES 4    // Número de sectores por vuelta

int main() {
    // Matriz para almacenar los tiempos por sector de cada carro
    int tiempos[NUM_CARROS][NUM_VUELTAS][NUM_SECTORES];
    // Arreglo para guardar el tiempo total de cada carro
    int tiempo_total[NUM_CARROS] = {0};
    // Variable para encontrar la mejor vuelta global (inicializada con un valor grande)
    int mejor_vuelta_global = 9999;
    int tiempo_base = 25; // Tiempo base para cada sector

    srand(time(NULL)); // Inicializar generador de números aleatorios

    // Inicializar matriz de tiempos con valores aleatorios para cada sector
    for (int i = 0; i < NUM_CARROS; i++) {
        for (int j = 0; j < NUM_VUELTAS; j++) {
            for (int k = 0; k < NUM_SECTORES; k++) {
                // Se suma una variación aleatoria al tiempo base
                int variacion = rand() % 4; // 0 a 3 segundos extra
                tiempos[i][j][k] = tiempo_base + variacion;
            }
        }
    }

        // Arreglo para guardar la mejor vuelta de cada carro
        int mejor_vuelta_carro[NUM_CARROS];
        for (int i = 0; i < NUM_CARROS; i++) mejor_vuelta_carro[i] = 9999;

        // Variables para identificar el carro y vuelta de la mejor vuelta global
        int carro_mejor_vuelta = -1;
        int num_vuelta_mejor = -1;

        // Calcular el tiempo total por piloto y la mejor vuelta global usando reduction
        // Cada hilo calcula el tiempo total de un carro y busca la mejor vuelta
        #pragma omp parallel for reduction(min:mejor_vuelta_global)
        for (int i = 0; i < NUM_CARROS; i++) {
            int tiempo_total_piloto = 0;
            for (int j = 0; j < NUM_VUELTAS; j++) {
                int tiempo_vuelta = 0;
                for (int k = 0; k < NUM_SECTORES; k++) {
                    tiempo_vuelta += tiempos[i][j][k]; // Sumar tiempos de los sectores
                }
                // Guardar la mejor vuelta de cada carro
                if (tiempo_vuelta < mejor_vuelta_carro[i]) {
                    mejor_vuelta_carro[i] = tiempo_vuelta;
                }
                // Actualizar la mejor vuelta global y quién la hizo
                #pragma omp critical
                {
                    if (tiempo_vuelta < mejor_vuelta_global) {
                        mejor_vuelta_global = tiempo_vuelta;
                        carro_mejor_vuelta = i;
                        num_vuelta_mejor = j;
                    }
                }
                tiempo_total_piloto += tiempo_vuelta; // Acumular tiempo total del carro
            }
            tiempo_total[i] = tiempo_total_piloto; // Guardar tiempo total del carro
        }

        // Mostrar resultados finales en formato de tabla
        printf("Resultados finales:\n");
        printf("---------------------------------------------\n");
        printf("Carro | Tiempo Total | Mejor Vuelta\n");
        printf("---------------------------------------------\n");
        for (int i = 0; i < NUM_CARROS; i++) {
            printf("  %2d  |    %4d seg   |   %3d seg\n", i+1, tiempo_total[i], mejor_vuelta_carro[i]);
        }
        printf("---------------------------------------------\n");
        printf("Mejor vuelta global: %d segundos (Carro %d, Vuelta %d)\n", mejor_vuelta_global, carro_mejor_vuelta+1, num_vuelta_mejor+1);

        return 0;

    return 0;
}
