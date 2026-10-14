# Monster Hunter Freedom Unite (PC Native Port)

Port nativo para PC de **Monster Hunter Freedom Unite** (`ULUS-10391`, v1.01) mediante recompilación estática anticipada (AOT - Ahead-Of-Time Recompilation) usando el framework **PSPRecomp**, renderizado por hardware con **Vulkan** y gestión de entrada y ventana con **SDL3**.

A diferencia de un emulador tradicional, el código de máquina MIPS Allegrex original del juego se traduce directamente a instrucciones nativas C++ y se compila para la máquina anfitriona (x86_64 / ARM64), logrando tiempos de carga instantáneos, soporte nativo de doble stick para la cámara, tasas de refresco altas (60+ FPS con interpolación de frames), resolución arbitraria y uso mínimo de CPU y memoria.

---

## Política "Bring Your Own Copy" y Aviso Legal

Este repositorio **NO CONTIENE ningún archivo con derechos de autor, código binario del juego, texturas, modelos 3D, música o cinemáticas**.

- Cada usuario debe proveer su propia copia legalmente adquirida de *Monster Hunter Freedom Unite* en formato `.iso` (versión US `ULUS-10391`, v1.01).
- El ejecutable `MHFU.exe` es ligero y carga directamente los recursos y cinemáticas desde la copia del usuario.
- Los archivos `.iso`, ejecutables desencriptados `.ELF`, directorios de juego y código generado por la recompilación están estrictamente excluidos por `.gitignore`.
- Proyecto de interoperabilidad independiente sin fines de lucro, no afiliado ni respaldado por CAPCOM Co., Ltd. ni Sony Interactive Entertainment.

---

## Características del Port Nativo

| Característica | Estado | Detalle |
| :--- | :--- | :--- |
| **Arranque y Ejecución** | ✅ Funcional | Carga nativa ultrarrápida desde ISO o carpeta local. |
| **Gráficos Vulkan** | ✅ Funcional | Renderizado acelerado por GPU, swapchain moderno, interpolación de cuadros. |
| **Audio Nativo** | ✅ Funcional | Capcom Sound Engine CORE v2.0 nativo con decodificación ATRAC3 y audio estéreo 44.1 kHz. |
| **Controles Modernos** | ✅ Funcional | Soporte nativo para mando con segundo stick analógico para cámara libre y teclado/ratón. |
| **Sistema de Overlays** | ✅ Funcional | 298 módulos dinámicos cargados como bibliotecas nativas `.dll` / `.so`. |
| **Partidas Guardadas** | ✅ Funcional | Compatible al 100% con saves originales de PSP (`ULUS10391`). |

---

## Requisitos de Compilación

### Windows (MSYS2 UCRT64 recomendado)
- **CMake** 3.25 o superior
- **GCC / Clang** con soporte para C++20 (`mingw-w64-ucrt-x86_64-gcc`)
- **Ninja** (`mingw-w64-ucrt-x86_64-ninja`)
- **SDL3** (`mingw-w64-ucrt-x86_64-sdl3`)
- **Vulkan Loader & Headers** (`mingw-w64-ucrt-x86_64-vulkan-loader`, `mingw-w64-ucrt-x86_64-vulkan-headers`)
- **Glslang** (`mingw-w64-ucrt-x86_64-glslang`)
- **Python 3.10+**

En MSYS2 UCRT64 se instalan con:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sdl3 \
          mingw-w64-ucrt-x86_64-vulkan-loader mingw-w64-ucrt-x86_64-vulkan-headers \
          mingw-w64-ucrt-x86_64-glslang python
```

---

## Guía Paso a Paso: De la ISO al Ejecutable Nativo

### 1. Preparar el juego con tu ISO
Coloca tu ISO legalmente adquirida de *Monster Hunter Freedom Unite* y ejecuta:
```bash
python profiles/mhfu/scripts/prepare_game.py "Monster Hunter Freedom Unite.iso"
```
Este script:
- Valida la integridad del archivo y el Disc ID (`ULUS-10391`).
- Extrae el ejecutable `BOOT.ELF` desencriptado a `profiles/mhfu/game/BOOT.ELF`.
- Vincula `disc.iso` para permitir streaming de audio y cinemáticas durante la partida.

### 2. Generar el código C++ reensamblado (AOT Recompilation)
```bash
python profiles/mhfu/scripts/generate.py
```
Traduce el 100% de las instrucciones MIPS Allegrex de `BOOT.ELF` en 68 unidades de compilación C++ en `profiles/mhfu/generated/`.

### 3. Configurar y compilar el ejecutable principal
```bash
cmake -B out/mhfu -S . -G Ninja -DPSPRECOMP_PROFILE=mhfu
cmake --build out/mhfu --target MHFU
```
El ejecutable nativo resultante se encontrará en `out/mhfu/bin/MHFU.exe`.

### 4. Compilar los overlays de juego (Opcional pero recomendado)
Para compilar los módulos de modo (`demo_task`, `lobby_task`, `game_task`, `game_sub`, etc.) a bibliotecas dinámicas nativas:
```bash
python profiles/mhfu/scripts/build_mode_overlays.py
```
*(Nota: Si un overlay no ha sido precompilado, el motor incluye un intérprete Allegrex de respaldo para ejecutarlo sin interrupciones).*

---

## Cómo Ejecutar el Juego

### Modo Instalador Rápido (Recomendado para usuarios)
Si tienes el ejecutable compilado y tu ISO:
```bash
MHFU.exe --install "Monster Hunter Freedom Unite.iso"
```
Instala los datos de forma limpia en el perfil de usuario y arranca directamente.

### Modo Portable
Para mantener configuraciones y partidas en la misma carpeta del ejecutable:
```bash
MHFU.exe --portable profiles/mhfu/game
```

---

## Estructura de la Memoria y Overlays

| Región | Dirección | Tamaño / Uso |
| :--- | :--- | :--- |
| **Base Load** | `0x08804000` | Código principal de `BOOT.ELF` (64 MiB RAM) |
| **Entry Point** | `0x08906A54` | Punto de entrada del juego |
| **Global Pointer** | `0x089D3BA0` | Registro `$gp` de Allegrex |
| **Slot 1 Overlays** | `0x09A5F300` | 12 overlays de modos de juego (`game_task`, `lobby_task`, etc.) |
| **Slot 2 Overlays** | `0x09C19100` | 2 sub-rutinas mayores (`demo_sub`, `game_sub`) |
| **Slot 3 Overlays** | `0x09D1A280` | 17 módulos de IA y monstruos (`em01` a `em17`) |
| **Slot 4 Overlays** | `0x09D63100` | 267 definiciones de misiones y áreas (`stage000` a `stage266`) |

---

## Controles por Defecto

- **Palanca Izquierda / WASD:** Movimiento del cazador
- **Palanca Derecha / Ratón:** Cámara analógica libre
- **Botón Sur / K / Barra Espaciadora:** Rodar / Esquivar
- **Botón Este / L:** Ataque secundario / Desenvainar
- **Botón Oeste / J:** Ataque principal
- **Botón Norte / I:** Usar objeto / Guardar arma
- **Gatillos L / R:** Correr / Bloqueo
- **Start / Enter:** Menú de pausa
- **Select / Tab:** Mapa
