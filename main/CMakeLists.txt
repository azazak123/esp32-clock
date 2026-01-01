idf_component_register(SRCS "main.c" "lcd.c" "ui.c" "background.c" "bsec_iaq.c"
                    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
