# Guía de Configuración y Rendimiento - Pokke (Monster Hunter Freedom Unite)

Esta guía explica en detalle el funcionamiento de cada ajuste del menú de configuración, cómo optimizar el rendimiento gráfico para tu tarjeta gráfica (AMD Radeon, NVIDIA, Intel o Steam Deck), el funcionamiento de los nuevos controles desacoplados y la gestión de partidas guardadas.

---

## 🎮 Cómo Abrir el Menú de Configuración

Puedes acceder al menú de ajustes en cualquier momento durante la partida o en la aldea de Pokke:
- **Teclado:** Presiona la tecla **`Esc`**.
- **Mando / Gamepad:** Presiona **`L3 + R3`** al mismo tiempo (pulsar ambos sticks hacia adentro) o el botón asignado en el perfil.

El menú está completamente disponible en español y todos los cambios se guardan automáticamente al instante en tu archivo `settings.ini`.

---

## 🖥️ 1. Pestaña de Vídeo (Ajustes Gráficos)

### Resolución Interna (`Resolution`)
Controla la resolución a la que la tarjeta gráfica dibuja los modelos 3D y los escenarios:
- **Auto:** Sigue la resolución actual de la ventana.
- **×1 (480×272):** Resolución nativa de la consola PSP original.
- **×2 (960×544):** Calidad similar a PlayStation Vita.
- **×3 (1440×816):** Ideal para pantallas 720p y consolas portátiles como la Steam Deck (ahorro de batería).
- **×4 (1920×1088) - RECOMENDADO PARA 1080p:** Calidad nativa Full HD nítida. Es la opción ideal para tarjetas gráficas dedicadas como la **AMD Radeon RX 570**, GTX 1060 o superiores, manteniendo 60 FPS fijos y 100% de velocidad.
- **×5 / ×6 (hasta 2880×1632, ~3K):** Para monitores 1440p y 4K con GPUs potentes. *(Nota: En GPUs de gama media, 6x puede saturar el ancho de banda y causar lentitud).*

### Modo de Sincronización (`Presentation Mode`)
Define cómo se envían los fotogramas terminados al monitor:
- **Mailbox (Triple búfer sin retraso) - RECOMENDADO:** Evita que el juego sufra caídas de velocidad de emulación si se pierde un microsegundo en un fotograma complejo. Garantiza fluidez absoluta sin cortes de pantalla (tearing).
- **VSync:** Sincroniza rígidamente con la frecuencia de refresco de la pantalla. Si la GPU tarda más de 16.6 ms en un fotograma puntual, la tasa cae a la mitad (30 Hz), lo que reducía la velocidad de emulación al 66%.
- **Immediate:** Sin sincronización (puede provocar líneas de corte horizontal en pantalla).

### Tasa de Fotogramas e Interpolación (`Frame Rate`)
- El motor del juego original corre internamente a **30 fotogramas por segundo** (físicas, animaciones y lógica del juego).
- Al activar **60 FPS**, Pokke realiza una **interpolación de movimiento suave** entre fotogramas, ofreciendo la suavidad visual de un juego de PC contemporáneo sin alterar ni acelerar la velocidad real de la cacería.

### Carga Rápida (`Fast Loading`)
- **Activado (Recomendado):** Durante las pantallas de carga en negro entre zonas del mapa o al entrar a misiones, el motor acelera la emulación al máximo de tu procesador, haciendo que las pantallas de carga duren apenas medio segundo en lugar de los 4 a 6 segundos de la PSP.

### Relación de Aspecto (`Aspect Ratio`)
- **Original:** Mantiene la proporción clásica 16:9 con barras negras laterales en monitores 16:10 (como Steam Deck) o ultra-panorámicos 21:9.
- **Llenar (Fill):** Expande el ángulo de visión de la cámara 3D para llenar toda la pantalla de forma panorámica sin deformar ni estirar a los personajes ni a los monstruos, manteniendo los menús e interfaz con sus proporciones correctas.
- **Estirar (Stretch):** Estira la imagen 16:9 para cubrir toda la ventana.

### Filtros de Pantalla y Texturas
- **Filtro de escalado (Scaling Filter):** `Suave` (filtrado bilineal para evitar bordes dentados al escalar la ventana) o `Nítido` (píxeles directos).
- **Filtro de texturas (Texture Filter):** `Suave` (suavizado bilineal en las superficies del suelo, rocas y monstruos) o `Nítido` (apariencia original con texturas retro).
- **Texturas de interfaz / HUD (`UI Textures`):**
  - *Desactivado:* Mantiene las texturas 2D en resolución estándar de PSP.
  - *Bilineal nítido:* Hace que las letras, barras de vida y mapa se vean limpios sin desenfoque a resoluciones altas.
  - *MMPX:* Aplica un algoritmo inteligente de escalado de pixel art que redondea contornos e iconos.

---

## 🎯 2. Pestaña de Controles (PC y Android)

### Desacoplamiento de Menús Internos del Juego y Cámara 3D
- **Stick Derecho (o teclas de cámara `I/J/K/L`):** Es el único que controla la rotación de la cámara tridimensional durante la exploración y el combate.
- **Teclas de Flechas / Cruceta (D-Pad):** Funcionan **dentro del juego exclusivamente para interactuar con todos los menús internos** (pantalla de título, menú Start en misión, baúl de objetos, baúl de equipamiento, herrería, tiendas, tablón de misiones, diálogos con NPCs y barra de objetos al mantener `L`). Durante el combate y movimiento libre en 3D, presionar las flechas **no mueve la cámara**.

