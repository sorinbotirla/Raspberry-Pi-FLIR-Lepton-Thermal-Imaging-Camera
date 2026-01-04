#!/usr/bin/env bash
set -euo pipefail

# Configs
REPO_URL="${REPO_URL:-https://github.com/groupgets/LeptonModule.git}"
REPO_DIRNAME="${REPO_DIRNAME:-LeptonModule}"
APP_SUBDIR="software/raspberrypi_video"
SERVICE_NAME="lepton-view.service"

# Composite / overscan (override by exporting vars before running)
# Example:
#   sudo OVERSCAN_RIGHT=-14 OVERSCAN_TOP=-14 OVERSCAN_BOTTOM=-14 ./setupleptonrpi.sh
SDTV_MODE="${SDTV_MODE:-2}"         # 2=PAL, 0=NTSC
SDTV_ASPECT="${SDTV_ASPECT:-3}"     # 3=16:9, 1=4:3
OVERSCAN_LEFT="${OVERSCAN_LEFT:-0}"
OVERSCAN_RIGHT="${OVERSCAN_RIGHT:--14}"
OVERSCAN_TOP="${OVERSCAN_TOP:--14}"
OVERSCAN_BOTTOM="${OVERSCAN_BOTTOM:--14}"

# App defaults
COLORMAP="${COLORMAP:-3}"          # 1=rainbow, 2=grayscale, 3=ironblack
SPI_SPEED_MHZ="${SPI_SPEED_MHZ:-20}" # 10..30

