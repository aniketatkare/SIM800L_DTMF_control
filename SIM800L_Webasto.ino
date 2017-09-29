/* Версия прошивки для вебасто
заменить номер +375000000001 на свой для управления по DTMF
заменить номер +375000000002 на свой если хотите включать глухарем
заменить точку доступа internet.mts.by на ТД вашего сотового оператора
заменить #SI-M8-00-12-34-58#... на свои придуманные циферки 12-74-12-34-77-D1  (пример)
  */
#include <SoftwareSerial.h>
SoftwareSerial SIM800(8, 7); // RX, TX
#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#define ONE_WIRE_BUS 9 // и настраиваем  пин 9 как шину подключения датчиков DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

#define WEBASTO_Pin 12  // на реле питания вебастЫ, через транзистор с 12-го пина ардуино
#define BAT_Pin A3      // на батарею, через делитель напряжения 39кОм / 11 кОм
#define STOP_Pin A1     // на концевик педали тормоза для отключения режима прогрева
#define RESET_Pin 6     // пин перезагрузки

float Temp0, Temp1, Temp2 ;  // переменные хранения температуры
int k = 0;
int poz = 0;            // позиция в массиве пинкода
int pin[20];             // сам массив набираемого пинкода
int interval = 5;      // интервал отправки данных на народмон 
String at = "";
unsigned long Time1 = 0; 
int Timer = 0;     // таймер времени прогрева двигателя по умолчанию = 0
int modem =0;            // переменная состояние модема 
bool heating = false;    // переменная состояния режим прогрева двигателя
bool SMS_send = false;   // флаг разовой отправки СМС
bool n_send = false;      // отправка данных на народмон по умолчанию отключена
bool sms_report = false;  // отправка SMS отчета по умолчанию отключена
float Vbat;              // переменная хранящая напряжение бортовой сети
float m = 66.91;         // делитель для перевода АЦП в вольты для резистров 39/11kOm

void setup() {
  pinMode(WEBASTO_Pin, OUTPUT);  // указываем пин на выход (светодиод)
  pinMode(STOP_Pin, INPUT);  // указываем пин на выход 
  Serial.begin(9600);  //скорость порта
  SIM800.begin(9600);
  Serial.println("Webasto SIM800+narodmon.ru. 1.0 09/07/2017"); 
  SIM800_reset();
             }

