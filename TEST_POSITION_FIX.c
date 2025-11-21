/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * TEST: Verify "Có xung nhưng không có position" bug is fixed
 * ═══════════════════════════════════════════════════════════════════════════════
 * 
 * BUG DESCRIPTION:
 * - Encoder_Count tăng (có xung)
 * - Nhưng Encoder_Calib_Current_Length_CM = 0 (không có position)
 * 
 * ROOT CAUSE:
 * - last_hardware_count được cập nhật trong Encoder_Read()
 * - Khi Encoder_MeasureLength() tính delta, nó so sánh:
 *   delta = stable_count - last_hardware_count = 0 (vì đã bằng nhau!)
 * 
 * FIX:
 * - Sử dụng last_encoder_count riêng biệt cho MeasureLength()
 * - Chỉ cập nhật last_encoder_count SAU KHI tính xong position
 * 
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#include "Encoder.h"
#include "MotorControl.h"
#include <stdio.h>

/**
 * @brief Test case 1: Basic movement detection
 * 
 * Expected: Position should increase when encoder count increases
 */
void test_position_tracking_basic(void) {
    printf("\n=== TEST 1: Basic Position Tracking ===\n");
    
    // Reset encoder
    Encoder_Reset(&encoder1);
    Encoder_ResetWireLength(&encoder1);
    
    // Set motor direction to FORWARD
    extern MotorRegisterMap_t motor1;
    motor1.Direction = FORWARD;
    
    // Simulate encoder pulses
    printf("Cycle | Raw Count | Encoder_Count | Position (mm) | Position (cm) | Delta | Status\n");
    printf("------|-----------|---------------|---------------|---------------|-------|--------\n");
    
    uint16_t test_counts[] = {0, 15, 32, 47, 63, 78, 95, 110};
    
    for (int i = 0; i < 8; i++) {
        // Simulate hardware counter
        __HAL_TIM_SET_COUNTER(&htim2, test_counts[i]);
        
        // Read encoder
        Encoder_Read(&encoder1);
        
        // Measure length
        uint16_t length_mm = Encoder_MeasureLength(&encoder1);
        uint16_t length_cm = encoder1.Encoder_Calib_Current_Length_CM;
        
        // Calculate delta
        uint16_t delta = (i > 0) ? (test_counts[i] - test_counts[i-1]) : 0;
        
        // Check if position is updating
        const char* status = (length_mm > 0 || i == 0) ? "✅ PASS" : "❌ FAIL";
        
        printf("  %d   |   %5u   |     %5u     |    %5u      |     %5u     |  %3u  | %s\n",
               i, test_counts[i], encoder1.Encoder_Count, length_mm, length_cm, delta, status);
    }
    
    // Final check
    if (encoder1.Encoder_Calib_Current_Length_CM > 0) {
        printf("\n✅ TEST PASSED: Position is tracking correctly\n");
    } else {
        printf("\n❌ TEST FAILED: Position is still 0 despite encoder pulses!\n");
    }
}

/**
 * @brief Test case 2: Verify delta calculation
 * 
 * Expected: Delta should be calculated correctly between cycles
 */
void test_delta_calculation(void) {
    printf("\n=== TEST 2: Delta Calculation ===\n");
    
    // Reset encoder
    Encoder_Reset(&encoder1);
    Encoder_ResetWireLength(&encoder1);
    
    extern MotorRegisterMap_t motor1;
    motor1.Direction = FORWARD;
    
    printf("Testing delta calculation between consecutive reads...\n");
    
    // Cycle 1: 0 → 10
    __HAL_TIM_SET_COUNTER(&htim2, 10);
    Encoder_Read(&encoder1);
    uint16_t length1 = Encoder_MeasureLength(&encoder1);
    printf("Cycle 1: Count=10, Length=%u mm (Expected: >0)\n", length1);
    
    // Cycle 2: 10 → 25 (delta = 15)
    __HAL_TIM_SET_COUNTER(&htim2, 25);
    Encoder_Read(&encoder1);
    uint16_t length2 = Encoder_MeasureLength(&encoder1);
    printf("Cycle 2: Count=25, Length=%u mm (Expected: >length1)\n", length2);
    
    // Cycle 3: 25 → 40 (delta = 15)
    __HAL_TIM_SET_COUNTER(&htim2, 40);
    Encoder_Read(&encoder1);
    uint16_t length3 = Encoder_MeasureLength(&encoder1);
    printf("Cycle 3: Count=40, Length=%u mm (Expected: >length2)\n", length3);
    
    // Verify
    if (length1 > 0 && length2 > length1 && length3 > length2) {
        printf("\n✅ TEST PASSED: Delta is calculated correctly\n");
    } else {
        printf("\n❌ TEST FAILED: Delta calculation error!\n");
        printf("   length1=%u, length2=%u, length3=%u\n", length1, length2, length3);
    }
}

