@echo off
REM HDF5文件合并脚本 (Windows版本)
REM 自动合并当前目录下的HDF5拆分文件

setlocal enabledelayedexpansion

set PART1=PBG08621_pass_6c7986d6_167483a9_0_part1.hdf5
set PART2=PBG08621_pass_6c7986d6_167483a9_0_part2.hdf5
set PART3=PBG08621_pass_6c7986d6_167483a9_0_part3.hdf5
set PART4=PBG08621_pass_6c7986d6_167483a9_0_part4.hdf5
set PART5=PBG08621_pass_6c7986d6_167483a9_0_part5.hdf5
set OUTPUT_FILE=PBG08621_pass_6c7986d6_167483a9_0.hdf5

REM 合并文件
echo 正在合并文件...
copy /b "%PART1%" + "%PART2%" + "%PART3%" + "%PART4%" + "%PART5%" "%OUTPUT_FILE%" >nul

REM 检查合并结果
if %errorlevel% equ 0 (
    echo 合并完成! 输出文件: %OUTPUT_FILE%
    for %%A in ("%OUTPUT_FILE%") do (
        set /a sizeMB=%%~zA/1024/1024
        echo 文件大小: !sizeMB! MB
    )
    echo ✅ 合并成功!
) else (
    echo ❌ 合并失败!
    exit /b 1
)

endlocal
