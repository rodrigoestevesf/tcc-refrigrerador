#include "header.h"

void setup() {

  pinMode(19, OUTPUT);
  pinMode(LDR_pin,INPUT);
  
  iniciarConexoes();
}

void loop() {
  
  mantemConexoes();
  controleAbertura();

  if (esperar(2000, "display")) Ntela += 1;

  // Para cada Ntela referente se mostra uma tela
  if (Ntela == 0) exibirValores(telas[Ntela], temperatura(0));
  else if(Ntela == 1) exibirValores(telas[Ntela], temperatura(1));
  else if(Ntela == 2) exibirValores(telas[Ntela], temperatura(2));
  else if(Ntela == 3) exibirValores(telas[Ntela], (float)corrente());
  else if(Ntela == 4) exibirValores(telas[Ntela], Potencia);
  else if(Ntela == 5) exibirValores(telas[Ntela], Valor);
  else Ntela = 0;
  
  if (esperar(1000, "MQTT")) {

    enviarTemp("Geladeira");
    enviarTemp("Freezer");
    enviarTemp("Motor");

    enviarCorr(corrente());
    enviarValor(Valor);
    enviarPot(Potencia);
    enviarLDR();

  }

  MQTT.loop();
}
