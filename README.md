# DOClient

Неофициальный клиент DDNet с набором удобств: вкладка DOClient в настройках, теги клиентов,
градиент, наблюдатели, свап-таймер, инфо дамми, свечение лазеров, анимации интерфейса.

> **База — [Entity Client](https://github.com/FoxNet-DDNet/Entity-Client-DDNet) (DDNet 20.3).**
> Это изменённая версия Entity Client; проект не связан с DDNet, FoxNet и BestProject.
> Лицензия та же (см. [license.txt](license.txt)), список изменений — в [NOTICE.md](NOTICE.md)

## Сборки

| Релиз | База |
|---|---|
| `v1.3-entity` и новее | Entity Client (DDNet 20.3) |
| `v1.2` и старше | DDNet (старый форк) |

## Скачать

Последняя сборка — **[v1.3-entity](../../releases/latest)**:

| Файл | Что это |
|---|---|
| `DOClient-Entity-win64.zip` | клиент (`DDNet.exe`) + data, Windows x64 |
| `Source code (zip/tar.gz)` | исходники этой сборки (генерирует GitHub) |

## Установка

1. Распакуй `DOClient-Entity-win64.zip` в отдельную папку.
2. Запусти `DDNet.exe`.
3. Настройки → вкладка **DOClient**.

Требования: Windows 10/11 x64, GPU с OpenGL 3.3+.
Используется общий профиль `%APPDATA%\DDNet`.

## Сборка из исходников

```
git clone https://github.com/DanilOrig/DOClient.git
cd DOClient
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDEV=OFF
cmake --build build --config Release --target game-client
```

---

## Возможности

Настройки клиента собраны во вкладке **DOClient** в меню настроек (`ec_*` конфиги, команды `ec_toggle_*`).

- **Дамми:** позиция/скорость/угол, улучшенные действия, «не копировать оружие» с биндом.
- **Swap Time:** таймер свапа, позиция по вертикали.
- **Чат:** мышь и посимвольное выделение, `Ctrl+C`, история (колесо, PgUp/PgDn), анимации.
- **Перевод чата:** входящие/свои сообщения, бэкенды Bing/Google, показ оригинала под переводом.
- **Клиенты:** теги клиентов, определение BestClient по списку, кастомные сообщества в браузере.
- **Голос (STT):** push-to-talk офлайн-распознавание (Vosk, RU/EN), отправка в чат.
- **Производительность:** высокий приоритет процесса клиента, понижение приоритета Discord.
- **Интерфейс:** плавные анимации вкладок настроек, чата и меню эмоций.

## Сборка (Windows)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target game-client
```

Результат: `build/Release/DDNet.exe`. Модели Vosk кладутся в `data/stt` (см. `data/stt/bin` и папки моделей).

## Лицензия и сторонние компоненты

- Код DDNet/Teeworlds/DDRace — `license.txt` (zlib-подобная), копирайты сохранены.
- Это изменённый форк: список изменений и требование об обозначении — `NOTICE.md`.
- Vosk (Alpha Cephei) — Apache-2.0; MinGW-w64 runtime — GCC Runtime Library Exception.
- Ассеты, шрифты, языки и скины в `data/` — собственные лицензии (см. `license.txt`).
## Откуда взяты функции

- **DDNet** — база клиента.
- **BestClient** — сообщества в браузере и определение BestClient по публичному списку (метки BC).
- **TaterClient / TClient** — формат списка сообществ, эмодзи в чате (шрифт Noto Emoji, OFL 1.1), Fast Input.
- **Rushie Client** — перевод чата (Bing/Google), показ оригинала, «убрать эффект хаммера».
- **Pulse Client** — свечение лазеров и вкладка настроек лазера с превью.
- **Entity Client** — референс настроек производительности.

Индикатор позиции/скорости/угла дамми, приоритеты процессов и метка DO — собственные реализации.
Подробнее — в [NOTICE.md](NOTICE.md).

