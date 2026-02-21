#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <algorithm>

using namespace std;

// Class to represent an individual appliance
class Appliance {
private:
    string name;
    double powerWatts;
    double hoursPerDay;
    double electricityRate;

public:
    // Constructor
    Appliance(string n, double power, double hours, double rate)
        : name(n), powerWatts(power), hoursPerDay(hours), electricityRate(rate) {}

    // Getters
    string getName() const { return name; }
    double getPowerWatts() const { return powerWatts; }
    double getHoursPerDay() const { return hoursPerDay; }
    double getElectricityRate() const { return electricityRate; }

    // Calculate energy consumption in kWh per day
    double getDailyKWh() const {
        return (powerWatts * hoursPerDay) / 1000.0;
    }

    // Calculate daily cost
    double getDailyCost() const {
        return getDailyKWh() * electricityRate;
    }

    // Calculate monthly cost (30 days)
    double getMonthlyCost() const {
        return getDailyCost() * 30.0;
    }

    // Calculate yearly cost (365 days)
    double getYearlyCost() const {
        return getDailyCost() * 365.0;
    }

    // Display appliance details
    void display() const {
        cout << left << setw(20) << name
             << setw(12) << powerWatts << "W"
             << setw(10) << hoursPerDay << "h"
             << setw(15) << fixed << setprecision(2) << getDailyKWh() << "kWh"
             << setw(12) << "$" << getDailyCost() << endl;
    }
};

// Class to manage the calculator
class PowerCalculator {
private:
    vector<Appliance> appliances;
    double defaultElectricityRate;

    // Clear input buffer
    void clearInputBuffer() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Get valid double input
    double getValidDouble(const string& prompt, double minVal = 0.0) {
        double value;
        while (true) {
            cout << prompt;
            if (cin >> value && value > minVal) {
                clearInputBuffer();
                return value;
            }
            cout << "Invalid input. Please enter a positive number.\n";
            clearInputBuffer();
        }
    }

    // Get valid string input
    string getValidString(const string& prompt) {
        string value;
        while (true) {
            cout << prompt;
            getline(cin, value);
            if (!value.empty()) {
                return value;
            }
            cout << "Invalid input. Please enter a non-empty string.\n";
        }
    }

public:
    // Constructor
    PowerCalculator(double rate = 0.12) : defaultElectricityRate(rate) {}

    // Add a new appliance
    void addAppliance() {
        cout << "\n=== Add New Appliance ===\n";
        
        string name = getValidString("Enter appliance name: ");
        double power = getValidDouble("Enter power rating (Watts): ");
        double hours = getValidDouble("Enter hours used per day (0-24): ");
        
        // Validate hours
        while (hours > 24.0) {
            cout << "Hours cannot exceed 24. Please enter again.\n";
            hours = getValidDouble("Enter hours used per day (0-24): ");
        }

        char useDefaultRate;
        cout << "Use default electricity rate ($" << fixed << setprecision(2) 
             << defaultElectricityRate << "/kWh)? (y/n): ";
        cin >> useDefaultRate;
        clearInputBuffer();

        double rate = defaultElectricityRate;
        if (useDefaultRate == 'n' || useDefaultRate == 'N') {
            rate = getValidDouble("Enter electricity rate ($/kWh): ");
        }

        appliances.push_back(Appliance(name, power, hours, rate));
        cout << "\nAppliance added successfully!\n";
    }

    // Remove an appliance by index
    void removeAppliance() {
        if (appliances.empty()) {
            cout << "\nNo appliances to remove.\n";
            return;
        }

        listAppliances();
        cout << "\nEnter appliance number to remove (1-" << appliances.size() << "): ";
        int index;
        cin >> index;
        clearInputBuffer();

        if (index >= 1 && index <= static_cast<int>(appliances.size())) {
            appliances.erase(appliances.begin() + index - 1);
            cout << "Appliance removed successfully!\n";
        } else {
            cout << "Invalid appliance number.\n";
        }
    }

    // List all appliances
    void listAppliances() const {
        if (appliances.empty()) {
            cout << "\nNo appliances added yet.\n";
            return;
        }

        cout << "\n=== Current Appliances ===\n";
        cout << string(80, '-') << endl;
        cout << left << setw(5) << "#"
             << setw(20) << "Name"
             << setw(12) << "Power"
             << setw(10) << "Hours/Day"
             << setw(15) << "Daily kWh"
             << setw(12) << "Daily Cost" << endl;
        cout << string(80, '-') << endl;

        for (size_t i = 0; i < appliances.size(); ++i) {
            cout << left << setw(5) << (i + 1);
            appliances[i].display();
        }
        cout << string(80, '-') << endl;
    }

