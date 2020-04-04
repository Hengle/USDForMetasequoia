setlocal
cd /d "%~dp0"
call toolchain.bat
call :DoBuild 470
exit /B 0

:DoBuild
    set MQ_VERSION=%~1
    set MQ_SDK_DIR=%cd%\Externals\mqsdk%MQ_VERSION%\mqsdk
    msbuild USDForMetasequoia.sln /t:Build /p:Configuration=Release /p:Platform=x64 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )
    msbuild USDForMetasequoia.sln /t:Build /p:Configuration=Release /p:Platform=Win32 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )

    set DIST_DIR32="_dist\mqusd_Windows_32bit"
    set CORE_DIR32="%DIST_DIR32%\mqusdCore"
    mkdir "%DIST_DIR32%"
    mkdir "%CORE_DIR32%"
    copy _out\Win32_Release\%MQ_VERSION%\mqabc.dll "%DIST_DIR32%"
    copy _out\Win32_Release\%MQ_VERSION%\mqusd.dll "%DIST_DIR32%"
    copy _out\x64_Release\SceneGraphUSD.dll "%CORE_DIR32%"
    copy _out\x64_Release\SceneGraphUSD.exe "%CORE_DIR32%"
    copy Externals\x64\lib\tbb.dll "%CORE_DIR32%"
    copy Externals\x64\lib\usd_ms.dll "%CORE_DIR32%"
    xcopy /EIY Externals\x64\lib\usd "%CORE_DIR32%\usd"

    set DIST_DIR64="_dist\mqusd_Windows_64bit"
    set CORE_DIR64="%DIST_DIR64%\mqusdCore"
    mkdir "%DIST_DIR64%"
    mkdir "%CORE_DIR64%"
    copy _out\x64_Release\%MQ_VERSION%\mqabc.dll "%DIST_DIR64%"
    copy _out\x64_Release\%MQ_VERSION%\mqusd.dll "%DIST_DIR64%"
    copy _out\x64_Release\SceneGraphUSD.dll "%CORE_DIR64%"
    copy Externals\x64\lib\tbb.dll "%CORE_DIR64%"
    copy Externals\x64\lib\usd_ms.dll "%CORE_DIR64%"
    xcopy /EIY Externals\x64\lib\usd "%CORE_DIR64%\usd"
    exit /B 0
