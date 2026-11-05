#!/bin/sh
# Experimental Canon LBP-810 / DIR-2150 print server install.
# Tested only in an OpenWrt 25.12.5 SDK / offline package test root.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
STATE=/overlay/lbp810-printkit

die() {
    echo "ОШИБКА: $*" >&2
    exit 1
}

check() {
    [ "$(id -u)" = 0 ] || die "запусти от root"
    [ -e /etc/openwrt_release ] || die "это не OpenWrt"
    command -v ubus >/dev/null 2>&1 || die "ubus недоступен"
    command -v jsonfilter >/dev/null 2>&1 || die "jsonfilter недоступен"
    command -v apk >/dev/null 2>&1 || die "apk недоступен"

    BOARD=$(ubus call system board)
    MODEL=$(printf '%s\n' "$BOARD" | jsonfilter -e '@.board_name')
    VER=$(printf '%s\n' "$BOARD" | jsonfilter -e '@.release.version')
    KERNEL=$(uname -r)
    # apk --print-arch reports the generic CPU arch (mipsel), not the
    # OpenWrt package ABI used to resolve mipsel_24kc packages.
    # Read the package architecture configuration instead.
    [ -r /etc/apk/arch ] || die "нет файла /etc/apk/arch"
    ARCH=$(sed -n '1p' /etc/apk/arch)
    [ "$MODEL" = dlink,dir-2150-r1 ] || die "ожидался DIR-2150 R1; получен $MODEL"
    [ "$VER" = 25.12.5 ] || die "ожидался OpenWrt 25.12.5; получен $VER"
    [ "$KERNEL" = 6.12.94 ] || die "ожидалось ядро 6.12.94; получено $KERNEL"
    grep -qx 'mipsel_24kc' /etc/apk/arch || die "в /etc/apk/arch нет mipsel_24kc; содержимое: $(tr '\n' ' ' < /etc/apk/arch)"
    [ -c /dev/usb/lp0 ] || die "принтер не виден как /dev/usb/lp0"

    LAN_IP=$(ubus call network.interface.lan status | jsonfilter -e '@["ipv4-address"][0].address' 2>/dev/null || true)
    [ -n "$LAN_IP" ] || LAN_IP=$(uci -q get network.lan.ipaddr || true)
    case "$LAN_IP" in
        ''|*[!0-9.]* ) die "не найден IPv4-адрес интерфейса LAN" ;;
    esac

    (cd "$HERE" && sha256sum -c SHA256SUMS) || die "проверка файлов не прошла"

    echo "Устройство: $MODEL; OpenWrt $VER; ядро $KERNEL; $ARCH"
    echo "LAN IP: $LAN_IP; принтер USB: /dev/usb/lp0"
    echo "На роутере понадобится приблизительно 13 МБ флеша сверх текущего использования."
    echo "Свободное место:"
    df -h /overlay
}

usage() {
    echo "Использование: sh install.sh --check | --apply"
    exit 2
}

case "${1:-}" in
    --check)
        check
        echo 'Проверка пройдена, никаких изменений не внесено.'
        exit 0
        ;;
    --apply)
        check
        ;;
    *) usage ;;
esac

[ ! -e "$STATE/installed" ] || die "комплект уже был установлен; смотри uninstall.sh"
[ ! -e "$STATE/in-progress" ] || die "есть незавершённая установка; смотри uninstall.sh"
mkdir -p "$STATE/original"
touch "$STATE/in-progress"

echo 'Сохраняю прежние настройки и список пакетов...'
if [ -d /etc/cups ]; then
    cp -a /etc/cups "$STATE/original/cups"
fi
if [ -f /etc/init.d/cupsd ]; then
    cp -a /etc/init.d/cupsd "$STATE/original/cupsd.init"
fi
apk list --installed > "$STATE/packages-before.txt" || true
: > "$STATE/new-packages.txt"
for name in \
    captppd pwgtocaptraster cups cups-client libcups libcupsimage \
    libusb-1.0-0 libstdcpp6 libatomic1 libpthread librt zlib libpng libjpeg-turbo
do
    if ! apk info --exists "$name" >/dev/null 2>&1; then
        printf '%s\n' "$name" >> "$STATE/new-packages.txt"
    fi
done

echo 'Устанавливаю подготовленные APK локально, без сетевых репозиториев...'
apk add --allow-untrusted --no-network "$HERE"/packages/*.apk

echo 'Настраиваю CUPS только на LAN...'
mkdir -p /etc/cups /overlay/lbp810-cups/spool /overlay/lbp810-cups/tmp
mkdir -p /tmp/lbp810-cups/cache /var/run/cups
sed "s/@ROUTER_LAN_IP@/$LAN_IP/g" "$HERE/config/cupsd.conf" > /etc/cups/cupsd.conf
cp "$HERE/config/cups-files.conf" /etc/cups/cups-files.conf
cp "$HERE/config/cupsd.init" /etc/init.d/cupsd
chmod 0644 /etc/cups/cupsd.conf
chmod 0640 /etc/cups/cups-files.conf
chmod 0755 /etc/init.d/cupsd
chmod 0700 /usr/lib/cups/backend/captusb
if [ ! -f "$STATE/original/cups/printers.conf" ]; then
    # The third-party CUPS package ships example USB/LP printers.
    # They are not useful here and must not be advertised via IPP.
    printf '# CUPS printer queues (managed by cupsd)\n' > /etc/cups/printers.conf
fi

echo 'Проверяю синтаксис конфигурации CUPS...'
/usr/sbin/cupsd -t

echo 'Запускаю сервер печати...'
/etc/init.d/cupsd enable
/etc/init.d/cupsd restart

echo 'Определяю USB-устройство через CAPT-бэкенд...'
/usr/lib/cups/backend/captusb > "$STATE/discovery.txt" 2> "$STATE/discovery-errors.txt" || true
DEVICE_URI=$(awk '$1 == "direct" && $2 ~ /^captusb:\/\// {print $2; exit}' "$STATE/discovery.txt")
if [ -z "$DEVICE_URI" ]; then
    echo 'CAPT-бэкенд пока не обнаружил принтер.' >&2
    echo "Смотри $STATE/discovery-errors.txt, logread и README.md." >&2
    echo 'Для отката: sh uninstall.sh' >&2
    exit 1
fi
printf '%s\n' "$DEVICE_URI" > "$STATE/device-uri.txt"

echo 'Создаю очередь печати LBP810...'
lpadmin -p LBP810 -E -v "$DEVICE_URI" \
    -P /usr/share/cups/model/LBP810CAPTPPD.ppd \
    -o printer-is-shared=true
lpadmin -d LBP810
lpstat -v -p LBP810

printf '%s\n' "$LAN_IP" > "$STATE/lan-ip.txt"
touch "$STATE/installed"
rm "$STATE/in-progress"

echo
echo "ГОТОВО: IPP-адрес для Windows: http://$LAN_IP:631/printers/LBP810"
echo 'Драйвер CAPT работает на роутере. Для Windows выбери Microsoft IPP Class Driver.'
echo 'Внимание: реальная печать с LBP-810 ещё не проверена.'
