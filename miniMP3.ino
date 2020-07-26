#include "SD.h"
#include "TMRpcm.h"
#include "SPI.h"
#define SDcard 4//SD卡模块
#define SOUND 9//音频信号输出引脚
#define BUTTON 2//摇杆的按键
#define MODE 3//切换播放模式的按键,这个用外部中断实现
#define VX A0 //摇杆方向
#define VY A1
//用户行为
#define NOP -1
#define PAUSE 0//暂停
#define PREV 1//前一首
#define NEXT 2//后一首
#define VOL_UP 3//调大音量
#define VOL_DN 4//减小音量
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))
File dir;//根目录,为简便先假定该目录下全是.wav文件，之后可以考虑改为有次级目录
File wavfile;//音频文件，需要获得它的文件名，不然无法实现按键切歌
char mode;//一个饼，按键模块切换模式：单曲循环1、顺序播放0、随机播放2
char vol;//音量,用于显示
unsigned int totalsong;//当前目录总曲目
unsigned int cur;//当前曲目
TMRpcm music;
void isr() {
  mode = (mode + 1) % 3; //简单地切模式
}
void hang();//等待摇杆回到初始状态
int input();//摇动且回复到初始状态算一次输入
void autonext();//当一首歌曲播放完毕时，按照模式选择播放下一首歌
void changesong(int option);//摇动摇杆换歌 0 左摇 1 右摇
void randomsong();//随机切一首歌
void setup() {
  //串口初始化
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(MODE, INPUT_PULLUP);
  mode = 0;
  attachInterrupt(1, isr, FALLING);
  //音乐播放初始化
  music.speakerPin = SOUND;
  Serial.begin(9600);
  if (!SD.begin(SDcard)) {
    Serial.println("SD fail");
    return;
  }
  vol = 5;
  music.setVolume(vol);
  music.quality(1);
  //文件初始化
  dir = SD.open("/");
  totalsong = 0;
  while (1) {
    wavfile = dir.openNextFile() ;
    if (!wavfile) break;
    totalsong++;
    wavfile.close();
  }
  if (totalsong == 0) {
    Serial.println("Empty directory.");
    return ;
  }
  dir.rewindDirectory();
  wavfile = dir.openNextFile();
  cur = 1;
  music.play(wavfile.name());//播放第一首曲子
}

void loop() {
  switch (input()) {
    case NOP: break;
    case PAUSE: music.pause(); break;
    case PREV: changesong(0);break;
    case NEXT: changesong(1);break;
    case VOL_UP: vol = MAX(vol + 1, 7); music.setVolume(vol); break;
    case VOL_DN: vol = MIN(vol - 1, 0); music.setVolume(vol); break;
  }
}
void hang()
{
  int x, y, b;
  while (1) { //也要考虑等的过程中歌曲完毕的情况？
    b = digitalRead(BUTTON);
    x = analogRead(VX);
    y = analogRead(VY);
    if (b && (x > 400 && x < 600) && (y > 400 && y < 600)) break;
    autonext();
  }
}
int input()
{
  int x, y;
  x = digitalRead(BUTTON);
  if (!x) {
    hang();
    return PAUSE;
  }
  x = analogRead(VX);
  y = analogRead(VY);
  if (x < 400) {
    hang();
    return PREV;
  }
  if (x > 600) {
    hang();
    return NEXT;
  }
  if (y > 600) {
    hang();
    return VOL_UP;
  }
  if (y < 400) {
    hang();
    return VOL_DN;
  }
  hang();
  return NOP;
}
void autonext()
{
  if (!music.isPlaying()) { //如果这首歌播完了，选择播放的下一首
    if (mode == 0) { //顺序播放
      wavfile.close();
      wavfile = dir.openNextFile();
      cur++;
      if (!wavfile) {
        cur = 1;
        dir.rewindDirectory();
        wavfile = dir.openNextFile();
      }
    } else if (mode == 2) { //随机播放,有可能是同一首歌
      randomsong();
    }
    music.play(wavfile.name());//单曲循环文件没有变化
  }
}
void randomsong()
{
  wavfile.close();
      randomSeed(analogRead(A2));
      int temp;
      temp = random(0, totalsong) + 1;
      cur = temp;
      dir.rewindDirectory();
      wavfile = dir.openNextFile();
      while (--temp) {
        wavfile.close();
        wavfile = dir.openNextFile();
      }
}
void changesong(int option)
{
  if(mode==2){//随机播放向左向右摇都是一样的
    randomsong();
  }else {//单曲循环顺序播放就是简单的加减
    if(option){
      wavfile.close();
      wavfile = dir.openNextFile();
      cur++;
      if (!wavfile) {
        cur = 1;
        dir.rewindDirectory();
        wavfile = dir.openNextFile();
      }
    }else {
      cur=(cur==1)?totalsong:(cur-1);
      dir.rewindDirectory();
      int i;
      for(i=0;i<cur;i++){
        wavfile.close();
        wavfile=dir.openNextFile();
      }
    }
  }
  music.play(wavfile.name());
}
