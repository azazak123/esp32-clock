idf_component_register(SRCS "main.c" "lcd.c" "ui.c" "bsec_iaq.c" "kitty_gif.c" "sensors_bme680.c" "dashboard.c" "net_mgr.c"
                    INCLUDE_DIRS ""
                    REQUIRES esp_psram nvs_flash wpa_supplicant esp_wifi bsec2
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
