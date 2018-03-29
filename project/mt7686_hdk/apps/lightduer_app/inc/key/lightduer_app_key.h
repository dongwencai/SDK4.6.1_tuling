#ifndef __LIGHTDUER_APP_KEY__
#define __LIGHTDUER_APP_KEY__
#ifdef __cplusplus
extern "C" {

#endif

#define LIGHTDUER_KEY_LONG_PRESS_TIME 20
#define LIGHTDUER_KEY_DEBOUNCE_TIME 1

/**
 *  @brief This enum is the key action.
 */
typedef enum {
    LIGHTDUER_KEY_ACT_NONE,                 /**< Key action: invalid key action. */
    LIGHTDUER_KEY_ACT_PRESS_DOWN,           /**< Key action: press down. */
    LIGHTDUER_KEY_ACT_PRESS_UP,             /**< Key action: press up. */
   	LIGHTDUER_KEY_ACT_LONG_PRESS_DOWN,      /**< Key action: long press down. */
    LIGHTDUER_KEY_ACT_LONG_PRESS_UP,        /**< Key action: long press up. */
} lightduer_app_key_action_t;


/**
 *  @brief This enum is the key value.
 */
typedef enum {
    LIGHTDUER_KEY_NONE = 0,       	/**< Invalid key. */
    LIGHTDUER_KEY_PLAY,       	/**< Play/stop key. */
    LIGHTDUER_KEY_NEXT,       	/**< Next key. */
    LIGHTDUER_KEY_PREV,       	/**< Previous key. */
    LIGHTDUER_KEY_RECORD,   	/**< record key. */
} lightduer_app_key_value_t;

typedef struct {
	bool bKeyPressed;
	uint8_t key_count;
	lightduer_app_key_value_t key_value;
	
} lightduer_app_key_context_t;


typedef struct {
    uint16_t min_adc_value;
	uint16_t max_adc_value;
} adc_vale_t;



void lightduer_app_key_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif

