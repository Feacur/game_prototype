@echo off
chcp 65001 > nul

rem @note: relies on global variables

rem https://learn.microsoft.com/windows/win32/menurc/resource-compiler
rem https://learn.microsoft.com/windows/win32/sbscs/mt-exe

rem |> REROUTE
call :%*

rem |> FUNCTIONS
goto :eof

:prep
	call %func% get_millis time_prep

	pushd ..
	if not exist bin mkdir bin
	pushd bin

	if not exist temp mkdir temp
	if exist "temp/*.tmp" del "temp\\*.tmp" /q

	if not %arch_mode% == shared (
		echo.%resource_compiler%
		%resource_compiler%
	)
goto :eof

:comp # flag, ext
	call %func% get_millis time_comp
	echo.%compiler%
	echo.%linker%
	if %build_mode% == normal (
		for /f %%v in (%source%.txt) do (
			set object=%%~v
			set object=!object:/=_!
			set object=!object:.c=%~2!
			set object="temp/!object!"
			%compiler% "../%%~v" %~1!object! || ( exit /b 1 )
			set objects=!objects! !object!
		)
	) else if %build_mode% == unity (
		set object="temp/!unity_object!%~2"
		%compiler% "%source%.c" %~1!object! || ( exit /b 1 )
		set objects=!object!
	) else if %build_mode% == unity_link (
		%compiler% "%source%.c" %output% %linker% || ( exit /b 1 )
	) else if %build_mode% == auto (
		for /f %%v in (%source%.txt) do (
			set sources=!sources! "../%%~v"
		)
		%compiler% !sources! %output% %linker% || ( exit /b 1 )
	)
goto :eof

:link
	call %func% get_millis time_link
	if not [!objects!] == [] (
		%linker% !objects! %output% || ( exit /b 1 )
	)
goto :eof

:post
	call %func% get_millis time_post
	if not %arch_mode% == shared (
		echo.%manifest_tool%
		%manifest_tool%
	)
	popd
	popd
goto :eof

:stat
	call %func% get_millis time_stat
	if [%time_link%] == [] ( set time_link=%time_stat% )
	call %func% report_millis_delta "head .." time_zero time_prep
	call %func% report_millis_delta "prep .." time_prep time_comp
	call %func% report_millis_delta "comp .." time_comp time_link
	call %func% report_millis_delta "link .." time_link time_post
	call %func% report_millis_delta "post .." time_post time_stat
goto :eof
