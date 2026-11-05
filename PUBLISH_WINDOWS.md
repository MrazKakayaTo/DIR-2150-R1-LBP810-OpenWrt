# Публикация проекта на GitHub с Windows

Эта папка — готовое содержимое пустого публичного репозитория:

https://github.com/MrazKakayaTo/DIR-2150-R1-LBP810-OpenWrt

**Не изменяйте бинарные APK и файл `DIR2150-LBP810-printkit.tar.gz` перед загрузкой.** Исходный архив уже прошёл тест установки на DIR-2150 R1 и его SHA-256 указан в README.

## Публикация

1. Установите **Git for Windows**, если он ещё не установлен: https://git-scm.com/downloads/win
2. Распакуйте ZIP так, чтобы получилась папка `DIR-2150-R1-LBP810-OpenWrt`.
3. Откройте в этой папке PowerShell (через Проводник: адресная строка → `powershell`).
4. Проверьте `git --version` и затем последовательно выполните:

   ```powershell
   git init
   git branch -M main
   git add .
   git commit -m "Initial beta: DIR-2150 R1 Canon LBP-810 IPP print server"
   git remote add origin https://github.com/MrazKakayaTo/DIR-2150-R1-LBP810-OpenWrt.git
   git push -u origin main
   ```

GitHub может запросить авторизацию в браузере через Git Credential Manager. **Не отправляйте пароль или токен в чат.**

Если Git сообщает, что имя автора не настроено, задайте **свои** значения и повторите `git commit`:

```powershell
git config --global user.name "ВАШЕ_ИМЯ_НА_GITHUB"
git config --global user.email "ВАША_ПОЧТА_ДЛЯ_GIT"
git commit -m "Initial beta: DIR-2150 R1 Canon LBP-810 IPP print server"
```

Если репозиторий успели наполнить через GitHub.com (например, создали README), **не делайте `push --force`**. Сначала разберите различие веток или начните с клонирования существующего репозитория.

## После публикации

Можно создать тег `v1.0.0-beta` через веб-интерфейс GitHub и прикрепить проверенный архив `DIR2150-LBP810-printkit.tar.gz` к Release. Сам архив **уже лежит в репозитории**, поэтому это необязательный шаг для первого коммита.
