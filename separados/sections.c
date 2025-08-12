#include <stdio.h>
#include <omp.h>

int main() {
    // Variables simuladas para cada sección
    int mejor_vuelta = 74; // mejor vuelta
    float promedio_vueltas = 77.8; // promedio
    int posiciones[5] = {5, 3, 1, 4, 2}; // posiciones finales para 5 carros
    char clima[20] = "Soleado";

    #pragma omp parallel sections
    {
        // Sección 1: Calcular estadísticas de la carrera
        #pragma omp section
        {
            printf("Seccion 1: Estadisticas\n");
            printf("  Mejor vuelta: %d segundos\n", mejor_vuelta);
            printf("  Promedio de vueltas: %.1f segundos\n", promedio_vueltas);
            printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
        }
        // Sección 2: Registrar orden de posiciones
        #pragma omp section
        {
            printf("Seccion 2: Posiciones finales\n");
            for (int i = 0; i < 5; i++) {
                printf("  Lugar %d: Carro %d\n", i+1, posiciones[i]);
            }
            printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
        }
        // Sección 3: Actualizar información del clima
        #pragma omp section
        {
            printf("Seccion 3: Clima\n");
            printf("  Estado del clima: %s\n", clima);
            printf("  Ejecutado por thread %d\n\n", omp_get_thread_num());
        }
    }
    return 0;
}