void loop() {
  if (SIM800.available()) { // если что-то пришло от SIM800 в направлении Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k),delay(1); // набиваем в переменную at
    
           if (at.indexOf("+CLIP: \"+375000000001\",") > -1){ // реакция на входящий звонок с номера 1
      delay(50), SIM800.println("ATA"), pin[0]=0, pin[1]=0,pin[2]=0, poz=0, modem=10;
      delay(50),SIM800.println("AT+VTS=\"3,5,7\""), Serial.println("Incoming call 1");

    } else if (at.indexOf("+CLIP: \"+375000000002\",") > -1){ // реакция на входящий звонок с номера 2
    if (heating == false){ // если вебаста выключена то включаем ее сразу
                  delay(50), SIM800.println("ATH0"), Timer = 60, webastoON();
                         } else { // если включена то выключаем 
                  delay(50), SIM800.println("ATH0"), webastoOFF();
                                }
            
    } else if (at.indexOf("NO CARRIER") > -1){modem=0;
    } else if (at.indexOf("+CME ERROR:") > -1){SIM800_reset();
    } else if (at.indexOf("+DTMF: 1") > -1) {pin[poz]=1, poz++; 
    } else if (at.indexOf("+DTMF: 2") > -1) {pin[poz]=2, poz++; 
    } else if (at.indexOf("+DTMF: 3") > -1) {pin[poz]=3, poz++; 
//  } else if (at.indexOf("+DTMF: 4") > -1) {pin[poz]=4, poz++;
//  } else if (at.indexOf("+DTMF: 5") > -1) {pin[poz]=5, poz++; 
    } else if (at.indexOf("+DTMF: 6") > -1) {pin[poz]=6, poz++; 
//  } else if (at.indexOf("+DTMF: 7") > -1) {pin[poz]=7, poz++; 
//  } else if (at.indexOf("+DTMF: 8") > -1) {pin[poz]=8, poz++; 
    } else if (at.indexOf("+DTMF: 9") > -1) {pin[poz]=9, poz++; 
//  } else if (at.indexOf("+DTMF: 0") > -1) {pin[poz]=0, poz++;
    } else if (at.indexOf("+DTMF: *") > -1) {pin[0]=0, pin[1]=0, pin[2]=0, poz=0, delay(100), SIM800.println("AT+VTS=\"9\""); 
    } else if (at.indexOf("+DTMF: #") > -1) {Serial.println("#"), SMS_send = true;                     
    } else if (at.indexOf("+CMTI: \"SM\",") > -1) {Serial.println(at), SIM800.println("AT+CMGR=1"), delay(20), SIM800.println("AT+CMGDA=\"DEL ALL\""), delay(20);  
    } else if (at.indexOf("123starting10") > -1 ) {Timer = 60, webastoON();
    } else if (at.indexOf("123starting20") > -1 ) {Timer = 120, webastoON();
    } else if (at.indexOf("123stop") > -1 )       {webastoOFF();     
    } else if (at.indexOf("narodmon=off") > -1 )  {Serial.println(at), n_send = false;  
    } else if (at.indexOf("narodmon=on") > -1 )   {Serial.println(at), n_send = true;  
    } else if (at.indexOf("sms=off") > -1 )       {Serial.println(at), sms_report = false;  
    } else if (at.indexOf("sms=on") > -1 )        {Serial.println(at), sms_report = true;        
    } else if (at.indexOf("SHUT OK") > -1 && modem == 2) {Serial.println("S H U T OK"), SIM800.println("AT+CMEE=1"), modem = 3;      
    } else if (at.indexOf("OK") > -1 && modem == 3) {Serial.println("AT+CMEE=1 OK"), SIM800.println("AT+CGATT=1"), modem = 4;   
    } else if (at.indexOf("OK") > -1 && modem == 4) {Serial.println("AT+CGATT=1 OK"), SIM800.println("AT+CSTT=\"internet.mts.by\""), modem = 5;  
    } else if (at.indexOf("OK") > -1 && modem == 5) {Serial.println("internet.mts.by OK"), SIM800.println("AT+CIICR"), modem = 6; 
    } else if (at.indexOf("OK") > -1 && modem == 6) {Serial.println("AT+CIICR OK"), SIM800.println("AT+CIFSR"), modem = 7;  
    } else if (at.indexOf(".") > -1 && modem == 7) {Serial.println(at), SIM800.println("AT+CIPSTART=\"TCP\",\"narodmon.ru\",\"8283\""), modem = 8;                                
    } else if (at.indexOf("CONNECT OK") > -1 && modem == 8 ) {SIM800.println("AT+CIPSEND"), Serial.println("CONNECT OK"), modem = 9; // меняем статус модема
    } else if (at.indexOf(">") > -1 && modem == 9 ) {  // "набиваем" пакет данными 
         Serial.print("#SI-M8-00-12-34-58#SIM800+webasto"); // индивидуальный номер для народмона. придумайте свой !!!!!!!!!!
         Serial.print("\n#Temp1#"), Serial.print(Temp0);       
         Serial.print("\n#Temp2#"), Serial.print(Temp1);       
         Serial.print("\n#Temp3#"), Serial.print(Temp2);       
         Serial.print("\n#Vbat#"),  Serial.print(Vbat);        
         Serial.println("\n##");      // обязательный параметр окончания пакета данных
         delay(500);  // и шлем на сервер 
         SIM800.print("#SI-M8-00-12-34-58#SIM800+webasto"); // индивидуальный номер для народмона см выше.
         if (Temp0 > -40 && Temp0 < 54) SIM800.print("\n#Temp1#"), SIM800.print(Temp0);       
         if (Temp1 > -40 && Temp1 < 54) SIM800.print("\n#Temp2#"), SIM800.print(Temp1);       
         if (Temp2 > -40 && Temp2 < 54) SIM800.print("\n#Temp3#"), SIM800.print(Temp2);       
         SIM800.print("\n#Vbat#"),  SIM800.print(Vbat);  
         SIM800.print("\n#Webasto#"),  SIM800.print(heating);  
         SIM800.println("\n##");      // обязательный параметр окончания пакета данных
         SIM800.println((char)26), delay (100);
         modem = 0, delay (100), SIM800.println("AT+CIPSHUT"); // закрываем пакет

     } else Serial.println(at);    // если пришло что-то другое выводим в серийный порт
     at = "";  // очищаем переменную
}

 if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    SIM800.println(at),at = "";  //очищаем
                         }
  //             первая       вторая       третья   цифра пин кода
        if (pin[0]==1 && pin[1]==2 && pin[2]==3){pin[0]= 0, pin[1]=0, pin[2]=0, poz=0, Timer = 60, webastoON(); 
  }else if (pin[0]==2 && pin[1]==5 && pin[2]==8){pin[0]= 0, pin[1]= 0, pin[2]=0, poz=0, Timer = 120, webastoON(); 
  }else if (pin[0]==3 && pin[1]==2 && pin[2]==1){pin[0]=0, pin[1]=0, pin[2]=0, poz=0, webastoOFF()  ;
  }

