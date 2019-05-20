#include <stdio.h>
#include <stdlib.h>  // para el NULL
#include <wiringPi.h>
#include "fsm.h"
#include "tmr.h"
#include <string.h>
#include <time.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>
#include <math.h>
#include <sys/time.h>
#include "fsm.h"
#include "tmr.h"

#include <assert.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>


#define CLK_MS 10

#define MRES 10 //máximo numero de respuestas
#define MFALLOS 3 //maximo numero de fallos
#define TPEN 3000 //tiempo de penalizacion al fallar
#define TIMEOUT0 5000 //tiempo maximo de respuesta
#define DEBOUNCE_TIME 300 //tiempo de rebote de los pulsadores


#define BTN_START	19		// El pin 8 pulsador de start.
#define BTN_1	20			// El pin 14 pulsador del led 1.
#define BTN_2	21			// El pin 15 pulsador del led 2.
#define BTN_3	26			// El pin 16 pulsador del led 3.
#define BTN_4	27			// El pin 17 pulsador del led 4.
#define GPIO_LIGHT_START	17	// El pin 16 boton start.
#define GPIO_LIGHT_1	00		// El pin 1 led 1.
#define GPIO_LIGHT_2	01		// El pin 2 led 2.
#define GPIO_LIGHT_3	02		// El pin 3 led 3.
#define GPIO_LIGHT_4	03		// El pin 4 led 4.

#define FLAG_BOTON_START	0x01	//Flag del boton del pulsador on/off.
#define FLAG_BOTON_1		0x02	//Flag del boton del primer pulsador.
#define FLAG_BOTON_2		0x04	//Flag del boton del segundo pulsador.
#define FLAG_BOTON_3		0x08	//Flag del boton del tercer pulsador.
#define FLAG_BOTON_4		0x10	//Flag del boton del cuarto pulsador.
#define FLAG_TIMER		0x20	//Flag del timer


enum fsm_state {
  s1, s2, s3		//s1 pantalla de entrada donde se espera a empezar
			//s2 estado del juego
			//s3 fin del juego por finalización
};
int pulsado=0;
int encendido=0;		//vble de led encendido
int flags = 0;		//variable de flags
int debounceTime=0;	//

int fallos=0;		//variable para el número de fallos
int respuestas=0;	//vble para el número de respuestas


int time1=0;		//variable 1 para hacer la diferencia de tiempos y sacar el tiempo de respuesta
int time2=0;		//variable 2 para hacer la diferencia de tiempos y sacar el tiempo de respuesta
int tt=0;		//vble de acumulación de tiempos
int tm=0;		//variable de tiempo medio
int dt=0;		//vble tiempo de respuesta
int tmax=0;		//vble para almacenar el valor maximo de tiempo
int tmin=0;		//vble para almacenar el valor maximo de tiempo


/***Modulos isr para captar interrupciones de botones
***/

void boton_start (void) {
	if(millis()< debounceTime){					//modulo antirrebotes
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
		}
	flags |= FLAG_BOTON_START;					
	//printf("botonstart\n"); 					//trazas
	fflush(stdout);
	debounceTime = millis() + DEBOUNCE_TIME;
}
void boton_1 (void) {
	if(millis()< debounceTime){					//modulo antirrebotes
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}
	flags |= FLAG_BOTON_1;					
	//printf("boton1\n"); 					//trazas
	fflush(stdout);
	debounceTime = millis() + DEBOUNCE_TIME;
	}
void boton_2 (void) {if(millis()< debounceTime){					//modulo antirrebotes
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}
	flags |= FLAG_BOTON_2;					
	//printf("boton2\n"); 					//trazas
	fflush(stdout);
	debounceTime = millis() + DEBOUNCE_TIME;
}
void boton_3 (void) {
	if(millis()< debounceTime){					//modulo antirrebotes
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}
	flags |= FLAG_BOTON_3;					
	//printf("boton3\n"); 					//trazas
	fflush(stdout);
	debounceTime = millis() + DEBOUNCE_TIME;
}
void boton_4 (void) {
	if(millis()< debounceTime){					//modulo antirrebotes
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}
	flags |= FLAG_BOTON_4;					
	//printf("boton4\n"); 					//trazas
	fflush(stdout);
	debounceTime = millis() + DEBOUNCE_TIME;
}

