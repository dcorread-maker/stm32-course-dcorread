/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Codex (adaptación del código base de STM32CubeIDE)
 * @brief          : Contador con fotocompuertas, GPIO del display y blinky.
 ******************************************************************************
 * Mostramos un contador de 0000 a 9999 en un display de cuatro cifras.
 * La fotocompuerta de PB7 avisa al subir su señal y solicita una suma.
 * La de PA1 avisa al bajar su señal y solicita una resta.
 * Al superar 9999 volvemos a 0000; al restar desde 0000 pasamos a 9999.
 *
 * Usamos un display de ánodo común con transistores PNP 2N3906 a 3,3 V.
 * TIM5 enciende una cifra por turno, cada 1 ms, para formar el número completo.
 * TIM10 cambia el nivel del LED de PH1 cada 250 ms como señal de actividad.
 * Configuramos los periféricos directamente con sus registros y CMSIS.
 *
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include "stm32f4xx.h"

/* Cada interrupción deja aquí un aviso para que main() lo procese.
 * Un 1 significa que hay un aviso pendiente; main() lo devuelve a 0 al tomarlo.
 * volatile indica que una interrupción puede cambiar estas variables.
 * Las banderas guardan un aviso pendiente, no la cantidad de eventos recibidos.
 */
volatile uint32_t evento_suma = 0U;  /* Aviso de la fotocompuerta conectada a PB7. */
volatile uint32_t evento_resta = 0U; /* Aviso de la fotocompuerta conectada a PA1. */

/* Aquí conservamos la cuenta que verá el usuario en el display.
 * Solo main() la lee y modifica, por eso no necesita volatile.
 * Empezamos en cero y mantenemos el valor entre 0 y 9999.
 */
uint32_t contador = 0U;

/* Separamos el número en millares, centenas, decenas y unidades, en ese orden.
 * main() prepara estas cifras y la interrupción de TIM5 las lee para mostrarlas.
 * static mantiene el arreglo dentro de este archivo; volatile conserva los
 * accesos a memoria porque se comparte con la interrupción.
 */
static volatile uint32_t cifras_display[4] = {0U, 0U, 0U, 0U};

/* Conexiones del display:
 * A = PB12, B = PA12, C = PC11, D = PC12, E = PC10, F = PA11, G = PD2.
 * D1 = PC5, D2 = PC6, D3 = PC2, D4 = PC3, de izquierda a derecha.
 * El punto decimal está sin conectar.
 * Es de ánodo común, con resistencias de 214 ohm en los segmentos.
 * Los dígitos usan PNP 2N3906, emisor a 3,3 V y resistencia de base de 1 kOhm.
 * Nivel alto apaga el segmento o el dígito; nivel bajo lo activa.
 * Un segmento se ve encendido cuando su línea y el transistor de su cifra
 * están activos al mismo tiempo.
 */

