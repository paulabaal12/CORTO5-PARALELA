#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

#define NUM_CARROS 5
#define NUM_VUELTAS 3
#define NUM_SECTORES 4

int main() {
	int tiempos[NUM_CARROS][NUM_VUELTAS][NUM_SECTORES]; // matriz de tiempos por sector
	int tiempo_total[NUM_CARROS] = {0}; // tiempo total por carro
	int tiempo_base = 25; // tiempo base inicial para cada sector
	int i, j, k;

	srand(time(NULL)); // inicializar generador de números aleatorios

	// Inicializar matriz de tiempos a 0
	for (i = 0; i < NUM_CARROS; i++) {
		for (j = 0; j < NUM_VUELTAS; j++) {
			for (k = 0; k < NUM_SECTORES; k++) {
				tiempos[i][j][k] = 0;
			}
		}
	}

	// Calcular tiempos por sector usando firstprivate
	// Cada hilo recibe su propia copia de tiempo_base
	#pragma omp parallel for private(j, k) firstprivate(tiempo_base) shared(tiempos)
	for (i = 0; i < NUM_CARROS; i++) {
		// Inicializar semilla (valores númericos aleatorios) única para cada hilo/carro
		unsigned int semilla = (unsigned int)time(NULL) + i * 100 + omp_get_thread_num();
		srand(semilla);
		for (j = 0; j < NUM_VUELTAS; j++) {
			for (k = 0; k < NUM_SECTORES; k++) {
				// Simular variación aleatoria en el tiempo de cada sector
				int variacion = rand() % 4; // 0 a 3 segundos extra
				int tiempo_sector = tiempo_base + variacion;
				tiempos[i][j][k] = tiempo_sector;
				// Mostrar cómo cada hilo usa su tiempo_base privado
				printf("Carro %d, Vuelta %d, Sector %d: tiempo_base=%d, variacion=%d, tiempo_sector=%d\n",
					   i+1, j+1, k+1, tiempo_base, variacion, tiempo_sector);
			}
		}
	}

	// Calcular tiempo total por carro
	for (i = 0; i < NUM_CARROS; i++) {
		for (j = 0; j < NUM_VUELTAS; j++) {
			for (k = 0; k < NUM_SECTORES; k++) {
				tiempo_total[i] += tiempos[i][j][k];
			}
		}
	}

	// Determinar el ganador
	int ganador = 1;
	int mejor_tiempo = tiempo_total[0];
	for (i = 1; i < NUM_CARROS; i++) {
		if (tiempo_total[i] < mejor_tiempo) {
			mejor_tiempo = tiempo_total[i];
			ganador = i + 1;
		}
	}

	printf("\nResultados finales:\n");
	for (i = 0; i < NUM_CARROS; i++) {
		printf("Carro %d: Tiempo total = %d segundos\n", i+1, tiempo_total[i]);
	}
	printf("Ganador: Carro %d\n", ganador);

	return 0;
}