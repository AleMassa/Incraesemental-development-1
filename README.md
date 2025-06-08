# Concrete Rebar Simulator

A desktop application for the preliminary structural design and visualization of reinforced concrete beams, tailored for educational and demonstration purposes in civil and structural engineering.

## Overview

The Concrete Rebar Simulator is a C++ application that allows users to input the geometric and loading parameters for a simply-supported concrete beam and receive a complete preliminary design. The application calculates the required flexural (bending) and shear reinforcement, provides a cost estimate, and generates clear cross-sectional and longitudinal diagrams of the resulting rebar layout.

It serves as an excellent tool for:
* **Civil Engineering Students:** To visualize how design parameters affect reinforcement requirements.
* **Structural Engineers:** For rapid preliminary sizing and cost estimation of beam elements.
* **Educators:** To demonstrate core concepts of reinforced concrete design in a dynamic and interactive way.

---

## Features

* **Parametric Design:** Input beam span, width, height, and vehicle loads.
* **Automated Calculations:** The core engine performs a complete ultimate limit state design to determine the optimal rebar configuration.
* **Economic Optimization:** The logic iterates through standard rebar diameters to find the most cost-effective solution considering the total price of steel, concrete, and labor.
* **Advanced Shear Logic:** The simulator uses realistic engineering criteria to decide when it's necessary to use bent-up bars to assist stirrups in resisting high shear forces.
* **Detailed Costing:** Provides a breakdown of estimated costs for concrete, steel, and labor.
* **Visual Feedback:** Generates two key diagrams:
    * **Cross-Section View:** Shows the layout of rebar, including multiple layers and color-coding for different bar types (straight vs. bent).
    * **Longitudinal View:** Provides a side profile of the beam, clearly illustrating the placement of straight bars, stirrups, and the characteristic shape of bent-up shear reinforcement.

---

## Engineering Principles

The calculations are based on fundamental principles of reinforced concrete design, primarily following the Ultimate Limit State (ULS) methodology.

* **Flexural Design:** The required area of steel is calculated based on the maximum bending moment, ensuring the section is under-reinforced for ductile behavior.
* **Shear Design:** The design accounts for the shear capacity of the concrete (`Vc`) and provides steel reinforcement (stirrups and bent bars) to resist the remaining shear force (`Vs`).
* **Load Distribution:** The simulation uses a simplified AASHTO "S-over" method to approximate the distribution of live loads to a single girder, which is a common approach for preliminary bridge design.

---

## Technology Stack

* **Language:** C++ (C++11 standard or newer)
* **GUI Framework:** [Qt Framework](https://www.qt.io/) (v5.15.2 or newer)
    * *Modules*: `QtWidgets`, `QtGui`, `QtCore`
* **Graphics Library:** [OpenCV](https://opencv.org/) (v4.5.0 or newer)
    * *Modules*: `imgproc`, `highgui`, `core`

---

## Setup and Installation

To compile and run this project, you will need to set up a development environment with the required libraries.

### 1. Prerequisites

* A C++ compiler that supports C++11 (e.g., MSVC on Windows, GCC/Clang on Linux/macOS).
* The Qt build system (`qmake`) or `CMake`.

### 2. Install Libraries

1.  **Qt Framework:** Download and install the open-source version of Qt from the [official website](https://www.qt.io/download). Make sure to install the version that matches your compiler.
2.  **OpenCV:** Download the pre-built libraries for your platform from the [OpenCV releases page](https://opencv.org/releases/) or build it from the source.

### 3. Configure the Project

The project is configured using a `.pro` file for `qmake`. You must edit this file to tell the compiler where to find the OpenCV libraries.

Open the `Concrete_Reinforcement_Front.pro` file and modify the `INCLUDEPATH` and `LIBS` variables to point to your OpenCV installation directory.

**Example `.pro` file configuration:**

```qmake
# Add the path to your OpenCV include files
INCLUDEPATH += C:/libs/opencv/build/include

# Add the path to your OpenCV library files and specify the library to link
# Note: The library file name (e.g., opencv_world455.lib) may vary based on your version
LIBS += -LC:/libs/opencv/build/x64/vc15/lib -lopencv_world455
```

### 4. Build and Run

1.  Open the `.pro` file in Qt Creator.
2.  Configure the project with your chosen compiler kit.
3.  Build and run the project from the Qt Creator IDE.

---

## How to Use

1.  **Enter Parameters:** Fill in the `Geometric Parameters` (Span, Width, Height) and `Load Parameters` (Vehicle Load, Wheel Spacing). All length units are in **mm**, except for Wheel Spacing which is in **m**. Load is in **kN**.
2.  **Auto-Generate (Optional):** Use the `Auto-Generate` button to get a reasonable starting beam size based on the entered span.
3.  **Generate Views:**
    * Click `Cross-Section View` to calculate the design and display the end-on rebar layout.
    * Click `Longitudinal View` to display the side profile with all reinforcement shown.
4.  **Interpret Results:** The calculated costs will update in the bottom-left panel, and the generated diagram will appear in the main view. If the design fails for the given parameters, an error message will be displayed.

---

## File Structure

* `main.cpp`: The main application entry point.
* `Concrete_Reinforcement_Front.h` / `.cpp`: Header and source for the main application window. Manages all UI events and user interaction.
* `Concrete_Reinforcement_Front.ui`: UI layout file created with Qt Designer.
* `RebarCalc.h`: The heart of the application. This header-only class contains all the engineering logic, structural calculations, cost optimization, and image generation routines.

---

## Contributing

Contributions are welcome! If you have ideas for improvements or find a bug, please feel free to open an issue or submit a pull request. Areas for potential improvement include:

* Implementing more advanced structural analysis methods.
* Adding more detailed serviceability checks (e.g., deflection, crack control).
* Expanding the material library and design code options.