/* funciones de cabecera */
static void gpio_Init(void);
static void timers_Init(void);
static void exti_Init(void);
static void display_Escribir(uint32_t cifra);
static void display_DrawNumber(uint32_t numero);
static void display_Actualizar(void);
/*------------------------------------------------------------------*/
int main(void)
{
	/* Trabajamos con el HSI de arranque a 16 MHz.
	 * Dejamos HSE apagado para poder usar PH1 como salida del LED.
	 */
	/* Primero dejamos listos el LED, el display y las entradas de los sensores. */
	gpio_Init();
	/* Dejamos listo el patrón de cero, todavía con las cuatro cifras apagadas. */
	display_Escribir(0U);
	/* El primer número que mostraremos es el valor inicial del contador: 0000. */
	display_DrawNumber(contador);
	/* Ponemos en marcha el parpadeo del LED y el refresco del display. */
	timers_Init();
	/* Por último, habilitamos los avisos de las dos fotocompuertas. */
	exti_Init();

	/* Los timers se encargan del LED y del display mientras atendemos los avisos.
	 * Usamos dos if independientes para revisar ambas fotocompuertas en cada vuelta.
	 */
	while (1)
	{
		/* Revisamos si la fotocompuerta de PB7 dejó un aviso de suma. */
		if (evento_suma != 0U)
		{
			evento_suma = 0U; /* Tomamos el aviso y dejamos libre la bandera. */
			/* Sumamos una unidad; después de 9999 volvemos a cero. */
			if (contador < 9999U)
			{
				contador++;
			}
			else
			{
				contador = 0U;
			}
			/* Pausamos la atención de TIM5 mientras cambiamos las cuatro cifras.
			 * Así el display no toma un número a medio actualizar.
			 * El timer sigue contando y las fotocompuertas pueden seguir avisando.
			 */
			NVIC_DisableIRQ(TIM5_IRQn);
			display_DrawNumber(contador);
			NVIC_EnableIRQ(TIM5_IRQn);
		}

		/* Revisamos también si PA1 dejó un aviso de resta. */
		if (evento_resta != 0U)
		{
			evento_resta = 0U; /* Tomamos el aviso de esta fotocompuerta. */
			/* Restamos una unidad; si estamos en cero, el siguiente valor es 9999. */
			if (contador > 0U)
			{
				contador--;
			}
			else
			{
				contador = 9999U;
			}
			/* Preparamos el nuevo número con la misma protección que en la suma. */
			NVIC_DisableIRQ(TIM5_IRQn);
			display_DrawNumber(contador);
			NVIC_EnableIRQ(TIM5_IRQn);
		}
	}

	return 0;
}

