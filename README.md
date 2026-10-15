# Pokke - Monster Hunter Freedom Unite (PC Native Port)

<p align="center">
  <img src="https://raw.githubusercontent.com/Kiridor123/MHFU-PC/main/docs/images/logo_placeholder.svg" alt="Pokke Logo" width="520" onerror="this.style.display='none'"/>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-b98335?style=flat-square&amp;labelColor=2a1a22" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue?style=flat-square&amp;labelColor=2a1a22" alt="Platform: Windows | Linux">
  <img src="https://img.shields.io/badge/vulkan-1.2+-red?style=flat-square&amp;labelColor=2a1a22" alt="Vulkan: 1.2+">
  <img src="https://img.shields.io/badge/framerate-30%20%7C%2060%20%7C%20120%20FPS-green?style=flat-square&amp;labelColor=2a1a22" alt="Framerate: 30 / 60 / 120 FPS">
</p>

**Pokke** es un port nativo para PC de **Monster Hunter Freedom Unite** (`ULUS-10391`, v1.01) creado mediante **recompilación estática anticipada (AOT)**: el código máquina MIPS Allegrex original de PSP es traducido a código C++ nativo y compilado directamente para tu procesador, ejecutándose sobre una reimplementación modular de bajo nivel con **Vulkan** y **SDL3**.

> **No es un emulador tradicional**: No hay intérpretes lentos ni compiladores JIT en el núcleo.
> **No es una descompilación**: El código fuente original de Capcom no se encuentra aquí.

---

## Reconocimientos y Agradecimientos

