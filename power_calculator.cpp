#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>
#include <ctime>

using namespace std;

// US average grid emission factor (kg CO2 per kWh)
const double CO2_KG_PER_KWH = 0.386;
const string SAVE_FILE = "appliances.dat";

// ─────────────────────────────────────────────────────────
//  Appliance
// ─────────────────────────────────────────────────────────
class Appliance {
private:
    string name;
    double powerWatts;
    double hoursPerDay;
    double electricityRate;

public:
    Appliance(string n, double power, double hours, double rate)
        : name(n), powerWatts(power), hoursPerDay(hours), electricityRate(rate) {}

    // Getters
    string getName()            const { return name; }
    double getPowerWatts()      const { return powerWatts; }
    double getHoursPerDay()     const { return hoursPerDay; }
    double getElectricityRate() const { return electricityRate; }

    // Setters (for edit feature)
    void setName(const string& n)  { name = n; }
    void setPowerWatts(double p)   { powerWatts = p; }
    void setHoursPerDay(double h)  { hoursPerDay = h; }
    void setElectricityRate(double r) { electricityRate = r; }

    // Calculations
    double getDailyKWh()    const { return (powerWatts * hoursPerDay) / 1000.0; }
    double getDailyCost()   const { return getDailyKWh() * electricityRate; }
    double getMonthlyCost() const { return getDailyCost() * 30.0; }
    double getYearlyCost()  const { return getDailyCost() * 365.0; }
    double getDailyCO2()    const { return getDailyKWh() * CO2_KG_PER_KWH; }
    double getYearlyCO2()   const { return getDailyCO2() * 365.0; }
};

// ─────────────────────────────────────────────────────────
//  PowerCalculator
// ─────────────────────────────────────────────────────────
class PowerCalculator {
private:
    vector<Appliance> appliances;
    double defaultRate;

    // ── input helpers ────────────────────────────────────
    void clearBuf() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Returns a double strictly greater than minVal
    double getDouble(const string& prompt, double minVal = 0.0) {
        double v;
        while (true) {
            cout << prompt;
            if (cin >> v && v > minVal) { clearBuf(); return v; }
            cout << "  enter a number > " << minVal << "\n";
            clearBuf();
        }
    }

    // Returns a double >= minVal
    double getDoubleMin(const string& prompt, double minVal = 0.0) {
        double v;
        while (true) {
            cout << prompt;
            if (cin >> v && v >= minVal) { clearBuf(); return v; }
            cout << "  enter a number >= " << minVal << "\n";
            clearBuf();
        }
    }

    string getString(const string& prompt) {
        string v;
        while (true) {
            cout << prompt;
            getline(cin, v);
            if (!v.empty()) return v;
            cout << "  name cannot be empty\n";
        }
    }

    void divider(int w = 60) const { cout << string(w, '-') << '\n'; }

    // ── appliance picker (returns 0-based index or -1) ──
    int pickAppliance(const string& action) {
        listAppliances();
        cout << "\n  # to " << action << ": ";
        int idx; cin >> idx; clearBuf();
        if (idx < 1 || idx > (int)appliances.size()) {
            cout << "  invalid selection\n";
            return -1;
        }
        return idx - 1;
    }

public:
    PowerCalculator(double rate = 0.12) : defaultRate(rate) {}

    double getDefaultRate() const { return defaultRate; }

    // ── add ──────────────────────────────────────────────
    void addAppliance() {
        cout << "\nadd appliance\n";
        divider(14);

        string name  = getString("  name: ");
        double power = getDouble("  power (W): ");
        double hours = getDoubleMin("  hours/day [0-24]: ");
        while (hours > 24.0) {
            cout << "  cannot exceed 24\n";
            hours = getDoubleMin("  hours/day [0-24]: ");
        }

        cout << "  use default rate ($" << fixed << setprecision(4)
             << defaultRate << "/kWh)? [y/n]: ";
        char c; cin >> c; clearBuf();
        double rate = defaultRate;
        if (c == 'n' || c == 'N')
            rate = getDouble("  rate ($/kWh): ");

        appliances.push_back(Appliance(name, power, hours, rate));
        cout << "  added.\n";
    }