### Controles Táctiles en Android
- **Joystick Izquierdo Fijo:** Permanece anclado en su posición fija en la esquina inferior izquierda para un control de movimiento preciso y consistente sin desplazarse por la pantalla.
- **Joystick Derecho Flotante:** Se mantiene dinámico en la mitad derecha de la pantalla para girar la cámara cómodamente desde cualquier punto donde apoyes el pulgar derecho.

### Ajustes de Apuntado y Sensibilidad
- **Sensibilidad de cámara:** Ajusta la velocidad de giro de la cámara (valor sugerido: 180 - 200).
- **Sensibilidad de apuntado:** Regula la precisión al usar arcos o ballestas en modo de primera persona.
- **Fijar objetivo (Target Lock-on):** Permite fijar y recentrar la cámara rápidamente hacia el monstruo grande activo presionando el botón `R3` o botón asignado.

---

## 💾 3. Partidas Guardadas y Compatibilidad con PPSSPP

Pokke utiliza el formato estándar y oficial de partidas guardadas de la PSP:
- Ubicación de las partidas en Linux:
  ```
  ~/.local/share/Pokke/ULUS10391/ms0/PSP/SAVEDATA/ULUS10391/
  ```
- **Compatibilidad total con emuladores (PPSSPP):** Puedes copiar tu partida guardada (`MHP2NDG.BIN` y `PARAM.SFO`) desde PPSSPP directamente a esa carpeta, o usar la opción **Importar partida…** desde el menú del juego.
- **Copias de seguridad automáticas:** En la pestaña **Sistema**, la opción **Crear copia de seguridad** genera un duplicado protegido de tu progreso con fecha y hora.

---

## 🔊 4. Audio

- **Volumen maestro:** Regula el volumen general de música ambiental, rugidos de monstruos y efectos sonoros.
- **Silenciar en segundo plano:** Si cambias de ventana a un navegador web o a Discord, el juego enmudece automáticamente su salida de audio hasta que vuelvas a la ventana del juego.

---

## 🌐 5. Multijugador Ad-hoc

Pokke cuenta con soporte nativo para el modo cooperativo de la sala de reunión de la aldea:
- Permite cazar en red local con otros jugadores en PC, Android, Steam Deck o incluso con consolas PSP físicas y emuladores PPSSPP conectados a la misma red o mediante servidores VPN virtuales (ZeroTier, Radmin VPN).
- En la pestaña **Red**, introduce el servidor o dirección IP del anfitrión para unirte a la sala de cazadores.

---

## ⚙️ Preajustes Recomendados (Presets) y Optimizaciones Nativas

El motor gráfico Vulkan de Pokke incluye optimizaciones de nivel nativo activas por defecto en todas las plataformas:
1. **Compactación dispersa de vértices indexados (`O(1)`):** Reduce la decodificación y subida de geometría de ~150 MiB/fotograma a menos de 1 MiB/fotograma en mapas grandes (Jungla, Desierto, Montañas Nevadas).
2. **Descarte de cámara por hardware (`GE BBOX + BJUMP`):** Descarta los bloques del mapa fuera del campo de visión antes de procesarlos.
3. **Lectura de framebuffer bajo demanda:** Elimina las pausas CPU-GPU por fotograma, vital para arquitecturas móviles (MediaTek Mali / Snapdragon Adreno) y gráficas integradas.
4. **Presupuesto de memoria adaptativo UMA/Móvil y shaders sin sobrecarga (`robustBufferAccess off`):** En PCs con gráficos integrados (como Intel Core i5 + Intel HD 620 con 8 GB de RAM) y teléfonos Android, ahorra más de 100 MiB de memoria RAM coherente y elimina comprobaciones redundantes por vértice en la GPU.

### 🚀 Preset 1: Gráficos Dedicados (AMD Radeon RX 570, GTX 1060+) - 1080p 60 FPS
Configuración pensada para máxima fidelidad visual y 100% de velocidad continua en todos los mapas:
- **Resolución interna:** `×4` (1920×1088, Full HD nativo).
- **Modo de sincronización:** `Mailbox`.
- **Tasa de fotogramas:** `60 FPS`.
- **Carga rápida:** `Activado`.
- **Filtro de texturas:** `Suave`.
- **Texturas de interfaz (HUD):** `Bilineal nítido`.

### ⚡ Preset 2: PCs con Gráficos Integrados (Intel Core i5 + Intel HD 620 / 8 GB RAM)
Configuración óptima para portátiles y PCs con memoria compartida:
- **Resolución interna:** `×2` (960×544) o `×3` (1440×816, 720p+).
- **Modo de sincronización:** `Mailbox`.
- **Tasa de fotogramas:** `30 FPS` (velocidad nativa sin gasto extra de interpolación) o `60 FPS`.
- **Texturas de interfaz (HUD):** `Bilineal nítido` o `Desactivado`.

### 📱 Preset 3: Teléfonos Android (MediaTek Dimensity/Helio, Mali, Snapdragon Adreno)
- **Resolución interna:** `×2` (960×544, por defecto) o `×3` (1440×816).
- **Relación de aspecto:** `Llenar` (pantalla completa panorámica).
- **Controles táctiles:** Joystick izquierdo fijo + Joystick derecho flotante.

### 🔋 Preset 4: Steam Deck / Portátiles (Batería Óptima)
- **Resolución interna:** `×3` (1440×816).
- **Relación de aspecto:** `Llenar` (Llena la pantalla 16:10 de 1280×800 de la Steam Deck sin barras negras).
- **Modo de sincronización:** `Mailbox`.
- **Tasa de fotogramas:** `60 FPS`.