/***Modulo para interrupciones de temporizador
***/


void timer_isr (union sigval value) {flags |= FLAG_TIMER; }

/***Evento de confirmacion de pulsación del boton START
***/

int EVENT_BTN_START_END (fsm_t* this) {
	return (flags & FLAG_BOTON_START); }

/***Evento de respuesta correcta
***/

int EVENT_BTN_OK (fsm_t* this) {
	int c=0;
		if((flags&FLAG_BOTON_1)&&(encendido==1)){
			c=1;
			printf("Boton 1 OK\n");
		}
		else if((flags&FLAG_BOTON_4)&&(encendido==4)){
			c= 1;
			printf("Boton 4 OK\n");
		}
		else if((flags&FLAG_BOTON_2)&&(encendido==2)){
			c= 1;
			printf("Boton 2 OK\n");
		}
		else if((flags&FLAG_BOTON_3)&&(encendido==3)){
			c= 1;
			printf("Boton 3 OK\n");
		}
		else{
			c=0;
		}
		return c;
}

/*** evento de fallo en el juego.
**** el juego puede fallar por pulsar erroneamente o por que haya pasado el tiempo de respuesta. 
****/

int EVENT_BTN_FAIL (fsm_t* this){
	if((flags&FLAG_BOTON_START)){
		printf("fallo por pulsar START\n");
		fflush(stdout);
		return 1;
	}
	if((flags&FLAG_BOTON_1)){
	if(encendido!=1){
		printf("fallo por pulsar 1 en ver de %d\n", encendido);
		fflush(stdout);
		return 1;
		}
	}
	else if(flags&FLAG_BOTON_4){
	if(encendido!=4){
		printf("fallo por pulsar 4 en ver de %d\n", encendido);
		fflush(stdout);
		return 1;
		}
	}
	else if((flags&FLAG_BOTON_2)){
	if(encendido!=2){
		printf("fallo por pulsar 2 en ver de %d\n", encendido);
		fflush(stdout);
		return 1;
		}
	}
	else if((flags&FLAG_BOTON_3)){
		if(encendido!=3){
			printf("fallo por pulsar 3 en ver de %d\n", encendido);
			fflush(stdout);
			return 1;
		}
	}
	else if (flags & FLAG_TIMER) {
		printf("fallo por tiempo\n");
		fflush(stdout);
		return 1;
	}
	return 0;
}

/*** Evento de final del juego.
***Salta si se han superado el número permitido de fallos o el numero max de respuestas
***/

int EVENT_END_GAME (fsm_t* this) {
	if(fallos >= MFALLOS||respuestas>=MRES){
		return 1;
	}
	else{
		return 0;
	}
}


/*** Sub rutina s1e1 
**** Inicializa todas las variables y las pone a cero para poderusarlas sin problemas.
**** Ademas prepara la primera partida del juego.
**** Por ultimo toma el dato del tiempo para calcular los tiempos
***/

void s1e1   (fsm_t* this) {
	flags=0;     		//inicializacion
	tt=0;		
	tmax=0;
	tmin=0;
	fallos=0;
	respuestas=0;

	dt=0;
	tm=0;
	time1=0;
	time2=0;

	encendido = (rand () % 4)+1;		//eleccion aleatoria de led

	digitalWrite (GPIO_LIGHT_START, 0) ;
	switch(encendido) {
	   	case 1  :
			digitalWrite (GPIO_LIGHT_1, 1);
			break; /* optional */

	   	case 2  :
		  	digitalWrite (GPIO_LIGHT_2, 1);
			break;
		case 3  :
			digitalWrite (GPIO_LIGHT_3, 1);
	   		break;
	   	case 4  :
		   	digitalWrite (GPIO_LIGHT_4, 1);
	   	   	break;
	   	default :
		   	break;
	}
	tmr_t* tmr = tmr_new (timer_isr); 		//lanzador de cuenta
	this->user_data = tmr;				
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT0);

	time1=millis();				//tomamos momento 1
	printf("EMPIEZA");
}

