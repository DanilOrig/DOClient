# DOClient — fork notice / уведомление о форке

Это модифицированный форк клиента **DDNet** (<https://github.com/ddnet/ddnet>).
Исходный код, лицензия и копирайты Teeworlds/DDRace/DDNet сохранены без изменений —
см. `license.txt`.

Изменения в этой версии (по сравнению с upstream DDNet):

- вкладка настроек **DOClient** (`ec_*` конфиги и команды `ec_toggle_*`);
- кастомный список сообществ в браузере серверов;
- перевод чата (Bing/Google) с показом оригинала сообщения;
- мышь и посимвольное выделение в чате, копирование `Ctrl+C`, история чата;
- анимации интерфейса: открытие вкладок настроек, чат, меню эмоций;
- улучшенные действия дамми и значки в HUD действий дамми;
- свечение лазеров с превью;
- настройки производительности: приоритет процесса клиента и Discord.

Код форка распространяется на тех же условиях, что и DDNet (см. `license.txt`, п. 2:
«Altered source versions must be plainly marked as such»).

## Откуда взяты идеи и код функций

Часть функций повторяет поведение других клиентов DDNet. Ниже — источники, чтобы
авторство и копирайты были указаны явно.

- **DDNet** — <https://github.com/ddnet/ddnet> — база клиента (лицензия в `license.txt`).
- **BestClient** — <https://github.com/BestProjectTeam/BestClient> — список сообществ в
  браузере серверов, теги клиентов и определение BestClient по списку, индикатор позиции/
  скорости/угла дамми, настройки приоритета процессов (DDNet/Discord).
- **TaterClient / TClient** — <https://github.com/TaterClient/TClient> — формат списка
  сообществ (`TaterClient/ddnet-custom-communities`), эмодзи в чате и шрифт
  `NotoEmoji-Regular.ttf` (SIL Open Font License 1.1), Fast Input.
- **Rushie Client** — <https://github.com/RushieClient/DDNetRushie> — бэкенды перевода чата
  (Bing/Google), показ оригинала под переводом, опция «убрать эффект хаммера».
- **Pulse Client** — <https://github.com/PulseClient-ddnet/DDNetPulse> — свечение лазеров
  (многослойное тело и голова, интенсивность) и вкладка настроек лазера с превью;
  отрисовка перенесена в `CItems::RenderLaser`.
- **Entity Client** — <https://github.com/FoxNet-DDNet/Entity-Client-DDNet> — референс
  настроек производительности, проверка отображения эмодзи в чате.

Остальные функции (выделение и история чата, анимации интерфейса, офлайн-распознавание
речи, перенос настроек в карточки DOClient) написаны в этом форке.

## Сторонние компоненты / third-party

- **Noto Emoji** (<https://fonts.google.com/noto>) — SIL Open Font License 1.1.
- **Font Awesome 6 Free** — SIL Open Font License 1.1 (иконки интерфейса).
- **Vosk** (<https://alphacephei.com/vosk/>), Apache License 2.0 — офлайн-распознавание
  речи, `libvosk.dll` и модели `vosk-model-small-ru-0.22`, `vosk-model-small-en-us-0.15`
  (© Alpha Cephei). Использовалось в **v1.0**; в **v1.1** голосовой модуль удалён.
- **MinGW-w64 runtime** — `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`
  (GCC Runtime Library Exception).
- Ассеты, шрифты, языковые файлы и скины в `data/` имеют собственные лицензии
  (перечислены в `license.txt`).