    // ── edit ─────────────────────────────────────────────
    void editAppliance() {
        if (appliances.empty()) { cout << "\n  no appliances\n"; return; }
        int i = pickAppliance("edit");
        if (i < 0) return;

        Appliance& a = appliances[i];
        cout << "\nedit: " << a.getName() << "  (Enter = keep current)\n";
        divider(40);

        string s;

        cout << "  name [" << a.getName() << "]: ";
        getline(cin, s);
        if (!s.empty()) a.setName(s);

        cout << "  power [" << fixed << setprecision(0) << a.getPowerWatts() << "W]: ";
        getline(cin, s);
        if (!s.empty()) {
            try { double v = stod(s); if (v > 0) a.setPowerWatts(v); }
            catch (...) { cout << "  skipped (invalid)\n"; }
        }

        cout << "  hours/day [" << setprecision(1) << a.getHoursPerDay() << "h]: ";
        getline(cin, s);
        if (!s.empty()) {
            try {
                double v = stod(s);
                if (v >= 0 && v <= 24) a.setHoursPerDay(v);
                else cout << "  skipped (must be 0-24)\n";
            } catch (...) { cout << "  skipped (invalid)\n"; }
        }

        cout << "  rate [" << setprecision(4) << a.getElectricityRate() << " $/kWh]: ";
        getline(cin, s);
        if (!s.empty()) {
            try { double v = stod(s); if (v > 0) a.setElectricityRate(v); }
            catch (...) { cout << "  skipped (invalid)\n"; }
        }

        cout << "  updated.\n";
    }

    // ── remove ───────────────────────────────────────────
    void removeAppliance() {
        if (appliances.empty()) { cout << "\n  no appliances\n"; return; }
        int i = pickAppliance("remove");
        if (i < 0) return;
        string removed = appliances[i].getName();
        appliances.erase(appliances.begin() + i);
        cout << "  removed: " << removed << "\n";
    }

    // ── list ─────────────────────────────────────────────
    void listAppliances() const {
        if (appliances.empty()) { cout << "\n  no appliances\n"; return; }

        cout << '\n';
        cout << left
             << setw(4)  << "#"
             << setw(22) << "name"
             << setw(9)  << "watts"
             << setw(10) << "hrs/day"
             << setw(12) << "kWh/day"
             << "daily cost\n";
        divider(68);

        for (size_t i = 0; i < appliances.size(); ++i) {
            const Appliance& a = appliances[i];
            cout << left
                 << setw(4)  << (i + 1)
                 << setw(22) << a.getName()
                 << fixed
                 << setw(9)  << setprecision(0) << a.getPowerWatts()
                 << setw(10) << setprecision(1) << a.getHoursPerDay()
                 << setw(12) << setprecision(3) << a.getDailyKWh()
                 << "$" << setprecision(4) << a.getDailyCost() << '\n';
        }
        divider(68);
    }

    // ── summary ──────────────────────────────────────────
    void displaySummary() const {
        if (appliances.empty()) { cout << "\n  no appliances\n"; return; }

        double kWh = 0, cost = 0;
        for (const auto& a : appliances) { kWh += a.getDailyKWh(); cost += a.getDailyCost(); }

        int n = (int)appliances.size();
        cout << "\nsummary  (" << n << " appliance" << (n != 1 ? "s" : "") << ")\n";
        divider(42);
        cout << fixed << setprecision(2);
        cout << "  daily    " << setw(9) << kWh        << " kWh   $" << cost        << '\n';
        cout << "  monthly  " << setw(9) << kWh * 30   << " kWh   $" << cost * 30   << '\n';
        cout << "  yearly   " << setw(9) << kWh * 365  << " kWh   $" << cost * 365  << '\n';
        cout << "  CO2/yr   " << setw(9) << kWh * 365 * CO2_KG_PER_KWH << " kg\n";
        divider(42);
    }

    // ── detailed breakdown ───────────────────────────────
    void detailedBreakdown() const {
        if (appliances.empty()) { cout << "\n  no appliances\n"; return; }

        double totalCost = 0;
        for (const auto& a : appliances) totalCost += a.getDailyCost();

        cout << '\n';
        cout << left
             << setw(22) << "appliance"
             << setw(11) << "daily"
             << setw(13) << "monthly"
             << setw(13) << "yearly"
             << setw(8)  << "share"
             << "CO2/yr (kg)\n";
        divider(80);

        for (const auto& a : appliances) {
            double pct = (a.getDailyCost() / totalCost) * 100.0;
            cout << fixed
                 << left  << setw(22) << a.getName()
                 << "$"   << setw(10) << setprecision(4) << a.getDailyCost()
                 << "$"   << setw(12) << a.getMonthlyCost()
                 << "$"   << setw(12) << a.getYearlyCost()
                 << setw(7)  << setprecision(1) << pct << "%"
                 << setprecision(2) << a.getYearlyCO2() << '\n';
        }
        divider(80);
    }

    // ── set default rate ─────────────────────────────────
    void setDefaultRate() {
        cout << "\n  current rate: $" << fixed << setprecision(4) << defaultRate << "/kWh\n";
        defaultRate = getDouble("  new rate ($/kWh): ");
        cout << "  rate updated to $" << fixed << setprecision(4) << defaultRate << "/kWh\n";
    }