/* ----------------------------------------- */
/* Dejamos cada pin preparado para la conexión que tiene en el montaje. */
static void gpio_Init(void)
{
	/* Antes de usar los puertos, habilitamos sus relojes. */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN; /* Habilitamos el reloj de GPIOH (bit 7). */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; /* GPIOA: segmentos F y B, y fotocompuerta PA1. */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; /* GPIOB: segmento A y fotocompuerta PB7. */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; /* GPIOC: segmentos E, C, D y dígitos D1 a D4. */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; /* GPIOD: segmento G. */
	(void)RCC->AHB1ENR; /* Lectura de espera antes de acceder a los puertos. */

	/* Dejamos los controles de los PNP en alto para que las cifras arranquen apagadas.
	 * Preparamos ese nivel antes de cambiar los pines a modo salida.
	 */
	GPIOC->ODR |= (GPIO_ODR_OD5 | GPIO_ODR_OD6 | GPIO_ODR_OD3 | GPIO_ODR_OD2);

	/* El LED comienza con PH1 en bajo; TIM10 se encargará de alternarlo. */
	GPIOH->ODR &= ~(GPIO_ODR_OD1); /* PH1 comienza en bajo. */

	/* Con push-pull podemos llevar PH1 tanto a nivel alto como a nivel bajo. */
	GPIOH->OTYPER &= ~GPIO_OTYPER_OT1;

	/* Elegimos velocidad media para los cambios eléctricos de PH1.
	 * El tiempo de parpadeo lo fija TIM10, no este ajuste.
	 */
	GPIOH->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED1;
	GPIOH->OSPEEDR |= GPIO_OSPEEDR_OSPEED1_0;
	/* PUPD1 = 00: sin resistencias internas de pull-up ni pull-down. */
	GPIOH->PUPDR &= ~GPIO_PUPDR_PUPD1;

	/* En MODER, 01 selecciona salida. Para PH1 usamos los bits 3:2. */
	GPIOH->MODER &= ~GPIO_MODER_MODER1;
	GPIOH->MODER |= GPIO_MODER_MODER1_0;

	/* Los segmentos también comienzan apagados, con sus salidas en alto.
	 * Solo cambiamos los bits de sus pines y conservamos los demás.
	 */
	GPIOA->ODR |= (GPIO_ODR_OD11 | GPIO_ODR_OD12); /* F y B apagados. */
	GPIOB->ODR |= (GPIO_ODR_OD12); /* A apagado. */
	GPIOC->ODR |= (GPIO_ODR_OD10 | GPIO_ODR_OD11 | GPIO_ODR_OD12); /* E, C y D apagados. */
	GPIOD->ODR |= (GPIO_ODR_OD2); /* G apagado. */

	/* PA11 maneja F y PA12 maneja B. Los configuramos juntos porque comparten puerto. */
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT11 | GPIO_OTYPER_OT12); /* 0: push-pull. */
	GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12); /* 00: baja velocidad. */
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD11 | GPIO_PUPDR_PUPD12); /* 00: sin resistencias internas. */
	GPIOA->MODER &= ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12); /* Limpiamos los campos. */
	GPIOA->MODER |= (GPIO_MODER_MODER11_0 | GPIO_MODER_MODER12_0); /* 01: salidas. */

	/* PB12 = A: configuramos únicamente este pin y conservamos los demás. */
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT12); /* 0: push-pull. */
	GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED12); /* 00: baja velocidad. */
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD12); /* 00: sin pull-up/pull-down. */
	GPIOB->MODER &= ~(GPIO_MODER_MODER12); /* Limpiamos el campo de PB12. */
	GPIOB->MODER |= (GPIO_MODER_MODER12_0); /* 01: salida. */

	/* PC10 = E, PC11 = C y PC12 = D: salidas de segmentos del puerto C. */
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT10 | GPIO_OTYPER_OT11 | GPIO_OTYPER_OT12); /* Push-pull. */
	GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11
		| GPIO_OSPEEDR_OSPEED12); /* Baja velocidad. */
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD10 | GPIO_PUPDR_PUPD11 | GPIO_PUPDR_PUPD12); /* Sin resistencias. */
	GPIOC->MODER &= ~(GPIO_MODER_MODER10 | GPIO_MODER_MODER11 | GPIO_MODER_MODER12); /* Limpiamos. */
	GPIOC->MODER |= (GPIO_MODER_MODER10_0 | GPIO_MODER_MODER11_0
		| GPIO_MODER_MODER12_0); /* 01 en cada campo: salidas. */

	/* PD2 = G: salida para la barra horizontal central. */
	GPIOD->OTYPER &= ~(GPIO_OTYPER_OT2); /* 0: push-pull. */
	GPIOD->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED2); /* 00: baja velocidad. */
	GPIOD->PUPDR &= ~(GPIO_PUPDR_PUPD2); /* 00: sin pull-up/pull-down. */
	GPIOD->MODER &= ~(GPIO_MODER_MODER2); /* Limpiamos el campo de PD2. */
	GPIOD->MODER |= (GPIO_MODER_MODER2_0); /* 01: salida. */

	/* Estas salidas van a las bases de los PNP por sus resistencias de 1 kOhm.
	 * PC5 selecciona D1, PC6 selecciona D2, PC2 selecciona D3 y PC3 selecciona D4.
	 */
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6
		| GPIO_OTYPER_OT3 | GPIO_OTYPER_OT2); /* Push-pull. */
	GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6
		| GPIO_OSPEEDR_OSPEED3 | GPIO_OSPEEDR_OSPEED2); /* Baja velocidad. */
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6
		| GPIO_PUPDR_PUPD3 | GPIO_PUPDR_PUPD2); /* Sin resistencias internas. */
	GPIOC->MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6
		| GPIO_MODER_MODER3 | GPIO_MODER_MODER2); /* Limpiamos los campos. */
	GPIOC->MODER |= (GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0
		| GPIO_MODER_MODER3_0 | GPIO_MODER_MODER2_0); /* 01: salidas, inicialmente en alto. */

	/* PB7 recibe la señal de suma. Lo dejamos como entrada sin resistencias internas,
	 * porque el montaje ya tiene una resistencia externa de 10 kOhm.
	 */
	GPIOB->MODER &= ~(GPIO_MODER_MODER7); /* PB7 como entrada. */
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD7); /* Sin pull-up ni pull-down internos. */

	/* PA1 recibe la señal de resta y también utiliza la polarización externa. */
	GPIOA->MODER &= ~(GPIO_MODER_MODER1); /* 00: PA1 como entrada. */
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD1); /* 00: sin resistencias internas. */

}

