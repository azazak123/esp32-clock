idf_component_register(SRCS "main.c" "lcd.c" "ui.c" "bsec_iaq.c" "kitty_gif.c" "sensors_bme680.c" "dashboard.c"
                    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
