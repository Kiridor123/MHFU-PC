#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mhp3rd::ui {

inline bool is_spanish() {
    static const bool es = [] {
        if (const char *env = std::getenv("MHP3RD_UI_LANG")) {
            return std::strncmp(env, "es", 2) == 0;
        }
        if (const char *lang = std::getenv("LANG")) {
            return std::strncmp(lang, "es", 2) == 0;
        }
        return false;
    }();
    return es;
}

inline const char *tr(const char *text) {
    if (!text || text[0] == '\0') return text;
    if (!is_spanish()) return text;

    static const std::unordered_map<std::string_view, const char *> kDict = {
        // Tabs
        {"Video", "Vídeo"},
        {"Audio", "Audio"},
        {"Controls", "Controles"},
        {"Network", "Red"},
        {"QoL", "QoL"},
        {"Mods", "Mods"},
        {"System", "Sistema"},
        {"Debug", "Depuración"},

        // Panel states
        {"Paused", "En pausa"},
        {"Running", "En ejecución"},
        {"Setup", "Configuración"},

        // Section headers
        {"Display Settings:", "Ajustes de pantalla:"},
        {"Textures:", "Texturas:"},
        {"Presentation:", "Presentación:"},
        {"Frame Rate:", "Tasa de fotogramas:"},
        {"Loading:", "Carga:"},
        {"Volume:", "Volumen:"},
        {"Presets:", "Perfiles de control:"},
        {"Camera:", "Cámara:"},
        {"Aiming:", "Apuntado:"},
        {"Mouse:", "Ratón:"},
        {"Touch Controls:", "Controles táctiles:"},
        {"Multiplayer:", "Multijugador:"},
        {"Recent Servers:", "Servidores recientes:"},
        {"Appearance:", "Apariencia:"},
        {"Installed Mods:", "Mods instalados:"},
        {"Hunter Name:", "Nombre del cazador:"},
        {"Game:", "Juego:"},
        {"Game Data:", "Datos del juego:"},
        {"Text:", "Texto:"},
        {"Saves:", "Partidas guardadas:"},
        {"About:", "Acerca de:"},

        // Video tab options
        {"Resolution", "Resolución interna"},
        {"Window Mode", "Modo de ventana"},
        {"Window Resolution", "Resolución de ventana"},
        {"Aspect Ratio", "Relación de aspecto"},
        {"Scaling Filter", "Filtro de escalado"},
        {"Texture Filter", "Filtro de texturas"},
        {"UI Textures", "Texturas de interfaz (HUD)"},
        {"Texture Pack", "Paquete de texturas"},
        {"Choose Texture Pack…", "Elegir paquete de texturas…"},
        {"Presentation Mode", "Sincronización (VSync)"},
        {"Performance Display", "Información de rendimiento"},
        {"Frame Rate", "Tasa de fotogramas"},
        {"Auto Frame Rate", "Tasa de fotogramas automática"},
        {"Fast Loading", "Carga rápida"},
        {"Fast Forward Speed", "Velocidad de avance rápido"},
        {"Fast Forward Action", "Modo de avance rápido"},

        // Audio tab options
        {"Volume", "Volumen maestro"},
        {"Mute", "Silenciar todo"},
        {"Mute in Background", "Silenciar en segundo plano"},

        // Controls tab options
        {"Preset", "Perfil activo"},
        {"Save as New Preset…", "Guardar como nuevo perfil…"},
        {"Rebind Controls…", "Reasignar controles…"},
        {"Controllers…", "Mandos conectados…"},
        {"Analog Camera", "Cámara analógica"},
        {"Camera Sensitivity", "Sensibilidad de cámara"},
        {"Invert Camera Horizontal", "Invertir cámara horizontal"},
        {"Invert Camera Vertical", "Invertir cámara vertical"},
        {"Aiming Sensitivity", "Sensibilidad de apuntado"},
        {"Target Lock-on", "Fijar objetivo (Lock-on)"},
        {"Mouse Control", "Control con ratón"},
        {"Mouse Sensitivity", "Sensibilidad del ratón"},
        {"Touch Controls", "Controles táctiles"},
        {"Touch Layout", "Distribución táctil"},
        {"Edit Touch Controls…", "Editar controles táctiles…"},

        // Network tab options
        {"Ad-hoc Networking", "Modo multijugador Ad-hoc"},
        {"Nickname", "Apodo en red"},
        {"Server", "Servidor"},
        {"Host on This Device", "Alojar partida en este equipo"},
        {"Online Chat", "Chat Online"},
        {"Chat Button", "Botón de Chat"},
        {"Show Button", "Mostrar botón"},
        {"Reset Position", "Restablecer posición"},
        {"Open Chat Window…", "Abrir ventana de chat…"},
        {"Chat (PPSSPP):", "Chat en línea (PPSSPP):"},
        {"Chat Message", "Mensaje de Chat"},

        // QoL tab options
        {"Free Camera", "Cámara libre"},
        {"Free Camera Speed", "Velocidad de cámara libre"},
        {"Hide HUD in Free Camera", "Ocultar HUD en cámara libre"},
        {"Layered Armor", "Armadura superpuesta (Transmog)"},

        // Mods tab options
        {"Open Mods Folder", "Abrir carpeta de mods"},
        {"Install Mod…", "Instalar mod…"},

        // System tab options
        {"When the Game Asks for a Name", "Al solicitar nombre"},
        {"Hunter Name", "Nombre"},
        {"Resume", "Reanudar juego"},
        {"Take a Screenshot", "Tomar captura de pantalla"},
        {"Open the Screenshots Folder", "Abrir carpeta de capturas"},
        {"Pause the Game When the Menu Opens", "Pausar juego al abrir el menú"},
        {"Pause During Multiplayer", "Pausar durante multijugador"},
        {"Quit Game", "Salir del juego"},
        {"Open the Data Folder", "Abrir carpeta de datos"},
        {"Set Up Game Data Again…", "Configurar datos del juego de nuevo…"},
        {"Game Text Language", "Idioma del texto del juego"},
        {"Import Translation…", "Importar traducción…"},
        {"Import Save…", "Importar partida…"},
        {"Export Save…", "Exportar partida…"},
        {"Backup Saves", "Crear copia de seguridad"},

        // Values
        {"Fullscreen", "Pantalla completa"},
        {"Window", "Ventana"},
        {"Sharp", "Nítido"},
        {"Smooth", "Suave"},
        {"Original", "Original"},
        {"Stretch", "Estirar"},
        {"Fill", "Llenar"},
        {"On", "Activado"},
        {"Off", "Desactivado"},
        {"Auto", "Automático"},
        {"Compact", "Compacto"},
        {"Detailed", "Detallado"},
        {"VSync", "VSync"},
        {"Mailbox", "Mailbox (Triple búfer)"},
        {"Immediate", "Inmediato (Sin VSync)"},
        {"Sharp bilinear", "Bilineal nítido"},
        {"MMPX", "MMPX (Pixel art HD)"},
        {"Toggle", "Alternar"},
        {"Hold", "Mantener pulsado"},
        {"On-screen keyboard", "Teclado en pantalla"},
        {"Use the name below", "Usar el nombre inferior"},

        // Footer hints and buttons
        {"Select", "Seleccionar"},
        {"Change", "Cambiar"},
        {"Back", "Volver"},
        {"Section", "Sección"},
        {"Rebind", "Reasignar"},
        {"Clear", "Borrar"},
        {"Reset action", "Restablecer acción"},
        {"Add", "Añadir"},
        {"Cancel", "Cancelar"},
        {"Delete", "Eliminar"},
        {"Choose Disc Image…", "Elegir imagen de disco…"},
        {"Copy Disc Image", "Copiar imagen de disco"},
        {"Play from Disc Image", "Jugar desde la imagen"},
        {"Close", "Cerrar"},
        {"Retry", "Reintentar"},
        {"Continue", "Continuar"},

        // Descriptions in footer
        {"Auto draws the game at the window's own size and follows it (at most 1632 lines). ×1 to ×6 draw 272 lines per step, 480×272 times the step unless the aspect ratio is Fill. Higher is sharper and needs more from the GPU.",
         "Auto dibuja a la resolución de ventana. ×1 a ×6 dibujan múltiplos de PSP (4x = 1080p, 6x = 1632p). 4x es el punto óptimo para 1080p nativo a 60 FPS estables."},

        {"Fill the screen, or play in a window you can resize.",
         "Pantalla completa para máxima inmersión y rendimiento, o modo ventana redimensionable."},

        {"The window's size, in multiples of the PSP's screen.",
         "Tamaño de la ventana del juego en múltiplos de la pantalla original de PSP."},

        {"Original keeps the PSP's shape with black bars at the sides or top. Stretch fills the window by stretching the picture. Fill widens (or narrows) the game's view to the window's shape, keeping its height, and keeps the interface in the PSP's proportions.",
         "Original mantiene la proporción 16:9 con barras negras si es necesario. Estirar llena la ventana. Llenar expande la vista 3D panorámica manteniendo la interfaz intacta."},

        {"How the finished picture is scaled to the window: smooth (bilinear) or sharp (nearest pixel).",
         "Cómo se escala la imagen a tu monitor: Suave (filtro bilineal sin pixelado) o Nítido (píxeles directos)."},

        {"How the game's textures are sampled: smooth (bilinear) or sharp (nearest texel).",
         "Filtrado de las texturas 3D del juego: Suave (bilineal) para gráficos más limpios, o Nítido (pixelado original)."},

        {"Above x1, how the game's 2D interface (HUD, menus, icons) is drawn; the 3D world is not touched. Off: as before. Sharp bilinear: crisp pixels at any scale, without blur. MMPX: the textures doubled by a pixel-art upscaler that rounds curves and diagonals, then drawn sharp.",
         "Escalado de la interfaz 2D (HUD, barras de vida, mapa, texto): Desactivado, Bilineal nítido (sin borrosidad), o MMPX (escalador inteligente de pixel art de alta calidad)."},

        {"Load replacement textures from the texture pack folder when one is chosen below.",
         "Carga paquetes de texturas HD de reemplazo (compatibles con PPSSPP) desde la carpeta de texturas."},

        {"How frames reach the screen. VSync waits for the display refresh. Mailbox drops late frames without tearing. Immediate presents at once.",
         "Sincronización con el monitor. Mailbox usa triple búfer rápido sin cortes ni caídas de velocidad. VSync sincroniza con el refresco."},

        {"Shows the frame rate and performance numbers in a corner of the screen.",
         "Muestra la tasa de fotogramas (FPS), velocidad de emulación (SPD) y tiempos de GPU/CPU en pantalla."},

        {"Presents frames through motion interpolation: 30 fps is the game's native rate, 60 fps doubles it smoothly. The game's simulation speed is not changed.",
         "Interpolación de fotogramas fluida: el juego corre internamente a 30 FPS de PSP y se presenta a 60 FPS suaves sin acelerar el juego."},

        {"Fast loading advances the game faster than real time while it loads silently, shortening loading screens.",
         "Carga rápida: acelera la emulación al máximo durante las pantallas de carga en negro, reduciendo drásticamente las esperas."},

        {"Master volume for all game sounds and music.",
         "Volumen maestro para todos los sonidos, efectos y música del juego."},

        {"Mute all audio output.",
         "Silencia todas las salidas de sonido del juego."},

        {"Silence the game while its window does not have focus.",
         "Silencia automáticamente el audio cuando la ventana del juego esté en segundo plano."},

        {"On: the game stops while this menu is open. Off: it keeps running and playing sound behind the menu, which takes all input. Applies the next time the menu opens.",
         "Activado: la partida se pausa mientras este menú está abierto. Desactivado: el juego continúa de fondo."},

        {"Back to the game.",
         "Vuelve a la partida."},

        {"Saves the game's picture behind the menu, at the size it is drawn at, as a PNG in the screenshots folder.",
         "Guarda una captura de pantalla PNG a la resolución interna actual en la carpeta de capturas."},

        {"Show the screenshots in the file manager.",
         "Abre la carpeta de capturas de pantalla en el explorador de archivos del sistema."},

        {"Close Yakumo. Progress since your last save is lost.",
         "Cierra el juego. Se perderá cualquier progreso que no hayas guardado en la partida."},

        {"Show Yakumo's data folder in the file manager.",
         "Abre la carpeta de datos de usuario en el explorador de archivos."},

        {"The language of the game's text itself, with the entries named in a translation file of the translations folder replaced. Original keeps the disc's own text, which is English on a patched image and Japanese on an original one, and any entry a translation leaves out falls back to it. A change applies the next time the game starts.",
         "Idioma del texto del juego. Original mantiene el texto del disco (español en ISO multi-5 europea/americana)."},

        {"Given when the game asks for a name and the on-screen keyboard is off, and to other players when the network nickname is empty. Letters, digits, spaces and simple punctuation.",
         "Nombre que se usará al crear un personaje o en partidas multijugador si el apodo de red está vacío."}
    };

    auto it = kDict.find(text);
    if (it != kDict.end()) return it->second;
    return text;
}

inline std::string tr(std::string text) {
    if (text.empty() || !is_spanish()) return text;
    const char *translated = tr(text.c_str());
    if (translated != text.c_str()) return translated;
    return text;
}

} // namespace mhp3rd::ui
