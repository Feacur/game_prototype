@echo off
chcp 65001 > nul

rem |> REROUTE
call :%*

rem |> FUNCTIONS
goto :eof

:get_millis
	rem @note: `time` is locale-dependent and can have leading space
	set local_time=%time: =%

	rem @note: the format could have been customized
	for /F "tokens=1-3 delims=0123456789" %%a in ("%local_time%") do ( set "delims=%%a%%b%%c" )

	rem @note: hours, minutes, seconds, centiseconds
	for /F "tokens=1-4 delims=%delims%" %%a in ("%local_time%") do (
		rem @note: force a leading zero, which anyway occur some of the times;
		rem        later, they'll be erased by prepending with 1 and subtracting 100
		if 1%%a lss 20 ( set "hh=0%%a" ) else ( set "hh=%%a" )
		if 1%%b lss 20 ( set "mm=0%%b" ) else ( set "mm=%%b" )
		if 1%%c lss 20 ( set "ss=0%%c" ) else ( set "ss=%%c" )
		if 1%%d lss 20 ( set "cc=0%%d" ) else ( set "cc=%%d" )
	)

	set /a millis = 0
	set /a millis = millis *  24 + "(1%hh% - 100)"
	set /a millis = millis *  60 + "(1%mm% - 100)"
	set /a millis = millis *  60 + "(1%ss% - 100)"
	set /a millis = millis * 100 + "(1%cc% - 100)"
	set /a millis = millis * 10

	rem return: argument `%~1`
	set "%~1=%millis%"
goto :eof

:report_millis_delta
	set /a delta = (86400000 + %~3 - %~2) %% 86400000
	echo.%~1 %delta% %~4
goto :eof

:check_executable_online
	tasklist -fi "IMAGENAME eq %project%.exe" -nh | findstr %project%.exe > nul
	rem return: `errorlevel`
goto :eof
