# CORTO5-PARALELA
Para este corto se nos solicitó crear una simulación de un sistema paralelo utilizando las clausulas de OpenMP:
- `parallel for`
- `sections`
- `shared`
- `firstprivate`
- `reduction`

---
## **Simulación: Carrera de Fórmula 1**

### Descripción
La simulación representa una carrera de Fórmula 1 donde varios autos compiten en un circuito con vueltas y sectores.
El objetivo de nuestra simulación es calcular el tiempo total de cada piloto y determinar el ganador, aprovechando el paralelismo para procesar diferentes partes de la simulación de forma simultánea.

### Partes que se ejecutarán en paralelo y cláusulas aplicadas

1. **`parallel for`**

   * Usaremos un `parallel for` para simular los tiempos por sector de cada piloto en cada vuelta.
   * Cada iteración corresponde a un piloto en una vuelta específica, y se calcula su tiempo en paralelo.

2. **`sections`**

   * Dividiremos tareas independientes en secciones:
     * Una sección calcula estadísticas de la carrera (mejor vuelta, promedio).
     * Otra registra el orden de posiciones.
     * Otra actualiza la información del clima.

3. **`firstprivate`**

   * Se aplicará para variables que deben inicializarse con un valor común, pero que no deben ser compartidas (por ejemplo, el tiempo base inicial para cada piloto antes de aplicar variaciones aleatorias).

4. **`shared`**

   * Variables globales como la matriz de tiempos, el número de pilotos, vueltas y sectores serán compartidas entre todos los hilos.

5. **`reduction`**

   * Para calcular:
     * El tiempo total de la carrera por piloto sumando sus sectores.
     * Encontrar el tiempo mínimo (mejor vuelta global).

---

### **Variables compartidas y privadas**
* **Privadas**:
    - Indices de bucles (`i`, `j`, `k`)
    - `tiempo_base` 
    - `tiempo_sector` 
* **Compartidas**:
    - `tiempos[NUM_PILOTOS][NUM_VUELTAS][NUM_SECTORES]`
    - `mejor_vuelta_global`
    - `NUM_PILOTOS`
    - `NUM_VUELTAS`
    - `NUM_SECTORES`

---

## **Pseudocódigo**

```txt
// Declaración de constantes
NUM_PILOTOS ← 20
NUM_VUELTAS ← 40
NUM_SECTORES ← 4

// Declaración de variables compartidas

tiempos[NUM_PILOTOS][NUM_VUELTAS][NUM_SECTORES]
mejor_vuelta_global ← valor grande
ganador ← -1

// Variables privadas en los bucles paralelos
i, j, k, tiempo_base, tiempo_sector

// Inicializar matriz de tiempos a 0
para i desde 0 hasta NUM_PILOTOS-1:
    para j desde 0 hasta NUM_VUELTAS-1:
        para k desde 0 hasta NUM_SECTORES-1:
            tiempos[i][j][k] ← 0

// Sección 1: Calcular tiempos por sector en paralelo

#pragma omp parallel for private(i, j, k, tiempo_sector) firstprivate(tiempo_base) shared(tiempos)
para i desde 0 hasta NUM_PILOTOS-1:
    para j desde 0 hasta NUM_VUELTAS-1:
        para k desde 0 hasta NUM_SECTORES-1:
            // tiempo_base es inicializado con un valor base común
            tiempo_sector ← tiempo_base + variación aleatoria
            tiempos[i][j][k] ← tiempo_sector

// Sección 2: Calcular tiempo total por piloto y mejor vuelta

#pragma omp parallel for reduction(min: mejor_vuelta_global)
para i desde 0 hasta NUM_PILOTOS-1:
    tiempo_total_piloto ← 0
    para j desde 0 hasta NUM_VUELTAS-1:
        tiempo_vuelta ← sumar(tiempos[i][j][:])
        si tiempo_vuelta < mejor_vuelta_global:
            mejor_vuelta_global ← tiempo_vuelta
        tiempo_total_piloto ← tiempo_total_piloto + tiempo_vuelta
    // Guardar el tiempo total en una variable compartida

// Sección 3: Estadísticas y clima (parallel sections)

#pragma omp parallel sections
{
    #section 1: Calcular posiciones finales
        // Ordenar pilotos por tiempo total

    #section 2: Registrar estadísticas adicionales
        // Mejor vuelta por piloto, promedio de vueltas

    #section 3: Actualizar información del clima
        // Simular cambios de temperatura, humedad, lluvia
}

// Imprimir resultados finales
Imprimir ganador, mejor_vuelta_global, posiciones finales
```