/*** Subrutina S2E2.
**** suma una respuesta, calcula el tiempo de erspuesta, y lo compara al maximo y minimo sustituyendolos si es preciso, tambien lo suma a la cuenta total de juego
**** Ademas lanza una partida más si pocede y lanza un contador más
***/

void s2e2 (fsm_t* this) {
	flags=0;
	respuestas++;
	tmr_destroy((tmr_t*)(this->user_data));

	printf("correcto. respuestas: %d fallos: %d \n", respuestas, fallos); //traza de acierto
	fflush(stdout);

	time2=millis(); 	//tiempo dos para calcular el tiempo de respuesta

	dt=time2-time1;

	if(dt>tmax){		//comparacion de tiempos con las estadísticas
		tmax=dt;
	}
	if(respuestas<2){	//condicion para qeu 'tmin' no valga siempre 0
	tmin=dt;
	}
	if(dt<tmin){
		tmin=dt;
	}
	tt+=dt;			//suma a tiempo total
	if((respuestas< 10)){	//en el ultimo momento se mete aun habiendo respodido a 10, pero asi evitamos que parpadee el led durante un momento

	encendido = (rand () % 4)+1;
	digitalWrite (GPIO_LIGHT_START, 0) ;
	digitalWrite (GPIO_LIGHT_3, 0) ;
	digitalWrite (GPIO_LIGHT_2, 0) ;
	digitalWrite (GPIO_LIGHT_1, 0) ;
	digitalWrite (GPIO_LIGHT_4, 0) ;
	switch(encendido) {
		case 1  :
			digitalWrite (GPIO_LIGHT_1, 1);
		    break; /* optional */
		case 2  :
			digitalWrite (GPIO_LIGHT_2, 1);
		    break;
		case 3  :
			digitalWrite (GPIO_LIGHT_3, 1);
		   	break;
		case 4  :
			digitalWrite (GPIO_LIGHT_4, 1);
		   	break;
		default :
			break;
		}
	time1=millis();
	}
	tmr_t* tmr = tmr_new (timer_isr);		//Lanzamos una cuenta
	this->user_data = tmr;
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT0);
}

/*** Subrutina S2E3.
**** subrutina del fallo. suma una respuesta y un fallo.
**** si fallos o respuestas es menor al establecido, lanza otra ronday una marca de tiempo
**** Actualiza los tiempos de igual manera que s2e3
***/

void s2e3 (fsm_t* this) {
	flags=0;
	time2=millis();
	dt=time2-time1+TPEN;	//tiempo de respuesta con penalizacion
	tmr_destroy((tmr_t*)(this->user_data));
	respuestas++;
	fallos++;
	if(dt>tmax){		//acctualizacion de estadísticas de tiempo
			tmax=dt;
	}
	if(respuestas<2){	//condicion para que 'tmin' no valga siempre 0
		tmin=dt;
	}
	if(dt<tmin){
			tmin=dt;
	}
	tt+=dt;
	printf("incorrecto. respuestas: %d fallos: %d\n", respuestas, fallos); //traza de fallo

	if((respuestas<MRES)||fallos<MFALLOS){ //cuando terminamos partida, durante un momento veulve a esta subrutina, pero asi evitamos que lance una pregunta más y vara al final directamente.

	encendido = (rand () % 4)+1;
	digitalWrite (GPIO_LIGHT_START, 0) ;
	digitalWrite (GPIO_LIGHT_3, 0) ;
	digitalWrite (GPIO_LIGHT_2, 0) ;
	digitalWrite (GPIO_LIGHT_1, 0) ;
	digitalWrite (GPIO_LIGHT_4, 0) ;
	switch(encendido) {
		case 1  :
			digitalWrite (GPIO_LIGHT_1, 1);
		    break; /* optional */
		case 2  :
			digitalWrite (GPIO_LIGHT_2, 1);
		    break;
		case 3  :
			digitalWrite (GPIO_LIGHT_3, 1);
		   	break;
		case 4  :
			digitalWrite (GPIO_LIGHT_4, 1);
		   	break;
		default :
			break;
	}
	time1=millis();
	}
	tmr_t* tmr = tmr_new (timer_isr);
		this->user_data = tmr;
		tmr_startms((tmr_t*)(this->user_data), TIMEOUT0);
}

