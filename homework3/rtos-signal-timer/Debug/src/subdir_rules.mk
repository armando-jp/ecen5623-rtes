################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
src/%.obj: ../src/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccs1110/ccs/tools/compiler/ti-cgt-arm_18.12.8.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -O0 --opt_for_speed=0 --include_path="D:/ti/FreeRTOSv202112.00/FreeRTOS/Source/include" --include_path="D:/ti/FreeRTOSv202112.00/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="D:/ti/arman/workspace_v11/rtos-signal-timer" --include_path="D:/ti/TivaWare_C_Series-2.2.0.295/examples/boards/ek-tm4c1294xl" --include_path="D:/ti/TivaWare_C_Series-2.2.0.295" --include_path="D:/ti/TivaWare_C_Series-2.2.0.295/utils" --include_path="D:/ti/ccs1110/ccs/tools/compiler/ti-cgt-arm_18.12.8.LTS/include" --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA1 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --ual --preproc_with_compile --preproc_dependency="src/$(basename $(<F)).d_raw" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


