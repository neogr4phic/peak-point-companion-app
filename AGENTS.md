
### Context document for PeakPoint Companion ###

Programming Guideline:
    - Use programming language C/C++, compatible to Arduino
    - Use and follow this programming guideline: https://docs.arduino.cc/programming/
    - Read the platformio.ini
    - Used hardware:
        - Seeed Studio XIAO nRF52840
        - 0.91" 128x32 OLED Display Modul, single color (white), I2C Interface with 4 Pins (VCC, GND, SDA, SCL)
        - Rotary Encoder KY-040 on breakoutboard with 4 Pins (GND, +, SW, DT, CLK)
    - Rotary Encoder lib for KY-040:
        - use library "mathertel/RotaryEncoder"
        - it has Built-in software debouncing
        - it detects direction reliably
        - it supports acceleration (faster spin = larger steps)


Software Architecture:
- the software should follow a modular, easily maintainable approach
- the functionally should be splitted into logical, dedicated modules/files
- the linking of all modules should be realized with header files (.h)


Basis Features:

    DayScore-Counter:
    - Variable name: dayScoreCounter
    - Data type: uint16_t (0 bis 65535)
    - Init value = 0
    - Min. value = 0
    - Max. value = 9999
    - Font size: setTextSize(4) (corresponds  to 24x32 pixels, full line height)
    - Position: float right, vertically centered
    
    DayScore-History:
    - Variable name: dayScoreHistory
    - Data type: Object (implement a corresponding logic in C!)
    - Init value = empty
    - Structure:
        dayScoreHistory = {
            ODBC timestamp <HH:MM:SS>:<selectedLevel> 
        }

        Example:
        dayScoreHistory = {
            "09:38:12":"3",
            "09:39:22":"4",
            "09:42:54":"7"
        }
    
    Days-History:
    - Variable name: daysHistory
    - Data type: Object (implement a corresponding logic in C!)
    - Init value = empty
    - Structure:
        daysHistory= {
            <dd:mm:yyyy> {
                score: <dayScore>,
                history: <dayScoreHistory>
            } 
        }

        Example:
        daysHistory= {
            "21.04.2026" {
                score: 101,
                history: {
                    "09:38:12":"3",
                    "09:39:22":"4",
                    "09:42:54":"7"
                }
            },
            "22.04.2026" {
                score: 98,
                history: {
                    "11:08:22":"2",
                    "11:10:02":"4",
                    "12:01:53":"7"
                }
            }  
        }



    Virtual Scroll-Wheel (tied to Rotary Encoder):
    - Variable name: selectedLevel
    - Data type: uint8_t (0 to 255)
    - Init value = 1
    - Min. value = 1
    - Max. value = 9
    - Mapping Level to Points, e.g. Level 4 = 15 Points:
        Record<number, number> {
            1: 1,
            2: 3,
            3: 6,
            4: 15,
            5: 25,
            6: 45,
            7: 80,
            8: 130,
            9: 200,
        };
    - Font size: setTextSize(3)
    - Position: float left, vertically centered
    


    Functionality:
        - The system initializes with its init values for dayScoreCounter and selectedLevel.
        
        - When turning the Rotary Encoder clockwise, each tick increases selectedLevel by one level, until max. level = 9 is reached.
        
        - When turning the Rotary Encoder counter-clockwise, each tick decreases 
        selectedLevel by one level, until min. level = 1 is reached.
        
        - When clicking the knob of the rotaray encoder, selectedLevel is submitted.
        
        - When submitting, the corresponding points of the submitted level are added to dayScoreCounter, e.g. if level 7 is submitted, 80 points are added to the dayScoreCounter and displayed.
        
        - When submitting, dayScoreHistory gets a new entry with a timestamp as key and the selectedLevel as value.
        
        - After submitting, selectedLevel is not reset, keeping its level.

        - When long-pressing the knob, the message "finishing day in <timer>" is displayed and a timer starts from 3 seconds to 0. When the timer runs out, the display changes to "Day finished! Good job!" and a new entry is saved to daysHistory.

        - When long-pressing the knob, the system resets to initial state by resetting dayScore and dayScoreHistory to its initial values. daysHistory should not reset and keep its values!



!!Defects:
    - Fast scrolling on the rotary encoder causes glitches of the selectedLevel: rotating clockwise makes the selectedLevel go up but then is randomly jumps back to a lower number when turning the know too fast
    - After entering the finishing state, the counter counts down from 3 to 0 but even when I release the knob, not holding it anymore


Future Features [DO NOT IMPLEMENT!]:

    Real Time Clock (RTC):
    - Correct the timings afterwards during sync with Smartphone (count the millis and use this value to correct)
    - Get time signal via bluetooth from Smartphone
    - Use this signal during standalone runtime with Adafruit RTClib

    Boot Screen:
    - PeakPoint Logo/Graphic
    - Text "PeakPoint Companion"
    - Version Number <Major.Minor.Patch>

    Screen Rotation
    - use portrait mode for screen
    - use display.width() and display.height() for rotated coordinate system