/* Formamos una sola cifra, de 0 a 9, usando los segmentos A a G.
 * En este display, alto apaga y bajo enciende cada segmento.
 * La selección de D1 a D4 se hace en display_Actualizar(): allí apagamos las
 * posiciones antes de cambiar el patrón y encendemos después la que corresponde.
 * Si recibimos un valor fuera de 0 a 9, dejamos los segmentos apagados.
 */
static void display_Escribir(uint32_t cifra)
{
	/* Apagamos los siete segmentos y conservamos los demás pines de cada puerto. */
	GPIOA->ODR |= (GPIO_ODR_OD11 | GPIO_ODR_OD12); /* F y B. */
	GPIOB->ODR |= GPIO_ODR_OD12; /* A. */
	GPIOC->ODR |= (GPIO_ODR_OD10 | GPIO_ODR_OD11 | GPIO_ODR_OD12); /* E, C y D. */
	GPIOD->ODR |= GPIO_ODR_OD2; /* G. */

	/* Cada caso enciende únicamente los segmentos necesarios para esa cifra. */
	switch (cifra)
	{
		case 0U: /* Cero: A, B, C, D, E y F; G permanece apagado. */
			GPIOA->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD10 | GPIO_ODR_OD11 | GPIO_ODR_OD12);
			break;

		case 1U: /* Uno: B y C. */
			GPIOA->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD11);
			break;

		case 2U: /* Dos: A, B, D, E y G. */
			GPIOA->ODR &= ~(GPIO_ODR_OD12);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD10 | GPIO_ODR_OD12);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		case 3U: /* Tres: A, B, C, D y G. */
			GPIOA->ODR &= ~(GPIO_ODR_OD12);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		case 4U: /* Cuatro: B, C, F y G. */
			GPIOA->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD11);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		case 5U: /* Cinco: A, C, D, F y G. */
			GPIOA->ODR &= ~(GPIO_ODR_OD11);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		case 6U: /* Seis: A, C, D, E, F y G. */
			GPIOA->ODR &= ~(GPIO_ODR_OD11);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD10 | GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		case 7U: /* Siete: A, B y C. */
			GPIOA->ODR &= ~(GPIO_ODR_OD12);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD11);
			break;

		case 8U: /* Ocho: todos los segmentos. */
			GPIOA->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD10 | GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		case 9U: /* Nueve: A, B, C, D, F y G. */
			GPIOA->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOB->ODR &= ~(GPIO_ODR_OD12);
			GPIOC->ODR &= ~(GPIO_ODR_OD11 | GPIO_ODR_OD12);
			GPIOD->ODR &= ~(GPIO_ODR_OD2);
			break;

		default: /* Un valor inválido deja todos los segmentos apagados. */
			break;
	}
}

/* Separamos el número en sus cuatro cifras. Por ejemplo, 23 queda como 0023.
 * La división entera y % 10 nos permiten obtener cada posición por separado.
 * Aquí solo preparamos los datos; TIM5 se encarga de mostrarlos después.
 * Si el número supera 9999, conservamos el que ya estaba preparado.
 * main() pausa la atención de TIM5 durante esta escritura; la primera llamada
 * se hace antes de poner en marcha los timers.
 */
static void display_DrawNumber(uint32_t numero)
{
	/* Validamos el rango antes de escribir cualquiera de las cuatro posiciones. */
	if (numero <= 9999U)
	{
		cifras_display[0] = numero / 1000U;         /* D1: millares. */
		cifras_display[1] = (numero / 100U) % 10U; /* D2: centenas. */
		cifras_display[2] = (numero / 10U) % 10U;  /* D3: decenas. */
		cifras_display[3] = numero % 10U;          /* D4: unidades. */
	}
}

/* Mostramos una cifra por llamada y seguimos el orden D1 -> D2 -> D3 -> D4.
 * TIM5 nos llama cada 1 ms, así que el recorrido completo tarda 4 ms.
 * Al empezar por D1 tomamos una copia del número y la usamos hasta terminar D4.
 * Si main() prepara otro valor durante el recorrido, lo tomamos en el siguiente.
 */
