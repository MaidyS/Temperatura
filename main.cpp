/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "TM1638.h"            // Librería para el módulo TM1638
#include "Adafruit_SSD1306.h"  // Librería para el display OLED 1306
#include "AHT20.h"             // Librería para el sensor I2C de temperatura/humedad AHT20

I2C i2c(PB_9, PB_8);  // Pines SDA, SCL del I2C en la Nucleo F401RE
AHT20 aht20(&i2c);    // Inicialización del sensor AHT20

AnalogIn termistor(A0);  // El termistor está conectado al pin A0
float Rfijo = 10000.0;   // Resistencia fija de 10K

float leer_temperatura_aht20() {
    float temperatura = aht20.readTemperature(); // Lee la temperatura desde el sensor AHT20
    return temperatura;
}

float leer_temperatura_termistor() {
    float Vout = termistor.read();  // Lee el valor analógico (0 a 1)
    float Rtermistor = Rfijo * (1.0f / Vout - 1.0f);

    // Convertir R a temperatura
    float T0 = 298.15;  // 25°C en Kelvin
    float R0 = 10000;   // Resistencia a 25°C
    float beta = 3950;  // Valor típico de Beta
    float temp_kelvin = 1.0f / (1.0f / T0 + log(Rtermistor / R0) / beta);

    return temp_kelvin - 273.15;  // Convertimos a °C
}

#define NUM_LECTURAS 10

float calcular_promedio(float lecturas[], int size) {
    float suma = 0;
    for (int i = 0; i < size; i++) {
        suma += lecturas[i]; // Suma todas las lecturas
    }
    return suma / size; // Retorna el promedio
}

void leer_todas_las_temperaturas(float lecturas_aht[], float lecturas_ntc[], int size) {
    for (int i = 0; i < size; i++) {
        lecturas_aht[i] = leer_temperatura_aht20(); // Lee la temperatura del AHT20
        lecturas_ntc[i] = leer_temperatura_termistor(); // Lee la temperatura del termistor
        ThisThread::sleep_for(500ms);  // Pausa entre lecturas
    }
}

void ordenar_burbuja(float arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) { // Compara dos elementos
                float temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp; // Intercambia si están en el orden incorrecto
            }
        }
    }
}

float calcular_error_absoluto(float t1, float t2) {
    return fabs(t1 - t2); // Calcula la diferencia absoluta entre t1 y t2
}

float calcular_error_relativo(float t1, float t2) {
    return calcular_error_absoluto(t1, t2) / t2 * 100.0f; // Calcula el error relativo
}

TM1638 tm1638(D0, D1, D2);  // Pines de datos, reloj y STB

void mostrar_tm1638(float promedio) {
    tm1638.displayInt((int)promedio); // Muestra el valor del promedio en el TM1638
}

Adafruit_SSD1306_I2c oled(&i2c, NC, 0x78);

void mostrar_oled(float promedio, float error_abs, float error_rel) {
    oled.clearDisplay(); // Limpia la pantalla
    oled.setTextCursor(0, 0); // Establece
    oled.printf("Prom: %.2f C\n", promedio); // Muestra el promedio en grados Celsius
    oled.printf("Error Abs: %.2f\n", error_abs); // Muestra el error absoluto
    oled.printf("Error Rel: %.2f %%\n", error_rel); // Muestra el error relativo
    oled.display(); // Actualiza la pantalla para mostrar los cambios
}

UnbufferedSerial pc(USBTX, USBRX, 9600);

void enviar_serial(float promedio, float error_abs, float error_rel) {
    printf("Promedio: %.2f\n", promedio);
    printf("Error Absoluto: %.2f\n", error_abs);
    printf("Error Relativo: %.2f %%\n", error_rel);
}

int main() {
    float lecturas_aht[NUM_LECTURAS];
    float lecturas_ntc[NUM_LECTURAS];

    // Leer temperaturas y calcular promedio
    leer_todas_las_temperaturas(lecturas_aht, lecturas_ntc, NUM_LECTURAS);
    float promedio_aht = calcular_promedio(lecturas_aht, NUM_LECTURAS);
    float promedio_ntc = calcular_promedio(lecturas_ntc, NUM_LECTURAS);

    // Calcular errores
    float error_abs = calcular_error_absoluto(promedio_aht, promedio_ntc);
    float error_rel = calcular_error_relativo(promedio_aht, promedio_ntc);

    // Mostrar en pantallas
    mostrar_tm1638(promedio_aht);
    mostrar_oled(promedio_aht, error_abs, error_rel);

    // Enviar por puerto serial
    enviar_serial(promedio_aht, error_abs, error_rel);
}



