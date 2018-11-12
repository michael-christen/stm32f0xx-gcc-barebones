#include <stdio.h>
#include <stdint.h>

#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"

#include "inc/stepper.h"
#include "inc/usart.h"
#include "inc/i2c.h"
#include "inc/std_utils.h"
#include "inc/quaternion_filters.h"
#include "inc/mpu_9250.h"
#include "inc/imu.h"
#include "inc/profile.h"

#define BLINK_DELAY_US	(50)

#define RAD_TO_DEG ( 180.0f / M_PI )
#define DEG_TO_RAD ( M_PI / 180.0f )

volatile uint32_t tickUs = 0;


void init() {
	// ---------- SysTick timer -------- //
	if (SysTick_Config(SystemCoreClock / 1000000)) {
		// Capture error
		while (1){};
	}
}


int usart_example(void) {
    usart_configure(9600);
    usart_send_string("HELLO WORLD");
    char c_str[32];
    ftoa(c_str, 13.24567, 7);
    while(1) {
        usart_block_receive_char();
        usart_send_string(c_str);
        usart_send_string("\r\n");
    }
}


int stepper_example(void) {
	init();
    // Configure direction pin/s
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_Init(
        GPIOC,
        &(GPIO_InitTypeDef){
            GPIO_Pin_8 | GPIO_Pin_9,
            GPIO_Mode_OUT,
            GPIO_Speed_2MHz,
            GPIO_OType_PP,
            GPIO_PuPd_NOPULL
        });
    bool forward = true;
    uint32_t last_count = 0;
    stepper_t stepper0 = stepper_init(
        GPIOC,
        GPIO_Pin_9,
        GPIO_Pin_9 | GPIO_Pin_8,
        BLINK_DELAY_US,
        forward,
        tickUs);

    // Timer based configuration
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_Init(
        GPIOA,
        &(GPIO_InitTypeDef){
            GPIO_Pin_8 | GPIO_Pin_9,
            GPIO_Mode_AF,
            GPIO_Speed_50MHz,
            GPIO_OType_PP,
            GPIO_PuPd_UP
        });
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_2);
    /* TIM1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);

    uint16_t frequency = 400;  // Hz

    stepper_set_speed(&stepper0, frequency);

    /* TIM1 counter enable */
    TIM_Cmd(TIM1, ENABLE);
    /* TIM1 Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    uint16_t speeds[] = {
            400,
            800,
            1000,
            2000,
            4000,
            6000,
            4000,
            2000,
            1000,
            800,
            400,
    };
    uint8_t speed_index = 0;
	for(;;) {
        if (tickUs / 1000000 > last_count) {
          last_count += 1;
          forward = !forward;
          // stepper_set_dir(&stepper0, forward);
          stepper_set_speed(&stepper0, speeds[speed_index++]);
          speed_index %= 11;
        }
		__WFI();
	}
	return 0;
}


int i2c_example(void) {
    const uint16_t ARDUINO_SLAVE = (0x08 << 1);
    uint8_t read_buf[255];
    uint8_t write_buf[255];
    write_buf[0] = 0x00;
    write_buf[1] = 0xDE;
    write_buf[2] = 0xAD;
    write_buf[3] = 0xBE;
    write_buf[4] = 0xEF;

    init();
    i2c_configure();
    i2c_send(ARDUINO_SLAVE, 5, write_buf, true);
    write_buf[0] = 0xFF;
    i2c_send(ARDUINO_SLAVE, 1, write_buf, false);
    i2c_receive(ARDUINO_SLAVE, 8, read_buf, true);
    return 0;
}

int delay_profile_example(void) {
    init();
    profile_init();
    for(;;) {
        delay(10);
        profile_toggle();
        delay(100);
        profile_toggle();
        delay(200);
        profile_toggle();
    }
}


// TODO: Update after removing quaternion math...
#define PERIOD 4000  // uS


int16_t constr(int16_t value, int16_t smallest, int16_t biggest) {
    if (value < smallest) {
        return smallest;
    } else if (value > biggest) {
        return biggest;
    } else {
        return value;
    }
}


float constrf(float value, float smallest, float biggest) {
    if (value < smallest) {
        return smallest;
    } else if (value > biggest) {
        return biggest;
    } else {
        return value;
    }
}


int main(void) {
    euler_t angles;
    char c_str[32];

    init();
    usart_configure(9600);
    usart_send_string("Configuring I2C\r\n");
    i2c_configure();
    usart_send_string("Initializing IMU\r\n");
    int imu_init_err = imu_init();
    if (imu_init_err) {
        usart_send_string("Failed to initialize IMU: ");
        itoa(c_str, imu_init_err, 5); usart_send_string(c_str);
        for(;;);
    }
    usart_send_string("Starting loop, retrieving Orientations\r\n");

    // Configure direction pin/s
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_Init(
        GPIOC,
        &(GPIO_InitTypeDef){
            GPIO_Pin_8 | GPIO_Pin_9,
            GPIO_Mode_OUT,
            GPIO_Speed_2MHz,
            GPIO_OType_PP,
            GPIO_PuPd_NOPULL
        });
    bool forward = true;
    stepper_t stepper0 = stepper_init(
        GPIOC,
        GPIO_Pin_9,
        GPIO_Pin_9 | GPIO_Pin_8,
        BLINK_DELAY_US,
        forward,
        tickUs);

    // Timer based configuration
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_Init(
        GPIOA,
        &(GPIO_InitTypeDef){
            GPIO_Pin_8 | GPIO_Pin_9,
            GPIO_Mode_AF,
            GPIO_Speed_50MHz,
            GPIO_OType_PP,
            GPIO_PuPd_UP
        });
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_2);
    /* TIM1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);


    /* TIM1 counter enable */
    TIM_Cmd(TIM1, ENABLE);
    /* TIM1 Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);


    uint32_t last_display = tickUs;
    uint32_t last_loop;


    float roll = 0, pitch = 0, rollAcc = 0, pitchAcc = 0;
    float speed = 0;

    float integral_error = 0;
    float last_pid_error = 0;
    float pid_output = 0;
    const float Kp = 4000.0;
    const float Ki = 0.0;
    const float Kd = 0.0;
    const float MAX_SPEED = 45000;
    const float MIN_SPEED = 230;
    const float MAX_PID_OUTPUT = 4000;
    uint16_t motor_speed = 0;
    for (;;) {
        last_loop = tickUs;
        imu_update_quaternion();

        float value = orientation.ay;

        float pid_error = value;
        integral_error += Ki * pid_error;
        integral_error = constrf(integral_error, -MAX_PID_OUTPUT, MAX_PID_OUTPUT);
        float error_derivative = pid_error - last_pid_error;
        pid_output = Kp * pid_error + integral_error + Kd * error_derivative;

        if (value > 0.75 || value < -0.75) {
            pid_output = 0;
            integral_error = 0;
        }
        last_pid_error = pid_error;
        float speed = (
            constrf(pid_output, -MAX_PID_OUTPUT, MAX_PID_OUTPUT) *
            (MAX_SPEED / MAX_PID_OUTPUT));
        motor_speed = (uint16_t)abs(speed);
        bool dir = (speed < 0);
        stepper_set_dir(&stepper0, dir);
        stepper_set_speed(&stepper0, motor_speed);
        /*
        if (abs(speed) < MIN_SPEED) {
            speed = 0;
        }
        */


        /*
        rollAcc = asin((float)orientation.ax) * RAD_TO_DEG;
        pitchAcc = asin((float)orientation.ay) * RAD_TO_DEG;

        roll -= orientation.gy * DEG_TO_RAD * (1000000.0 / PERIOD);
        pitch += orientation.gx * DEG_TO_RAD * (1000000.0 / PERIOD);

        // NOTE: roll ended up as 0xFFFFFF...
        roll = roll * 0.999 + rollAcc * 0.01;
        pitch = pitch * 0.999 + pitchAcc * 0.001;
        */

        // TODO: Use PID loop to determine speed to set motors

        // Every 0.5 second serially print the angles
        if (tickUs - last_display  > 500000) {
            /*
            usart_send_string("PID: ");
            ftoa(c_str, pid_output, 2); usart_send_string(c_str);
            usart_send_string("\r\n");

            usart_send_string("SPD: ");
            ftoa(c_str, speed, 2); usart_send_string(c_str);
            usart_send_string(" ");
            itoa(c_str, motor_speed, 2); usart_send_string(c_str);
            usart_send_string("\r\n");
            */

            /*
            usart_send_string("NEW: ");
            ftoa(c_str, roll, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, pitch, 2); usart_send_string(c_str);
            usart_send_string("\r\n");

            imu_get_euler_orientation(&angles);
            // Accelerometer data
            usart_send_string("ACC: ");
            ftoa(c_str, orientation.ax, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, orientation.ay, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, orientation.az, 2); usart_send_string(c_str);
            usart_send_string("\r\n");

            // Magnetometer data
            usart_send_string("MAG: ");
            ftoa(c_str, orientation.mx, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, orientation.my, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, orientation.mz, 2); usart_send_string(c_str);
            usart_send_string("\r\n");

            // Quaternion
            float *q = getQ();
            usart_send_string("QTN: ");
            ftoa(c_str, q[0], 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, q[1], 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, q[2], 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, q[3], 2); usart_send_string(c_str);
            usart_send_string("\r\n");

            // Yaw, Pitch, Roll
            usart_send_string("Orientation: ");
            ftoa(c_str, angles.yaw, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, angles.pitch, 2); usart_send_string(c_str);
            usart_send_string(" ");
            ftoa(c_str, angles.roll, 2); usart_send_string(c_str);
            usart_send_string("\r\n");
            */

            last_display = tickUs;
        }
        /*
        if ((tickUs - last_loop) > PERIOD) {
            usart_send_string("ERROR loop too short\r\n");
        }
        */
        while((tickUs - last_loop) < PERIOD);
    }
    for(;;);
    return 0;
}


void SysTick_Handler(void)
{
	tickUs++;
}
