# UA3REO-DDC-Transceiver
DDC SDR Tranceiver project https://ua3reo.ru/

Проект является учебным, в нём постоянно что-то меняется, дорабатывается. Поэтому собирать только на свой страх и риск, вы можете зря потерять свои деньги и время.<br>

## Принцип работы

ВЧ сигнал оцифровывается высокоскоростной микросхемой АЦП, и подаётся на FPGA процессор.<br>
В нём происходит DDC/DUC преобразование (цифровое смещение частоты вниз или вверх по спектру) - по аналогии с приёмником прямого преобразования.<br>
I и Q квадратурные сигналы, полученные в ходе преобразований, поступают на микропроцессор STM32.<br>
В нём происходит фильтрация, (де)модуляция и вывод звука на аудио-кодек/USB. Также он обрабатывает весь пользовательский интерфейс.<br>
При передаче процесс происходит в обратном порядке, только в конце цепочки стоит ЦАП, преобразующий цифровой сигнал обратно в аналоговый ВЧ.<br>

## Технические характеристики

* Частота приёма: 0.5mHz - 450+mHz
* Частота передачи: 0.5mHz - 30mHz
* Мощность TX: 5Вт
* Виды модуляции (TX/RX): CW, LSB, USB, AM, FM, WFM, DIGI
* Предусилитель на 20дБ и аттенюатор на 12дБ
* Динамический диапазон АЦП 100дБ
* Напряжение питания: 13.8в
* Потребляемый ток при приёме: ~0.48А
* Потребляемый ток при передаче: ~2.0А

### Чувствительность

При соотношении сигнал-шум 10dB, предусилитель включен, ATT, LPF, BPF выключены

Частота, mHz | Чувствительность, dBm | Чувствительность, мкВ
------------ | ------------- | -------------
1.9	| -103	| 1.6
3.6	| -113	| 0.5
7.1	| -111	| 0.6
10  | -110	| 0.7
14  | -110  | 0.7
28  | -110  | 0.7
100	| -115	| 0.4
145	| -114	| 0.45
435	| -100	| 2.2

## Функции трансивера

* Панорама (спектр+водопад) шириной в 48кГц
* Регулируемая полоса пропускания: нижний порог от 60гц до 500гц, верхний порог от 300гц до 15кГц
* Notch фильтр, регулируемый в полосе пропускания
* Отключаемое АРУ (AGC) с регулируемой скоростью атаки
* Карта диапазонов, с возможностью автоматического переключения моды
* Цифровое уменьшение шумов (DNR) на основе LMS фильтра
* Работа по USB, в том числе и передача звука
* CAT/PTT виртуальные COM-порты (эмуляция FT-450)
* Автоматическое управление предусилителем и аттенюатором
* CW декодер
* Анализатор спектра

## Прошивка

Прошивка STM32 производится через Keil или по USB шнурку в DFU Mode. Прошивка FPGA происходит через программу Quartus.

## Управление

* **AF GAIN** - Громкость
* **SHIFT** - Плавная отстройка от частоты трансивера
* **ENC MAIN** - Главный энкодер для управления частотой и настройками меню
* **ENC 2** - Вспомогательный энкодер для работы с меню. В обычном режиме быстро переключает частоту, при включенном Notch-фильтре регулирует его полосу
* **BAND -** - Переключение на диапазон ниже
* **BAND +** - Переключение на диапазон выше
* **MODE -** - Переключение на моду ниже
* **MODE +** - Переключение на моду выше
* **FAST** - Режим ускоренной х10 перемотки частоты основным энкодером
* **PRE/ATT** - Переключение режимов предусилителя и аттенюатора
* **TUNE** - Включение несущей для настройки антенны
* **A/B** - Переключение между банками настроек приёмника VFO-A/VFO-B
* **AGC** - Включение АРУ (автоматической регулировки усиления)
* **DNR** - Включение цифрового шумоподавления
* **A=B** - Установка настроек второго банка приёмника равным текущему
* **NOTCH** - Включение Notch-фильтра для устранения узкополосной помехи
* **CLAR** - Позволяет разнести передачу и приём на разные банки VFO
* **MENU** - Переход в меню
* **MENU[зажатие]** - Включение блокировки клавиатуры LOCK
* **MENU[при включении]** - Сброс настроек трансивера

## Настройки

### TRX Settings

* **RF Power** - Мощность передачи, %
* **FM Squelch** - Уровень шумодава FM
* **RF Gain** - Усиление ВЧ (цифровое, используется при отключенном AGC)
* **AGC Speed** - Скорость срабатывания АРУ (автоматического регулятора уровня сигнала)
* **Band Map** - Карта диапазонов, автоматически переключает моду в зависимости от частоты
* **AutoGainer** - Автоматическое управление ATT/PREAMP в зависимости от уровня сигнала на АЦП
* **LPF Filter** - Управление аппаратным ФНЧ Фильтром
* **BPF Filter** - Управление аппаратным полосовым фильтром
* **Encoder slow rate** - Коэффициент замедления основного энкодера
* **MIC IN** - Выбор микрофонного входа
* **LINE IN** - Выбор линейного входа
* **USB IN** - Выбор входа аудио по USB

### BW Settings

* **SSB HPF Pass** - Частота среза ФНЧ при работе в SSB
* **SSB LPF Pass** - Частота среза ФВЧ при работе в SSB
* **CW LPF Pass** - Частота среза ФНЧ при работе в CW
* **FM LPF Pass** - Частота среза ФНЧ при работе в FM

### CW Settings

* **CW Key timeout** - Время до остановки режима передачи после отпускания ключа
* **CW Generator shift** - Отстройка генератора приёма от частоты передачи
* **CW Self Hear** - Самоконтроль CW (слышно нажатие ключа)
* **CW Decoder** - Программный декодер CW приёма

### LCD Settings

* **LCD Brightness** - Яркость LCD
* **LCD Sleep Time** - Время до снижения яркости LCD при неактивности

### FFT Settings

* **FFT Zoom** - Увеличение (приближение) FFT
* **FFT Enabled** - Включение водопада и FFT
* **FFT Averaging** - Уровень усреднения всплесков FFT
* **FFT Window** - Выбор окна FFT (Hamming/Blackman-Harris/Hanning)

### ADC Settings

* **ADC Preamp** - Включение предусилителя, встроенного в АЦП
* **ADC Dither** - Включение дизеринга АЦП для приёма слабых сигналов
* **ADC Randomizer** - Включение шифрования цифровой линии АЦП
* **ADC Shutdown** - Выключение АЦП

### WIFI Settings

* **WIFI Enabled** - Включение WiFi модуля
* **WIFI Select AP** - Выбор точки доступа WiFi
* **WIFI Set AP Pass** - Установка пароля для точки доступа WiFi
* **WIFI Timezone** - Временная зона (для обновления времени через интернет)

### Set Clock Time

* Установка часов

### Flash update

* Запуск обнеовления прошивки STM32

### Spectrum Analyzer

* **Spectrum START** - Запуск спектрального анализатора
* **Begin, 10kHz** - Стартовая частота анализатора с шагом в 10kHz
* **End, 10kHz** - Конечная частота анализатора с шагом в 10kHz
