#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

/**
 * BatteryMonitor class handles battery voltage monitoring and percentage calculation
 * for LiPo batteries using ADC readings and a voltage divider circuit.
 */
class BatteryMonitor {
  private:
    // Battery discharge curve lookup table for LiPo batteries
    struct BatteryPoint {
        float voltage;
        int percentage;
    };

    static const BatteryPoint lipoTable[];
    static const int tableSize;

    int batteryPin;
    float voltageDivider;
    float lastVoltage;
    int lastPercentage;
    bool debug;

    /**
     * Calculate battery percentage from voltage using LiPo discharge curve
     * @param voltage Battery voltage in volts
     * @return Battery percentage (0-100)
     */
    int calculatePercentage(float voltage) const;

  public:
    /**
     * Constructor
     * @param pin ADC pin connected to battery voltage divider
     * @param divider Voltage divider ratio (Vbat / Vadc)
     */
    BatteryMonitor(int pin, float divider);

    /**
     * Constructor with default values from config.h
     */
    BatteryMonitor();

    /**
     * Enable/disable debug output
     */
    void setDebug(bool enable) { debug = enable; }

    /**
     * Update battery readings
     * Reads ADC value and calculates voltage and percentage
     */
    void update();

    /**
     * Get last measured battery voltage
     * @return Battery voltage in volts
     */
    float getVoltage() const { return lastVoltage; }

    /**
     * Get last calculated battery percentage
     * @return Battery percentage (0-100)
     */
    int getPercentage() const { return lastPercentage; }

    /**
     * Check if battery is low (<20%)
     * @return true if battery is low
     */
    bool isLow() const { return lastPercentage < 20; }

    /**
     * Check if battery is critical (<10%)
     * @return true if battery is critical
     */
    bool isCritical() const { return lastPercentage < 10; }

    /**
     * Check if battery is charging (voltage > 4.25V)
     * @return true if battery appears to be charging
     */
    bool isCharging() const { return lastVoltage > 4.25; }

    /**
     * Print battery status to serial
     */
    void printStatus() const;

    /**
     * Get battery status as a formatted string
     * @return String with voltage and percentage
     */
    String getStatusString() const;
};

#endif // BATTERY_MONITOR_H