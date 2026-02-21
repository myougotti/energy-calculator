# Power Consumption Calculator (C++)

A console-based application to calculate and track the power consumption and energy costs of household appliances.

## Features

- **Add/Remove Appliances**: Manage multiple appliances with custom power ratings and usage hours
- **Cost Calculations**: Calculate daily, monthly, and yearly energy costs
- **Detailed Breakdown**: View percentage breakdown of costs by appliance
- **Input Validation**: Robust error handling for user inputs
- **Clean OOP Design**: Uses classes for better code organization and maintainability

## Key Concepts Demonstrated

### Object-Oriented Programming
- **Encapsulation**: Private data members with public getters
- **Const Correctness**: Proper use of const for read-only operations
- **Constructor Initialization**: Member initializer lists

### C++ Standard Library
- `vector<T>` for dynamic appliance storage
- `string` for text handling
- `<iomanip>` for formatted output
- `<limits>` for input validation

### Software Engineering Principles
- **Separation of Concerns**: Separate classes for data (Appliance) and logic (PowerCalculator)
- **Input Validation**: Defensive programming with error checking
- **User Experience**: Clear menus and formatted output

## Compilation & Usage

### Using Make (Recommended)
```bash
# Compile the program
make

# Run the program
make run

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild
```

### Manual Compilation
```bash
# Compile with g++
g++ -std=c++11 -Wall -Wextra -pedantic -O2 power_calculator.cpp -o power_calculator

# Run the program
./power_calculator
```

### Compilation Requirements
- C++11 or later
- g++ or any C++11 compatible compiler

## How to Use

1. **Launch** the program
2. **Choose** from the menu options:
   - Add appliances with power rating (Watts) and daily usage hours
   - View current appliances and their costs
   - See summary statistics
   - Get detailed breakdown with percentages
3. **Calculate** energy costs based on your electricity rate

### Example Appliances

| Appliance      | Power (W) | Hours/Day | Notes                    |
|----------------|-----------|-----------|--------------------------|
| Refrigerator   | 150       | 24        | Runs continuously        |
| LED TV         | 100       | 5         | Average viewing time     |
| Laptop         | 65        | 8         | Work/study usage         |
| Air Conditioner| 1500      | 6         | During hot months        |
| Microwave      | 1000      | 0.5       | ~30 minutes daily        |
| LED Light Bulb | 10        | 6         | Per bulb                 |

## Energy Cost Formula

```
Daily Energy (kWh) = (Power in Watts × Hours per Day) / 1000
Daily Cost ($) = Daily Energy (kWh) × Electricity Rate ($/kWh)
Monthly Cost ($) = Daily Cost × 30
Yearly Cost ($) = Daily Cost × 365
```

## Project Structure

```
.
├── power_calculator.cpp    # Main source code
├── Makefile               # Build automation
└── README.md              # This file
```

## Code Architecture

### Class: `Appliance`
Represents a single appliance with its power consumption characteristics.
- **Data**: name, power rating, usage hours, electricity rate
- **Methods**: Cost calculations, energy consumption, display formatting

### Class: `PowerCalculator`
Manages the collection of appliances and user interface.
- **Data**: Vector of appliances, default electricity rate
- **Methods**: Add/remove appliances, display statistics, input validation

## Future Enhancements

- [ ] Save/load appliance data to/from file
- [ ] Support for different electricity rate tiers (peak/off-peak)
- [ ] Energy efficiency recommendations
- [ ] Comparison with average household consumption
- [ ] Carbon footprint calculations
- [ ] Export reports to CSV/text file

## Learning Outcomes

This project demonstrates:
- **Memory Management**: Using STL containers instead of raw pointers
- **Input Handling**: Robust validation and error recovery
- **Code Organization**: Clean separation between data and presentation
- **Modern C++**: Using C++11 features appropriately
- **Best Practices**: Const correctness, meaningful names, comments

## Author Notes

This project is designed as a portfolio piece for demonstrating C++ proficiency and software engineering principles. It showcases practical application of OOP concepts in a real-world utility program.

## License

This project is provided as-is for educational and portfolio purposes.
