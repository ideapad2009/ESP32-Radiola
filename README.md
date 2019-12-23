# ESP32-Media
Миграция проекта https://github.com/karawin/Ka-Radio32 на VSCode+PlatformIO

Проект делаю для себя. Никаких хроник изменений не веду. Как только реализую основные задачи, напишу развёрнутый мануал.
Проект на стадии бодрой Альфы :-)

**Вырезано из Ka-Radio32 сейчас:**
  - Вырезаны все аудиокодеки, аудиодекодеры, кроме VS1053b
  - Вырезана поддержка всех дисплеев, кроме TFT ILI9341 18x240x320
  - Вырезаны все интерфейсы, кроме веб-интерфейса, тачскрина, энкодеров и ик-пульта

**TODO:**
  - ...будет вырезано всё, что не используется :-)

**Реализовано сейчас:**
  + Реализован датчик температуры DS18B20 для контроля температуры радиаторов усилителя.
  + Реализован вывод показаний датчика температуры DS18B20 на странице веб-интерфейса
  + Реализован тахометр (счётчик оборотов) вентилятора охлаждения радиаторов усилителя
  + Реализован вывод показаний тахометра на странице веб-интерфейса
  + Русифицирован веб-интерфейс

**TODO (В ближайшее время):**
  + Добавить ручное управление оборотами вентилятора охлаждения радиаторов усилителя
  + Добавить автоматическое управление оборотами вентилятора охлаждения радиаторов усилителя в зависимости то температуры радиаторов
  + Добавить звуковую индикацию событий BUZZER (пищалки).
  + etc...

![Image alt](https://github.com/SinglWolf/ESP32-Media/raw/master/pictures/ESP32-Media.png)
