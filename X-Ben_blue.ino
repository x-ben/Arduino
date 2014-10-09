/*  LED ドライバの設定
-----------------------------------------------*/
#include <Tlc5940.h>
#include <tlc_animations.h>
#include <tlc_config.h>
#include <tlc_fades.h>
#include <tlc_progmem_utils.h>
#include <tlc_servos.h>
#include <tlc_shifts.h>

#define MAX_BRIGHTNESS 4095 // 0-4095 
#define POWAN_SPEED 100     // microsecond 

TLC_CHANNEL_TYPE ch; 
char curCh;  // current channel 
int curBr;   // current brightness 

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

/*  Konashi から Arduino に送られる制御信号
-----------------------------------------------*/
#define ARD_SIG_NONE 0   
#define ARD_SIG_DEAL_REQUEST 1  // はなれた場所で欲しい「おかず」リクエスト
#define ARD_SIG_DEAL_ACCEPT  2  // はなれた場所で交換承認
#define ARD_SIG_APPROACH     3  // 出逢う
#define ARD_SIG_LIKE         4  // うまいねボタン
 
/*  Arduino からの通知に使う Konashi のピン
-----------------------------------------------*/
// ピースの挿入・取出を検出する用の PIO
// Arduino のピンと物理的につなげておく
// ピースがある: HIGH, ない: LOW
#define KON_PIN_PIECE 2

//フォトリフレクタ
int photoRef[4]={1000,1000,1000,1000};
int threshold[4]={200,200,200,200}; //しきい値の設定
boolean okazuFlug = false;
int okazuNum = 0;
int tempNum = 0;

//振動センサ
#define VIBRATOR_PIN 7

/*=== 初期化処理
==============================================================================================*/
void setup() { 
  Tlc.init();
  Tlc.clear();
  delay(100);
  for(ch = 0; ch < 16; ch++) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  } 
  Tlc.update();
  
  // 9600bps で serial ポートを開く
  Serial.begin(9600);
  
  // for Konashi
  pinMode(KON_PIN_PIECE, OUTPUT);
  digitalWrite(KON_PIN_PIECE, HIGH);
  pinMode(VIBRATOR_PIN, OUTPUT);
  digitalWrite(VIBRATOR_PIN, LOW);
} 

 
/*=== ループ
==============================================================================================*/
void loop() { 
  
  /*  Konashiからシリアルでリクエストがあった時
  -----------------------------------------------*/
  // print the string when a newline arrives:
  if (stringComplete) {
    //convert a string to a integer
    char floatbuf[32]; // make this at least big enough for the whole string
    inputString.toCharArray(floatbuf, sizeof(floatbuf));
    int received = atoi(floatbuf);
    //Serial.println(received);
    
    switch (received) {
      case ARD_SIG_NONE:         set_default();    break;
      case ARD_SIG_DEAL_REQUEST: deal_requested(); break;
      case ARD_SIG_DEAL_ACCEPT:  deal_accepted();  break;
      case ARD_SIG_APPROACH:     approached();     break;
      case ARD_SIG_LIKE:         liked();          break;
    }
 
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  
  /* おかずピースの有無を確認
  -----------------------------------------------*/
  photoRef[0] = analogRead(0);
  photoRef[1] = analogRead(2);
  photoRef[2] = analogRead(1);
  photoRef[3] = analogRead(3);
  /*
  for (int i=0; i<4; i++){
    Serial.print(photoRef[i]);
    Serial.print(",");
  }
  Serial.println();
  */
  
  if (okazuFlug == false){
    for (int i=0; i<4; i++){
      if (photoRef[i] > threshold[i]) {
        okazuFlug = true;
        okazuNum = i;
      }
    } 
  }
  
  /* おかずピースが１つ抜かれた時 ⇒ Konashiへ通知
  -----------------------------------------------*/
  if (okazuFlug == true){
    digitalWrite(KON_PIN_PIECE, LOW);
    piece_ejected(okazuNum);
    tempNum = okazuNum;
    /*  おかずピースが入れられた時（交換完了）
        交換成立イルミネーション ⇒ イルミ終了時にKonashiへ通知
    -----------------------------------------------*/ 
    if(photoRef[tempNum] < threshold[tempNum]){ 
        digitalWrite(KON_PIN_PIECE, HIGH);
        piece_inserted(tempNum);
        okazuFlug = false;
    }
  }
  
}


/*=== アクション(LEDのパターン)
==============================================================================================*/
/*　消灯があった時
-----------------------------------------------*/
void set_default()
{
  for(ch = 0; ch < 16; ch++) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  for(ch = 1; ch < 13; ch++) { 
    Tlc.set(ch, 0); 
  }
  Tlc.update();
  delay(500);
}


/*  おかずリクエストがあった時
-----------------------------------------------*/
void deal_requested()
{
  for(ch = 1; ch < 13; ch++) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  Tlc.update();
  digitalWrite(VIBRATOR_PIN, HIGH); //追加！
  delay(500);
  digitalWrite(VIBRATOR_PIN, LOW);
}

/*  交換が承認された時
-----------------------------------------------*/
void deal_accepted()
{
  // 赤/青 点滅（以下は赤の場合）
  for(int i=0; i<5; i++){
    for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
    }
    Tlc.update();
    digitalWrite(VIBRATOR_PIN, HIGH); //追加！
    delay(200);
    for(ch = 3; ch < 13; ch=ch+3) { 
      Tlc.set(ch, MAX_BRIGHTNESS); 
    }
    Tlc.update();
    digitalWrite(VIBRATOR_PIN, LOW); //追加！
    delay(200);
  }
  // 消灯で終わる
  for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
  }
  Tlc.update();
  delay(200);
}

