/* UNIVERSIDADE FEDERAL DE UBERLANDIA
   Biomedical Engineering
   Autors: Ítalo G S Fernandes
   contact: italogsfernandes@gmail.com
   URLs: https://github.com/italogfernandes/SEB
  Este codigo realiza a leitura de um sensor de pulso e um sensor inercial.
  Envia para a serial os seguintes pacotes:
      Accel: -#.## -#.## -#.##\n
      Heart: ##.##\n
  O pacote de aceleração é enviado em G (gravidade), sendo 1g = 9.8m/s^2. Sendo enviado na mesma
  frequência de aquisição.
  O pacote heart é o valor de bpm processado pelo arduino. Sendo enviado a cada atualização do valor.
  Para desativar algum destes, comente as funções chamadas em doAquire()
  Caso o modo DEBUG_CARDIO esteja ativado
  Sera enviado uma string de informações que pode ser visualizada no serial plotter. [ctrl] + [shift] + [L]
  
  Conectando:
  Dispositivo - Arduino
  Alimentação (7V até 30V) - Vin
  Alimentação Ground  - GND

  Sensor de Pulso '+'   - 5V ou 3.3V - funciona em ambos
  Sensor de Pulso '-'   - GND
  Sensor de Pulso 'S'   - A0

  GY521 VCC             - 5V preferivel, mas funciona com 3.3V se necessario
  GY521 GND             - GND
  GY521 SCL             - A5
  GY521 SDA             - A4

  HC-06 VCC*             - 5V
  HC-06 GND*             - GND
  HC-06 TX*              - RX0**
  HC-06 RX*             - TX0** (Necessario conversor de tensão de 5V para 3.3V)

  *Obs1: Não é possível enviar códigos e utilizar o monitor serial se o módulo bluetooth estiver conectado
  **Obs2: Para liberar a porta serial para gravação, é possível utilizar o módulo bluetooth em pinos diferentes
          atraves de uma porta serial por Software consulte SoftwareSerial.h para mais informações.
*/
#include<Wire.h>
#include<Timer.h>

//#define DEBUG_CARDIO
#define UART_BAUD 115200 //TODO: Ajustar para a baudrate do bluetooth 
#define AquirePeriod 5 //TODO: Ajustar para a frequencia de aquisição so sinal desejada
const float limiar_batida = 3.3; //Quando acima de limiar_batida V considera como uma batida, para ajustar visualize a onnda descomentando o DEBUG_CARDIO

//Heart sensor:
float data_pulse; //Valor em volts da leitura do sensor
float bpm; //Valor da qnt de batidas por minuto
bool is_above_limiar;
bool was_above_limiar;
bool new_data_ready;
unsigned long time_last_beat;
unsigned long time_actual_beat;
unsigned long delta_time_beat;

//Accelerometer sensor
//Register Address:
#define MPU6050_ADDRESS     0x68 // address pin low (GND), default for InvenSense evaluation board
#define MPU6050_RA_PWR_MGMT_1       0x6B
#define MPU6050_RA_ACCEL_XOUT_H     0x3B
float data_accel[3];

//Aquirer Control
Timer t;

void setup() {
  //Configura Porta Serial:
  Serial.begin(UART_BAUD);

  //Configure Accelerometro:
  Wire.begin();
  setupAccel();

  //Inicia Aquisição
  t.every(AquirePeriod, doAquire);//10ms = 100Hz
}

void loop() {
  t.update();
}

void doAquire() {
  doAquireCardio();
#ifndef DEBUG_CARDIO
  doAquireAccel();
  doSendData();
#endif
}

void setupAccel() {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(MPU6050_RA_PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission(true);

}

void doAquireCardio() {
  data_pulse = analogRead(A0) * 5.0 / 1024.0;
  is_above_limiar = data_pulse > limiar_batida;
  if (is_above_limiar && !was_above_limiar) {
    time_actual_beat = micros();
    delta_time_beat = time_actual_beat - time_last_beat;
    bpm =  60 * (1.0 / (delta_time_beat / 1000000.0));
    time_last_beat = time_actual_beat;
    new_data_ready = true;
  }
  was_above_limiar = is_above_limiar;
#ifdef DEBUG_CARDIO
  Serial.println("Heart: " + String(data_pulse * 20, 2) + " " + String(bpm) + " "
                 + String(is_above_limiar * 100) + " " + String(limiar_batida * 20));
#endif

}

void doAquireAccel() {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(MPU6050_RA_ACCEL_XOUT_H);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);

  Wire.requestFrom(MPU6050_ADDRESS, 6, true); //Solicita os 6 registradores do sensor
  //Armazena o valor dos sensores nas variaveis correspondentes
  data_accel[0] = (float) (Wire.read() << 8 | Wire.read()) / 16384.0; //0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  data_accel[1] = (float) (Wire.read() << 8 | Wire.read()) / 16384.0; //0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  data_accel[2] = (float) (Wire.read() << 8 | Wire.read()) / 16384.0; //0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
}

void doSendData() {
  //Medidas do acelerometro:
  Serial.println("Accel: " + String(data_accel[0]) + " " + String(data_accel[1]) + " " +  String(data_accel[2]));
  //Se tiver uma nova medida de batimento cardiaco, então ela sera enviada
  if (new_data_ready) {
    Serial.println("Heart: " + String(bpm));
    new_data_ready = false;
  }
}
