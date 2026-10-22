# xfce4-lockkeys-plugin

*[Español abajo](#español)*

---

An Xfce panel plugin that shows the state of **Caps Lock** and **Num Lock** using icons from your icon theme.

### Features

- One indicator for Caps Lock and one for Num Lock (each can be hidden)
- Icons from the GTK icon theme (`input-caps-on`, `input-num-on`, with `input-keyboard-symbolic` as fallback); an inactive key is shown dimmed
- Option to hide each icon while its key is off (the plugin takes no space when both are off)
- Optional desktop notification when a lock key changes
- Icon size follows the panel, or can be set manually
- Works on **X11** and on **Wayland** (experimental):
  - X11: instant updates through the XKB extension
  - Wayland: reads the keyboard LEDs (`/sys/class/leds/*::capslock` and `*::numlock`) four times per second, because a panel does not receive key events from other windows. If your hardware exposes no keyboard LEDs (some laptops and virtual machines), the state only updates while the panel has keyboard focus.

### Installation

#### Dependencies

**Arch Linux (CachyOS, Manjaro, EndeavourOS, Garuda, etc.)**

```bash
sudo pacman -S git meson ninja pkgconf gtk3 xfce4-panel libxfce4ui libxfce4util libnotify libx11
```

**Debian 12 / Debian 13**

```bash
sudo apt install git meson ninja-build pkg-config libgtk-3-dev libxfce4panel-2.0-dev libxfce4ui-2-dev libxfce4util-dev libnotify-dev libx11-dev
```

#### Build & install

```bash
git clone https://github.com/Tantin1/xfce4-lockkeys-plugin.git
cd xfce4-lockkeys-plugin
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
xfce4-panel -r
```

`--prefix=/usr` matters: without it the plugin is installed under `/usr/local` and the panel will not find it.

Then add **Lock Keys** from *Panel → Add New Items*.

### Compatibility

- Xfce 4.18+ on X11
- Xfce 4.20+ on Wayland (tested with labwc and xfwl4)

### License

LGPL 2.1 or later.

### Credits

Developed by [Tantin1](https://github.com/Tantin1).

---

## Español

<a name="español"></a>

Plugin de panel para Xfce que muestra el estado de **Bloq Mayús** y **Bloq Num** con íconos de tu tema.

### Características

- Un indicador para Bloq Mayús y otro para Bloq Num (cada uno se puede ocultar)
- Íconos del tema de íconos de GTK (`input-caps-on`, `input-num-on`, con `input-keyboard-symbolic` como reserva); una tecla inactiva se ve atenuada
- Opción para ocultar cada ícono mientras su tecla está apagada (el plugin no ocupa lugar si las dos están apagadas)
- Notificación de escritorio opcional al cambiar una tecla
- El tamaño del ícono sigue al panel, o se puede fijar a mano
- Funciona en **X11** y en **Wayland** (experimental):
  - X11: se actualiza al instante mediante la extensión XKB
  - Wayland: lee las luces del teclado (`/sys/class/leds/*::capslock` y `*::numlock`) cuatro veces por segundo, porque un panel no recibe las teclas que se presionan en otras ventanas. Si tu equipo no publica las luces del teclado (algunas notebooks y máquinas virtuales), el estado solo se actualiza cuando el panel tiene el foco del teclado.

### Instalación

#### Dependencias

**Arch Linux (CachyOS, Manjaro, EndeavourOS, Garuda, etc.)**

```bash
sudo pacman -S git meson ninja pkgconf gtk3 xfce4-panel libxfce4ui libxfce4util libnotify libx11
```

**Debian 12 / Debian 13**

```bash
sudo apt install git meson ninja-build pkg-config libgtk-3-dev libxfce4panel-2.0-dev libxfce4ui-2-dev libxfce4util-dev libnotify-dev libx11-dev
```

#### Compilación e instalación

```bash
git clone https://github.com/Tantin1/xfce4-lockkeys-plugin.git
cd xfce4-lockkeys-plugin
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
xfce4-panel -r
```

`--prefix=/usr` es importante: sin eso el plugin se instala en `/usr/local` y el panel no lo encuentra.

Después agregá **Lock Keys** desde *Panel → Agregar elementos nuevos*.

### Compatibilidad

- Xfce 4.18+ sobre X11
- Xfce 4.20+ sobre Wayland (probado con labwc y xfwl4)

### Licencia

LGPL 2.1 o posterior.

### Créditos

Desarrollado por [Tantin1](https://github.com/Tantin1).
