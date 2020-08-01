#include "SdFat.h"
#include "TMRpcm.h"//由于要支持长文件名，用SdFat库，需在TMRconf里改宏
//单声道 16kHZ 8bit ,wav文件放在music文件夹里 名称不可以是中文！！！！！！
#include "SPI.h"
#include"U8glib.h"
/*
  //频谱相关
  #include <arduinoFFT.h>
  #define SAMPLES 64            //Must be a power of 2
  #define  xres 32      // Total number of  columns in the display, must be <= SAMPLES/2
  #define  yres 24
  double vReal[SAMPLES];
  double vImag[SAMPLES];
  char data_avgs[xres];
  char y[xres];
  char displayvalue;
  char peaks[xres]={0};
  arduinoFFT FFT = arduinoFFT();
*/
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0); //对应型号的构造函数
#define SDcard 4//SD卡模块
#define SOUND 9//音频信号输出引脚
#define BUTTON 2//暂停
#define MODE 3//切换播放模式的按键
#define LEFT 6
#define RIGHT 7

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))
SdFat sd;
SdFile dir;//根目录,为简便先假定该目录下全是.wav文件，之后可以考虑改为有次级目录
SdFile wavfile;//音频文件，需要获得它的文件名，不然无法实现按键切歌
long unsigned t;//时间，用于区分长按短按
char flag_vary = 0; //用于标定切换是否在进行中
char changevol = 0; //按下是切歌还是调音量
char lock = 0; //方向锁，即左右键同时只能按一个 0不锁 1左 2右
char flag_mode = 0; //切换模式
char flag_pause = 0; //暂停
char pau = 0; //歌曲是否暂停
char mode;//按键模块切换模式：单曲循环1、顺序播放0、随机播放2
char vol;//音量,用于显示
unsigned int totalsong;//当前目录总曲目
unsigned int cur;//当前曲目
char name[44] = "/music/"; //歌曲名称
TMRpcm music;
//常字符
const uint8_t dan[] U8G_PROGMEM = {
  0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x3F, 0xF8, 0x21, 0x08, 0x21, 0x08, 0x3F, 0xF8, 0x21, 0x08,
  0x21, 0x08, 0x3F, 0xF8, 0x01, 0x00, 0x01, 0x00, 0xFF, 0xFE, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00
};
const uint8_t qu[] U8G_PROGMEM = {
  0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x7F, 0xFC, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
  0x44, 0x44, 0x7F, 0xFC, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x7F, 0xFC, 0x40, 0x04
};
const uint8_t xun[] U8G_PROGMEM = {
  0x10, 0x3C, 0x17, 0xE0, 0x24, 0x20, 0x44, 0x20, 0x97, 0xFE, 0x14, 0x20, 0x25, 0xFC, 0x65, 0x04,
  0xA5, 0x04, 0x25, 0xFC, 0x25, 0x04, 0x25, 0xFC, 0x29, 0x04, 0x29, 0x04, 0x31, 0xFC, 0x21, 0x04
};
const uint8_t huan[] U8G_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xFD, 0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x20, 0x10, 0x20, 0x7C, 0x68,
  0x10, 0xA4, 0x11, 0x22, 0x12, 0x22, 0x10, 0x20, 0x1C, 0x20, 0xE0, 0x20, 0x40, 0x20, 0x00, 0x20
};
const uint8_t sui[] U8G_PROGMEM = {
  0x00, 0x10, 0x78, 0x10, 0x49, 0xFE, 0x54, 0x20, 0x52, 0x40, 0x60, 0xFC, 0x51, 0x44, 0x48, 0x44,
  0x4E, 0x7C, 0x4A, 0x44, 0x6A, 0x7C, 0x52, 0x44, 0x42, 0x54, 0x42, 0x48, 0x45, 0x00, 0x48, 0xFE
};
const uint8_t ji[] U8G_PROGMEM = {
  0x10, 0x00, 0x11, 0xF0, 0x11, 0x10, 0x11, 0x10, 0xFD, 0x10, 0x11, 0x10, 0x31, 0x10, 0x39, 0x10,
  0x55, 0x10, 0x55, 0x10, 0x91, 0x10, 0x11, 0x12, 0x11, 0x12, 0x12, 0x12, 0x12, 0x0E, 0x14, 0x00
};
const uint8_t bo[] U8G_PROGMEM = {
  0x20, 0x78, 0x27, 0xC0, 0x22, 0x48, 0x21, 0x50, 0xFB, 0xFC, 0x21, 0x50, 0x22, 0x48, 0x2C, 0x06,
  0x33, 0xF8, 0xE2, 0x48, 0x22, 0x48, 0x23, 0xF8, 0x22, 0x48, 0x22, 0x48, 0xA3, 0xF8, 0x42, 0x08
};
const uint8_t fang[] U8G_PROGMEM = {
  0x20, 0x40, 0x10, 0x40, 0x00, 0x40, 0xFE, 0x80, 0x20, 0xFE, 0x21, 0x08, 0x3E, 0x88, 0x24, 0x88,
  0x24, 0x88, 0x24, 0x50, 0x24, 0x50, 0x24, 0x20, 0x44, 0x50, 0x54, 0x88, 0x89, 0x04, 0x02, 0x02
};
const uint8_t shun[] U8G_PROGMEM = {
  0x04, 0x00, 0x45, 0xFE, 0x54, 0x20, 0x54, 0x40, 0x55, 0xFC, 0x55, 0x04, 0x55, 0x24, 0x55, 0x24,
  0x55, 0x24, 0x55, 0x24, 0x55, 0x24, 0x55, 0x44, 0x54, 0x50, 0x54, 0x88, 0x85, 0x04, 0x06, 0x02
};
const uint8_t xu[] U8G_PROGMEM = {
  0x01, 0x00, 0x00, 0x80, 0x3F, 0xFE, 0x20, 0x00, 0x23, 0xF8, 0x20, 0x10, 0x20, 0xA0, 0x20, 0x40,
  0x2F, 0xFE, 0x20, 0x42, 0x20, 0x44, 0x20, 0x40, 0x40, 0x40, 0x40, 0x40, 0x81, 0x40, 0x00, 0x80
};
const uint8_t yin[] U8G_PROGMEM = {
  0x02, 0x00, 0x01, 0x00, 0x3F, 0xF8, 0x00, 0x00, 0x08, 0x20, 0x04, 0x40, 0xFF, 0xFE, 0x00, 0x00,
  0x1F, 0xF0, 0x10, 0x10, 0x10, 0x10, 0x1F, 0xF0, 0x10, 0x10, 0x10, 0x10, 0x1F, 0xF0, 0x10, 0x10
};
const uint8_t liang[] U8G_PROGMEM = {
  0x00, 0x00, 0x1F, 0xF0, 0x10, 0x10, 0x1F, 0xF0, 0x10, 0x10, 0xFF, 0xFE, 0x00, 0x00, 0x1F, 0xF0,
  0x11, 0x10, 0x1F, 0xF0, 0x11, 0x10, 0x1F, 0xF0, 0x01, 0x00, 0x1F, 0xF0, 0x01, 0x00, 0x7F, 0xFC
};