if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin)==1) webastoOFF() ;//если нажали на педаль тормоза в режиме прогрева
 
}

void detection(){ // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();   // читаем температуру с трех датчиков
    Temp0 = sensors.getTempCByIndex(0);
    Temp1 = sensors.getTempCByIndex(1);
    Temp2 = sensors.getTempCByIndex(2);   
    Vbat = analogRead(BAT_Pin);  // замеряем напряжение на батарее
    Vbat = Vbat / m ; // переводим попугаи в вольты
    Serial.print("Vbat= "),Serial.print(Vbat), Serial.print (" V.");  
    Serial.print(" || Temp1 : "), Serial.print(Temp0);
    Serial.print(" || Temp2 : "), Serial.print(Temp1);
    Serial.print(" || Temp3: "), Serial.print(Temp2);  
    Serial.print(" || Interval : "), Serial.print(interval);
    Serial.print(" || Modem : "), Serial.print(modem);    
    Serial.print(" || Timer ="), Serial.println (Timer);
    
  
    if (SMS_send == true && sms_report == true) {  // если фаг SMS_send равен 1 высылаем отчет по СМС
        delay(500), Serial.print("SMS send start...");
        SIM800.println("AT+CMGS=\"+375000000002\""), delay(100);
        SIM800.print("Status SIM800 v 1.6 ");
        SIM800.print("\n Temp.Dvig: "),  SIM800.print(Temp0);
        SIM800.print("\n Temp.Salon: "), SIM800.print(Temp1);
        SIM800.print("\n Temp.Ulica: "), SIM800.print(Temp2); 
        if (n_send == true) SIM800.print("\n narodmon.ru ON ");
        SIM800.print("\n Vbat: "), SIM800.print(Vbat), SIM800.print((char)26), SMS_send = false;
              }
             
    
    if (Timer > 0 ) Timer--;    // если таймер больше ноля  
    if (heating == true && Timer <1) Serial.println("End timer"), webastoOFF(); 
    interval--;
    if (interval <1 ) { interval = 30, modem = 0; 
                    if ( n_send == true) SIM800.println("AT+CIPSHUT"), modem = 2;
                      }
}             
 
void webastoON()   // цикл включения реле вебастЫ
{
    Serial.println ("webastoON() -----"), SIM800.println("AT+VTS=\"7\"") ;
    digitalWrite(WEBASTO_Pin, HIGH), heating = true, SIM800.println("ATH0");
}

void SIM800_reset() {  // функция перезагрузки и преднастройки модуля Sim800
  digitalWrite(RESET_Pin, LOW),delay(2000);
  digitalWrite(RESET_Pin, HIGH), delay(5000);
  //SIM800.println("AT+CFUN=1,1");
  SIM800.println("ATE0"),             delay(50); // отключаем режим ЭХА 
  SIM800.println("AT+CLIP=1"),        delay(50); // включаем определитель номера
  SIM800.println("AT+DDET=1"),        delay(50); // включаем DTMF-декодер
  SIM800.println("AT+CMGF=1"),        delay(50); // включаем текстовый режим для СМС  
  SIM800.println("AT+CSCS=\"GSM\""),  delay(50); // кодировка текстового режима СМС
  SIM800.println("AT+CMGDA=\"DEL ALL\""), delay(50); // Удаление вех СМС из памяти
  SIM800.println("AT+CNMI=2,1,0,0,0"), delay(50); // включаем уведомление о входящем СМС
  SIM800.println("AT+VTD=1"),         delay(50); // задаем длительность DTFM ответа
                   } 


void webastoOFF() {  // функция остановки прогрева двигателя
    digitalWrite(WEBASTO_Pin, LOW), heating= false;
    Timer = 0, Serial.println ("Warming stopped"), delay(100); 
    SIM800.println("AT+VTS=\"8,8,8,8,8\""), delay (1500);
                   }