    // Display summary statistics
    void displaySummary() const {
        if (appliances.empty()) {
            cout << "\nNo appliances to summarize.\n";
            return;
        }

        double totalDailyKWh = 0.0;
        double totalDailyCost = 0.0;

        for (const auto& appliance : appliances) {
            totalDailyKWh += appliance.getDailyKWh();
            totalDailyCost += appliance.getDailyCost();
        }

        double totalMonthlyCost = totalDailyCost * 30.0;
        double totalYearlyCost = totalDailyCost * 365.0;

        cout << "\n=== Energy Cost Summary ===\n";
        cout << string(50, '=') << endl;
        cout << fixed << setprecision(2);
        cout << "Total Daily Consumption:    " << setw(10) << totalDailyKWh << " kWh\n";
        cout << "Total Daily Cost:          $" << setw(10) << totalDailyCost << endl;
        cout << "Total Monthly Cost (30d):  $" << setw(10) << totalMonthlyCost << endl;
        cout << "Total Yearly Cost (365d):  $" << setw(10) << totalYearlyCost << endl;
        cout << string(50, '=') << endl;
    }

    // Display detailed breakdown
    void detailedBreakdown() const {
        if (appliances.empty()) {
            cout << "\nNo appliances to analyze.\n";
            return;
        }

        cout << "\n=== Detailed Cost Breakdown ===\n";
        cout << string(90, '=') << endl;
        cout << left << setw(20) << "Appliance"
             << setw(15) << "Daily Cost"
             << setw(18) << "Monthly Cost"
             << setw(18) << "Yearly Cost"
             << setw(15) << "% of Total" << endl;
        cout << string(90, '=') << endl;

        double totalDailyCost = 0.0;
        for (const auto& appliance : appliances) {
            totalDailyCost += appliance.getDailyCost();
        }

        for (const auto& appliance : appliances) {
            double percentage = (appliance.getDailyCost() / totalDailyCost) * 100.0;
            cout << left << setw(20) << appliance.getName()
                 << "$" << setw(14) << fixed << setprecision(2) << appliance.getDailyCost()
                 << "$" << setw(17) << appliance.getMonthlyCost()
                 << "$" << setw(17) << appliance.getYearlyCost()
                 << setw(15) << setprecision(1) << percentage << "%" << endl;
        }
        cout << string(90, '=') << endl;
    }

    // Clear all appliances
    void clearAll() {
        if (appliances.empty()) {
            cout << "\nNo appliances to clear.\n";
            return;
        }

        char confirm;
        cout << "\nAre you sure you want to clear all appliances? (y/n): ";
        cin >> confirm;
        clearInputBuffer();

        if (confirm == 'y' || confirm == 'Y') {
            appliances.clear();
            cout << "All appliances cleared!\n";
        }
    }

    // Check if calculator has appliances
    bool hasAppliances() const {
        return !appliances.empty();
    }
};

// Display main menu
void displayMenu() {
    cout << "\n╔════════════════════════════════════════╗\n";
    cout << "║  POWER CONSUMPTION CALCULATOR          ║\n";
    cout << "╚════════════════════════════════════════╝\n";
    cout << "1. Add Appliance\n";
    cout << "2. Remove Appliance\n";
    cout << "3. List All Appliances\n";
    cout << "4. View Summary\n";
    cout << "5. Detailed Breakdown\n";
    cout << "6. Clear All Appliances\n";
    cout << "7. Exit\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "Enter your choice (1-7): ";
}

int main() {
    PowerCalculator calculator;
    int choice;

    cout << "\n⚡ Welcome to the Power Consumption Calculator! ⚡\n";
    cout << "Track and estimate the energy costs of your appliances.\n";

    while (true) {
        displayMenu();
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                calculator.addAppliance();
                break;
            case 2:
                calculator.removeAppliance();
                break;
            case 3:
                calculator.listAppliances();
                break;
            case 4:
                calculator.displaySummary();
                break;
            case 5:
                calculator.detailedBreakdown();
                break;
            case 6:
                calculator.clearAll();
                break;
            case 7:
                cout << "\nThank you for using the Power Consumption Calculator!\n";
                cout << "Saving energy saves money! 💡\n\n";
                return 0;
            default:
                cout << "\nInvalid choice. Please enter a number between 1 and 7.\n";
        }

        // Pause before showing menu again
        if (choice >= 1 && choice <= 6) {
            cout << "\nPress Enter to continue...";
            cin.get();
        }
    }

    return 0;
}
