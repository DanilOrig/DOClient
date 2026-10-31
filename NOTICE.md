# DOClient — fork notice / уведомление о форке

Это модифицированный форк клиента **DDNet** (<https://github.com/ddnet/ddnet>).
Исходный код, лицензия и копирайты Teeworlds/DDRace/DDNet сохранены без изменений —
см. `license.txt`.

Изменения в этой версии (по сравнению с upstream DDNet):

- вкладка настроек **DOClient** (`ec_*` конфиги и команды `ec_toggle_*`);
- кастомный список сообществ в браузере серверов (TaterClient/BestClient-совместимый формат);
- перевод чата через бэкенды Bing/Google (с показом оригинала сообщения);
- мышь и посимвольное выделение в чате, копирование `Ctrl+C`, история чата (колесо, PgUp/PgDn);
- анимации интерфейса: открытие вкладок настроек, чат, меню эмоций;
- офлайн распознавание речи (push-to-talk) на базе Vosk;
- настройки производительности: приоритет процесса клиента и Discord.

Код форка распространяется на тех же условиях, что и DDNet (см. `license.txt`, п. 2:
«Altered source versions must be plainly marked as such»).

## Сторонние компоненты / third-party

- **Vosk** (<https://alphacephei.com/vosk/>), Apache License 2.0 — офлайн-распознавание речи,
  `libvosk.dll` и модели `vosk-model-small-ru-0.22`, `vosk-model-small-en-us-0.15`
  (© Alpha Cephei, Apache-2.0).
- **MinGW-w64 runtime** — `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`
  (GCC Runtime Library Exception).
- Ассеты, шрифты, языковые файлы и скины в `data/` имеют собственные лицензии
  (перечислены в `license.txt`).