    // ── export CSV ───────────────────────────────────────
    void exportCSV() const {
        if (appliances.empty()) { cout << "\n  no appliances to export\n"; return; }

        time_t now = time(nullptr);
        char buf[40];
        strftime(buf, sizeof(buf), "energy_report_%Y%m%d_%H%M%S.csv", localtime(&now));

        ofstream f(buf);
        if (!f) { cout << "\n  could not create file\n"; return; }

        f << "name,watts,hours_per_day,rate_usd_per_kwh,"
          << "daily_kwh,daily_usd,monthly_usd,yearly_usd,yearly_co2_kg\n";

        for (const auto& a : appliances) {
            f << fixed << setprecision(4)
              << a.getName()             << ','
              << a.getPowerWatts()       << ','
              << a.getHoursPerDay()      << ','
              << a.getElectricityRate()  << ','
              << a.getDailyKWh()         << ','
              << a.getDailyCost()        << ','
              << a.getMonthlyCost()      << ','
              << a.getYearlyCost()       << ','
              << a.getYearlyCO2()        << '\n';
        }
        cout << "\n  exported to " << buf << "\n";
    }

    // ── save / load ──────────────────────────────────────
    void saveData() const {
        ofstream f(SAVE_FILE);
        if (!f) { cout << "\n  save failed\n"; return; }

        f << fixed << setprecision(6);
        f << defaultRate << '\n';
        for (const auto& a : appliances) {
            f << a.getName()            << '|'
              << a.getPowerWatts()      << '|'
              << a.getHoursPerDay()     << '|'
              << a.getElectricityRate() << '\n';
        }
        cout << "  saved " << appliances.size() << " appliance(s)\n";
    }

    void loadData() {
        ifstream f(SAVE_FILE);
        if (!f) return;

        string line;
        // First line: saved default rate
        if (getline(f, line) && !line.empty()) {
            try { defaultRate = stod(line); } catch (...) {}
        }

        appliances.clear();
        while (getline(f, line)) {
            if (line.empty()) continue;
            istringstream ss(line);
            string name, tok;
            double power, hours, rate;
            try {
                getline(ss, name, '|');
                getline(ss, tok,  '|'); power = stod(tok);
                getline(ss, tok,  '|'); hours = stod(tok);
                getline(ss, tok,  '|'); rate  = stod(tok);
                appliances.push_back(Appliance(name, power, hours, rate));
            } catch (...) {}
        }

        if (!appliances.empty())
            cout << "  loaded " << appliances.size() << " appliance(s)\n";
    }

    // ── clear all ────────────────────────────────────────
    void clearAll() {
        if (appliances.empty()) { cout << "\n  no appliances\n"; return; }
        cout << "\n  clear all " << appliances.size() << " appliance(s)? [y/n]: ";
        char c; cin >> c; clearBuf();
        if (c == 'y' || c == 'Y') { appliances.clear(); cout << "  cleared\n"; }
    }
};

// ─────────────────────────────────────────────────────────
//  Menu
// ─────────────────────────────────────────────────────────
void displayMenu(double rate) {
    cout << "\npower calculator  [$" << fixed << setprecision(4) << rate << "/kWh]\n";
    cout << "─────────────────────────────────\n";
    cout << "  1  add appliance\n";
    cout << "  2  edit appliance\n";
    cout << "  3  remove appliance\n";
    cout << "  4  list appliances\n";
    cout << "  5  summary\n";
    cout << "  6  detailed breakdown\n";
    cout << "  7  set electricity rate\n";
    cout << "  8  export CSV report\n";
    cout << "  9  clear all\n";
    cout << "  0  exit\n";
    cout << "─────────────────────────────────\n";
    cout << "> ";
}

int main() {
    PowerCalculator calc;

    cout << "power consumption calculator\n";
    calc.loadData();

    int choice;
    while (true) {
        displayMenu(calc.getDefaultRate());

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  invalid input\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: calc.addAppliance();      break;
            case 2: calc.editAppliance();     break;
            case 3: calc.removeAppliance();   break;
            case 4: calc.listAppliances();    break;
            case 5: calc.displaySummary();    break;
            case 6: calc.detailedBreakdown(); break;
            case 7: calc.setDefaultRate();    break;
            case 8: calc.exportCSV();         break;
            case 9: calc.clearAll();          break;
            case 0:
                calc.saveData();
                cout << "\ngoodbye\n\n";
                return 0;
            default:
                cout << "  enter 0-9\n";
        }

        if (choice >= 1 && choice <= 9) {
            cout << "\npress Enter to continue...";
            cin.get();
        }
    }
}
