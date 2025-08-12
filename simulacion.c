#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>




// Número de carros, vueltas y sectores en la carrera
#define NUM_CARROS 5
#define NUM_VUELTAS 3
#define NUM_SECTORES 4
#define CIRCUITO "Monza, Italia" // Nombre del circuito


// Nombres y equipos de los pilotos
const char* nombres[NUM_CARROS] = {"Hamilton", "Verstappen", "Leclerc", "Norris", "Sainz"};
const char* equipos[NUM_CARROS] = {"Ferrari", "Red Bull", "Ferrari", "McLaren", "Williams"};
// Climas posibles para cada vuelta
const char* climas_posibles[] = {"Soleado", "Nublado", "Lluvia"};

int main() {

	// Matriz para almacenar los tiempos por sector de cada carro, vuelta y sector
	int tiempos[NUM_CARROS][NUM_VUELTAS][NUM_SECTORES];
	// Arreglo para guardar el tiempo total de cada carro
	int tiempo_total[NUM_CARROS] = {0};
	// Variable para encontrar la mejor vuelta global (inicializada con un valor grande)
	int mejor_vuelta_global = 9999;
	// Arreglo para guardar la mejor vuelta de cada carro
	int mejor_vuelta_carro[NUM_CARROS];
	// Arreglo para guardar el orden final de los carros (según tiempo total)
	int posiciones[NUM_CARROS];
	// Clima de cada vuelta
	char clima_vuelta[NUM_VUELTAS][20];
	// Tiempo base para cada sector
	int tiempo_base = 25;
	// Promedio de las mejores vueltas de todos los carros
	float promedio_vueltas = 0.0;
	// Índice del carro y número de vuelta de la mejor vuelta global
	int carro_mejor_vuelta = -1;
	int num_vuelta_mejor = -1;
	// Variables auxiliares para clima
	const char* clima[] = {"Soleado", "Nublado", "Lluvia"};
	int num_climas = 3;

	// Inicializar generador de números aleatorios
	srand(time(NULL));
	printf("\n========================================\n");
	printf("  ¡Comienza la carrera de F1 en %s!\n", CIRCUITO);
	printf("========================================\n\n");

	// Inicializar matriz de tiempos y mejor vuelta de cada carro
	for (int i = 0; i < NUM_CARROS; i++) {
		for (int j = 0; j < NUM_VUELTAS; j++) {
			for (int k = 0; k < NUM_SECTORES; k++) {
				tiempos[i][j][k] = 0;
			}
		}
		mejor_vuelta_carro[i] = 9999;
	}

	// Generar clima aleatorio para cada vuelta antes del cálculo paralelo
	// Así cada vuelta tiene un clima diferente
	for (int j = 0; j < NUM_VUELTAS; j++) {
		int clima_idx = rand() % 3;
		snprintf(clima_vuelta[j], sizeof(clima_vuelta[j]), "%s", climas_posibles[clima_idx]);
	}

	// Calcular tiempos por sector usando firstprivate
	// Cada hilo recibe su propia copia de tiempo_base y generador local
	#pragma omp parallel for firstprivate(tiempo_base) shared(tiempos)
	for (int i = 0; i < NUM_CARROS; i++) {
		unsigned int seed = (unsigned int)time(NULL) + i * 1000;
		for (int j = 0; j < NUM_VUELTAS; j++) {
			for (int k = 0; k < NUM_SECTORES; k++) {
				// Generador local simple para cada piloto
				seed = seed * 1103515245 + 12345;
				int variacion = (seed % 7); // 0 a 6 segundos extra
				int tiempo_sector = tiempo_base + variacion;
				tiempos[i][j][k] = tiempo_sector;
			}
		}
	}

	// Calcular tiempo total por carro y mejores vueltas usando reduction
	// Cada hilo calcula el tiempo total de un carro y busca la mejor vuelta
	#pragma omp parallel for reduction(min:mejor_vuelta_global)
	for (int i = 0; i < NUM_CARROS; i++) {
		int tiempo_total_piloto = 0;
		for (int j = 0; j < NUM_VUELTAS; j++) {
			int tiempo_vuelta = 0;
			for (int k = 0; k < NUM_SECTORES; k++) {
				// Sumar tiempos de los sectores para cada vuelta
				tiempo_vuelta += tiempos[i][j][k];
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
			tiempo_total_piloto += tiempo_vuelta;
		}
		tiempo_total[i] = tiempo_total_piloto;
	}

	// Calcular promedio de las mejores vueltas de todos los carros
	int suma_vueltas = 0;
	int total_vueltas = NUM_CARROS * NUM_VUELTAS;
	for (int i = 0; i < NUM_CARROS; i++) {
		suma_vueltas += mejor_vuelta_carro[i];
	}
	promedio_vueltas = (float)suma_vueltas / NUM_CARROS;

	// Mostrar resultados usando sections
	// Cada sección ejecuta una tarea independiente en paralelo
	#pragma omp parallel sections
	{
		// Sección 1: Estadísticas de la carrera
		#pragma omp section
		{
			printf("Seccion 1: Estadisticas\n");
			printf("  Mejor vuelta global: %d segundos (Piloto: %s, Vuelta %d)\n", mejor_vuelta_global, nombres[carro_mejor_vuelta], num_vuelta_mejor+1);
			printf("  Promedio de mejores vueltas: %.1f segundos\n", promedio_vueltas);
			printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
		}
		// Sección 2: Posiciones finales (podio)
		#pragma omp section
		{
			printf("Seccion 2: Posiciones finales\n");
			for (int i = 0; i < NUM_CARROS; i++) {
				int idx = posiciones[i];
				printf("  Lugar %d: %s (%s) - Tiempo total: %d seg, Mejor vuelta: %d seg\n", i+1, nombres[idx], equipos[idx], tiempo_total[idx], mejor_vuelta_carro[idx]);
			}
			printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
		}
		// Sección 3: Clima por vuelta
		#pragma omp section
		{
			printf("Seccion 3: Clima por vuelta\n");
			for (int j = 0; j < NUM_VUELTAS; j++) {
				printf("  Vuelta %d: %s\n", j+1, clima_vuelta[j]);
			}
			printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
		}
	}

	// Calcular posiciones finales (ordenar por tiempo total)
	// Se ordena el arreglo de posiciones para mostrar el podio
	for (int i = 0; i < NUM_CARROS; i++) posiciones[i] = i;
	// Simple bubble sort para ordenar posiciones por tiempo total
	for (int i = 0; i < NUM_CARROS-1; i++) {
		for (int j = 0; j < NUM_CARROS-1-i; j++) {
			if (tiempo_total[posiciones[j]] > tiempo_total[posiciones[j+1]]) {
				int tmp = posiciones[j];
				posiciones[j] = posiciones[j+1];
				posiciones[j+1] = tmp;
			}
		}
	}

	// Tabla resumen final de la carrera
	printf("\n---------------------------------------------\n");
	printf("Piloto      | Escuderia     | Tiempo Total | Mejor Vuelta\n");
	printf("---------------------------------------------\n");
	for (int i = 0; i < NUM_CARROS; i++) {
		printf("%-11s | %-10s |   %4d seg   |   %3d seg\n", nombres[i], equipos[i], tiempo_total[i], mejor_vuelta_carro[i]);
	}
	printf("---------------------------------------------\n");

	// Mostrar el podio final
	printf("\nPodio final en %s:\n", CIRCUITO);
	for (int i = 0; i < NUM_CARROS; i++) {
		int idx = posiciones[i];
		printf("  Lugar %d: %s (%s) - Tiempo total: %d seg, Mejor vuelta: %d seg\n", i+1, nombres[idx], equipos[idx], tiempo_total[idx], mejor_vuelta_carro[idx]);
	}
	// Mostrar la mejor vuelta global
	printf("\nMejor vuelta global: %d segundos (Piloto: %s, Vuelta %d)\n", mejor_vuelta_global, nombres[carro_mejor_vuelta], num_vuelta_mejor+1);
	printf("\n========================================\n");
	printf("   ¡Fin de la carrera en %s!\n", CIRCUITO);
	printf("========================================\n");

	return 0;
}