static void display_Actualizar(void)
{
	/* Recordamos qué cifra sigue y el número que estamos mostrando.
	 * static conserva estos valores para la próxima llamada.
	 */
	static uint32_t estado_display = 0U;
	static uint32_t cifras_en_uso[4] = {0U, 0U, 0U, 0U};

	/* Apagamos las cuatro cifras antes de cambiar los segmentos.
	 * Así evitamos mostrar el patrón nuevo en la posición del turno anterior.
	 */
	GPIOC->ODR |= (GPIO_ODR_OD5 | GPIO_ODR_OD6 | GPIO_ODR_OD3 | GPIO_ODR_OD2);

	switch (estado_display)
	{
		case 0U: /* D1: comienzo de un nuevo recorrido, cifra de millares. */
			cifras_en_uso[0] = cifras_display[0];
			cifras_en_uso[1] = cifras_display[1];
			cifras_en_uso[2] = cifras_display[2];
			cifras_en_uso[3] = cifras_display[3];
			display_Escribir(cifras_en_uso[0]);
			GPIOC->ODR &= ~(GPIO_ODR_OD5); /* PC5 en bajo habilita D1. */
			estado_display = 1U;
			break;

		case 1U: /* D2: cifra de centenas. */
			display_Escribir(cifras_en_uso[1]);
			GPIOC->ODR &= ~(GPIO_ODR_OD6); /* PC6 en bajo habilita D2. */
			estado_display = 2U;
			break;

		case 2U: /* D3: cifra de decenas. */
			display_Escribir(cifras_en_uso[2]);
			GPIOC->ODR &= ~(GPIO_ODR_OD2); /* PC2 en bajo habilita D3. */
			estado_display = 3U;
			break;

		case 3U: /* D4: cifra de unidades; el siguiente turno vuelve a D1. */
			display_Escribir(cifras_en_uso[3]);
			GPIOC->ODR &= ~(GPIO_ODR_OD3); /* PC3 en bajo habilita D4. */
			estado_display = 0U;
			break;

		default: /* Recuperamos el estado inicial y dejamos los dígitos apagados. */
			estado_display = 0U;
			break;
	}
}

/* Conectamos cada fotocompuerta con su línea EXTI y elegimos cuándo debe avisar.
 * PB7 utiliza EXTI7 por subida y PA1 utiliza EXTI1 por bajada.
 * Al final damos permiso a esas líneas y habilitamos su atención en el NVIC.
 */
static void exti_Init(void)
{
	/* Habilitamos el reloj de SYSCFG; EXTI no necesita un reloj separado. */
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	(void)RCC->APB2ENR; /* Espera antes de acceder a SYSCFG. */

	/* Mientras configuramos las entradas, no dejamos pasar sus solicitudes. */
	EXTI->IMR &= ~(EXTI_IMR_MR7 | EXTI_IMR_MR1);

	/* Seleccionamos el puerto B para EXTI7: de ahí recibiremos la señal de PB7.
	 * Ese campo está en EXTICR2, que en el arreglo se escribe EXTICR[1].
	 */
	SYSCFG->EXTICR[1] &= ~(SYSCFG_EXTICR2_EXTI7);
	SYSCFG->EXTICR[1] |= (SYSCFG_EXTICR2_EXTI7_PB);

	/* Para EXTI1 seleccionamos el puerto A, cuyo código es 0000.
	 * Este campo está en EXTICR1, es decir, EXTICR[0].
	 */
	SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI1);
	SYSCFG->EXTICR[0] |= (SYSCFG_EXTICR1_EXTI1_PA);

	/* Queremos un aviso por el flanco elegido, no por ambos cambios de señal.
	 * PB7 debe avisar al subir (0 -> 1) y PA1 al bajar (1 -> 0).
	 */
	EXTI->FTSR &= ~(EXTI_FTSR_TR7); /* PB7: deshabilitamos bajada. */
	EXTI->RTSR &= ~(EXTI_RTSR_TR1); /* PA1: deshabilitamos subida. */
	EXTI->RTSR |= EXTI_RTSR_TR7; /* PB7: habilitamos solo subida para sumar. */
	EXTI->FTSR |= EXTI_FTSR_TR1; /* PA1: habilitamos solo bajada para restar. */

	/* Usamos la vía de interrupciones; dejamos la vía de eventos deshabilitada. */
	EXTI->EMR &= ~(EXTI_EMR_MR7 | EXTI_EMR_MR1);

	/* Borramos posibles avisos anteriores antes de empezar.
	 * En PR se escribe 1 para limpiar el pendiente; los demás bits se conservan.
	 */
	EXTI->PR = (EXTI_PR_PR7 | EXTI_PR_PR1);

	/* Ahora sí dejamos pasar las solicitudes: un 1 en IMR da permiso a la línea. */
	EXTI->IMR |= (EXTI_IMR_MR7 | EXTI_IMR_MR1);

	/* Damos prioridad 2 a los sensores, por delante de los timers con prioridad 3. */
	NVIC_SetPriority(EXTI9_5_IRQn, 2U);
	NVIC_SetPriority(EXTI1_IRQn, 2U);
	NVIC_EnableIRQ(EXTI9_5_IRQn); /* Atiende EXTI7, compartida con EXTI5 a EXTI9. */
	NVIC_EnableIRQ(EXTI1_IRQn); /* Atiende EXTI1 de forma independiente. */
}

