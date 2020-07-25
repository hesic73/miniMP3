#include "SD.h"
#include "TMRpcm.h"
#include "SPI.h"
#define SDcard 4
#define BUTTON 2
#define VX A0
#define VY A1
//用户行为
#define NOP -1
#define PAUSE 0//暂停
#define PREV 1//前一首
#define NEXT 2//后一首
#define VOL_UP 3//调大音量
#define VOL_DN 4//减小音量
File root;//根目录,为简便先假定该目录下全是.wav文件，之后可以考虑改为有次级目录
File wavfile;//音频文件，需要获得它的文件名，不然无法实现按键切歌
int n_all;//歌曲的位次
int cur;//当前曲目
TMRpcm music;
void hang()//等待摇杆回到初始状态
{
  int x,y,b;
  while(1){
    b = digitalRead(BUTTON);
    x = analogRead(VX);
    y = analogRead(VY);
    if(b&&(x>400&&x<600)&&(y>400&&y<600)) break;
  }
}
int input()//摇动且回复到初始状态算一次输入
{
  int x, y;
  x = digitalRead(BUTTON);
  if (!x){
    hang();
    return PAUSE;
  }
  x = analogRead(VX);
  y = analogRead(VY);
  if (x < 400){
    hang();
    return PREV;
  }
  if (x > 600){
    hang();
    return NEXT;
  }
  if (y > 600){
    hang();
    return VOL_UP;
  }
  if (y < 400){
    hang();
    return VOL_DN;
  }
  return NOP;
}
void setup() {
  pinMode(BUTTON,INPUT_PULLUP);
  music.speakerPin = 9; //设置音频输出针脚 9
  Serial.begin(9600);
  if (!SD.begin(SDcard)) {
    Serial.println("SD fail");
    return;
  }
  music.setVolume(5);    //   设置音量0 ~7
  music.quality(1);    
    
  root=SD.open("/");
  wavfile=root.openNextFile() ;
  if(!wavfile){
    Serial.println("No file available.");
  }
  n_all=1;//先数一下一共有多少歌
  while(!wavfile){
    wavfile=root.openNextFile();
    n_all++;
    wavfile.close();
  }
}

void loop() {
  switch (input()) {
    case NOP: break;
    case PAUSE: music.pause(); break;
    case PREV: break;
    case NEXT: break;
    case VOL_UP: music.volume(1);break;
    case VOL_DN: music.volume(0);break;
  }
}