/*** Subrutina S2E4
**** Apaga todos los LEDs de juego y enciende el de inicio
**** No es necesario administrar tiempos porque directamente pasamos por el estado de pregunta una ultima vez comporbando todo
***/

void s2e4   (fsm_t* this) {  
	tmr_destroy((tmr_t*)(this->user_data));
	flags=0;
	digitalWrite(GPIO_LIGHT_1, 0);
	digitalWrite(GPIO_LIGHT_2, 0);
	digitalWrite(GPIO_LIGHT_3, 0);
	digitalWrite(GPIO_LIGHT_4, 0);
	digitalWrite (GPIO_LIGHT_START, 1);
}

/*** Subrutina S3E1
**** Haya el tiempo medio y presenta el resto de estadisticas, deja el juego en estado de presentacion con el LED de comienzo 
***/

void s3e1   (fsm_t* this) {
	flags=0;
	tm=tt/respuestas;
	printf("Tiempo medio: %dms \nTiempo máximo: %dms \nTiempo minimo: %dms \nFallos: %d \n Rondas: %d \n" ,tm, tmax,tmin, fallos, respuestas);
	fflush(stdout);
	delay(500);
}

void delay_until (unsigned int next){
  unsigned int now = millis();
  if (next > now) {
	  delay (next - now);
  }
}

int main (){
	tmr_t* tmr = tmr_new (timer_isr);

	// Máquina de estados: lista de transiciones
	// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
	fsm_trans_t interruptor_tmr[] = {
		{s1,   EVENT_BTN_START_END, s2, s1e1 },
		{ s2,   EVENT_BTN_OK, s2, s2e2 },
		{ s2,   EVENT_BTN_FAIL, s2, s2e3 },
		{ s2,   EVENT_END_GAME, s3, s2e4 },
		{s3,  EVENT_BTN_START_END, s1, s3e1 },
		{-1, NULL, -1, NULL },
	};

	fsm_t* interruptor_tmr_fsm = fsm_new (s1, interruptor_tmr, tmr);
	unsigned int next;

	wiringPiSetupGpio();
	pinMode (BTN_4, INPUT); //definicion de entradas
	pinMode (BTN_3, INPUT);
	pinMode (BTN_2, INPUT);
	pinMode (BTN_1, INPUT);
	pinMode (BTN_START, INPUT);

	pinMode (GPIO_LIGHT_START, OUTPUT); 	//definicion de salidas
	pinMode (GPIO_LIGHT_4, OUTPUT);
	pinMode (GPIO_LIGHT_3, OUTPUT);
	pinMode (GPIO_LIGHT_2, OUTPUT);
	pinMode (GPIO_LIGHT_1, OUTPUT);

	wiringPiISR (BTN_START, INT_EDGE_FALLING, boton_start); //creador de threads de los pulsadores
	wiringPiISR (BTN_1, INT_EDGE_FALLING, boton_1);
	wiringPiISR (BTN_2, INT_EDGE_FALLING, boton_2);
	wiringPiISR (BTN_3, INT_EDGE_FALLING, boton_3);
	wiringPiISR (BTN_4, INT_EDGE_FALLING, boton_4);
	digitalWrite (GPIO_LIGHT_1, 0);		//situacion de comienzo en cunato a LEDs
	digitalWrite (GPIO_LIGHT_2, 0);
	digitalWrite (GPIO_LIGHT_3, 0);
	digitalWrite (GPIO_LIGHT_4, 0);
	digitalWrite (GPIO_LIGHT_START, 1);

	next = millis();
	while (1) {
		fsm_fire (interruptor_tmr_fsm);
		next += CLK_MS;
		delay_until (next);
		}

	tmr_destroy ((tmr_t*)(interruptor_tmr_fsm->user_data));
	fsm_destroy (interruptor_tmr_fsm);
}