/*  出逢った時
-----------------------------------------------*/
void approached()
{
  // 赤/青 点灯 ＋ バイブ
  for(ch = 1; ch < 13; ch++) { 
    Tlc.set(ch, 0); 
  }
  for(ch = 3; ch < 13; ch=ch+3) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  Tlc.update();
  digitalWrite(VIBRATOR_PIN, HIGH); //追加！
  delay(500);
  digitalWrite(VIBRATOR_PIN, LOW);
}

/*  うまいね！された時
-----------------------------------------------*/
void liked()
{
  // えげつなイルミ (0.6秒)
  for (int i=0; i<2; i++){
    //red
    for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
    }
    for(ch = 1; ch < 13; ch=ch+3) { 
      Tlc.set(ch, MAX_BRIGHTNESS); 
    }
    Tlc.update();
    delay(100);
  
    //green
    for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
    }
    for(ch = 2; ch < 13; ch=ch+3) { 
      Tlc.set(ch, MAX_BRIGHTNESS); 
    }
    Tlc.update();
    delay(100);
    
    //blue
    for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
    }
    for(ch = 3; ch < 13; ch=ch+3) { 
      Tlc.set(ch, MAX_BRIGHTNESS); 
    }
    Tlc.update();
    delay(100);
  }
  
  //OFF
  for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
  }
  Tlc.update();
  delay(100);
}

/*  自分のピースを抜いた時
-----------------------------------------------*/
void piece_ejected(int num) { // <---put okazuNum!!!
  curCh = 3*num + 1;
  // 抜いてない3つは  赤/青 点灯
  // まずは全部、青にする
  for(ch = 1; ch < 13; ch++) { 
    Tlc.set(ch, 0); 
  }
  for(ch = 3; ch < 13; ch=ch+3) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  // 抜いたところ(curCh): 白 点滅
  for(curBr = 0; curBr < MAX_BRIGHTNESS; curBr++) { 
    Tlc.set(curCh, curBr);
    Tlc.set(curCh+1, curBr);
    Tlc.set(curCh+2, curBr); 
    Tlc.update(); 
    delayMicroseconds(POWAN_SPEED); 
  } 
  for(curBr = MAX_BRIGHTNESS; curBr >= 0 ; curBr--) { 
    Tlc.set(curCh, curBr);
    Tlc.set(curCh+1, curBr);
    Tlc.set(curCh+2, curBr); 
    Tlc.update(); 
    delayMicroseconds(POWAN_SPEED); 
  }
}
 
 
/*  相手のピースを差し込んだ時
-----------------------------------------------*/
void piece_inserted(int num) { // <---put okazuNum!!!
  curCh = 3*num + 1;
  // 1.おかずピースが入れられた時（交換完了）
  // 抜いたところ: 青/赤 点灯
  // 残りの3つ:    赤/青 点灯
  // まずは全部、青にする
  for(ch = 1; ch < 13; ch++) { 
    Tlc.set(ch, 0); 
  }
  for(ch = 3; ch < 13; ch=ch+3) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  // 抜いたところ(curCh): 赤点灯
  for(curBr = 0; curBr < MAX_BRIGHTNESS; curBr++) { 
    Tlc.set(curCh, curBr);
    Tlc.set(curCh+2, 0); 
    Tlc.update(); 
    delayMicroseconds(POWAN_SPEED); 
  }
  delay(1000);
 
  // 2.交換成立イルミネーション
  // 赤 と 青 が混ざってだんだん 紫 になる
  // ループする時間をだんだん短くする
  for (int loopSpeed = 200; loopSpeed > 0; loopSpeed = loopSpeed - 20){
    // LEDのループ
    for (int count = 0; count < 800/loopSpeed; count++){
      //全部、青にする
      for(ch = 1; ch < 13; ch++) { 
        Tlc.set(ch, 0); 
      }
      for(ch = 3; ch < 13; ch=ch+3) { 
        Tlc.set(ch, MAX_BRIGHTNESS); 
      }
      //交換したところだけ赤
      Tlc.set(curCh, MAX_BRIGHTNESS); 
      Tlc.set(curCh+2, 0); 
      Tlc.update();
      delay(loopSpeed);
      //隣のLEDへ
      curCh = curCh+3; 
      if(curCh > 13) curCh = 1; 
    }
  }
  // 紫点灯
  for(ch = 0; ch < 16; ch++) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  for(ch = 1; ch < 13; ch++) { 
    Tlc.set(ch, 0); 
  }
  for(ch = 1; ch < 13; ch=ch+3) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  for(ch = 3; ch < 13; ch=ch+3) { 
    Tlc.set(ch, MAX_BRIGHTNESS); 
  }
  //フェードで消灯する
  for(curBr = MAX_BRIGHTNESS; curBr >= 0 ; curBr--) { 
    for(ch = 1; ch < 13; ch++) { 
      Tlc.set(ch, 0); 
    }
    for(ch = 1; ch < 13; ch=ch+3) { 
      Tlc.set(ch, curBr); 
    }
    for(ch = 3; ch < 13; ch=ch+3) { 
      Tlc.set(ch, curBr); 
    } 
    Tlc.update(); 
    delayMicroseconds(POWAN_SPEED); 
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}
