#!/usr/bin/env bash
SBER=${SBER:-$HOME/sberportal}   # рабочая копия архива (OTA, разделы, kernel_config.txt)
# Provisioning внутри Lima (Ubuntu 24.04 arm64): билд-зависимости + x86_64 runtime для Rosetta + тулчейн.
set -euxo pipefail
export DEBIAN_FRONTEND=noninteractive

# --- 1) СНАЧАЛА чиним sources: arm64 -> ports, amd64 -> archive ---
sudo rm -f /etc/apt/sources.list.d/ubuntu.sources /etc/apt/sources.list
sudo tee /etc/apt/sources.list.d/arm64.sources >/dev/null <<'EOF'
Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports
Suites: noble noble-updates noble-backports noble-security
Components: main restricted universe multiverse
Architectures: arm64
EOF
sudo tee /etc/apt/sources.list.d/amd64.list >/dev/null <<'EOF'
deb [arch=amd64] http://archive.ubuntu.com/ubuntu noble main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu noble-updates main restricted universe multiverse
EOF
sudo dpkg --add-architecture amd64 || true

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential bc bison flex libssl-dev libelf-dev \
  git python3 rsync xz-utils wget ca-certificates file jq \
  libc6:amd64 libstdc++6:amd64 libgcc-s1:amd64 zlib1g:amd64

# --- 2) тулчейн в гостевую ФС ---
sudo rm -rf /opt/tc
sudo cp -a ${SBER}/build/tc /opt/tc

echo "--- rosetta mount ---"; ls -la /media/rosetta 2>/dev/null || true
echo "--- toolchain check (x86_64 bin via Rosetta) ---"
/opt/tc/bin/aarch64-linux-gnu-gcc --version | head -1
echo OK
