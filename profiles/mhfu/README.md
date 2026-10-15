# Pokke - Monster Hunter Freedom Unite (PC Native Port)

Perfil de recompilación estática para **Monster Hunter Freedom Unite** (`ULUS-10391`, v1.01).

Basado y logrado gracias al proyecto **[Yakumo](https://github.com/TeamGDB/Yakumo)** de **TeamGDB** y al framework **PSPRecomp**.

## Identificadores del Juego

- **Disc ID:** `ULUS10391` (`ULUS-10391`)
- **Título:** `Monster Hunter Freedom Unite`
- **Región:** USA / NTSC-U (v1.01)
- **Hash SHA-256 de `BOOT.BIN` / `BOOT.ELF`:** `3070954e2dc88fd0b0836c0c1dc526531aca4a213b0f18806e324963efb514f4`
- **Hash SHA-256 de `EBOOT.BIN` original:** `3354adb3ee40821f230c4055b9bf7cc5d4c864e42abb74ce8ac022933f9697a7`

## Ejecutable del Port

- **Nombre:** `Pokke.exe`
- **Directorio de salida:** `out/mhfu/bin/Pokke.exe`
- **Launcher / Asistente Gráfico:** Se abre en el primer inicio o con `Pokke.exe --launcher`
- **Menú de configuración en tiempo real:** Tecla **`F1`** o `Esc`
- **Controles de cámara:** Joystick derecho emula la cruceta D-Pad para rotación suave en 360 grados sin claw grip.
- **Tasa de fotogramas:** 30, 45, 60, 90, 120 FPS o Match Display con interpolación de sub-cuadros.
- **Contador de FPS:** Tecla **`F3`** para activar o desactivar el contador de FPS en pantalla.

## Módulos de Modo (Overlays)

| Slot | Módulos Principales | Estado |
| :--- | :--- | :--- |
| `0x09A5F300` | `game_task`, `lobby_task`, `download_task`, `gallery_task`, `option_task`, `edit_task`, `arcade_task`, `cont_task`, `demo_task`, `install_task`, `movie_task`, `show_select_task` | Compilados nativos (DLL) |
| `0x09C19100` | `game_sub`, `demo_sub` | Compilados nativos (DLL) |

Consulte el [README principal](../../README.md) para detalles completos de uso y compilación.