/**
 * @brief Test case 3: Continuous operation
 * 
 * Expected: Position should continuously increase with encoder pulses
 */
void test_continuous_operation(void) {
    printf("\n=== TEST 3: Continuous Operation ===\n");
    
    // Reset encoder
    Encoder_Reset(&encoder1);
    Encoder_ResetWireLength(&encoder1);
    
    extern MotorRegisterMap_t motor1;
    motor1.Direction = FORWARD;
    motor1.Speed = 50;
    
    printf("Running 20 cycles with 8 pulses per cycle...\n");
    
    uint16_t count = 0;
    bool all_pass = true;
    
    for (int i = 0; i < 20; i++) {
        count += 8;  // Simulate 8 pulses per cycle
        
        __HAL_TIM_SET_COUNTER(&htim2, count);
        Encoder_Read(&encoder1);
        uint16_t length = Encoder_MeasureLength(&encoder1);
        
        if (i % 5 == 0) {  // Print every 5 cycles
            printf("Cycle %2d: Count=%u, Length=%u mm\n", i, count, length);
        }
        
        // Check if position is increasing
        static uint16_t last_length = 0;
        if (i > 0 && length <= last_length) {
            printf("❌ FAIL at cycle %d: Position not increasing!\n", i);
            all_pass = false;
        }
        last_length = length;
    }
    
    if (all_pass) {
        printf("\n✅ TEST PASSED: Continuous operation working correctly\n");
    } else {
        printf("\n❌ TEST FAILED: Position tracking error in continuous operation\n");
    }
}

/**
 * @brief Test case 4: Reverse direction
 * 
 * Expected: Position should decrease when motor reverses
 */
void test_reverse_direction(void) {
    printf("\n=== TEST 4: Reverse Direction ===\n");
    
    // Reset and move forward first
    Encoder_Reset(&encoder1);
    Encoder_ResetWireLength(&encoder1);
    
    extern MotorRegisterMap_t motor1;
    motor1.Direction = FORWARD;
    
    // Move forward 100 counts
    __HAL_TIM_SET_COUNTER(&htim2, 100);
    Encoder_Read(&encoder1);
    uint16_t length_forward = Encoder_MeasureLength(&encoder1);
    printf("Forward: Count=100, Length=%u mm\n", length_forward);
    
    // Now reverse
    motor1.Direction = REVERSE;
    
    // Move reverse 50 counts (counter still increases to 150)
    __HAL_TIM_SET_COUNTER(&htim2, 150);
    Encoder_Read(&encoder1);
    uint16_t length_reverse = Encoder_MeasureLength(&encoder1);
    printf("Reverse: Count=150, Length=%u mm\n", length_reverse);
    
    // Verify
    if (length_reverse < length_forward) {
        printf("\n✅ TEST PASSED: Reverse direction working correctly\n");
    } else {
        printf("\n❌ TEST FAILED: Position should decrease in reverse!\n");
    }
}

/**
 * @brief Main test runner
 */
void run_all_encoder_tests(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  ENCODER POSITION TRACKING TEST SUITE\n");
    printf("  Testing fix for: 'Có xung nhưng không có position'\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    test_position_tracking_basic();
    test_delta_calculation();
    test_continuous_operation();
    test_reverse_direction();
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  TEST SUITE COMPLETED\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
}

/**
 * @brief Quick diagnostic check
 * 
 * Call this in your main loop to verify encoder is working
 */
void encoder_quick_check(void) {
    static uint32_t last_check_time = 0;
    uint32_t now = HAL_GetTick();
    
    // Check every 1 second
    if (now - last_check_time < 1000) {
        return;
    }
    last_check_time = now;
    
    // Read current state
    uint16_t count = encoder1.Encoder_Count;
    uint16_t length_cm = encoder1.Encoder_Calib_Current_Length_CM;
    uint32_t noise = Encoder_GetNoiseRejectCount();
    
    // Print status
    printf("[ENCODER] Count=%u, Length=%u cm, Noise=%lu\n", count, length_cm, noise);
    
    // Warning if position not tracking
    static uint16_t last_count = 0;
    static uint16_t last_length = 0;
    
    if (count != last_count && length_cm == last_length) {
        printf("⚠️ WARNING: Encoder count changed but position did not!\n");
        printf("   This indicates the bug is still present!\n");
    }
    
    last_count = count;
    last_length = length_cm;
}

