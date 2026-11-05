#!/bin/sh
# Remove the experimental setup and restore saved CUPS configuration.
set -eu

STATE=/overlay/lbp810-printkit
[ "$(id -u)" = 0 ] || { echo 'Нужен root' >&2; exit 1; }
[ -d "$STATE" ] || { echo 'Состояние установки не найдено'; exit 1; }

if command -v lpadmin >/dev/null 2>&1; then
    lpadmin -x LBP810 2>/dev/null || true
fi

echo 'Останавливаю экспериментальный сервер...'
/etc/init.d/cupsd stop 2>/dev/null || true
/etc/init.d/cupsd disable 2>/dev/null || true

if [ -f "$STATE/new-packages.txt" ]; then
    set --
    while IFS= read -r name; do
        case "$name" in
            ''|*[!a-zA-Z0-9._+-]*) continue ;;
        esac
        set -- "$@" "$name"
    done < "$STATE/new-packages.txt"
    if [ "$#" -gt 0 ]; then
        echo 'Удаляю пакеты, которых не было до установки...'
        apk del --no-network "$@" || echo 'Не все пакеты удалились; проверь apk list --installed' >&2
    fi
fi

if [ -d "$STATE/original/cups" ]; then
    echo 'Возвращаю прежнюю конфигурацию CUPS...'
    mkdir -p /etc/cups
    cp -a "$STATE/original/cups/." /etc/cups/
fi
if [ -f "$STATE/original/cupsd.init" ]; then
    cp -a "$STATE/original/cupsd.init" /etc/init.d/cupsd
fi

echo 'Конфигурационные файлы/распечатки, оставшиеся от APK, не удаляю автоматически.'
echo "Резервная копия сохранена в $STATE/original."
echo 'Откат выполнен в пределах зарегистрированных изменений.'