const uint8_t num[8][16] U8G_PROGMEM = { //0-7
  {0x00, 0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x08, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x02, 0x04, 0x08, 0x10, 0x20, 0x42, 0x7E, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x02, 0x04, 0x18, 0x04, 0x02, 0x42, 0x42, 0x3C, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x04, 0x0C, 0x0C, 0x14, 0x24, 0x24, 0x44, 0x7F, 0x04, 0x04, 0x1F, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x7E, 0x40, 0x40, 0x40, 0x78, 0x44, 0x02, 0x02, 0x42, 0x44, 0x38, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x18, 0x24, 0x40, 0x40, 0x5C, 0x62, 0x42, 0x42, 0x42, 0x22, 0x1C, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x7E, 0x42, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00}
};

void autonext();//当一首歌曲播放完毕时，按照模式选择播放下一首歌
void user_option();//用户输入判定
void changesong(int option);//摇动摇杆换歌 0 左摇 1 右摇
void randomsong();//随机切一首歌
void draw();//图形界面
void genfft();//频谱相关，是在网络代码的基础上改的，也不是很懂
void setup() {
  //串口初始化
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(MODE, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  //音乐播放初始化
  music.speakerPin = SOUND;
  if (!sd.begin(4, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }
  vol = 5;
  music.setVolume(vol);
  music.quality(1);
  //音频处理初始化
  ADCSRA = 0b11100101;
  ADMUX = 0b00000000;
  delay(50);
  //文件初始化
  if (!dir.open("/music/", O_RDONLY)) {
    sd.errorHalt("open:");
  }
  totalsong = 0;
  while (wavfile.openNext(&dir, O_RDONLY)) {
    totalsong++;
    wavfile.close();
  }
  if (totalsong == 0) {
    while (1) ;
  }
  dir.rewind();
  wavfile.openNext(&dir, O_RDONLY);
  cur = 1;
  wavfile.getName(name + 7, 36);
  music.play(name);//播放第一首曲子
}

void loop() {
  u8g.firstPage();
  do {
    autonext();//检测是否需要切歌
    user_option();//检测用户操作
    //genfft();//频谱
    draw();//图形界面

  } while (u8g.nextPage());
}
void autonext()
{
  if (!music.isPlaying()) { //如果这首歌播完了，选择播放的下一首
    if (mode == 0) { //顺序播放
      wavfile.close();
      cur++;
      if (!wavfile.openNext(&dir)) {
        cur = 1;
        dir.rewind();
        wavfile.openNext(&dir);
      }
    } else if (mode == 2) { //随机播放,有可能是同一首歌
      randomsong();
    }
    wavfile.getName(name + 7, 36);
    music.play(name);//单曲循环文件没有变化
  }
}
void user_option()
{
  char state;
  state = digitalRead(MODE);//切换模式
  if (!flag_mode) { //上次已结束，读取新信息
    flag_mode = !state;
  }
  if (flag_mode && state) {//若是操作开始且复位，执行模式切换
    mode = (mode + 1) % 3;
    flag_mode = 0;
  }
  state = digitalRead(BUTTON);//暂停,同上
  if (!flag_pause) {
    flag_pause = !state;
  }
  if (flag_pause && state) {
    music.pause();
    pau = !pau;
    flag_pause = 0;
  }


  state = digitalRead(LEFT);
  if(lock!=2){
    if (state && flag_vary) { //按键复位且正在进行
    if (!changevol) { //如果按得够久调整音量，便不执行切歌
      changesong(0);
      pau = 0;
    }
    flag_vary = 0; //操作结束
    lock = 0; //解锁
  } else if (state && !flag_vary) { //按键复位且已结束
    //什么也不做
  } else if (!state && flag_vary) { //按键按下且正在进行中
    if (millis() - t > 1000) { //按的时间超过1秒
      vol = MAX(vol - 1, 0);
      music.setVolume(vol);
      t = millis(); //重新开始计时
      changevol = 1; //已经改过音量
    }
  } else {//按键按下且已经结束
    flag_vary = 1; //开始
    t = millis();
    changevol = 0; //先假定不调音量
    lock = 1; //方向锁开启
  }
  }
  state = digitalRead(RIGHT);
  if(lock!=1){
    if (state && flag_vary) { //按键复位且正在进行
    if (!changevol) { //如果按得够久调整音量，便不执行切歌
      changesong(1);
      pau = 0; 
    }
    flag_vary = 0;
    lock = 0; //解锁
  } else if (state && !flag_vary) { //按键复位且已结束
    //什么也不做
  } else if (!state && flag_vary) { //按键按下且正在进行中
    if (millis() - t > 1000) { //按的时间超过1秒
      vol = MIN(vol + 1, 7);
      music.setVolume(vol);
      t = millis(); //重新开始计时
      changevol = 1; //已经改过音量
    }
  } else {//按键按下且已经结束
    flag_vary = 1; //开始
    t = millis();
    changevol = 0; //先假定不调音量
    lock = 2; //方向锁开启
  }
  }
}
void randomsong()
{
  wavfile.close();
  randomSeed(analogRead(A3));
  int temp;
  temp = random(0, totalsong) + 1;
  cur = temp;
  dir.rewind();
  wavfile.openNext(&dir);
  while (--temp) {
    wavfile.close();
    wavfile.openNext(&dir);
  }
}
void changesong(int option)
{
  if (mode == 2) { //随机播放向左向右摇都是一样的
    randomsong();
  } else {//单曲循环顺序播放就是简单的加减
    if (option) {
      wavfile.close();
      cur++;
      if (!wavfile.openNext(&dir)) {
        cur = 1;
        dir.rewind();
        wavfile.openNext(&dir);
      }
    } else {
      cur = (cur == 1) ? totalsong : (cur - 1);
      dir.rewind();
      int i;
      for (i = 0; i < cur; i++) {
        wavfile.close();
        wavfile.openNext(&dir);
      }
    }
  }
  wavfile.getName(name + 7, 36);
  music.play(name);
}
void draw()
{
  switch (mode) { //显示播放模式
    case 0: u8g.drawBitmapP(0, 0, 2, 16, shun); u8g.drawBitmapP(16, 0, 2, 16, xu); u8g.drawBitmapP(32, 0, 2, 16, bo); u8g.drawBitmapP(48, 0, 2, 16, fang); break;
    case 1: u8g.drawBitmapP(0, 0, 2, 16, dan); u8g.drawBitmapP(16, 0, 2, 16, qu); u8g.drawBitmapP(32, 0, 2, 16, xun); u8g.drawBitmapP(48, 0, 2, 16, huan); break;
    case 2: u8g.drawBitmapP(0, 0, 2, 16, sui); u8g.drawBitmapP(16, 0, 2, 16, ji); u8g.drawBitmapP(32, 0, 2, 16, bo); u8g.drawBitmapP(48, 0, 2, 16, fang); break;
  }
  if (pau) { //暂停？
    u8g.drawTriangle(115, 2, 115, 14, 125, 8);
  } else {
    u8g.drawBox(116, 2, 2, 12);
    u8g.drawBox(122, 2, 2, 12);
  }
  u8g.drawBitmapP(64 + 8, 0, 2, 16, yin); u8g.drawBitmapP(80 + 8, 0, 2, 16, liang);
  u8g.drawBitmapP(96 + 8, 0, 1, 16, num[vol]);
  u8g.setFont(u8g_font_fur14);
  u8g.drawStr(0, 32, name + 7);
  /*
    int i;
    for (i = 0; i < 32; i++) {
    u8g.drawBox(16 + 3 * i, 60 - y[i], 3, y[i]);
    }
  */
}
/*void genfft()
  {
  for (int i = 0; i < SAMPLES; i++)
  {
    while (!(ADCSRA & 0x10));       // wait for ADC to complete current conversion ie ADIF bit set
    ADCSRA = 0b11110101 ;               // clear ADIF bit so that ADC can do next operation (0xf5)
    int value = ADC - 512 ;                 // Read from ADC and subtract DC offset caused value
    vReal[i] = value / 8;                   // Copy to bins after compressing
    vImag[i] = 0;
  }
  // -- Sampling
  // ++ FFT
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  // -- FFT
  // ++ re-arrange FFT result to match with no. of columns on display ( xres ) 也不是很懂，就是取个平均值吧
  int step = (SAMPLES / 2) / xres;
  for (int i = 0; i < (SAMPLES / 2); i += step)
  {
    data_avgs[i] = 0;
    for (int k = 0 ; k < step ; k++) {
      data_avgs[i] = data_avgs[i] + vReal[i + k];
    }
    data_avgs[i] = data_avgs[i] / step;
  }
  // -- re-arrange FFT result to match with no. of columns on display ( xres )
  // ++ send to display according measured value 略微修改，最大长度8->24
  for (int i = 0; i < xres; i++)
  {
    data_avgs[i] = constrain(data_avgs[i], 0, 80);          // set max & min values for buckets
    data_avgs[i] = map(data_avgs[i], 0, 80, 0, yres);        // remap averaged values to yres
    y[i] = data_avgs[i];
    peaks[i] = peaks[i] - 1;  // decay by one light
    if (y[i] > peaks[i])
      peaks[i] = y[i] ;
    y[i] = peaks[i];
  }
  }
*/