Este proyecto fue posible gracias a la asombrosa arquitectura, investigación y desarrollo del proyecto **[Yakumo](https://github.com/TeamGDB/Yakumo)** creado por **TeamGDB**, así como al framework de recompilación estática **[PSPRecomp](https://github.com/TeamGDB/Yakumo)**. 

Queremos expresar nuestro más sincero agradecimiento y crédito a todos los contribuidores de **Yakumo** y **PSPRecomp** por haber sentado las bases que hicieron viable este port para la aldea de Pokke.

---

## Política "Bring Your Own Copy" y Aviso Legal

> [!IMPORTANT]
> **Este repositorio NO CONTIENE ningún archivo con derechos de autor, código binario del juego, texturas, modelos 3D, música, cinemáticas ni imágenes de disco (.iso).**

- **Pokke** es un proyecto de software libre, independiente y sin fines de lucro. No está afiliado, autorizado, respaldado ni patrocinado de ninguna manera por **CAPCOM Co., Ltd.** ni por **Sony Interactive Entertainment Inc.**
- *Monster Hunter*, *Monster Hunter Freedom Unite*, *CAPCOM*, *PlayStation* y *PSP* son marcas comerciales y derechos reservados de sus respectivos propietarios.
- Para jugar o compilar, **cada usuario debe proporcionar su propia copia legalmente adquirida** del juego en formato `.iso` (*Monster Hunter Freedom Unite*, edición US `ULUS-10391`, v1.01).
- El software incluye un verificador de integridad SHA-256 para asegurar que únicamente se procesen copias legítimas y sin modificar.
- Los usuarios son enteramente responsables de extraer y utilizar su copia de acuerdo con las leyes aplicables en su territorio. Este proyecto rechaza la piratería.
- El código original de este repositorio se distribuye bajo la **Licencia MIT**.

---

## Características Principales

- **Ejecutable Nativo Ligero (`Pokke.exe`)**:
  - Un único binario `.exe` nativo sin necesidad de scripts `.bat` ni lanzadores externos.
- **Launcher y Asistente Gráfico Integrado**:
  - En el primer inicio (o al ejecutar `Pokke.exe --launcher`), una interfaz gráfica intuitiva te permite seleccionar tu archivo `.iso` mediante explorador de archivos o arrastrar y soltar (Drag & Drop).
  - Permite escoger si copiar el ISO al perfil de usuario o utilizarlo donde está (*in-place*).
  - Permite elegir la ubicación de guardado (por defecto en `%APPDATA%/Pokke/ULUS10391/` o modo portátil en `./data/`).
- **Menú de Configuración en el Juego con tecla `F1`**:
  - Presiona **`F1`** (o `Esc` / `L3+R3` en mando) en cualquier momento de la partida para abrir el menú en tiempo real.
- **Resolución Arbitraria y Escalado Gráfico**:
  - Escalado interno desde **1x PSP (480×272)** hasta **6x (4K / 3840×2160)**.
  - Modos de ventana libre, pantalla completa exclusiva y soporte para monitores ultra-panorámicos (Ultrawide).
  - Filtros de pantalla: Bilineal suave o píxel perfecto (Sharp).
- **Tasa de Cuadros a 60 y 120 FPS (Sin acelerar el juego)**:
  - Soporte de interpolación de fotogramas a **30, 45, 60, 90, 120 FPS** o sincronizado a la tasa de refresco de tu monitor (*Match display*).
  - La lógica y física de la simulación de caza se mantienen exactamente a 30 FPS originales, logrando una fluidez visual moderna sin alterar la velocidad del juego.
- **Soporte Moderno de Mandos y Doble Joystick (Cámara Analógica)**:
  - Detección automática de mandos de Xbox, PlayStation (DualShock / DualSense), Nintendo Switch Pro y genéricos.
  - **Control de cámara con el joystick derecho**: Permite girar e inclinar la vista del cazador de forma fluida y natural sin necesidad de usar "la garra" de PSP.
  - Editor visual para reasignar cualquier tecla o botón de PSP: Cruceta direccional (D-Pad), Joystick analógico, Botones de acción (Cuadrado □, Triángulo △, Círculo ○, Equis ✕), Gatillos L y R, Start y Select.
- **Compatibilidad Total de Partidas Guardadas**:
  - Lee y escribe partidas en el formato estándar de PSP (`PSP/SAVEDATA/ULUS10391`). Puedes transferir tus partidas desde y hacia una consola PSP o emuladores sin conversión.
- **Arquitectura de Overlays Dinámicos**:
  - 14 módulos de modos de juego (`game_task`, `lobby_task`, `download_task`, `gallery_task`, `option_task`, etc.) compilados como DLLs nativas de alto rendimiento.

---

## Cómo Jugar

### Inicio Rápido:
1. Descarga el ejecutable `Pokke.exe` (junto con las DLLs requeridas) desde la sección de **Releases**.
2. Haz doble clic en **`Pokke.exe`**.
3. En la pantalla de bienvenida, selecciona tu archivo `Monster Hunter Freedom Unite.iso` (o arrástralo sobre la ventana).
4. ¡A cazar! Durante la partida, presiona **`F1`** para ajustar gráficos, fotogramas o controles.

### Opciones de Línea de Comandos:
```bash
# Abrir el lanzador / asistente gráfico para cambiar la ISO o ajustes
Pokke.exe --launcher

# Modo portátil (guarda configuraciones y partidas en ./data/ junto al ejecutable)
Pokke.exe --portable

# Especificar una carpeta personalizada para datos y partidas
Pokke.exe --data-dir "D:/MisPartidas/MHFU"

# Instalar silenciosamente desde un archivo ISO
Pokke.exe --install "C:/Juegos/MHFU.iso" --in-place
```

---

## Compilación desde el Código Fuente

### Requisitos (Windows con MSYS2 UCRT64):
- **Compilador C++20**: GCC 14+ / Clang 17+ (`mingw-w64-ucrt-x86_64-gcc`)
- **Herramientas de construcción**: CMake 3.25+ y Ninja (`mingw-w64-ucrt-x86_64-cmake`, `mingw-w64-ucrt-x86_64-ninja`)
- **Bibliotecas**: SDL3, Vulkan Loader & Headers, Glslang (`mingw-w64-ucrt-x86_64-sdl3`, `mingw-w64-ucrt-x86_64-vulkan-loader`, `mingw-w64-ucrt-x86_64-vulkan-headers`, `mingw-w64-ucrt-x86_64-glslang`)
- **Python 3.10+**

### Instrucciones de Compilación:
```bash
# 1. Clonar el repositorio
git clone https://github.com/Kiridor123/MHFU-PC.git
cd MHFU-PC

# 2. Preparar el juego con tu copia legal de MHFU
python profiles/mhfu/scripts/prepare_game.py "Monster Hunter Freedom Unite.iso"

# 3. Generar el código C++ recompilado (AOT)
python profiles/mhfu/scripts/generate.py

# 4. Compilar los overlays de juego dinámicos
python profiles/mhfu/scripts/build_mode_overlays.py

# 5. Configurar y compilar Pokke.exe
cmake -B out/mhfu -S . -G Ninja -DPSPRECOMP_PROFILE=mhfu
cmake --build out/mhfu --target Pokke

# El ejecutable Pokke.exe estará listo en out/mhfu/bin/Pokke.exe
```

---

## Licencia

Este proyecto está licenciado bajo la **Licencia MIT** — consulta el archivo [LICENSE](LICENSE) para más detalles.

- Copyright (c) 2026 PSPRecomp contributors
- Copyright (c) 2026 Yakumo contributors
- Copyright (c) 2026 Pokke contributors