# Helpers
log() { printf '%s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
have_cmd() { command -v "$1" >/dev/null 2>&1; }

backup_file() {
  local f="$1"
  [[ -f "$f" ]] || return 0
  cp -a "$f" "${f}.bak.$(date +%Y%m%d-%H%M%S)"
}

set_or_append() {
  local key="$1"
  local val="$2"
  local file="$3"
  if grep -qE "^${key}=" "$file"; then
    sed -i "s|^${key}=.*|${key}=${val}|" "$file"
  else
    printf '%s=%s\n' "$key" "$val" >> "$file"
  fi
}

# Permission and OS Requirements
[[ "${EUID:-$(id -u)}" -eq 0 ]] || die "Run as root: sudo $0"
have_cmd apt-get || die "apt-get not found."

# Find target user/home (not installing under /root folder)
TARGET_USER="${SUDO_USER:-}"
if [[ -z "$TARGET_USER" || "$TARGET_USER" == "root" ]]; then
  TARGET_USER="$(awk -F: '$3>=1000 && $1!="nobody"{print $1; exit}' /etc/passwd || true)"
fi
[[ -n "$TARGET_USER" ]] || die "Could not determine a non-root target user."
TARGET_HOME="$(getent passwd "$TARGET_USER" | cut -d: -f6)"
[[ -n "$TARGET_HOME" && -d "$TARGET_HOME" ]] || die "Target home not found for $TARGET_USER: $TARGET_HOME"

INSTALL_DIR="$TARGET_HOME/$REPO_DIRNAME"
APP_DIR="$INSTALL_DIR/$APP_SUBDIR"
BIN_PATH="$APP_DIR/raspberrypi_video"

# Clock skew error fix, well it might happen otherwise
log "[1/10] Time sync..."
systemctl enable --now systemd-timesyncd.service >/dev/null 2>&1 || true

# Package Requirements
log "[2/10] Packages..."
apt-get update
apt-get install -y --no-install-recommends \
  git ca-certificates \
  raspi-config \
  build-essential make g++ pkg-config \
  qt5-qmake qtbase5-dev qtbase5-dev-tools \
  libqt5gui5 libqt5widgets5 libqt5core5a \
  libgles2-mesa-dev \
  fbset

# Interface Requirements (SPI/I2C/rootfs)
log "[3/10] Enable SPI + I2C, expand rootfs..."
raspi-config nonint do_spi 0 || true
raspi-config nonint do_i2c 0 || true
raspi-config nonint do_expand_rootfs || true

# Boot configuration
# Apply to /boot/firmware if present, and also /boot if it contains config/cmdline
apply_boot_edits() {
  local dir="$1"
  local cfg="$dir/config.txt"
  local cmd="$dir/cmdline.txt"
  [[ -f "$cfg" && -f "$cmd" ]] || return 0

  log "  Boot edits in: $dir"
  backup_file "$cfg"
  backup_file "$cmd"

  # Ensure fb0 + linuxfb workable: remove full KMS, enforce FKMS
  sed -i '/^dtoverlay=vc4-kms-v3d$/d' "$cfg"
  grep -qE '^dtoverlay=vc4-fkms-v3d$' "$cfg" || printf '%s\n' 'dtoverlay=vc4-fkms-v3d' >> "$cfg"

  # Composite video (CVBS) + force ignore HDMI hotplug
  set_or_append "enable_tvout" "1" "$cfg"
  set_or_append "hdmi_ignore_hotplug" "1" "$cfg"
  set_or_append "sdtv_mode" "$SDTV_MODE" "$cfg"
  set_or_append "sdtv_aspect" "$SDTV_ASPECT" "$cfg"

  # Overscan (negative values allowed)
  set_or_append "disable_overscan" "0" "$cfg"
  set_or_append "overscan_left" "$OVERSCAN_LEFT" "$cfg"
  set_or_append "overscan_right" "$OVERSCAN_RIGHT" "$cfg"
  set_or_append "overscan_top" "$OVERSCAN_TOP" "$cfg"
  set_or_append "overscan_bottom" "$OVERSCAN_BOTTOM" "$cfg"

  # Match fb0 depth (CVBS commonly 16bpp)
  set_or_append "framebuffer_depth" "16" "$cfg"
  set_or_append "framebuffer_ignore_alpha" "1" "$cfg"

  # Keep 2 framebuffers to reduce fb flicker/blanking on some setups
  set_or_append "max_framebuffers" "2" "$cfg"
  set_or_append "disable_fw_kms_setup" "1" "$cfg"

  # cmdline (single line): move console off tty1 and quiet it
  local line
  line="$(cat "$cmd")"
  line="$(echo "$line" | sed -E 's/\bconsole=tty(0|1)\b//g' | tr -s ' ')"
  echo "$line" | grep -qE '\bconsole=tty3\b' || line="$line console=tty3"
  line="$(echo "$line" | sed -E 's/\bloglevel=[^ ]+\b//g; s/\bquiet\b//g; s/\bconsoleblank=[^ ]+\b//g' | tr -s ' ')"
  echo "$line quiet loglevel=0 consoleblank=0" > "$cmd"
}

log "[4/10] Apply boot edits (embedded settings, no zip)..."
apply_boot_edits "/boot/firmware"
apply_boot_edits "/boot"

# Services and boot journals cleanup, you won't get direct login anymore, but ssh will still work
log "[5/10] Prevent console from reclaiming tty1..."
systemctl disable --now getty@tty1.service >/dev/null 2>&1 || true
systemctl mask getty@tty1.service >/dev/null 2>&1 || true
systemctl disable --now serial-getty@serial0.service >/dev/null 2>&1 || true
systemctl mask serial-getty@serial0.service >/dev/null 2>&1 || true

# Qt runtime dir
log "[6/10] Qt runtime dir..."
mkdir -p /tmp/runtime-root
chown root:root /tmp/runtime-root
chmod 0700 /tmp/runtime-root

# Clone / update repo
log "[7/10] Repo: $REPO_URL"
if [[ -d "$INSTALL_DIR/.git" ]]; then
  sudo -u "$TARGET_USER" git -C "$INSTALL_DIR" fetch --all --prune

  BRANCH="$(sudo -u "$TARGET_USER" git -C "$INSTALL_DIR" rev-parse --abbrev-ref HEAD)"
  # If HEAD is detached or branch lookup fails, fall back to master/main later
  if [[ -z "$BRANCH" || "$BRANCH" == "HEAD" ]]; then
    BRANCH="master"
    sudo -u "$TARGET_USER" git -C "$INSTALL_DIR" show-ref --verify --quiet refs/remotes/origin/main && BRANCH="main"
  fi

  sudo -u "$TARGET_USER" git -C "$INSTALL_DIR" reset --hard "origin/$BRANCH"
  sudo -u "$TARGET_USER" git -C "$INSTALL_DIR" clean -fdx
else
  sudo -u "$TARGET_USER" git clone "$REPO_URL" "$INSTALL_DIR"
fi
[[ -d "$APP_DIR" ]] || die "Expected app dir missing: $APP_DIR"

# Patch app (stable scaling + fullscreen + fb16 safety) or at least "It worked for me :)"
log "[8/10] Write known-good sources + enforce fixes..."

cat > "$APP_DIR/MyLabel.h" <<'EOF'
#ifndef MYLABEL_H
#define MYLABEL_H

#include <QtCore>
#include <QWidget>
#include <QLabel>

class MyLabel : public QLabel {
  Q_OBJECT;

  public:
    MyLabel(QWidget *parent = 0);
    ~MyLabel();

  public slots:
    void setImage(QImage);
};

#endif
EOF

cat > "$APP_DIR/MyLabel.cpp" <<'EOF'
#include "MyLabel.h"

MyLabel::MyLabel(QWidget *parent) : QLabel(parent)
{
}
MyLabel::~MyLabel()
{
}

void MyLabel::setImage(QImage image) {
  QPixmap pixmap = QPixmap::fromImage(image);
  int w = this->width();
  int h = this->height();
  setPixmap(pixmap.scaled(w, h, Qt::IgnoreAspectRatio));
}
EOF

cat > "$APP_DIR/main.cpp" <<'EOF'
#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>

#include "LeptonThread.h"
#include "MyLabel.h"

int main(int argc, char **argv)
{
        int typeColormap = 3;
        int typeLepton = 2;
        int spiSpeed = 20;
        int rangeMin = -1;
        int rangeMax = -1;
        int loglevel = 0;

        for(int i=1; i < argc; i++) {
                if ((strcmp(argv[i], "-cm") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((val == 1) || (val == 2)) { typeColormap = val; i++; }
                } else if ((strcmp(argv[i], "-tl") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if (val == 3) { typeLepton = val; i++; }
                } else if ((strcmp(argv[i], "-ss") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((10 <= val) && (val <= 30)) { spiSpeed = val; i++; }
                } else if ((strcmp(argv[i], "-min") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((0 <= val) && (val <= 65535)) { rangeMin = val; i++; }
                } else if ((strcmp(argv[i], "-max") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((0 <= val) && (val <= 65535)) { rangeMax = val; i++; }
                } else if ((strcmp(argv[i], "-d") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if (0 <= val) { loglevel = val & 0xFF; i++; }
                }
        }

        QApplication a(argc, argv);

        QWidget *w = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(w);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);

        MyLabel *label = new MyLabel(w);
        layout->addWidget(label);

        LeptonThread *thread = new LeptonThread();
        thread->setLogLevel(loglevel);
        thread->useColormap(typeColormap);
        thread->useLepton(typeLepton);
        thread->useSpiSpeedMhz(spiSpeed);
        thread->setAutomaticScalingRange();
        if (0 <= rangeMin) thread->useRangeMinValue(rangeMin);
        if (0 <= rangeMax) thread->useRangeMaxValue(rangeMax);

        QObject::connect(thread, SIGNAL(updateImage(QImage)), label, SLOT(setImage(QImage)));
        thread->start();

        w->showFullScreen();
        return a.exec();
}
EOF

# LeptonThread.cpp patching:
# - fix autoRangeMin init bug
# - force RGB16 for 16bpp fb0
# Not replacing the entire file,
# but enforcing the two load-bearing fixes idempotently.
perl -0777 -pi -e 's/if\s*\(\s*autoRangeMin\s*==\s*true\s*\)\s*\{\s*\n\s*maxValue\s*=\s*65535\s*;/if (autoRangeMin == true) {\n                                minValue = 65535;/g' \
  "$APP_DIR/LeptonThread.cpp" 2>/dev/null || true

sed -i 's/QImage::Format_RGB888/QImage::Format_RGB16/g' \
  "$APP_DIR/LeptonThread.cpp" "$APP_DIR/main.cpp" 2>/dev/null || true

# Build
log "[9/10] Build..."
sudo -u "$TARGET_USER" sh -c "cd '$APP_DIR' && qmake && make -j1"
[[ -x "$BIN_PATH" ]] || die "Build failed: $BIN_PATH not found."

# Create the lepton view service
log "[10/10] Install systemd service..."
cat > "/etc/systemd/system/$SERVICE_NAME" <<EOF
[Unit]
Description=Lepton thermal view (Qt linuxfb)
After=local-fs.target systemd-udev-trigger.service
Wants=systemd-udev-trigger.service

[Service]
Type=simple
WorkingDirectory=$APP_DIR

Environment=QT_QPA_PLATFORM=linuxfb
Environment=QT_QPA_FB=/dev/fb0
Environment=XDG_RUNTIME_DIR=/tmp/runtime-root

StandardInput=tty
StandardOutput=tty
StandardError=tty
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
TTYVTDisallocate=yes

ExecStart=$BIN_PATH -cm $COLORMAP -ss $SPI_SPEED_MHZ
Restart=always
RestartSec=1
StartLimitIntervalSec=0

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable "$SERVICE_NAME"

log "Rebooting to apply boot settings..."
sleep 2

# Reboot and say hello to your new FLIR Cam
reboot
