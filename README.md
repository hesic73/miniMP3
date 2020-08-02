# miniMP3
- OLED屏图形界面
  - 文件名（支持长文件名，不可以是中文！！！）
  - 播放模式
  - 音量
  - 音频频谱
  - 是否暂停
- 音频播放
  - TMRpcm库，支持mono 8bit 16kHZ .wav文件
  - 播放、暂停、音量调节、模式切换
  - 一首歌曲播放结束后，自动播放下一首
- 频谱采样
  - 参考[Arduino实现32分频音频频谱显示器]( https://arduino.nxez.com/2019/03/28/arduino-32-frequency-audio-spectrum-display.html )。
  - 代码部分只是略作修改，音频信号直接由功放正极代替耳机+转换头。



---

# 已知BUG
- OLED屏会随机出现长条，似乎是硬件问题。
- 打开某些文件等操作会随机性失败，用while(!wavfile.openNext(&dir));可以应付大多数情况。
- 短时间内多次切歌会出现异常行为，上一条不能完全解决，而且cur值会不正常地跳变，考虑对切歌操作最小时间间隔加以限制。
