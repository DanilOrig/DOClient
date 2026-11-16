
# DOClient v1.3 (Entity base)

Сборка переведена на **Entity Client (DDNet 20.3)**.

Добавлено относительно Entity Client:
- вкладка **DOClient** (8-я в настройках) с прокруткой и карточками;
- теги клиентов `[DO]`/`[EC]`/`[RC]`/`[BC]`/`[DD]` в табло и нейм-плейтах, иконки в `data/clienttags`, определение BestClient через их API;
- анимированный градиент ников, кланов, скина и чата;
- список наблюдателей (ники под счётчиком);
- свап-таймер (логика адаптирована из Rushie/TClient, DDNet licence): компактная прозрачная панель внизу по центру, зелёный отсчёт, скрытие после `/swap`/`/cancelswap`;
- улучшенные действия дамми, «не копировать оружие» (`ec_no_copy_weapon`, бинд + `ec_toggle_no_copy_weapon`);
- позиция/скорость/угол дамми в HUD с подсветкой совпадающих координат;
- свечение лазеров с превью rifle/shotgun в настройках;
- анимация открытия чата.

Убрано относительно Entity Client:
- функция «Anti Ping Smoothing» (нестандартное сглаживание предсказания чужих тиев) — остаётся штатный механизм DDNet.

Атрибуция:
- база: DDNet 20.3 (Teeworlds/DDRace/DDNet, zlib-подобная лицензия);
- основа клиента: Entity Client — https://github.com/FoxNet-DDNet/Entity-Client-DDNet
- визуал панели свапа — собственный (DOClient), логика — из Rushie/TClient.

Исходники этой сборки — в релизе (`DOClient-Entity-source.zip`).

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
  браузере серверов и определение BestClient по публичному списку (метки BC).
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

Остальные функции написаны в этом форке: выделение и история чата, анимации интерфейса,
индикатор позиции/скорости/угла дамми, приоритеты процессов (DDNet/Discord),
метка клиента DO, офлайн-распознавание речи (в v1.0) и перенос настроек в карточки DOClient.

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
