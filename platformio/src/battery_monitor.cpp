#include "battery_monitor.h"
#include "config.h"
#include "debug_config.h"

// LiPo discharge curve lookup table
const BatteryMonitor::BatteryPoint BatteryMonitor::lipoTable[] = {
    {4.20, 100},
    {4.15, 95 },
    {4.10, 90 },
    {4.05, 85 },
    {4.00, 80 },
    {3.95, 75 },
    {3.90, 70 },
    {3.85, 65 },
    {3.80, 60 },
    {3.75, 55 },
    {3.70, 50 },
    {3.65, 45 },
    {3.60, 40 },
    {3.55, 35 },
    {3.50, 30 },
    {3.45, 25 },
    {3.40, 20 },
    {3.35, 15 },
    {3.30, 10 },
    {3.20, 5  },
    {3.00, 0  }
};

const int BatteryMonitor::tableSize =
    sizeof(BatteryMonitor::lipoTable) / sizeof(BatteryMonitor::lipoTable[0]);

BatteryMonitor::BatteryMonitor(int pin, float divider) :
    batteryPin(pin),
    voltageDivider(divider),
    lastVoltage(0.0),
    lastPercentage(0),
    debug(false) {}

BatteryMonitor::BatteryMonitor() :
    batteryPin(BATTERY_PIN),
    voltageDivider(BATTERY_VOLTAGE_DIVIDER),
    lastVoltage(0.0),
    lastPercentage(0),
    debug(false) {}

void BatteryMonitor::update() {
    // Read ADC value
    int adcValue = analogRead(batteryPin);

    // Convert ADC reading to voltage (ESP32 ADC is 12-bit, 0-3.3V range)
    float measuredVoltage = (adcValue / 4095.0) * 3.3;

    // Apply voltage divider ratio to get actual battery voltage
    lastVoltage = measuredVoltage * voltageDivider;

    // Calculate percentage from voltage
    lastPercentage = calculatePercentage(lastVoltage);

    if (debug) {
        DEBUG_VERBOSE_PRINT("[BatteryMonitor] ADC: ");
        DEBUG_VERBOSE_PRINT(adcValue);
        DEBUG_VERBOSE_PRINT(" | Measured: ");
        DEBUG_VERBOSE_PRINT_FLOAT(measuredVoltage, 3);
        DEBUG_VERBOSE_PRINT("V | Battery: ");
        DEBUG_VERBOSE_PRINT_FLOAT(lastVoltage, 2);
        DEBUG_VERBOSE_PRINT("V (");
        DEBUG_VERBOSE_PRINT(lastPercentage);
        DEBUG_VERBOSE_PRINTLN("%)");
    }
}

int BatteryMonitor::calculatePercentage(float voltage) const {
    // Handle edge cases
    if (voltage >= lipoTable[0].voltage) {
        return 100;
    }
    if (voltage <= lipoTable[tableSize - 1].voltage) {
        return 0;
    }

    // Find the two points to interpolate between
    for (int i = 0; i < tableSize - 1; i++) {
        if (voltage >= lipoTable[i + 1].voltage) {
            // Linear interpolation between two points
            float v1    = lipoTable[i].voltage;
            float v2    = lipoTable[i + 1].voltage;
            int p1      = lipoTable[i].percentage;
            int p2      = lipoTable[i + 1].percentage;

            float slope = (p1 - p2) / (v1 - v2);
            return p2 + slope * (voltage - v2);
        }
    }

    return 0;
}

void BatteryMonitor::printStatus() const {
    Serial.print("Battery: ");
    Serial.print(lastVoltage, 2);
    Serial.print("V (");
    Serial.print(lastPercentage);
    Serial.print("%)");

    if (isCharging()) {
        Serial.print(" - CHARGING");
    } else if (isCritical()) {
        Serial.print(" - CRITICAL!");
    } else if (isLow()) {
        Serial.print(" - LOW");
    }

    Serial.println();
}

String BatteryMonitor::getStatusString() const {
    String status = String(lastVoltage, 2) + "V (" + String(lastPercentage) + "%)";

    if (isCharging()) {
        status += " CHG";
    } else if (isCritical()) {
        status += " CRIT";
    } else if (isLow()) {
        status += " LOW";
    }

    return status;
}