/* Atendemos la fotocompuerta de suma.
 * Como esta rutina se comparte con EXTI5 a EXTI9, primero comprobamos PR7.
 */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & EXTI_PR_PR7)
	{
		/* Reconocemos el aviso escribiendo 1 solo en PR7.
		 * Usamos = para no borrar otros pendientes que podrían aparecer al leer PR.
		 */
		EXTI->PR = EXTI_PR_PR7;
		evento_suma = 1U; /* Dejamos el aviso de suma; main() se encarga de procesarlo. */
	}
}

/* Atendemos la fotocompuerta de resta y dejamos su aviso para main(). */
void EXTI1_IRQHandler(void)
{
	if (EXTI->PR & EXTI_PR_PR1)
	{
		EXTI->PR = EXTI_PR_PR1; /* Borra únicamente el pendiente de EXTI1. */
		evento_resta = 1U; /* main() revisará esta bandera en su próxima vuelta. */
	}
}

/* Preparamos dos ritmos independientes: el del LED y el del display. */
static void timers_Init(void)
{
	/* TIM10 está conectado al bus APB2 y necesita su reloj habilitado. */
	RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
	(void)RCC->APB2ENR;

	/* Dejamos TIM10 en sus valores de arranque antes de configurarlo. */
	RCC->APB2RSTR |= RCC_APB2RSTR_TIM10RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM10RST;

	TIM10->CNT = 0U; /* Inicia el contador desde cero, todavía detenido. */

	/* TIM10 recibe 16 MHz con la configuración de reloj que dejamos al arrancar.
	 * Dividimos entre 1600 para que avance a 10 000 Hz: un paso cada 0,1 ms.
	 * El divisor es PSC + 1, por eso guardamos 1599.
	 */
	TIM10->PSC = (1600U - 1U);

	/* Necesitamos 2500 pasos de 0,1 ms para esperar 250 ms.
	 * Como contamos desde cero hasta ARR, usamos ARR = 2499.
	 * Periodo = (PSC + 1) * (ARR + 1) / reloj del timer.
	 * Aquí resulta: 1600 * 2500 / 16 000 000 = 0,25 s.
	 */
	TIM10->ARR = (2500U - 1U); /* 2499: 2500 pasos por periodo. */

	/* Dejamos sin división el reloj de muestreo de los filtros digitales.
	 * CKD no cambia el periodo que acabamos de calcular con PSC y ARR.
	 */
	TIM10->CR1 &= ~TIM_CR1_CKD; /* Limpia el campo CKD. */

	/* Activamos la precarga de ARR: un cambio de periodo se aplicaría
	 * al cargar la nueva configuración en un evento de update.
	 */
	TIM10->CR1 |= TIM_CR1_ARPE; /* Habilita la precarga de ARR. */

	/* TIM10 solo cuenta en sentido ascendente; no tiene un bit DIR configurable. */

	/* Cargamos PSC y ARR antes de arrancar para que el primer intervalo
	 * también dure los 250 ms calculados.
	 */
	TIM10->EGR = TIM_EGR_UG; /* Carga PSC y ARR, y reinicia el contador. */
	TIM10->SR = 0U; /* Descarta las banderas generadas durante la configuración. */
	TIM10->DIER |= TIM_DIER_UIE; /* Habilita la interrupción por update. */

	/* TIM10 comparte su rutina con el update de TIM1; aquí solo usamos TIM10. */

	NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 3U);
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

	TIM10->CR1 |= TIM_CR1_CEN; /* CEN = 1: arranca el contador. */

	/* Ahora preparamos TIM5 para cambiar de cifra cada 1 ms. Su reloj viene de APB1. */
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
	(void)RCC->APB1ENR; /* Esperamos la propagación del reloj. */

	/* Partimos de sus valores de arranque, con el contador detenido. */
	RCC->APB1RSTR |= RCC_APB1RSTR_TIM5RST;
	RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM5RST);

	/* ARPE = 1: usamos la precarga del registro ARR. */
	TIM5->CR1 |= TIM_CR1_ARPE;

	/* TIM5 también recibe 16 MHz. Con PSC = 1599 avanza cada 0,1 ms.
	 * ARR = 9 completa diez pasos y genera un update cada 1 ms.
	 * Frecuencia = 16 000 000 / (1600 * 10) = 1000 Hz.
	 * Como son cuatro cifras, cada posición vuelve a encenderse cada 4 ms (250 Hz).
	 */
	TIM5->PSC = (1600U - 1U);
	TIM5->ARR = (10U - 1U);
	TIM5->CNT = 0U; /* Comenzamos desde cero. */

	/* Cargamos la configuración y borramos los pendientes de inicialización
	 * antes de permitir las interrupciones de TIM5.
	 */
	TIM5->EGR = TIM_EGR_UG;
	TIM5->SR = 0U;

	/* Permitimos que el update de TIM5 solicite atención al procesador. */
	TIM5->DIER |= TIM_DIER_UIE;
	NVIC_SetPriority(TIM5_IRQn, 3U); /* Las fotocompuertas tienen prioridad 2. */
	NVIC_EnableIRQ(TIM5_IRQn);

	/* Con todo listo, ponemos en marcha el contador de TIM5. */
	TIM5->CR1 |= TIM_CR1_CEN;
}

