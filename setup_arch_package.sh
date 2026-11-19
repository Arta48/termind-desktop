#!/bin/bash
set -e

if [[ "$(uname)" != "Linux" ]] || ! command -v pacman > /dev/null; then
    echo "This script can only be run on Arch-based Linux!"
    exit 1
fi

# Request sudo privileges upfront
sudo echo > /dev/null

if [[ ! -f build/termind-desktop ]]; then
    bash compile.sh
fi

# 1. Prepare clean staging directory
mkdir termind-desktop_pkg
cp build/termind-desktop termind-desktop_pkg
cp assets/icon.png termind-desktop_pkg/icon.png
cp README.md LICENSE termind-desktop_pkg/
cd termind-desktop_pkg

export PACKAGER="Arta <arta@gmail.com>"

# 2. Generate PKGBUILD
cat > PKGBUILD << 'EOF'
# Maintainer: Arta <arta@gmail.com>
pkgname=termind-desktop
pkgver=1.0.0
pkgrel=1
pkgdesc="High-performance native desktop GUI for Termind autonomous AI engineering assistant"
arch=("any")
url="https://github.com/Arta48/termind-desktop"
license=("MIT")
depends=(
    "qt6-base"
    "md4c"
)
optdepends=(
    "termind"
)
makedepends=(
    imagemagick
)
source=(
    "termind-desktop"
    "icon.png"
    "README.md"
    "LICENSE"
)
sha256sums=(
    "SKIP"
    "SKIP"
    "SKIP"
    "SKIP"
)

prepare() {
    cd "${srcdir}"

    _icon="${pkgname//-/}" # terminddesktop

    # Generate icons of different sizes
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        magick icon.png -resize "${size}x${size}" -gravity center -background transparent -extent "${size}x${size}" "icon-${size}.png"
    done

    # Creating a desktop file
    cat > "${pkgname}.desktop" <<_DESKTOP_EOF
#!/usr/bin/env xdg-open
[Desktop Entry]
Type=Application
Name=Termind Desktop
Comment=High-performance native desktop GUI for Termind autonomous AI engineering assistant
Exec=termind-desktop %U
Icon=${_icon}
Terminal=false
Categories=Development;IDE;
StartupWMClass=${pkgname}
Keywords=ai;agent;terminal;cli;developer;assistant;gemini;claude;openai;deepseek;ollama;
_DESKTOP_EOF
}

package() {
    cd "${srcdir}"

    _icon="${pkgname//-/}" # terminddesktop

    # Installing the binary in /opt
    install -Dm755 termind-desktop "${pkgdir}/opt/${pkgname}/termind-desktop"

    install -Dm644 README.md "${pkgdir}/opt/${pkgname}/README.md"

    # Installing license
    install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"

    # Creating a symbolic link in /usr/bin
    install -d "${pkgdir}/usr/bin"
    ln -s /opt/${pkgname}/termind-desktop "${pkgdir}/usr/bin/${pkgname}"

    # Installing Icons
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        install -Dm644 "icon-${size}.png" "${pkgdir}/usr/share/icons/hicolor/${size}x${size}/apps/${_icon}.png"
    done

    # Installing a desktop file
    install -Dm644 "${pkgname}.desktop" "${pkgdir}/usr/share/applications/${pkgname}.desktop"
}
EOF

# 3. Build and install package into the system
makepkg -si --skipinteg --noconfirm

cd .. && rm -rf termind-desktop_pkg

echo "
Done!
"
