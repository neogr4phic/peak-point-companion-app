
### Context document for PeakPoint Companion ###

Programming Guideline:
    - Use plain C language (compatible to Arduino)
    

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
    - Data type: Object (implement corresponding logic in C!)
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
    

    Virtual Scroll-Wheel (tied to Rotary Encoder):
    - Variable name: selectedLevel
    - Data type: uint16_t (0 bis 65535)
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
        - The system initializes with its init values for dayScoreCounter and selectedLevel
        - When turning the Rotary Encoder clockwise, each tick increases selectedLevel by one level, until max. level = 9 is reached
        - When turning the Rotary Encoder counter-clockwise, each tick decreases selectedLevel by one level, until min. level = 1 is reached
        - Only when clicking the knob of the rotaray encoder, selectedLevel is submitted
        - After submitting, selectedLevel is not reset, keeping its level
        - After submitting, the corresponding points of the submitted level are added to dayScoreCounter, e.g. if level 7 is submitted, 80 points are added to the dayScoreCounter
        - After submitting, dayScoreHistory gets a new entry with a timestamp as key and the selectedLevel as value
        


Future Features [DO NOT IMPLEMENT!]:

    Boot Screen:
    - PeakPoint Logo/Graphic
    - Text "PeakPoint Companion"
    - Version Number <Major.Minor.Patch>

    Screen Rotation
    - use portrait mode for screen
    - use display.width() and display.height() for rotated coordinate system