/* Cada 1 ms atendemos TIM5 y mostramos la siguiente cifra del display.
 * El nombre de la función coincide con el de la tabla de vectores del arranque.
 */
void TIM5_IRQHandler(void)
{
	if (TIM5->SR & TIM_SR_UIF)
	{
		/* Reconocemos el update sin detener el timer.
		 * En SR, escribir 0 borra UIF y escribir 1 conserva las otras banderas.
		 */
		TIM5->SR = ~TIM_SR_UIF;

		display_Actualizar(); /* Apaga dígitos, prepara segmentos y habilita el siguiente. */
	}
}

/* TIM10 nos avisa cada 250 ms para cambiar el estado del LED.
 * El procesador entra aquí al atender su interrupción, sin una llamada de main().
 * El nombre incluye TIM1 porque el vector es compartido, aunque solo usamos TIM10.
 */

void TIM1_UP_TIM10_IRQHandler(void)
{
	/* Comprobamos UIF para saber si TIM10 completó su periodo.
	 * El operador & consulta esa bandera sin cambiar el registro.
	 */
	if (TIM10->SR & TIM_SR_UIF)
	{
		/* Borramos UIF escribiendo 0 en ese bit y conservamos las demás banderas.
		 * Si no lo limpiamos, la solicitud puede seguir activa al salir de la rutina.
		 * El timer continúa contando y volverá a avisar en el siguiente periodo.
		 */
		TIM10->SR = ~TIM_SR_UIF;

		/* Alternamos solo PH1: si estaba en 0 pasa a 1, y si estaba en 1 pasa a 0.
		 * Así el LED permanece 250 ms en cada estado y completa un ciclo en 500 ms.
		 */
		GPIOH->ODR ^= GPIO_ODR_OD1;
	}
}