---
## **Ejemplo de Simulación**
A continuación se describirá una carrera de 2 carros, 3 vueltas y en un circuito de 4 sectores.

**Variables:**
* `num_carros` = 2
* `num_vueltas` = 3
* `num_sectores` = 4
* `tiempos[num_carros][num_vueltas][num_sectores]` → Matriz con tiempos por sector.
* `tiempo_total[num_carros]` → Tiempo acumulado total por carro.
* `ganador` → Índice del carro con menor tiempo total.

---

## Ejemplo de carrera

Supongamos que estos son los tiempos de los carros en segundos para cada sector y vuelta:

| Carro | Vuelta | Sector 1 | Sector 2 | Sector 3 | Sector 4 |
| ----- | ------ | -------- | -------- | -------- | -------- |
| 1     | 1      | 25       | 27       | 26       | 28       |
| 2     | 1      | 26       | 28       | 25       | 27       |
| 1     | 2      | 24       | 26       | 25       | 27       |
| 2     | 2      | 27       | 27       | 26       | 28       |
| 1     | 3      | 25       | 25       | 26       | 27       |
| 2     | 3      | 26       | 28       | 25       | 26       |

---

### Orden de ejecución

1. **Inicialización**

   * `tiempo_total[1] = 0`
   * `tiempo_total[2] = 0`

    <br>

2. **Recorrido de vueltas y sectores** (usando `parallel for` para que cada carro calcule en paralelo sus tiempos por vuelta y sector).

   * **Vuelta 1**

     * **Sector 1:**

       * Carro 1: +25 seg → total 25
       * Carro 2: +26 seg → total 26

        > **Va ganando Carro 1**

     * **Sector 2:**

       * Carro 1: +27 seg → total 52
       * Carro 2: +28 seg → total 54

        > **Carro 1 sigue ganando**
     * **Sector 3:**

       * Carro 1: +26 seg → total 78
       * Carro 2: +25 seg → total 79
        > **Carro 1 sigue ganando**
     * **Sector 4:**

       * Carro 1: +28 seg → total 106
       * Carro 2: +27 seg → total 106
        > **Empate al final de la vuelta 1**

   * **Vuelta 2**

     * Sectores 1 a 4 se calculan igual, acumulando tiempos.
     * Al final de la vuelta 2:

       * Carro 1 total: 216 seg
       * Carro 2 total: 208 seg
        > **Carro 2 adelante por 8 segundos**

   * **Vuelta 3**

     * Sectores 1 a 4 suman al tiempo total.
     * Tiempo final:

       * Carro 1: 321 seg
       * Carro 2: 311 seg
       
    <br>

3. **Determinación del ganador** (`reduction(min:tiempo_total)`)

   * Mínimo tiempo: 311 → **Gana Carro 2**

---

### Flujo resumido

1. Cada **carro** calcula su tiempo por sector en paralelo.
2. Cada **sector** se suma a su tiempo total (`shared: tiempo_total`, pero cada hilo trabaja sobre su índice).
3. Al final de cada vuelta se puede determinar quién va ganando.
4. Se usa **reduction(min\:tiempo\_total)** para encontrar el ganador.

## Cómo ejecutar firstprivate.c

1. Abre una terminal (PowerShell) en la carpeta del proyecto.
2. Compila el archivo usando GCC con soporte OpenMP:
    ```powershell
    gcc -fopenmp firstprivate.c -o firstprivate.exe
    ```
3. Ejecuta el programa:
    ```powershell
    ./firstprivate.exe
    ```
## Cómo ejecutar sections.c

1. Abre una terminal (PowerShell) en la carpeta del proyecto.
2. Compila el archivo usando GCC con soporte OpenMP:
    ```powershell
    gcc -fopenmp sections.c -o sections.exe
    ```
3. Ejecuta el programa:
    ```powershell
    ./sections.exe
    ```
